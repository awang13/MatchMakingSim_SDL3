#include "Utility.h"

#include "WorldClock.h"

const std::unordered_map<EColor, FColor> NamedColorMap = {
    {Black,         FColor(0, 0, 0)},
    {Blue,          FColor(0, 0, 255)},
    {Blue_Dark,     FColor(2, 0, 36)},
    {Gold,          FColor(255, 215, 0)},
    {Green,         FColor(0, 255, 0)},
    {Green_Dark,    FColor(0, 50, 0)},
    {LightGrey,     FColor(211, 211, 211)},
    {Orange_Light,  FColor(255, 180, 121)},
    {Orange,        FColor(249, 132, 4)},
    {Orange_Dark,   FColor(255, 69, 0)},
    {Purple_Dark,   FColor(9, 9, 121)},
    {Red,           FColor(255, 0, 0)},
    {Red_Dark,      FColor(50, 0, 0)},
    {SkyBlue,       FColor(135, 206, 235)},
    {White,         FColor(255, 255, 255)},
};

ImVec4 ColorAsImVec4(FColor color)
{
    return {
        static_cast<float>(color.r) / 255,
        static_cast<float>(color.g) / 255,
        static_cast<float>(color.b) / 255,
        static_cast<float>(color.a) / 255
    };
}

ImVec4 ColorAsImVec4(EColor colorName)
{
    return ColorAsImVec4(GetColor(colorName));
}

bool CheckUpdateDelay_RealTime(const int& interval, std::chrono::steady_clock::time_point& lastUpdateTimeRef)
{
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTimeRef).count() < static_cast<int>(static_cast<float>(interval) * GetWorldClock().GetSpeed()))
    {
        return false;
    }
    lastUpdateTimeRef = now;
    return true;
}

FColor LerpColor(const FColor& a, const FColor& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t};
}

FColor LerpColor(const EColor& na, const EColor& nb, float t)
{
    FColor a = GetColor(na);
    FColor b = GetColor(nb);
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t};
}