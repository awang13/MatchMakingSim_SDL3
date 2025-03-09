#include "RandomGenerator.h"

Xoshiro256SS rng;

void SeedRandomGenerator(uint64_t seed)
{
    rng.Seed(seed);
}

int RandomInt(int min, int max)
{
    return min + static_cast<int>(rng.Next() % (max - min + 1));
}

uint64_t RandomInt64(uint64_t min, uint64_t max)
{
    return min + (rng.Next() % (max - min + 1));
}

float RandomFloat(float min, float max)
{
    return min + (static_cast<float>(rng.Next()) / static_cast<float>(UINT64_MAX)) * (max - min);
}

double RandomDouble(double min, double max)
{
    return min + (static_cast<double>(rng.Next()) / static_cast<double>(UINT64_MAX)) * (max - min);
}

float RandomFloat()
{
    return static_cast<float>(rng.Next()) / static_cast<float>(UINT64_MAX);
}

double RandomDouble()
{
    return static_cast<double>(rng.Next()) / static_cast<double>(UINT64_MAX);
}

int RandomIntWithAnchor(int anchor, int deviation)
{
    return RandomInt(anchor - deviation, anchor + deviation);
}

uint64_t RandomInt64WithAnchor(uint64_t anchor, uint64_t deviation)
{
    return RandomInt64(anchor - deviation, anchor + deviation);
}

float RandomFloatWithAnchor(float anchor, float deviation)
{
    return RandomFloat(anchor - deviation, anchor + deviation);
}

double RandomDoubleWithAnchor(double anchor, double deviation)
{
    return RandomDouble(anchor - deviation, anchor + deviation);
}

bool GetRandomResult(float probability)
{
    return RandomFloat() < probability;
}

bool GetRandomResult_IntPercentage(int percentage)
{
    return RandomInt(0, 100) < percentage;
}
