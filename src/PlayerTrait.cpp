#include "PlayerTrait.h"

// Define trait descriptions in a fast lookup table.
// ***Keep it alphabetical in this list
const std::unordered_map<EPlayerTrait, FTraitInfo> TraitDatabase = {
    {EPlayerTrait::Aggressive,    {Common,      +3, +0, -2, -1, +2, +1, -1, "Aggressive", "Prefers risky, high-damage plays"}},
    {EPlayerTrait::Casual,        {Majority,    -1, +1, -1, -1, +0, +0, -1, "Casual", "Plays for fun, not highly competitive"}},
    {EPlayerTrait::Competitive,   {Common,      +2, +1, +2, +2, +1, -1, +2, "Competitive", "Prefers ranked play, always tries to win"}},
    {EPlayerTrait::Confident,     {Common,      +2, +0, +2, +1, +1, -1, +0, "Confident", "More aggressive after wins"}},
    {EPlayerTrait::Defensive,     {Common,      -2, +1, +3, +2, -1, -2, +2, "Defensive", "Avoids risk, plays conservatively"}},
    {EPlayerTrait::Leader,        {Rare,        +1, +2, +2, +1, +2, +1, +2, "Leader", "Plays better when leading a team"}},
    {EPlayerTrait::LoneWolf,      {Uncommon,    +2, -2, +1, +1, +1, +1, +0, "LoneWolf", "Prefers solo play, avoids teamwork"}},
    {EPlayerTrait::MetaAdaptive,  {Rare,        +1, +3, +1, +1, +3, +0, +1, "MetaAdaptive", "Learns from opponents, adjusts strategy"}},
    {EPlayerTrait::Nervous,       {Uncommon,    -2, -1, -3, -2, -1, -1, -1, "Nervous", "Worse performance under high-pressure"}},
    {EPlayerTrait::RiskAverse,    {Rare,        -3, -1, +2, +2, -1, -3, +3, "RiskAverse", "Avoids unnecessary risks, values survival"}},
    {EPlayerTrait::Specialist,    {Rare,        +1, -3, +2, +2, -1, -2, +3, "Specialist", "Sticks to one play-style or weapon"}},
    {EPlayerTrait::Streaky,       {Uncommon,    +2, -1, -2, -1, -1, +3, -1, "Streaky", "Recent results affects performance"}},
    {EPlayerTrait::TeamOriented,  {Uncommon,    -1, +2, +2, +1, +1, +0, +1, "TeamOriented", "Performs better in familiar teams"}},
    {EPlayerTrait::TiltProne,     {Rare,        +3, -3, -3, -2, -1, +3, -1, "TiltProne", "Becomes reckless after consecutive losses"}},
    {EPlayerTrait::Unpredictable, {Rare,        +1, +1, -2, -1, +1, +3, -1, "Unpredictable", "Inconsistent performance, high variance"}},
    {EPlayerTrait::Versatile,     {Rare,        +0, +3, +1, +1, +2, +1, +1, "Versatile", "Adapts frequently, changes play-style"}},
};

const std::unordered_map<ETraitRarity, FRarityInfo> TraitRarityLookup = {
    {Majority,    {70, LightGrey}},
    {Common,      {55, White}},
    {Uncommon,    {25, Green}},
    {Rare,        {10, SkyBlue}},
    {Unique,      {5, Gold}},
};