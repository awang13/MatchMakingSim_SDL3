#pragma once
#include <cstdint>

// ✅ State for the xoshiro256** generator (must be seeded properly)
struct Xoshiro256SS
{
    uint64_t state[4];

    // ✅ SplitMix64 to initialize the state (helps with good randomness)
    static uint64_t SplitMix64(uint64_t& seed)
    {
        seed += 0x9e3779b97f4a7c15;
        uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }

    // ✅ Seed function
    void Seed(uint64_t seed)
    {
        state[0] = SplitMix64(seed);
        state[1] = SplitMix64(seed);
        state[2] = SplitMix64(seed);
        state[3] = SplitMix64(seed);
    }

    // ✅ xoshiro256** core PRNG function
    uint64_t Next()
    {
        uint64_t result = RotateLeft(state[1] * 5, 7) * 9; // xoshiro256** variant

        uint64_t t = state[1] << 17;

        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];

        state[2] ^= t;

        state[3] = RotateLeft(state[3], 45); // Final rotation step
        return result;
    }

    // ✅ Helper function for rotating bits left
    static uint64_t RotateLeft(uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }
};
