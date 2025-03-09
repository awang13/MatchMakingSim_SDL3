#pragma once
#include "Xoshiro256SS.h"

extern Xoshiro256SS rng;

// Init RNG with a seed
void SeedRandomGenerator(uint64_t seed);

// Generate a random number in range based on type of value passed in
int RandomInt(int min, int max);
uint64_t RandomInt64(uint64_t min, uint64_t max);
float RandomFloat(float min, float max);
double RandomDouble(double min, double max);

// Generate a random number between 0 and 1
float RandomFloat();
double RandomDouble();

// Generate a random float using anchor and deviation
int RandomIntWithAnchor(int anchor, int deviation);
uint64_t RandomInt64WithAnchor(uint64_t anchor, uint64_t deviation);
float RandomFloatWithAnchor(float anchor, float deviation);
double RandomDoubleWithAnchor(double anchor, double deviation);

// Returns a random result based on probability between 0 and 1
bool GetRandomResult(float probability);

// Returns a random result based on probability between 0 and 100
bool GetRandomResult_IntPercentage(int percentage);