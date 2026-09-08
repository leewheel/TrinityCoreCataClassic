#include "Playerbots.h"
#include "ACTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"
//By leewheel 2026-08-24: 合并 the-lab——补充 Playerbots.h
#include "Playerbots.h"
//End By leewheel

// Shirrak the Dead Watcher

bool ShirrakTankPositionBossTrigger::IsActive()
{
    //By leewheel 2026-08-24: 保留 entry(#18371=Shirrak the Dead Watcher), 采用 the-lab 的 PlayerbotAI::IsTank API
    return PlayerbotAI::IsTank(bot) &&
           AI_VALUE2(Unit*, "find target", "18371");
    //End By leewheel
}

bool ShirrakFleeFocusFireTrigger::IsActive()
{
    if (!AI_VALUE2(Unit*, "find target", "18371"))
        return false;

    //By leewheel 2026-09-04: 上游89a4c459——FindNearestCreature 第三参默认 true, 显式传参删除
    return bot->FindNearestCreature(NPC_FOCUS_FIRE, FLARE_SEARCH_RADIUS);
}

bool ShirrakRangedKeepDistanceTrigger::IsActive()
{
    //By leewheel 2026-08-24: 保留 entry(#18371), 采用 the-lab 的 PlayerbotAI::IsRanged API
    return PlayerbotAI::IsRanged(bot) &&
           AI_VALUE2(Unit*, "find target", "18371");
    //End By leewheel
}
