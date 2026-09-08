#ifndef PLAYERBOTS_ACTRIGGERS_H
#define PLAYERBOTS_ACTRIGGERS_H

//By leewheel 2026-08-24: the-lab 引入 Common.h
#include "Common.h"
//End By leewheel
#include "Trigger.h"
#include "GenericTriggers.h"
#include "DungeonStrategyUtils.h"

//By leewheel 2026-09-03 修复C4099警告：TC中Position是struct(Position.h)，前向声明须一致
struct Position;

inline constexpr uint32 NPC_FOCUS_FIRE = 18374;
inline constexpr float FLARE_SEARCH_RADIUS = 20.0f;
inline Position const SHIRRAK_RANGED_POSITION = { -21.777f, -162.700f, 26.062f };
inline Position const SHIRRAK_TANK_POSITION =   { -65.171f, -162.920f, 26.504f };

class ShirrakTankPositionBossTrigger : public Trigger
{
public:
    ShirrakTankPositionBossTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "shirrak tank position boss") {}
    bool IsActive() override;
};

class ShirrakFleeFocusFireTrigger : public Trigger
{
public:
    ShirrakFleeFocusFireTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "shirrak flee focus fire") {}
    bool IsActive() override;
};

class ShirrakRangedKeepDistanceTrigger : public Trigger
{
public:
    ShirrakRangedKeepDistanceTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "shirrak ranged keep distance") {}
    bool IsActive() override;
};

#endif
