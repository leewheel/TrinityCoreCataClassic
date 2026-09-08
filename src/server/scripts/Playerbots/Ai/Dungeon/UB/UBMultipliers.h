/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBMULTIPLIERS_H
#define PLAYERBOTS_UBMULTIPLIERS_H

#include "Multiplier.h"

class HungarfenFoulSporesMultiplier : public Multiplier
{
public:
    HungarfenFoulSporesMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "hungarfen foul spores") {}
    float GetValue(Action* action) override;
};

class HungarfenMushroomIgnoreMultiplier : public Multiplier
{
public:
    HungarfenMushroomIgnoreMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "hungarfen mushroom ignore") {}
    float GetValue(Action* action) override;
};

class UnderbatFacingMultiplier : public Multiplier
{
public:
    UnderbatFacingMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "underbat facing") {}
    float GetValue(Action* action) override;
};

#endif
