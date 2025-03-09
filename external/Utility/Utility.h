#pragma once

#include <chrono>
#include <unordered_map>
#include "imgui.h"

// Common struct representing a color with transparency
struct FColor
{
    double r = 255;
    double g = 255;
    double b = 255;
    double a = 255;

    FColor(double inR, double inG, double inB, double inA) {r = inR; g = inG; b = inB; a = inA;} // common constructor
    FColor(double inR, double inG, double inB) {r = inR; g = inG; b = inB;} // solid color don't require alpha input
};

// Commonly used named color for faster lookup
enum EColor
{
    Black,
    Blue,
    Blue_Dark,
    Gold,
    Green,
    Green_Dark,
    LightGrey,
    Orange_Light,
    Orange,
    Orange_Dark,
    Purple_Dark,
    Red,
    Red_Dark,
    SkyBlue,
    White,
};

extern const std::unordered_map<EColor, FColor> NamedColorMap;

static FColor GetColor(EColor color)
{
    return NamedColorMap.find(color)->second;
}

FColor LerpColor(const FColor& a, const FColor& b, float t);
FColor LerpColor(const EColor& na, const EColor& nb, float t);
static float* ColorAsFloatArray(FColor color){
    static float colorVec[4];
    colorVec[0] = static_cast<float>(color.r) / 255.0f;
    colorVec[1] = static_cast<float>(color.g) / 255.0f;
    colorVec[2] = static_cast<float>(color.b) / 255.0f;
    colorVec[3] = static_cast<float>(color.a) / 255.0f;
    return colorVec;
};
ImVec4 ColorAsImVec4(FColor color);
ImVec4 ColorAsImVec4(EColor colorName);

// returns true if delay passes, this is not affected by in game time scale, useful for UI display
bool CheckUpdateDelay_RealTime(const int& interval, std::chrono::steady_clock::time_point& lastUpdateTimeRef);

inline float MapRange(float v, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin);
}

inline float MapRangeNormal(float v, float inMin, float inMax)
{
    return MapRange(v, inMin, inMax, 0.0f, 1.0f);
}