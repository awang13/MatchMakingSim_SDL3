#pragma once

#include <map>
#include <vector>
#include <queue>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "MM_Elements.h"
#include "WorldClock.h"

class WorldClock;
enum class EPlayerState;
class VirtualPlayer;

struct FPlayersStateEvent
{
    uint64_t time;
    int playerId;
    EPlayerState newState;

    FPlayersStateEvent(uint64_t inTime, int inPlayerId, EPlayerState inState)
        : time(inTime), playerId(inPlayerId), newState(inState) {}

    bool operator>(const FPlayersStateEvent& other) const
    {
        return time > other.time;
    }
};

inline std::priority_queue<FPlayersStateEvent, std::vector<FPlayersStateEvent>, std::greater<>> playersStateEvent;

// carries settings of the current world. Defines world time and population
struct FWorldSetting
{
    // World info
    int avgPlayerPerBatch = 25; // only add up to this amount +-50% at a time
    int playerCreationCheckInterval = 15;
};

// carries settings of the Match of the game that's offering the MatchMaking system
struct FMatchSetting
{
    // System info
    int draftInterval = 1000; // in millisec, frequency of system to check queue to make match
    int draftedPoolCheckInterval = 500;
    int routineCheckInterval = 200;
    int matchesPerCycle = 30; // how many matches can system make at a time
    int maxLeaderListSize = 24; // we'll only try to find the top of bottom players of this size
    int minGameThresholdForList = 0;
    
    // Match info
    int numTeams = 2;
    int teamSize = 1;
    int totalPlayer = numTeams * teamSize;
    int matchDuration = 16000; // in millisec, this checks against RawMMSystemTime
    int maxSkillGap = 10;
};

// Types of algorithm of match making, each have a different complexity and can affect the system's efficiency and balance
enum EMatchMakeAlgorithm
{
    LIFO,
    FIFO,
    SkillBased,
    TraitGrouping,
};

struct MatchmakingComparator
{
    bool operator()(const VirtualPlayer* a, const VirtualPlayer* b) const
    {
        return a->GetSkillRating() < b->GetSkillRating();
    }
};

// This system simulates the match making process
class MatchMakingSystem
{
public:
    MatchMakingSystem(EMatchMakeAlgorithm SelectedAlgorithm);

    bool AddPlayerToQueue(VirtualPlayer* player);
    void RemovePlayerFromQueue(VirtualPlayer* player);
    void TryAssignPlayerToTeam(VirtualPlayer* player);
    void Update();
    
    void CreatePlayer();
    std::vector<VirtualPlayer> GetSortedPlayerList(EPlayerSortingType type, bool bAscend = false) const;
    int GetNumPlayerOfState(EPlayerState state) const;
    double GetAvgQueueTime() const;
    void AddToPlayerCreationQueue(int count) { playersToCreate += count; }
    
    // Getters and Setters
    FMatchSetting GetMatchSetting() const { return MatchSetting; }
    void SetMatchSetting(const FMatchSetting& Settings) { MatchSetting = Settings; }
    FWorldSetting GetWorldSetting() const { return WorldSetting; }
    void SetWorldSetting(const FWorldSetting& Settings) { WorldSetting = Settings; }
    const std::unordered_set<int>& GetOngoingMatchIds() const { return ongoingMatchIds; }
    const std::unordered_map<int, VirtualPlayer>& GetAllPlayers() const { return allPlayersLookupMap; }
    const std::unordered_map<int, FMatch>& GetAllMatches() const { return allMatchesLookupMap; }
    std::unordered_map<EPlayerState, int> GetPlayerStateMap() const { return playerStateMap; }
    std::vector<std::vector<VirtualPlayer*>> GetDraftedPools() const { return draftedPools; }

private:
    void Update_DraftQueuedPlayers(); // interval in millisecond
    void Update_StartMatchFromQueuedPools();
    void Update_Matches();
    void Update_PlayerRoutine();
    void Update_CheckPlayerCreation();

    // try to start a match with a drafted team, returns the list of players actually joined
    std::vector<VirtualPlayer*> StartMatch(const std::vector<VirtualPlayer*>& draftedTeam);
    bool IsPlayerMatchable(const VirtualPlayer& player, std::vector<VirtualPlayer*> draftedPool) const;

    void OnPlayerStateChange(VirtualPlayer* player, EPlayerState oldState, EPlayerState newState);

    // general settings determining how the System operates
    FWorldSetting WorldSetting;
    FMatchSetting MatchSetting;
    EMatchMakeAlgorithm algorithm;

    // All ref data cache
    std::unordered_map<int, VirtualPlayer> allPlayersLookupMap;
    std::unordered_map<int, FMatch> allMatchesLookupMap;
    
    // smaller data cache, for faster cache that changes a lot
    std::unordered_set<int> ongoingMatchIds;
    std::unordered_map<EPlayerState, int> playerStateMap;
    std::vector<std::vector<VirtualPlayer*>> draftedPools;
    
    // delay time caches
    uint64_t lastPoolCheckTime = 0;
    uint64_t lastPlayerCreationCheckTime = 0;

    // Cached lists for Display
    std::map<EPlayerSortingType, std::vector<VirtualPlayer>> TopLists;
    std::map<EPlayerSortingType, std::vector<VirtualPlayer>> BottomLists;
    void ReportToLeaderLists(EPlayerSortingType type, const VirtualPlayer& player);
    
    std::deque<VirtualPlayer*> queuedPlayers;
    std::unordered_set<int> queuedPlayerIds; // additional int array to manage existing player lookup

    // remaining players waiting to be created, this is more of a simulation trait, mimicking players creating their account for the game.
    // also serves as a queue to prevent adding thousands of players at a time
    int playersToCreate = 0;
};
