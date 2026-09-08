/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AttackerWithoutAuraTargetValue.h"

#include "Playerbots.h"
//By leewheel 2026-07-26: 引入Strategy.h/TargetValue.h以使用排除类型与汇总函数。
#include "Strategy.h"
#include "TargetValue.h"
//End By leewheel

Unit* AttackerWithoutAuraTargetValue::Calculate()
{
    GuidVector attackers = botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get();
    //By leewheel 2026-07-26: 移植动态排除(Attacker类型；休眠时为空集)。
    GuidSet const dynamicExclusions = GatherStrategyTargetExclusions(botAI, TargetValueExclusionType::Attacker);
    //End By leewheel
    // Unit* target = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    uint32 max_health = 0;
    Unit* result = nullptr;
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        //By leewheel 2026-07-26: 移植排除过滤。
        if (dynamicExclusions.find(guid) != dynamicExclusions.end())
            continue;
        //End By leewheel

        if (!bot->IsWithinCombatRange(unit, botAI->GetRange(range)))
            continue;

        if (unit->GetHealth() < max_health)
        {
            continue;
        }

        if (!botAI->HasAura(qualifier, unit, false, true))
        {
            max_health = unit->GetHealth();
            result = unit;
        }
    }

    return result;
}

Unit* MeleeAttackerWithoutAuraTargetValue::Calculate()
{
    GuidVector attackers = botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get();
    //By leewheel 2026-07-26: 移植动态排除(Attacker类型；休眠时为空集)。
    GuidSet const dynamicExclusions = GatherStrategyTargetExclusions(botAI, TargetValueExclusionType::Attacker);
    //End By leewheel
    // Unit* target = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    uint32 max_health = 0;
    Unit* result = nullptr;
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        //By leewheel 2026-07-26: 移植排除过滤。
        if (dynamicExclusions.find(guid) != dynamicExclusions.end())
            continue;
        //End By leewheel

        if (!bot->IsWithinMeleeRange(unit))
            continue;

        if (checkArc && !bot->HasInArc(CAST_ANGLE_IN_FRONT, unit))
            continue;

        if (unit->GetHealth() < max_health)
        {
            continue;
        }

        if (!botAI->HasAura(qualifier, unit, false, true))
        {
            max_health = unit->GetHealth();
            result = unit;
        }
    }

    return result;
}
