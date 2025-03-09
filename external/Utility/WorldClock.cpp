#include "WorldClock.h"

void WorldClock::Update()
{
    if (bIsPaused) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime);

    // Apply time scaling
    worldTimeMillis += static_cast<uint64_t>(static_cast<float>(elapsed.count()) * timeScale);
    lastUpdateTime = now;
}

void WorldClock::Resume()
{
    bIsPaused = false;
    lastUpdateTime = std::chrono::steady_clock::now();
}

// SAVE & LOAD TBD
void WorldClock::SaveToFile(const std::string& filename) const
{
    
}
void WorldClock::LoadFromFile(const std::string& filename)
{
    
}

bool WorldClock::CheckUpdateDelay(const uint64_t& interval, uint64_t& lastUpdateTimeRef)
{
    if (static_cast<int>(worldTimeMillis) - lastUpdateTimeRef < interval)
    {
        return false;
    }
    lastUpdateTimeRef = static_cast<int>(worldTimeMillis);
    return true;
}





