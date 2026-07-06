#pragma once
#include <string>
#include <vector>

namespace exob {

struct ClothingItem {
    std::string name;
    std::string slot;
    std::string location;
    std::string notes;
};

struct Personality {
    std::string coreTrait;
    std::string socialBehavior;
    std::string emotionalRegulation;
    std::string cognitiveTendencies;
    std::string behavioralNote;
    std::string stressReaction;
};

struct NPC {
    std::string name;
    std::string occupation;
    int age = 0;
    std::string gender;
    std::string race;
    std::string subrace;
    std::string wealth;
    int sanityPoints = 0;
    std::vector<ClothingItem> clothing;
    std::string clothingStyle;
    Personality personality;
    std::string occultSensitivity;
    std::string secret;
};

} // namespace exob
