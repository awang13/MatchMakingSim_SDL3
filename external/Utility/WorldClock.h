#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include "Utility.h"

/*
 * The clock that simulates time in the virtual world that our systems live in. Other than user control panel & info
 * display data. Every entity shares the singleton global clock that can be sped up or down
 */
class WorldClock
{
public:
    static WorldClock& GetInstance()
    {
        static WorldClock instance; // Global instance
        return instance;
    }

    void Update();
    void SetSpeed(float speedMultiplier) { timeScale = speedMultiplier; }
    void Pause() { bIsPaused = true; }
    void Resume();

    uint64_t GetGameTimeMillis() const { return worldTimeMillis; }
    float GetSpeed() const { return timeScale; }
    bool GetIsPaused() const { return bIsPaused; }
    void SaveToFile(const std::string& filename) const;
    void LoadFromFile(const std::string& filename);

    // returns true if delay passes
    bool CheckUpdateDelay(const uint64_t& interval, uint64_t& lastUpdateTimeRef);

private:
    uint64_t worldTimeMillis = 0;
    float timeScale = 1.0f;
    bool bIsPaused = false;

    std::chrono::steady_clock::time_point lastUpdateTime;
};

inline WorldClock& GetWorldClock() { return WorldClock::GetInstance(); }

class WorldTime
{
public:
    static constexpr uint64_t MILLISENCONDS_PER_MINUTE = 1000;
    static constexpr uint64_t MINUTE_PER_HOUR = 60;
    static constexpr uint64_t HOUR_PER_DAY = 24;
    static constexpr uint64_t DAY_PER_MONTH = 30;
    static constexpr uint64_t MONTH_PER_YEAR = 12;
    
    static constexpr uint64_t MILLISENCONDS_PER_HOUR = MILLISENCONDS_PER_MINUTE * MINUTE_PER_HOUR;
    static constexpr uint64_t MILLISENCONDS_PER_DAY = MILLISENCONDS_PER_HOUR * HOUR_PER_DAY;
    static constexpr uint64_t MILLISENCONDS_PER_MONTH = MILLISENCONDS_PER_DAY * DAY_PER_MONTH;
    static constexpr uint64_t MILLISENCONDS_PER_YEAR = MILLISENCONDS_PER_MONTH * MONTH_PER_YEAR;

    static int GetYear(uint64_t startTimeMillis = 0) { return static_cast<int>(1 + GetWorldTimeMillis(startTimeMillis) / MILLISENCONDS_PER_YEAR); }
    static int GetMonth(uint64_t startTimeMillis = 0) { return static_cast<int>(1 + (GetWorldTimeMillis(startTimeMillis) % MILLISENCONDS_PER_YEAR) / MILLISENCONDS_PER_MONTH); }
    static int GetDay(uint64_t startTimeMillis = 0) { return static_cast<int>(1 + (GetWorldTimeMillis(startTimeMillis) % MILLISENCONDS_PER_MONTH) / MILLISENCONDS_PER_DAY); }
    static int GetHour(uint64_t startTimeMillis = 0) { return static_cast<int>((GetWorldTimeMillis(startTimeMillis) % MILLISENCONDS_PER_DAY) / MILLISENCONDS_PER_HOUR); }
    static int GetMinute(uint64_t startTimeMillis = 0) { return static_cast<int>((GetWorldTimeMillis(startTimeMillis) % MILLISENCONDS_PER_HOUR) / MILLISENCONDS_PER_MINUTE); }

    static uint64_t GetWorldTimeMillis(uint64_t startTimeMillis = 0){ return WorldClock::GetInstance().GetGameTimeMillis() - startTimeMillis; }

    // take a time stamp and return the percentage progress of the day, regardless of which day is it in the Month or Year
    // if no input then return the progress of current time
    static float GetDayProgress(uint64_t startTimeMillis = GetWorldTimeMillis())
    {
        return static_cast<float>(startTimeMillis % MILLISENCONDS_PER_DAY) / static_cast<float>(MILLISENCONDS_PER_DAY);
    }
    static uint64_t GetDayProgressMillis(uint64_t startTimeMillis = GetWorldTimeMillis())
    {
        return startTimeMillis % MILLISENCONDS_PER_DAY;
    }

    static float conv_Min(uint64_t duration) { return static_cast<float>(duration) / MILLISENCONDS_PER_MINUTE; }
    static std::pair<int, int> conv_DayTimePair(uint64_t duration)
    {
        float sec = conv_Min(duration);
        return {static_cast<int>(sec) / static_cast<int>(MINUTE_PER_HOUR), (static_cast<int>(sec) % static_cast<int>(MINUTE_PER_HOUR))};
    }

    static FColor GetColorOfDay()
    {
        float Midnight = 0.0f;
        float Dawn = 0.25f;
        float Sunrise = 0.35f;
        float Noon = 0.5f;
        float Sunset = 0.75f;
        
        float prog = GetDayProgress();
        
        if (prog < Dawn)    {return LerpColor(Blue_Dark, Purple_Dark, MapRangeNormal(prog, Midnight, Dawn));}
        if (prog < Sunrise) {return LerpColor(Purple_Dark, Orange_Light, MapRangeNormal(prog, Dawn, Sunrise));}
        if (prog < Noon)    {return LerpColor(Orange_Light, SkyBlue, MapRangeNormal(prog, Sunrise, Noon));}
        if (prog < Sunset)  {return LerpColor(SkyBlue, Orange_Dark, MapRangeNormal(prog, Noon, Sunset));}
        
        return LerpColor(Orange_Dark, Blue_Dark, MapRangeNormal(prog, Sunset, 1.0f));} // Midnight is 1.0f here for proper calc
};
