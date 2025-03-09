#pragma once

#include <vector>
#include <chrono>
#include <functional>
#include <string>

#include "PlayerTrait.h"
#include "WorldClock.h"

enum EPlayerSortingType
{
    WinRate,
    Aggressiveness,
    Flexibility,
    Grit,
    Endurance,
    Instinct,
    Creativity,
    Precision,
    TotalScore, // may change with how we calculate wining possibility. For now it's just adding everything together
    
    IterationRef // keep at last for iteration
};

struct FPlayerSortingTypeDisplay
{
    std::string name = "Undefined";
    std::string abbrev = "N/A";
    bool bAutoFit = false; // if true then ignore displayValue min/max
    float minDisplayValue = 0.0f;
    float maxDisplayValue = 1.0f;
    const char* format = "%.2f";
    bool isEnabled = true; // dev option to remove a type from being displayed

    FPlayerSortingTypeDisplay(EPlayerSortingType type);
};

// ===== VIRTUAL PLAYER BEGIN =====

enum EPlayerSortingType : int;

// States
enum class EPlayerState
{
    None, // init state, should not be in this state at all time
    Offline, // Not in the system
    Online, // Logged in but idle
    InQueue, // Waiting for MM
    InGame, // in a match
    Disconnected, // Left the game but can return
    Rejoining, // Temporarily offline due to unexpected game closure. Expected to return

    IterationRef // keep at last for iteration
};

inline std::string ToString(EPlayerState state)
{
    switch (state)
    {
    case EPlayerState::None:         break;
    case EPlayerState::Offline:      return "Offline";
    case EPlayerState::Online:       return "Online";
    case EPlayerState::InQueue:      return "In Queue";
    case EPlayerState::InGame:       return "In Game";
    case EPlayerState::Disconnected: return "Disconnected";
    case EPlayerState::Rejoining:    return "Rejoining";
    case EPlayerState::IterationRef: break;
    }
    return "Unknown State";
}

// base class for a player that goes online and plays matches in an imaginary game hosted by the MatchMakingSystem
class VirtualPlayer
{
public:
    using StateChangeCallback = std::function<void(VirtualPlayer*, EPlayerState, EPlayerState)>;

    // Register a listener for all state changes
    static void RegisterOnStateChange(StateChangeCallback callback)
    {
        globalListeners.push_back(callback);
    }

    // Register a listener for a specific state
    static void RegisterOnSpecificStateChange(EPlayerState state, StateChangeCallback callback)
    {
        stateSpecificListeners[state].push_back(callback);
    }

    bool operator==(const VirtualPlayer& other) const { return id == other.id; }
    
    VirtualPlayer() = default;
    VirtualPlayer(int inId); // create a player with everything randomized
    VirtualPlayer(int inId, EPlayerTrait inTrait);
    
    void RegisterMatchResult(int matchId, bool bIsWon);
    void UpdateWinRate();
    void SetState(EPlayerState inState, bool forceUpdate = false);
    bool CanChangeToState(EPlayerState inState);

    // information & getters
    double GetAvgQueueTime() const;
    double GetAvgGameTime() const;
    uint64_t GetOnlineTime() const { return totalOnlineTime; }
    int GetTotalMatchesPlayed() const { return static_cast<int>(wonMatches.size() + lostMatches.size()); }
    double GetStatByTypeForSort(EPlayerSortingType type) const;
    bool GetIsInOnlineTime(uint64_t time = WorldTime::GetDayProgressMillis()) const; // check if certain time is within any section of the generated OnlineTimes
    uint64_t GetTimeInCurrentState() const;
    
    bool GetNextStateChangeTimestamp(uint64_t& nextTime, EPlayerState& nextState) const;
    uint64_t GetNextJoinQueueTimestamp() const;
    uint64_t GetNextOnlineTimestamp() const;
    uint64_t GetNextOfflineTimestamp() const;
    
    int GetId() const { return id; }
    EPlayerState GetState() const { return state; }
    std::vector<int> GetWonMatches() const { return wonMatches; }
    std::vector<int> GetLostMatches() const { return lostMatches; }
    std::vector<int> GetMatchHistory() const { return matchHistory; }
    int GetOngoingMatchId() const { return ongoingMatchId; }
    void SetOngoingMatchId(int value) { ongoingMatchId = value; }
    double GetWinRate() const { return winRate; }
    int GetAgr() const { return agr; }
    int GetFle() const { return fle; }
    int GetGri() const { return gri; }
    int GetEdr() const { return edr; }
    int GetIns() const { return ins; }
    int GetCre() const { return cre; }
    int GetPre() const { return pre; }
    int GetTotalScore() const { return agr + fle + gri + edr + ins + cre + pre; }
    std::vector<std::pair<uint64_t, uint64_t>> GetDesiredOnlineTimes() const { return desiredOnlineTimes; }
    uint64_t GetCurrentIdleTime() const { return currentIdleTime; }
    int GetSkillRating() const { return 1; } // TBD
    std::vector<std::string> GetActivityLog() const { return activityLog; }

    // Trait management
    static EPlayerTrait GenerateRandomTraits();
    void ValidateTraits();
    bool HasTrait(EPlayerTrait trait) const {return ::HasTrait(traits, trait); }
    void AddTrait(EPlayerTrait newTrait) {traits |= newTrait; }
    void RemoveTrait(EPlayerTrait traitToRemove) { traits = traits & ~traitToRemove; }
    void HandleConflictTrait_PickOne(const std::vector<EPlayerTrait>& conflictingTraits); // if player has multiple of the conflicting traits, randomly (evenly) pick one and remove otehrs 
    void ApplyTraitModifiers();
    void AddToActivityLog(std::string string);
    
    // misc
    std::string TraitsToString() const;

private:
    static std::vector<StateChangeCallback> globalListeners;
    static std::unordered_map<EPlayerState, std::vector<StateChangeCallback>> stateSpecificListeners;
    
    int id;
    EPlayerState state = EPlayerState::None;
    EPlayerTrait traits = EPlayerTrait::None; // Supports multiple traits through bitmask
    int ongoingMatchId = -1;
    std::vector<int> matchHistory;
    std::vector<int> wonMatches;
    std::vector<int> lostMatches;
    
    // idle time: time when player stays online but not in queue
    float winRate = 0.0f;

    // Quantified play style
    int agr = 0; // Aggressiveness - Willingness to take risks and engage in high-pressure plays
    int fle = 0; // Flexibility - Ability to adapt to new strategies and opponents
    int gri = 0; // Grit - Mental resilience and ability to recover from set-bakcs
    int edr = 0; // Endurance - Long-term consistency across multiple matches
    int ins = 0; // Instinct - Quick and accurate decision-making under pressure
    int cre = 0; // Creativity - Likelihood of turning the tide unexpectedly; wildcard behavior
    int pre = 0; // Precision - Ability to execute mechanical actions with accuracy and efficiency
    
    // time when last state changed. Use this to record player activity history
    uint64_t stateChangeTimeStamp = 0;
    uint64_t currentIdleTime = 0;
    uint64_t totalOnlineTime = 0;
    std::pair<int, uint64_t> queueTimePair;
    std::pair<int, uint64_t> gameTimePair;
    std::vector<std::pair<uint64_t, uint64_t>> desiredOnlineTimes;
    std::vector<std::string> activityLog;
    
    void GenerateOnlineTimes();
};

// ===== VIRTUAL PLAYER END =====

// ===== VIRTUAL MATCH BEGIN =====

enum class EMatchState
{
    Initiated,  // when match is first created
    Ongoing,    // when all conditions are met and match has started
    Finished,   // when the game provided by this match is concluded
    Completed,  // when match is completed and players are removed
};

inline std::string ToString(EMatchState state)
{
    switch (state)
    {
    case EMatchState::Initiated:    return "Created";
    case EMatchState::Ongoing:      return "Ongoing";
    case EMatchState::Finished:     return "Finished";
    case EMatchState::Completed:    return "Completed";
    }
    return "Unknown State";
}

// Struct that stores a match's data
struct FMatch
{    
    // general information
    int matchId = -1;
    std::vector<std::vector<VirtualPlayer>> teams; // supports multiple team and uneven player counts on each team
    uint64_t matchStartTime = 0;
    uint64_t matchDuration = 3000;
    EMatchState state = EMatchState::Initiated;
    std::vector<float> predictedWinRates;

    // end of match info
    std::vector<VirtualPlayer> winningTeam;

    // Process
    void StartMatch();
    std::vector<float> PredictWinProbability() const;
    void EndMatch();
    
    bool IsPlayerWinner(int playerId) const;
    EMatchState GetState() const { return state; }
};

// ===== VIRTUAL MATCH END =====