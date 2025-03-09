#pragma once

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <unordered_map>

#include "imgui.h"
#include "implot.h"
#include "MatchMakingSystem.h"

class MatchMakingSystem;
class VirtualPlayer;
extern float COLOR_CLEAR[4];

// ImGui Rendering
void DrawMMUI(MatchMakingSystem* mmSystem);

// MMSystem UI
void DrawControlPanel(MatchMakingSystem* mmSystem); // the only panel that has access to change the mmSystem settings (or should we move that out too?)
void DrawStatusPanel(const MatchMakingSystem* mmSystem);
void DrawStatsGraph(const MatchMakingSystem* mmSystem);
void DrawPlayerDetail(const MatchMakingSystem* mmSystem);
void DrawPlayerStatusGraph(const MatchMakingSystem* mmSystem);
void DrawDebug(const MatchMakingSystem* mmSystem);

// Virtual Player Display
void MakePlayerEntry(const MatchMakingSystem* mmSystem, const VirtualPlayer& player);
bool MakeMatchEntry_PlayerCentric(const VirtualPlayer& player, const FMatch& match);
void MakePlayerTimeLine(const VirtualPlayer& player, bool bShowCurrentTime = true);

// Utility
void GetPlayerList(const MatchMakingSystem* mmSystem, std::vector<VirtualPlayer>& listRef, bool bSkipDelay);

