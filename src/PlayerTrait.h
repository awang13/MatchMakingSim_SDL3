#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "Utility.h"

// Keeps all look-up information for PlayerTraits
enum class EPlayerTrait : uint32_t
{
    None                = 0,
    Aggressive          = 1 << 0,
    Defensive           = 1 << 1,
    Unpredictable       = 1 << 2, 
    Casual              = 1 << 3,
    Competitive         = 1 << 4,
    MetaAdaptive        = 1 << 5, 
    Specialist          = 1 << 6, 
    Versatile           = 1 << 7, 
    RiskAverse          = 1 << 8,
    Streaky             = 1 << 9,
    Confident           = 1 << 10,
    Nervous             = 1 << 11, 
    TiltProne           = 1 << 12, 
    Leader              = 1 << 13, 
    LoneWolf            = 1 << 14, 
    TeamOriented        = 1 << 15,

    /*
     * This is a special entry created for looping through the enum. Generally don't loop unless necessary.
     * Update the (n) in (1 << n) when the enum content is changed.
     */
    AllTraits           = (1 << 16) - 1 // Represents all traits combined.
};

// Representing picking chance of a trait
enum ETraitRarity
{
    Majority,
    Common,
    Uncommon,
    Rare,
    Unique
};

// The details each rarity entails
struct FRarityInfo
{
    int pScore = 0; // aka the percentage, in int from 0-100
    EColor color = EColor::White;
};

// The details each trait entails
struct FTraitInfo
{
    ETraitRarity rarity;
    int agr = 0;
    int fle = 0;
    int gri = 0;
    int edr = 0;
    int ins = 0;
    int cre = 0; 
    int pre = 0;
    std::string displayName = "Undefined";
    std::string description = "Undefined";
};

// Global database for lookup
extern const std::unordered_map<EPlayerTrait, FTraitInfo> TraitDatabase;
extern const std::unordered_map<ETraitRarity, FRarityInfo> TraitRarityLookup;

// ===== Bitwise Operations for EPlayerTrait BEGIN =====
// Combine two traits using bitwise OR
// Aggressive | Competitive -> 00001 | 10000 = 10001 (Both traits)
inline EPlayerTrait operator|(EPlayerTrait a, EPlayerTrait b) { return static_cast<EPlayerTrait>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }

// Assign multiple traits using OR-equals
// player.traits |= EPlayerTrait::Aggressive
inline EPlayerTrait& operator|=(EPlayerTrait& a, EPlayerTrait b){ a = a | b;    return a; }

// Check if two traits overlap
inline EPlayerTrait operator&(EPlayerTrait a, EPlayerTrait b){ return static_cast<EPlayerTrait>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));}

// Invert all bits (everything but...)
inline EPlayerTrait operator~(EPlayerTrait a){ return static_cast<EPlayerTrait>(~static_cast<uint32_t>(a));}

// check if a specific trait exists within multiple assigned traits
inline bool HasTrait(EPlayerTrait traits, EPlayerTrait trait){ return (static_cast<uint32_t>(traits) & static_cast<uint32_t>(trait)) != 0;}
// ===== Bitwise Operations for EPlayerTrait END =====