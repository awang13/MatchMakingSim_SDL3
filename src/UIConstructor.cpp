#include "UIConstructor.h"

#include <sstream>

#include "Logger.h"
#include "MatchMakingSystem.h"
#include "MM_Elements.h"
#include "Utility.h"

std::unordered_map<int, bool> matchListHeaderState;
int numOfPlayersToAdd = 5000;
int currentViewingPlayerId = -1;

std::vector<VirtualPlayer> currentPlayerList;
std::vector<VirtualPlayer> sortedPlayersForImPlot;
int playerListUpdateDelay = 1000; // in millisec
int playerPlotUpdateDelay = 200;
std::chrono::steady_clock::time_point lastPlayerListUpdateTime;
std::chrono::steady_clock::time_point lastPlayerPlotUpdateTime;

EPlayerSortingType sortingType = WinRate;
bool bIsAscSort;
int matchDisplayIndex = 0;
std::string textDebugLogResult = " ";

// Runs the MM simulator on update and renders UI
void DrawMMUI(MatchMakingSystem* mmSystem)
{
    // Draw MMSystem UIs
    DrawControlPanel(mmSystem);
    DrawStatusPanel(mmSystem);
    //DrawStatsGraph(mmSystem);
    DrawPlayerDetail(mmSystem);
    DrawPlayerStatusGraph(mmSystem);

    // DEPRICATED
    //DrawMatchHistory(mmSystem);
    //DrawDebug(mmSystem);
}

void DrawControlPanel(MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Controls");

    ImGui::SeparatorText("World Time Control");
    if (ImGui::Button("x.5")) { GetWorldClock().SetSpeed(0.5f); }
    ImGui::SameLine();
    if (ImGui::Button("x1")) { GetWorldClock().SetSpeed(1.0f); }
    ImGui::SameLine();
    if (ImGui::Button("x3")) { GetWorldClock().SetSpeed(3.0f); }
    ImGui::SameLine();
    if (ImGui::Button("x20")) { GetWorldClock().SetSpeed(20.0f); }
    ImGui::SameLine();
    if (ImGui::Button(GetWorldClock().GetIsPaused() ? "Resume" : "Pause"))
    {
        GetWorldClock().GetIsPaused() ? GetWorldClock().Resume() : GetWorldClock().Pause();
    }
    if (GetWorldClock().GetIsPaused()) { ImGui::SameLine(); ImGui::Text("SYSTEM PAUSED"); }
    
    ImGui::SeparatorText("MMSystem Control");
    if (ImGui::Button("Create Player"))
    {
        mmSystem->AddToPlayerCreationQueue(numOfPlayersToAdd);
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputInt("##numPlayerToAdd", &numOfPlayersToAdd);
    ImGui::PopItemWidth();

    FWorldSetting wSetting = mmSystem->GetWorldSetting();
    mmSystem->SetWorldSetting(wSetting);
    
    FMatchSetting mSetting = mmSystem->GetMatchSetting();
    ImGui::Text("System refresh speed: ");
    ImGui::InputInt("##sysSpeed", &mSetting.draftInterval);
    ImGui::Text("Time per match: ");
    ImGui::InputInt("##duration", &mSetting.matchDuration);
    //ImGui::Text("# Match/Cycle: ");
    //ImGui::InputInt("##matchPerCycle", &Setting.matchesPerCycle);
    mmSystem->SetMatchSetting(mSetting);
    
    ImGui::SeparatorText("Debugging");
    if (ImGui::Button("Validate player in game state"))
    {
        std::vector<int> foundIllegalIds;
        for (int i = 0; i < static_cast<int>(mmSystem->GetAllPlayers().size()); ++i)
        {
            const auto& it_player = mmSystem->GetAllPlayers().find(i);
            if (it_player != mmSystem->GetAllPlayers().end())
            {
                if (it_player->second.GetState() == EPlayerState::InGame && it_player->second.GetTimeInCurrentState() > (static_cast<float>(mSetting.matchDuration) * 1.5f))
                {
                    foundIllegalIds.push_back(it_player->second.GetId());
                }
            }
        }
        if (!foundIllegalIds.empty())
        {
            std::string idStr;
            for (int id : foundIllegalIds)
            {
                idStr += std::to_string(id) + " ";
            }
            textDebugLogResult = "Found players with excessive long game time:" + idStr;
        }
    }

    ImGui::BeginChild("Debug logging", ImVec2(0, 75), true);
    ImGui::TextWrapped(textDebugLogResult.c_str());
    ImGui::EndChild();
    
    ImGui::End();
}

void DrawStatusPanel(const MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Current Status");

    ImGui::SeparatorText("System Status");
    ImGui::Text("Year %d, %d/%02d, %02d:%02d", WorldTime::GetYear(), WorldTime::GetMonth(), WorldTime::GetDay(), WorldTime::GetHour(), WorldTime::GetMinute());
    ImGui::Text("%d", static_cast<int>(WorldTime::GetWorldTimeMillis()));
    ImGui::Text("Total players: %d", static_cast<int>(mmSystem->GetAllPlayers().size()));
    ImGui::Text("# of ongoing matches: %d", static_cast<int>(mmSystem->GetOngoingMatchIds().size()));
    std::pair<int, int> timePair = WorldTime::conv_DayTimePair(static_cast<uint64_t>(mmSystem->GetAvgQueueTime()));
    ImGui::Text("Average Queue time: %02d:%02d", timePair.first, timePair.second);

    ImGui::SeparatorText("Player Status");
    std::vector<std::string> sortingOrder = {"ASC", "DSC"};

    // precalc order button size and location before the method buttons because they're on the same line
    ImVec2 orderBtnSize = ImGui::CalcTextSize(bIsAscSort ? "ASC" : "DSC");
    float orderBtnLocation = ImGui::GetContentRegionAvail().x - orderBtnSize.x;

    // Draw sorting buttons
    bool bSkipDelay = false;
    for (int i = 0; i < static_cast<int>(EPlayerSortingType::IterationRef); ++i)
    {
        EPlayerSortingType it_type = static_cast<EPlayerSortingType>(i);
        FPlayerSortingTypeDisplay display = FPlayerSortingTypeDisplay(it_type);
        
        if (display.isEnabled)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, sortingType == it_type ? 5.0f : 0.0f); // highlight current
            if (ImGui::Button(display.abbrev.c_str()))
            {
                sortingType = it_type;
                bSkipDelay = true;
                sortedPlayersForImPlot = mmSystem->GetSortedPlayerList(sortingType, true);
            }
        }
        
        ImGui::PopStyleVar();
        ImGui::SameLine();
    }

    ImGui::SetCursorPosX(orderBtnLocation);
    if (ImGui::Button(bIsAscSort ? "ASC" : "DSC"))
    {
        bIsAscSort = !bIsAscSort;
        bSkipDelay = true;
    }
    ImGui::Separator();
    
    // Player entry list
    GetPlayerList(mmSystem, currentPlayerList, bSkipDelay);
    if (!currentPlayerList.empty())
    {
        FPlayerSortingTypeDisplay display = FPlayerSortingTypeDisplay(sortingType);

        if (ImGui::BeginTable("Player Display", 4, ImGuiTableFlags_NoBordersInBody))
        {
            for (int c = 0; c < 4; ++c)
            {
                float weight = (c % 2 == 0) ? 0.7f : 0.3f;
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, weight);
            }
            
            for (int i = 0; i < (currentPlayerList.size() + 1) / 2; ++i)
            {
                
                ImGui::TableNextRow();
                for (int c = 0; c < 4; ++c)
                {
                    ImGui::TableSetColumnIndex(c);
                    
                    int index = (c >= 2) ? i + ((currentPlayerList.size() + 1) / 2) : i;
                    if (index >= static_cast<int>(currentPlayerList.size()))
                    {
                        break;
                    }
                    
                    const VirtualPlayer& cachedPlayer = currentPlayerList[index];
                    auto it = mmSystem->GetAllPlayers().find(cachedPlayer.GetId());
                    VirtualPlayer playerRef = it->second;
                
                    if (c % 2 == 0) // on odd columns fill in player buttons
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, currentViewingPlayerId == playerRef.GetId() ? 5.0f : 0.0f); // highlight current
                        
                        ImVec4 btnColor;
                        if (playerRef.GetState() == EPlayerState::InGame)
                        {
                            btnColor = ColorAsImVec4(EColor::Green_Dark);
                        }
                        else
                        {
                            btnColor = playerRef.GetState() == EPlayerState::Offline ? ImVec4(btnColor.x / 2.0f, btnColor.y / 2.0f, btnColor.z / 2.0f, btnColor.w) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                        }
                        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
                            
                        char btnText[128];
                        (void)sprintf_s(btnText, "ID: %d", cachedPlayer.GetId());
                        
                        if (ImGui::Button(btnText, {ImGui::GetContentRegionAvail().x, 0.0f}))
                        {
                            currentViewingPlayerId = cachedPlayer.GetId();
                        }

                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                    }
                    else
                    {
                        char format[128];
                        (void)sprintf_s(format, display.format);
                        
                        ImGui::Text("%.2f", static_cast<float>(cachedPlayer.GetStatByTypeForSort(sortingType))); // displaying the individual 
                    }
                }
            }
            ImGui::EndTable();
        }

        // in the first instance when player count goes above 0, auto select the top player for better UX
        if (currentViewingPlayerId < 0)
        {
            currentViewingPlayerId = currentPlayerList[0].GetId();
        }
    }
    ImGui::End();
}

void DrawPlayerDetail(const MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Player detail");

    if (currentViewingPlayerId > -1)
    {
        MakePlayerEntry(mmSystem, mmSystem->GetAllPlayers().find(currentViewingPlayerId)->second);
    }
    else
    {
        ImGui::Text("no focused player");
    }
    
    ImGui::End();
}

void MakePlayerEntry(const MatchMakingSystem* mmSystem, const VirtualPlayer& player)
{
    // Player info
    ImGui::SeparatorText("General Info");
    ImGui::Text("Player ID: %d", player.GetId());
    std::pair<int, int> timePair = WorldTime::conv_DayTimePair(player.GetTimeInCurrentState());
    ImGui::Text("is [%s] for %02d:%02d", ToString(player.GetState()).c_str(), timePair.first, timePair.second);
    
    // Day life timeline
    MakePlayerTimeLine(player);
    
    // draw traits
    ImGui::SeparatorText("Player Stats");
    int addedTrait = 0;
    for (uint32_t bit = 1; bit <= static_cast<uint32_t>(EPlayerTrait::AllTraits); bit <<= 1)
    {
        auto TraitPair = TraitDatabase.find(static_cast<EPlayerTrait>(bit));
        FTraitInfo TraitInfo = TraitPair->second;
        if (player.HasTrait(TraitPair->first))
        {
            FColor c = GetColor(TraitRarityLookup.find(TraitInfo.rarity)->second.color);
            ImGui::TextColored(ColorAsImVec4(c), (TraitPair->second.displayName + " ").c_str());
            
            ++addedTrait;
            
            if (addedTrait % 2 == 1)
            {
                ImGui::SameLine();
            }
        }
    }
    
    ImGui::NewLine();

    // draw stats
    if (ImGui::BeginTable("Stats", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableNextRow();
        const char* modifierNames[] = {"Agr", "Fle", "Gri", "Edr", "Ins", "Cre", "Pre"};
        for (int i = 0; i < 7; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::TextUnformatted(modifierNames[i]);
        }

        ImGui::TableNextRow();
        int values[] = {
            player.GetAgr(), player.GetFle(), player.GetGri(),
            player.GetEdr(), player.GetIns(), player.GetCre(), player.GetPre()
        };
        for (int i = 0; i < 7; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::Text("%d", values[i]);
        }

        ImGui::EndTable();
    }

    ImGui::NewLine();

    ImGui::SeparatorText("Ongoing Match");
    if (player.GetState() == EPlayerState::InGame && player.GetOngoingMatchId() >= 0)
    {
        auto it = mmSystem->GetAllMatches().find(player.GetOngoingMatchId());
        if (it != mmSystem->GetAllMatches().end())
        {
            MakeMatchEntry_PlayerCentric(player, it->second);
        }
        else
        {
            ImGui::Text("No match ongoing");
        }
    }
    else if(player.GetState() == EPlayerState::InGame && player.GetOngoingMatchId() < 0)
    {
        std::cout << "no match available on player " << player.GetId() << "\n";
    }

    ImGui::SeparatorText("Activity Log");
    ImGui::BeginChild("Activity Log", ImVec2(0, 100), true);
    bool maxedOut = player.GetActivityLog().size() > 100;
    int begin = maxedOut ? player.GetActivityLog().size() - 100 : 0;
    int end = player.GetActivityLog().size();
    for ( int i = begin; i < end - 1; ++i)
    {
        ImGui::Text(player.GetActivityLog()[i].c_str());
    }
    ImGui::EndChild();
    
    // Match history and general info
    ImGui::SeparatorText("Match History");
    ImGui::Text("W: %d, L: %d", static_cast<int>(player.GetWonMatches().size()), static_cast<int>(player.GetLostMatches().size()));
    ImGui::Text("Total Online Time: %.2f", static_cast<float>(player.GetOnlineTime())/1000.0f);
    timePair = WorldTime::conv_DayTimePair(static_cast<uint64_t>(player.GetAvgQueueTime()));
    ImGui::Text("Average Queue Time: %02d:%02d", timePair.first, timePair.second);
    timePair = WorldTime::conv_DayTimePair(static_cast<uint64_t>(player.GetAvgGameTime()));
    ImGui::Text("Average Game Time: %02d:%02d", timePair.first, timePair.second);
    
    if (player.GetMatchHistory().empty())
    {
        ImGui::Text("No matches available.");
    }
    else
    {
        ImGui::Text("Matches completed: %d", static_cast<int>(player.GetMatchHistory().size()));
        ImGui::Text("Display up to 5 from: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(100.0f);
        if (ImGui::SliderInt("##matchDisplayStart", &matchDisplayIndex,
            0, static_cast<int>(player.GetMatchHistory().size() - 1),
            "match id: %d"))
        {
            matchDisplayIndex = std::clamp(matchDisplayIndex, 0, static_cast<int>(player.GetMatchHistory().size() - 1));
        }
        ImGui::PopItemWidth();

        int lastIndex = (std::min)(matchDisplayIndex + 5, static_cast<int>(player.GetMatchHistory().size()));
        for (int i = matchDisplayIndex; i < lastIndex; ++i)
        {
            if (matchListHeaderState.find(i) == matchListHeaderState.end())
            {
                matchListHeaderState[i] = false;
            }

            ImGui::SetNextItemOpen(matchListHeaderState[i]);

            FMatch match = mmSystem->GetAllMatches().find(player.GetMatchHistory()[i])->second;
            matchListHeaderState[i] = MakeMatchEntry_PlayerCentric(player, match);
        }
    }
}

bool MakeMatchEntry_PlayerCentric(const VirtualPlayer& player, const FMatch& match)
{
    bool bEntryOpened = false;

    EColor HeaderColor = match.GetState() == EMatchState::Ongoing ? EColor::Gold : match.IsPlayerWinner(player.GetId()) ? EColor::Green_Dark : EColor::Red_Dark;
    ImGui::PushStyleColor(ImGuiCol_Header, ColorAsImVec4(HeaderColor));
    
    if (ImGui::CollapsingHeader(("["+ ToString(match.GetState()) +"] ID: " + std::to_string(match.matchId)).c_str()))
    {
        bEntryOpened = true;
        int numOfTeams = static_cast<int>(match.teams.size());

        std::pair<int, int> timePair = WorldTime::conv_DayTimePair(match.matchDuration);
        ImGui::Text("Duration: %02d:%02d", timePair.first, timePair.second);

        ImGui::NewLine();
        
        // DRAW TEAMS
        for (int t = 0; t < numOfTeams; ++t)
        {
            ImGui::Text("Team %d: ", t);
            ImGui::SameLine();
            for (size_t p = 0; p < match.teams[t].size(); ++p)
            {
                const VirtualPlayer& pl = match.teams[t][p];
                if (pl.GetId() == player.GetId())
                {
                    ImGui::TextColored(ColorAsImVec4(EColor::Gold),"[%d]", pl.GetId());
                }
                else
                {
                    ImGui::Text("[%d]", pl.GetId());
                }
                
                if (p < match.teams[t].size() - 1)
                {
                    ImGui::SameLine();
                    ImGui::Text("+");
                    ImGui::SameLine();
                }
            }
            if (t < numOfTeams - 1)
            {
                ImGui::SameLine();
                ImGui::Text("vs");
            }
        }

        ImGui::NewLine();

        if (ImGui::BeginTable("Predicted prob", static_cast<int>(match.teams.size()), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            for (int i = 0; i < numOfTeams; ++i)
            {
                ImGui::TableSetColumnIndex(i);
                ImGui::Text("Team %d", i);
            }

            ImGui::TableNextRow();
            for (int i = 0; i < numOfTeams; ++i)
            {
                ImGui::TableSetColumnIndex(i);
                float perc = match.predictedWinRates[i] * 100.0f;
                ImGui::Text("%.2f%%", perc);
            }

            ImGui::EndTable();
        }
    }
    ImGui::PopStyleColor();
    return bEntryOpened;
}

void DrawStatsGraph(const MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Win Rate vs Modifiers graph");
    const std::unordered_map<int, VirtualPlayer>& playerList = mmSystem->GetAllPlayers();
    
    if (playerList.size() < 10)
    {
        ImGui::Text("Not enough player pool (10)");
    }
    else if (ImPlot::BeginPlot("Sorted player graph"))
    {
        // We want to plot out all players so this is an expensive execution. Add a delay that's long enough or we should async this?
        if (CheckUpdateDelay_RealTime(playerPlotUpdateDelay, lastPlayerPlotUpdateTime))
        {
            sortedPlayersForImPlot = mmSystem->GetSortedPlayerList(sortingType, true);
            SIMPLOG(PlotSort, "Player plot updated!")
        }
        std::vector<double> yData;
        yData.reserve(sortedPlayersForImPlot.size());

        for (const VirtualPlayer& p : sortedPlayersForImPlot)
        {
            yData.push_back(p.GetStatByTypeForSort(sortingType));
        }

        // auto fix X because player count is fixed
        ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
        
        FPlayerSortingTypeDisplay display = FPlayerSortingTypeDisplay(sortingType);
        if(display.bAutoFit)
        {
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
        }
        else
        {
            ImPlot::SetupAxisLimits(ImAxis_Y1, display.minDisplayValue, display.maxDisplayValue);
            ImPlot::SetupAxisZoomConstraints(ImAxis_Y1, 1.0f, 1.0f);
        }

        ImPlot::PlotStairs((display.name).c_str(), yData.data(), static_cast<int>(yData.size()));
        ImPlot::EndPlot();
    }

    ImGui::End();
}

void DrawPlayerStatusGraph(const MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Status Graph");

    int total = static_cast<int>(mmSystem->GetAllPlayers().size());
    if (total > 0)
    {
        for (int i = 1; i < static_cast<int>(EPlayerState::IterationRef); ++i)
        {
            EPlayerState state = static_cast<EPlayerState>(i);
            ImGui::Text("%s", ToString(state).c_str());
            ImGui::SameLine();
            
            int count = mmSystem->GetNumPlayerOfState(state);
            ImGui::SetCursorPosX(100.0f);
            ImGui::ProgressBar(static_cast<float>(count)/static_cast<float>(total), ImVec2(0.0f, 15.0f));
        }
    }
    
    ImGui::End();
}

void GetPlayerList(const MatchMakingSystem* mmSystem, std::vector<VirtualPlayer>& listRef, bool bSkipDelay)
{
    //if (!CheckUpdateDelay_RealTime(playerListUpdateDelay, lastPlayerListUpdateTime) && !bSkipDelay) return;
    listRef = mmSystem->GetSortedPlayerList(sortingType, bIsAscSort);
}

void MakePlayerTimeLine(const VirtualPlayer& player, bool bShowCurrentTime)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float len = ImGui::GetContentRegionAvail().x;
    float height = 15.0f;
    
    float startX = p.x;
    float startY = p.y;
    float endX = p.x + len;
    float endY = p.y + height;

    // Draw bg bar
    drawList->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(50, 50, 50, 255));

    // Draw sections
    for (const std::pair<uint64_t, uint64_t>& section : player.GetDesiredOnlineTimes())
    {
        float sectionStartX = startX + WorldTime::GetDayProgress(section.first) * len;
        float sectionEndX = startX + WorldTime::GetDayProgress(section.second) * len;

        drawList->AddRectFilled(ImVec2(sectionStartX, startY), ImVec2(sectionEndX, endY), IM_COL32(150, 150, 50, 255));
    }

    if (bShowCurrentTime)
    {
        float sectionStartX = startX + WorldTime::GetDayProgress() * len - 2.0f;
        float sectionEndX = startX + WorldTime::GetDayProgress() * len + 2.0f;
        drawList->AddRectFilled(ImVec2(sectionStartX, startY), ImVec2(sectionEndX, endY), IM_COL32(150, 20, 20, 255));
    }
    
    ImGui::Dummy(ImVec2(len, height + 5));
}

void DrawDebug(const MatchMakingSystem* mmSystem)
{
    ImGui::Begin("Debug draw");
    
    if (ImPlot::BeginPlot("Sorted player graph"))
    {
        const std::vector<std::vector<VirtualPlayer*>> pools = mmSystem->GetDraftedPools();
        if (!pools.empty())
        {
            std::vector<int> yData;
            yData.reserve(pools.size());

            for (const auto& p : pools)
            {
                yData.push_back(static_cast<int>(p.size()));
            }

            //ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
            //ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);

            ImPlot::PlotBars("pools", yData.data(), static_cast<int>(yData.size()));
        }
        ImPlot::EndPlot();
    }
    
    ImGui::End();
}