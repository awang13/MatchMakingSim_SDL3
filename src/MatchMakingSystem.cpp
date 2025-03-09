#include "MatchMakingSystem.h"

#include <iomanip>
#include <algorithm>
#include <numeric>

#include "MM_Elements.h"
#include "WorldClock.h"
#include "Logger.h"
#include "RandomGenerator.h"

MatchMakingSystem::MatchMakingSystem(EMatchMakeAlgorithm SelectedAlgorithm) : algorithm(SelectedAlgorithm)
{
    // delegate binds
    VirtualPlayer::RegisterOnStateChange([this](VirtualPlayer* player, EPlayerState oldState, EPlayerState newState)
    {
        this->OnPlayerStateChange(player, oldState, newState);
    });

    // initiate cached lists
    for (int i = 0; i < static_cast<int>(EPlayerSortingType::IterationRef); ++i)
    {
        EPlayerSortingType type = static_cast<EPlayerSortingType>(i);
        TopLists[type] = {};
        BottomLists[type] = {};
    }
}

void MatchMakingSystem::Update()
{
    Update_CheckPlayerCreation();
    
    Update_Matches();
    Update_PlayerRoutine();

    Update_DraftQueuedPlayers();
    Update_StartMatchFromQueuedPools();
}

void MatchMakingSystem::Update_CheckPlayerCreation()
{
    if (!GetWorldClock().CheckUpdateDelay(WorldSetting.playerCreationCheckInterval, lastPlayerCreationCheckTime)) return;

    int count = 0;
    if (playersToCreate > 0)
    {
        int rand = RandomIntWithAnchor(WorldSetting.avgPlayerPerBatch, WorldSetting.avgPlayerPerBatch / 2);
        count = playersToCreate < rand ? playersToCreate : rand;
    }

    if (count > 0)
    {
        for(int i = 0; i < count; ++i)
        {
            CreatePlayer();
        }
    }

    playersToCreate -= count;
}

void MatchMakingSystem::CreatePlayer()
{
    int id = static_cast<int>(allPlayersLookupMap.size());
    allPlayersLookupMap.emplace(id, VirtualPlayer(id));
    ReportToLeaderLists(EPlayerSortingType::Aggressiveness, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Flexibility, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Grit, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Endurance, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Instinct, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Creativity, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::Precision, allPlayersLookupMap.find(id)->second);
    ReportToLeaderLists(EPlayerSortingType::TotalScore, allPlayersLookupMap.find(id)->second);
}

void MatchMakingSystem::OnPlayerStateChange(VirtualPlayer* player, EPlayerState oldState, EPlayerState newState)
{
    // Old state checks
    if (oldState == EPlayerState::InQueue && newState != EPlayerState::InGame) // Queue->InGame flow has its own logic in removing player from queue
    {
        RemovePlayerFromQueue(player);
    }
    
    if (newState == EPlayerState::InQueue)
    {
        bool bIsQueued = AddPlayerToQueue(player);
        if (!bIsQueued)
        {
            player->AddToActivityLog("failed to join queue");
            player->SetState(EPlayerState::Online);
        }
    }
    
    // schedule time to do next action
    uint64_t nextTime;
    EPlayerState nextState;
    if (player->GetNextStateChangeTimestamp(nextTime, nextState))
    {
        playersStateEvent.push(FPlayersStateEvent(nextTime, player->GetId(), nextState));
        char log[128];
        (void)sprintf_s(log, "scheduled to %s", ToString(nextState).c_str());
        player->AddToActivityLog(log);
    }

    // Update state map
    bool bFound = playerStateMap.find(oldState) != playerStateMap.end();
    playerStateMap.insert_or_assign(oldState, bFound ? playerStateMap.find(oldState)->second - 1 : 0);
    bFound = playerStateMap.find(newState) != playerStateMap.end();
    playerStateMap.insert_or_assign(newState, bFound ? playerStateMap.find(newState)->second + 1 : 1);
}

void MatchMakingSystem::Update_PlayerRoutine()
{
    int MAX_EVENTS_PER_FRAME = (static_cast<int>(allPlayersLookupMap.size()) / 100) + 5; // Batch size per frame
    int eventProcessed = 0;
    
    while (!playersStateEvent.empty()
        && playersStateEvent.top().time <= WorldTime::GetWorldTimeMillis()
        && eventProcessed < MAX_EVENTS_PER_FRAME)
    {
        FPlayersStateEvent event = playersStateEvent.top();
        playersStateEvent.pop();
        ++eventProcessed;

        auto it = allPlayersLookupMap.find(event.playerId);
        if (it == allPlayersLookupMap.end()) continue;
        
        VirtualPlayer* player = &it->second;
        player->SetState(event.newState);
    }

    if (eventProcessed >= MAX_EVENTS_PER_FRAME)
    {
        //std::cout << "hitting max event per frame! \n";
    }
}

void MatchMakingSystem::Update_DraftQueuedPlayers()
{
    size_t maxDraftablePools = 100;
    while (!queuedPlayers.empty() && draftedPools.size() < maxDraftablePools)
    {
        VirtualPlayer* player = (algorithm == LIFO) ? queuedPlayers.back() : queuedPlayers.front();
        (algorithm == LIFO) ? queuedPlayers.pop_back() : queuedPlayers.pop_front();
        TryAssignPlayerToTeam(player);
    }
}

void MatchMakingSystem::Update_StartMatchFromQueuedPools()
{
    if(!GetWorldClock().CheckUpdateDelay(MatchSetting.draftedPoolCheckInterval, lastPoolCheckTime)){ return; }
    
    int startedMatches = 0;
    for (auto it = draftedPools.begin(); it != draftedPools.end(); )
    {
        if (static_cast<int>(it->size()) == MatchSetting.numTeams * MatchSetting.teamSize)
        {
            std::vector<VirtualPlayer*> newJoinedPlayers = StartMatch(*it);
            
            it = draftedPools.erase(it);
            if (++startedMatches >= MatchSetting.matchesPerCycle)
            {
                break;
            }
        }
        else
        {
            ++it;   
        }
    }
}

std::vector<VirtualPlayer*> MatchMakingSystem::StartMatch(const std::vector<VirtualPlayer*>& draftedTeam)
{
    if(draftedTeam.empty()) return {};

    FMatch newMatch;
    static std::vector<VirtualPlayer*> joinedPlayer;
    // set expected avg, StartMatch function will determine the actual match time, because this could be affected by player traits
    newMatch.matchId = static_cast<int>(allMatchesLookupMap.size());
    newMatch.matchDuration = MatchSetting.matchDuration;

    for (int t = 0; t < MatchSetting.numTeams; ++t)
    {
        std::vector<VirtualPlayer> team;
        for (int j = 0; j < MatchSetting.teamSize; ++j)
        {
            int index = t * MatchSetting.teamSize + j;
            if (index < static_cast<int>(draftedTeam.size()))
            {
                VirtualPlayer* player = draftedTeam[index];
                team.emplace_back(*player);
                player->SetOngoingMatchId(newMatch.matchId);
                player->SetState(EPlayerState::InGame);
                
                char log[128];
                (void)sprintf_s(log, "joined match: %d", newMatch.matchId);
                player->AddToActivityLog(log);
                
                joinedPlayer.push_back(player); // saving a copy here for more complicated logic later. e.g. SetState
            }
            else
            {
                // ERROR MSG for when playerRef isn't valid
                std::cout << "Starting a match with at least one invalid players \n";
            }
        }
        newMatch.teams.push_back(std::move(team));
    }
/*
    for (VirtualPlayer* player : joinedPlayer)
    {
        player->SetState(EPlayerState::InGame);
        player->SetOngoingMatchId(newMatch.matchId);
    }
 */
    
    newMatch.StartMatch();
    allMatchesLookupMap.emplace(newMatch.matchId, newMatch);
    ongoingMatchIds.insert(newMatch.matchId);
    return joinedPlayer;
}

void MatchMakingSystem::Update_Matches()
{
    if (ongoingMatchIds.empty())
    {
        return;
    }

    //START_PERF_MEASURE(Matches)
    std::vector<FMatch*> queuedMatches;
    for (int matchId : ongoingMatchIds)
    {
        auto it = allMatchesLookupMap.find(matchId);
        if (it != allMatchesLookupMap.end())
        {
            queuedMatches.emplace_back(&(it->second));
        }
    }
    
    for (FMatch* match : queuedMatches)
    {
        if (WorldTime::GetWorldTimeMillis(match->matchStartTime) >= match->matchDuration)
        {
            // conclude match
            match->EndMatch();
            
            for (const std::vector<VirtualPlayer>& team : match->teams)
            {
                for (const VirtualPlayer& p : team)
                {
                    if (auto it = allPlayersLookupMap.find(p.GetId()); it != allPlayersLookupMap.end())
                    {
                        it->second.RegisterMatchResult(match->matchId, match->IsPlayerWinner(it->first));

                        char log[128];
                        (void)sprintf_s(log, "match %d ended", match->matchId);
                        it->second.AddToActivityLog(log);
                        
                        it->second.SetState(it->second.GetIsInOnlineTime() ? EPlayerState::Online : EPlayerState::Offline, true);

                        if (it->second.GetTotalMatchesPlayed() > MatchSetting.minGameThresholdForList)
                        {
                            ReportToLeaderLists(EPlayerSortingType::WinRate, it->second);
                        }

                        it->second.SetOngoingMatchId(-1);
                    }
                }
            }
            ongoingMatchIds.erase(match->matchId);
        }
    }
}

void MatchMakingSystem::ReportToLeaderLists(EPlayerSortingType type, const VirtualPlayer& player)
{
    const auto& it_top = std::find(TopLists[type].begin(), TopLists[type].end(), player);
    it_top == TopLists[type].end() ? TopLists[type].push_back(player) : *it_top = player;
    const auto& it_bot = std::find(BottomLists[type].begin(), BottomLists[type].end(), player);
    it_bot == BottomLists[type].end() ? BottomLists[type].push_back(player) : *it_bot = player;
    
    std::sort(TopLists[type].begin(), TopLists[type].end(), [type](const VirtualPlayer& a, const VirtualPlayer& b){return a.GetStatByTypeForSort(type) > b.GetStatByTypeForSort(type);});
    std::sort(BottomLists[type].begin(), BottomLists[type].end(),[type](const VirtualPlayer& a, const VirtualPlayer& b){return a.GetStatByTypeForSort(type) < b.GetStatByTypeForSort(type);});

    if(static_cast<int>(TopLists[type].size()) > MatchSetting.maxLeaderListSize) {TopLists[type].resize(MatchSetting.maxLeaderListSize);}
    if(static_cast<int>(BottomLists[type].size()) > MatchSetting.maxLeaderListSize) {BottomLists[type].resize(MatchSetting.maxLeaderListSize);}
}

std::vector<VirtualPlayer> MatchMakingSystem::GetSortedPlayerList(EPlayerSortingType type, bool bAscend) const
{
    if (bAscend)
    {
        auto it = BottomLists.find(type);
        if (it != BottomLists.end())
        {
            return it->second;
        }
    }
    else
    {
        auto it = TopLists.find(type);
        if (it != TopLists.end())
        {
            return it->second;
        }
    }
    return {};
}

int MatchMakingSystem::GetNumPlayerOfState(EPlayerState state) const
{
    auto it = playerStateMap.find(state);
    return it == playerStateMap.end()? 0 : it->second;
}

double MatchMakingSystem::GetAvgQueueTime() const
{
    if (!allPlayersLookupMap.empty())
    {
        double totalQTime = std::accumulate(allPlayersLookupMap.begin(), allPlayersLookupMap.end(), 0.0,
            [](double total, const std::pair<const int, VirtualPlayer>& entry)
            {
                const VirtualPlayer& player = entry.second;
                return total + player.GetAvgQueueTime();
            }
        );
        return totalQTime / static_cast<double>(allPlayersLookupMap.size());
    }
    return 0;
}

bool MatchMakingSystem::AddPlayerToQueue(VirtualPlayer* player)
{
    if (queuedPlayerIds.find(player->GetId()) != queuedPlayerIds.end())
    {
        return false;
    }

    queuedPlayerIds.insert(player->GetId());
    queuedPlayers.push_back(player);
    return true;
}

void MatchMakingSystem::RemovePlayerFromQueue(VirtualPlayer* player)
{
    if (queuedPlayerIds.find(player->GetId()) == queuedPlayerIds.end())
    {
        return;  // Player is not in queue
    }

    queuedPlayerIds.erase(player->GetId());  // Remove from tracking set

    // Remove player from queue
    queuedPlayers.erase(std::remove_if(queuedPlayers.begin(), queuedPlayers.end(),
        [&](VirtualPlayer* p) { return p->GetId() == player->GetId(); }),
        queuedPlayers.end());
        
    for (auto& team : draftedPools)
    {
        auto it = std::remove(team.begin(), team.end(), player);
        if (it != team.end())
        {
            team.erase(it, team.end());  // Remove player from the team

            // If the team is now empty, remove it
            if (team.empty())
            {
                team = std::move(draftedPools.back());  // Move last team here
                draftedPools.pop_back();
            }
            return;  // Exit after removal (assuming player is only in one team)
        }
    }
}


void MatchMakingSystem::TryAssignPlayerToTeam(VirtualPlayer* player)
{
    bool bMatchablePoolFound = false;

    if (player->GetState() != EPlayerState::InQueue)
    {
        // if player no longer in queue, skip this step. DraftTeamsFromQueue() function will remove this player from queue
        return;
    }
    
    // Try to fit the player into an existing team
    for (std::vector<VirtualPlayer*>& team : draftedPools)
    {
        if (IsPlayerMatchable(*player, team))
        {
            team.push_back(player);
            bMatchablePoolFound = true;
            break;
        }
    }

    // If no team was found, create a new pool
    if (!bMatchablePoolFound)
    {
        draftedPools.push_back({player});
    }
}

bool MatchMakingSystem::IsPlayerMatchable(const VirtualPlayer& player, std::vector<VirtualPlayer*> draftedPool) const 
{
    // TO BE EXTENDED
    if (static_cast<int>(draftedPool.size()) >= MatchSetting.numTeams * MatchSetting.teamSize)
    {
        return false;
    }

    // Skill based check
    if (algorithm == SkillBased)
    {
        for (const VirtualPlayer* teammate : draftedPool)
        {
            if (std::abs(player.GetSkillRating() - teammate->GetSkillRating()) > MatchSetting.maxSkillGap)
            {
                return false;
            }
        }
    }

    // Trait Based
    if (algorithm == TraitGrouping)
    {
        // ?? do we want this? what does trait grouping really mean? need to explore
        return true;
    }
    
    return true;
}
