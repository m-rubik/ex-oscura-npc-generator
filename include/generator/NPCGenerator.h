#pragma once
#include "GenerationContext.h"

namespace exob {

class NPCGenerator {
public:
    NPC generate(GenerationContext& ctx);
protected:
    void generateAge(GenerationContext& ctx);
    void generateClothing(GenerationContext& ctx);
    void generateRace(GenerationContext& ctx);
    void generateSanity(GenerationContext& ctx);
    void generatePersonality(GenerationContext& ctx);
    void generateSecret(GenerationContext& ctx);
    void generateOccupation(GenerationContext& ctx);
};

} // namespace exob
