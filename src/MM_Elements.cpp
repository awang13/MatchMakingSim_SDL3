#include "MM_Elements.h"

#include <iomanip>
#include <iostream>
#include <queue>

#include "RandomGenerator.h"
#include <random>

#include "MatchMakingSystem.h"

std::vector<VirtualPlayer::StateChangeCallback> VirtualPlayer::globalListeners;
std::unordered_map<EPlayerState, std::vector<VirtualPlayer::StateChangeCallback>> VirtualPlayer::stateSpecificListeners;

VirtualPlayer::VirtualPlayer(int inId, EPlayerTrait inTrait)
{
    id = inId;
    traits = inTrait;
    GenerateOnlineTimes();
}

VirtualPlayer::VirtualPlayer(int inId)
{
    id = inId;
    traits = GenerateRandomTraits();
    ValidateTraits();
    ApplyTraitModifiers();
    GenerateOnlineTimes();

    GetIsInOnlineTime() ? SetState(EPlayerState::Online) : SetState(EPlayerState::Offline);
}

EPlayerTrait VirtualPlayer::GenerateRandomTraits()
{
    EPlayerTrait newTraits = EPlayerTrait::None; // init
    for (const auto& [trait, traitInfo] : TraitDatabase)
    {
        if (GetRandomResult_IntPercentage(TraitRarityLookup.at(traitInfo.rarity).pScore))
        {
            newTraits |= trait;
        }
    }
    return newTraits == EPlayerTrait::None ? EPlayerTrait::Casual : newTraits; // if randomized to have no traits then default to casual player
}

void VirtualPlayer::ValidateTraits()
{
    HandleConflictTrait_PickOne({EPlayerTrait::Aggressive, EPlayerTrait::Defensive});
    HandleConflictTrait_PickOne({EPlayerTrait::Casual, EPlayerTrait::Competitive});
}

void VirtualPlayer::ApplyTraitModifiers()
{
    for (const auto& [trait, traitInfo] : TraitDatabase)
    {
        if (HasTrait(trait))
        {
            agr += traitInfo.agr;
            fle += traitInfo.fle;
            gri += traitInfo.gri;
            edr += traitInfo.edr;
            ins += traitInfo.ins;
            cre += traitInfo.cre;
            pre += traitInfo.pre;
        }
    }
}

void VirtualPlayer::AddToActivityLog(std::string string)
{
    activityLog.push_back(string);   
}

void VirtualPlayer::HandleConflictTrait_PickOne(const std::vector<EPlayerTrait>& conflictingTraits)
{
    if (conflictingTraits.size() > 1)
    {
        int foundTraits = 0;
        for (const EPlayerTrait& trait : conflictingTraits)
        {
            if (HasTrait(trait))
            {
                ++foundTraits;
            }
        }
        
        if (foundTraits > 1) // player actually has more than 2 conflicting traits found. Pick one and remove all others
        {
            int rand = RandomInt(0, static_cast<int>(conflictingTraits.size()) - 1);
            for (int i = 0; i < static_cast<int>(conflictingTraits.size()); ++i)
            {
                if (i != rand)
                {
                    RemoveTrait(conflictingTraits[i]);
                }
            }
        }
    }   
}

void VirtualPlayer::RegisterMatchResult(int matchId, bool bIsWon)
{
    matchHistory.push_back(matchId);
    
    if (bIsWon)
    {
        wonMatches.push_back(matchId);
    }
    else
    {
        lostMatches.push_back(matchId);
    }
    
    UpdateWinRate();
}

void VirtualPlayer::UpdateWinRate()
{
    winRate = matchHistory.empty() ? 0.0f : static_cast<float>(wonMatches.size()) / static_cast<float>(matchHistory.size());
}

void VirtualPlayer::SetState(EPlayerState inState, bool forceUpdate)
{
    if (!CanChangeToState(inState) && !forceUpdate) return;
    
    if (inState == EPlayerState::Online)
    {
        currentIdleTime = RandomInt64WithAnchor(4000, 1500);
    }

    // Apply pre-state change
    uint64_t durationInState = WorldTime::GetWorldTimeMillis(stateChangeTimeStamp);

    // record by cases
    // if old state isn't offline, add duration to total online time
    if (state != EPlayerState::Disconnected && state != EPlayerState::Offline && state != EPlayerState::None)
    {
        totalOnlineTime += durationInState;   
    }

    // if old state was in queue, update queue time
    if (state == EPlayerState::InQueue)
    {
        ++queueTimePair.first;
        queueTimePair.second += durationInState;
    }

    // if old state was in game, update game time
    if (state == EPlayerState::InGame)
    {
        ++gameTimePair.first;
        gameTimePair.second += durationInState;
    }

    // finished applying, update to new state
    EPlayerState oldState = state;
    state = inState;
    stateChangeTimeStamp = WorldTime::GetWorldTimeMillis();

    char log[128];
    (void)sprintf_s(log, "set to state: %s", ToString(inState).c_str());
    AddToActivityLog(log);

    // Notify global listeners
    for (const auto& listener : globalListeners)
    {
        listener(this, oldState, state);
    }

    // Notify specific state listeners
    if (stateSpecificListeners.find(inState) != stateSpecificListeners.end())
    {
        for (const auto& listener : stateSpecificListeners[inState])
        {
            listener(this, oldState, state);
        }
    }
}

bool VirtualPlayer::CanChangeToState(EPlayerState inState) 
{
    char log[128];
    if (state == inState)
    {
        (void)sprintf_s(log, "failed: tried setting same state: %s", ToString(inState).c_str());
        AddToActivityLog(log);
        return false;
    }

    if (state == EPlayerState::InGame && inState == EPlayerState::Offline)
    {
        (void)sprintf_s(log, "failed: tried setting from InGame to Offline");
        AddToActivityLog(log);
        return false;
    }

    if (state == EPlayerState::Offline && inState == EPlayerState::InQueue)
    {
        (void)sprintf_s(log, "failed: tried setting from Offline to InQueue");
        AddToActivityLog(log);
        return false;
    }

    if (state == EPlayerState::Offline && inState == EPlayerState::InGame)
    {
        (void)sprintf_s(log, "failed: tried setting from Offline to InGame");
        AddToActivityLog(log);
        return false;
    }
    
    return true;
}

double VirtualPlayer::GetAvgQueueTime() const
{
    uint64_t total = queueTimePair.second;
    int de = queueTimePair.first;
    if (state == EPlayerState::InQueue)
    {
        total += GetTimeInCurrentState();
        ++de;
    }
    return de == 0 ? 0 : static_cast<double>(total) / static_cast<double>(de);
}

double VirtualPlayer::GetAvgGameTime() const
{
    uint64_t total = gameTimePair.second;
    int de = gameTimePair.first;
    if (state == EPlayerState::InGame)
    {
        total += GetTimeInCurrentState();
        ++de;
    }
    return de == 0 ? 0 : static_cast<double>(total) / static_cast<double>(de);
}

std::string VirtualPlayer::TraitsToString() const
{
    std::string result;

    for (uint32_t bit = 1; bit <= static_cast<uint32_t>(EPlayerTrait::AllTraits); bit <<= 1)
    {
        auto TraitPair = TraitDatabase.find(static_cast<EPlayerTrait>(bit));
        FTraitInfo TraitInfo = TraitPair->second;
        if (HasTrait(TraitPair->first)) result += TraitPair->second.displayName + " ";
    }
    
    return result.empty() ? "None" : result;
}

uint64_t VirtualPlayer::GetTimeInCurrentState() const
{
    return WorldTime::GetWorldTimeMillis(stateChangeTimeStamp);
}

void VirtualPlayer::GenerateOnlineTimes()
{
    /*
     * A day goes from 0 - 1440. Online times are stored in <int> pairs as sections, so we need a random even number and
     * spread it out throughout the day, with a minimal up time and maximum section per day
     */

    int minInterval = 60;
    int maxSections = 6;
    int numStamps = RandomInt(1, maxSections) * 2; // every 2 stamps will form a section
    
    std::vector<int> stamps;

    while (stamps.size() < static_cast<size_t>(numStamps))
    {
        int candidate = RandomInt(0 , 1440);

        bool valid = true;
        if (!stamps.empty())
        {
            for (int s : stamps)
            {
                if (std::abs(s - candidate) < minInterval)
                {
                    valid = false;
                    break;
                }
            }
        }

        if (valid)
        {
            stamps.push_back(candidate);
        }
    }

    std::sort(stamps.begin(), stamps.end());

    desiredOnlineTimes.clear();
    for (int i = 0; i < static_cast<int>(stamps.size() - 1); i += 2)
    {
        std::pair<uint64_t, uint64_t> section = {stamps[i]*1000, stamps[i+1]*1000};
        desiredOnlineTimes.push_back(section);
    }
}

bool VirtualPlayer::GetIsInOnlineTime(uint64_t time) const
{
    for (std::pair<uint64_t, uint64_t> section : desiredOnlineTimes)
    {
        if (time >= section.first && time <= section.second)
        {
            return true;
        }
    }
    return false;
}

double VirtualPlayer::GetStatByTypeForSort(EPlayerSortingType type) const
{
    switch (type)
    {
        case(WinRate): return GetWinRate();
        case(Aggressiveness): return GetAgr();
        case(Flexibility): return GetFle();
        case(Grit): return GetGri();
        case(Endurance): return GetEdr();
        case(Instinct): return GetIns();
        case(Creativity): return GetCre();
        case(Precision): return GetPre();
        case(TotalScore): return GetTotalScore();
        case IterationRef:
        break;
    }
    return 0.0f;
}

bool VirtualPlayer::GetNextStateChangeTimestamp(uint64_t& nextTime, EPlayerState& nextState) const
{
    uint64_t queueTime;
    uint64_t onlineStateChangeTime;
    bool queueFirst;
    
    if (state == EPlayerState::Online)
    {
        queueTime = GetNextJoinQueueTimestamp();
        onlineStateChangeTime = GetNextOfflineTimestamp();
        queueFirst = queueTime < onlineStateChangeTime;
        
        nextTime = queueFirst ? queueTime : onlineStateChangeTime;
        nextState = queueFirst ? EPlayerState::InQueue : EPlayerState::Offline;
        return true;
    }

    if (state == EPlayerState::InQueue)
    {
        onlineStateChangeTime = GetNextOfflineTimestamp();
        
        nextTime = onlineStateChangeTime;
        nextState = EPlayerState::Offline;
        return true;
    }

    if (state == EPlayerState::Offline)
    {
        onlineStateChangeTime = GetNextOnlineTimestamp();
        
        nextTime = onlineStateChangeTime;
        nextState = EPlayerState::Online;
        return true;
    }
    
    return false;
}

uint64_t VirtualPlayer::GetNextJoinQueueTimestamp() const
{
    return WorldTime::GetWorldTimeMillis() + GetCurrentIdleTime();
}

uint64_t VirtualPlayer::GetNextOnlineTimestamp() const
{
    uint64_t timeOfDay = WorldTime::GetDayProgressMillis();
    uint64_t nextTimeOfDay = timeOfDay;
    uint64_t startOfDay = WorldTime::GetWorldTimeMillis() - WorldTime::GetDayProgressMillis();
    
    if (!desiredOnlineTimes.empty())
    {
        bool bTimeFound = false;
        for (const auto& timePair : desiredOnlineTimes)
        {
            // return the first timePair.first that's later than timeOfDay.
            if (timeOfDay <= timePair.first)
            {
                nextTimeOfDay = timePair.first;
                bTimeFound = true;
                break;
            }
        }
        // Otherwise it means there's no online day today. return the first time tmr
        if (!bTimeFound)
        {
            nextTimeOfDay = desiredOnlineTimes[0].first;
        }
    }
    
    if (nextTimeOfDay < timeOfDay)
    {
        nextTimeOfDay = nextTimeOfDay + WorldTime::MILLISENCONDS_PER_DAY;
    }
    return startOfDay + nextTimeOfDay;
}

uint64_t VirtualPlayer::GetNextOfflineTimestamp() const
{
    uint64_t timeOfDay = WorldTime::GetDayProgressMillis();
    uint64_t nextTimeOfDay = timeOfDay;
    uint64_t startOfDay = WorldTime::GetWorldTimeMillis() - WorldTime::GetDayProgressMillis();
    
    if (!desiredOnlineTimes.empty())
    {
        bool bTimeFound = false;
        for (const auto& timePair : desiredOnlineTimes)
        {
            // return the first timePair.first that's later than timeOfDay.
            if (timeOfDay <= timePair.second)
            {
                nextTimeOfDay = timePair.second;
                bTimeFound = true;
                break;
            }
        }
        // Otherwise it means there's no online day today. return the first time tmr
        if (!bTimeFound)
        {
            nextTimeOfDay = desiredOnlineTimes[0].second;
        }
    }
    
    if (nextTimeOfDay < timeOfDay)
    {
        nextTimeOfDay = nextTimeOfDay + WorldTime::MILLISENCONDS_PER_DAY;
    }
    return startOfDay + nextTimeOfDay;
}


// ===== FMatch BEGIN =====

void FMatch::StartMatch()
{
    // Set a unique randomized duration for each started match, this can be affected by game mode and player stats
    matchDuration = RandomInt64WithAnchor(matchDuration, matchDuration / 2);
    matchStartTime = WorldTime::GetWorldTimeMillis();
    state = EMatchState::Ongoing;
    predictedWinRates = PredictWinProbability();
}

std::vector<float> FMatch::PredictWinProbability() const
{
    if (teams.size() < 2) return {1.0f}; // no opponent, no predicting

    std::vector<int> teamScores;
    for (const auto& team : teams)
    {
        int teamStrength = 0;
        for (const VirtualPlayer& player : team)
        {
            teamStrength += player.GetAgr() + player.GetFle() + player.GetGri() +
                            player.GetEdr() + player.GetIns() + player.GetCre() + player.GetPre();
        }
        teamScores.push_back(teamStrength);
    }

    // Apply softmax function
    float K = 10.0f;
    std::vector<float> expValues;
    float sumExp = 0.0f;

    for (int S : teamScores)
    {
        float expVal = expf(static_cast<float>(S) / K);
        expValues.push_back(expVal);
        sumExp += expVal;
    }

    // Normalize probabilities
    std::vector<float> probabilities;
    for (float expVal : expValues)
    {
        probabilities.push_back(expVal / sumExp);
    }
    
    return probabilities;
}

void FMatch::EndMatch()
{
    if (teams.size() > 1)
    {
        std::vector<float> cumulativeProbs;
        float cumulativeSum = 0.0f;
        for (float p : predictedWinRates)
        {
            cumulativeSum += p;
            cumulativeProbs.push_back(cumulativeSum);
        }

        float randomValue = RandomFloat();

        for (size_t i = 0; i < cumulativeProbs.size(); ++i)
        {
            if (randomValue <= cumulativeProbs[i])
            {
                winningTeam = teams[i];
                break;
            }
        }
    }
    
    state = EMatchState::Completed;
}

bool FMatch::IsPlayerWinner(int playerId) const
{
    return !winningTeam.empty() && std::any_of(winningTeam.begin(), winningTeam.end(),
        [playerId](const VirtualPlayer& player) { return player.GetId() == playerId;} );
}

FPlayerSortingTypeDisplay::FPlayerSortingTypeDisplay(EPlayerSortingType type)
{
    switch(type)
    {
        case WinRate:
            name = "Win Rate";
            abbrev = "WR";
            bAutoFit = false;
            minDisplayValue = 0.0f;
            maxDisplayValue = 1.0f;
            break;
        
        case Aggressiveness:
            name = "Aggressiveness";
            abbrev = "AGR";
            bAutoFit = true;
            break;
        
        case Flexibility:
            name = "Flexibility";
            abbrev = "FLE";
            bAutoFit = true;
            break;
        
        case Grit:
            name = "Grit";
            abbrev = "GRI";
            bAutoFit = true;
            break;
        
        case Endurance:
            name = "Endurance";
            abbrev = "END";
            bAutoFit = true;
            break;
        
        case Instinct:
            name = "Instinct";
            abbrev = "INS";
            bAutoFit = true;
            break;
        
        case Creativity:
            name = "Creativity";
            abbrev = "CRE";
            bAutoFit = true;
            break;
        
        case Precision:
            name = "Precision";
            abbrev = "PRE";
            bAutoFit = true;
            break;
        
        case TotalScore:
            name = "TotalScore";
            abbrev = "TTL";
            bAutoFit = true;
            break;
        
        case IterationRef:
            isEnabled = false;
            break;
    }
}