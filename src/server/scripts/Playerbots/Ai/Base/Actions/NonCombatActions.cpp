/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "NonCombatActions.h"

#include "Event.h"
#include "Playerbots.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
//By leewheel 2026-08-18: 移植 brighton-chi the-lab 5ad5bb03(修复吃喝延迟#2607)所需头文件
#include <cmath>
//End By leewheel

namespace
{
constexpr uint32 BG_WS_SPELL_WARSONG_FLAG = 23333;
constexpr uint32 BG_WS_SPELL_SILVERWING_FLAG = 23335;
constexpr uint32 BG_EY_NETHERSTORM_FLAG_SPELL = 34976;

bool IsDisallowedShapeshiftForm(Player* bot)
{
    if (bot->getClass() == CLASS_DRUID)
    {
        ShapeshiftForm form = bot->GetShapeshiftForm();
        return form == FORM_TRAVEL || form == FORM_AQUA ||
               form == FORM_FLIGHT || form == FORM_FLIGHT_EPIC ||
               form == FORM_BEAR || form == FORM_DIREBEAR ||
               form == FORM_CAT;
    }
    else if (bot->getClass() == CLASS_PRIEST)
    {
        return bot->GetShapeshiftForm() == FORM_SPIRITOFREDEMPTION;
    }

    return false;
}

//By leewheel 2026-09-02: 硬编码喝水/进食技能链（扫描全法术库会误分类智慧祝福、激活、绷带等）
// 喝水技能链：5级/15级/25级/35级/45级/55级/65级/75级
static uint32 const DrinkChain[] = { 430, 431, 432, 1133, 1135, 1137, 10250, 43706 };
static uint32 const FoodChain[]  = { 433, 434, 435, 1127, 1129, 1131, 28616, 33255 };
static uint32 GetEatDrinkSpellForLevel(uint8 level, bool drink)
{
    uint32 const* chain = drink ? DrinkChain : FoodChain;
    constexpr uint8 chainSize = 8;
    uint32 best = 0;
    // 5级/15级/25级/35级/45级/55级/65级/75级
    uint8 const levelThresholds[chainSize] = { 5, 15, 25, 35, 45, 55, 65, 75 };
    for (uint8 i = 0; i < chainSize; ++i)
    {
        if (levelThresholds[i] <= level)
            best = chain[i];
        else
            break;
    }
    return best;
}
//End By leewheel
}

bool DrinkAction::Execute(Event event)
{
    //By leewheel 2026-09-02: 修正——不依赖 food cheat，无条件直接使用最高等级喝水技能坐下恢复
    bot->ClearUnitState(UNIT_STATE_CHASE);
    bot->ClearUnitState(UNIT_STATE_FOLLOW);

    if (bot->isMoving())
    {
        bot->GetMotionMaster()->Clear();
        bot->StopMoving();
    }
    bot->SetStandState(UNIT_STAND_STATE_SIT);
    bot->CastStop();

    // 按回复光环整tick等待(每2秒一跳)
    //By leewheel 2026-09-03 修复C5055警告：IN_MILLISECONDS为枚举常量，与float相乘已弃用，显式转float
    float delay = std::max(1.0f, std::ceil((100.0f - bot->GetPowerPct(POWER_MANA)) / 5.0f)) * 2 * float(IN_MILLISECONDS);
    //End By leewheel
    botAI->SetNextCheckDelay(delay);

    // 按等级直接施放最高喝水技能
    uint32 drinkSpell = GetEatDrinkSpellForLevel(bot->GetLevel(), true);
    if (drinkSpell)
    {
        if (!bot->HasSpell(drinkSpell))
            bot->learnSpell(drinkSpell, false);
        if (!bot->HasAura(drinkSpell))
        {
            if (!botAI->CastSpell(drinkSpell, bot))
                bot->AddAura(drinkSpell, bot);  // 施放被拒时兜底直接附加喝水光环
        }
        return true;
    }
    //End By leewheel

    // 降级：使用背包物品（原逻辑保留）
    if (botAI->HasCheat(BotCheatMask::food))
    {
        bot->AddAura(25990, bot);
        return true;
    }

    return UseItemAction::Execute(event);
}

bool DrinkAction::isUseful()
{
    return UseItemAction::isUseful() && AI_VALUE2(bool, "has mana", "self target") &&
           AI_VALUE2(uint8, "mana", "self target") < 100;
}

bool DrinkAction::isPossible()
{
    if (bot->IsInCombat() || bot->IsMounted() || IsDisallowedShapeshiftForm(bot))
        return false;

    if (bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG) ||
        bot->HasAura(BG_EY_NETHERSTORM_FLAG_SPELL))
    {
        return false;
    }

    //By leewheel 2026-09-02: 修复——无条件可喝水(Execute 直接使用最高等级喝水技能,不再依赖 food cheat 或背包物品)
    return true;
}

bool EatAction::Execute(Event event)
{
    if (botAI->HasCheat(BotCheatMask::food))
    {
        // if (bot->IsNonMeleeSpellCast(true))
        //     return false;

        bot->ClearUnitState(UNIT_STATE_CHASE);
        bot->ClearUnitState(UNIT_STATE_FOLLOW);

        if (bot->isMoving())
        {
            //By leewheel 2026-08-18: 移植 brighton-chi the-lab 5ad5bb03——先清空移动生成器再停走,避免坐姿光环被打断(TC适配: Clear无参数)
            bot->GetMotionMaster()->Clear();
            //End By leewheel
            bot->StopMoving();
        }

        bot->SetStandState(UNIT_STAND_STATE_SIT);
        bot->CastStop();

        //By leewheel 2026-08-18: 移植 brighton-chi the-lab 5ad5bb03——按回复光环整tick等待(每2秒一跳)
        //By leewheel 2026-09-03 修复C5055警告：IN_MILLISECONDS为枚举常量，与float相乘已弃用，显式转float
        float delay = std::max(1.0f, std::ceil((100.0f - bot->GetHealthPct()) / 5.0f)) * 2 * float(IN_MILLISECONDS);
        //End By leewheel

        botAI->SetNextCheckDelay(delay);

        //By leewheel 2026-08-18: 按等级直接学会并施放进食技能（回血）
        uint32 foodSpell = GetEatDrinkSpellForLevel(bot->GetLevel(), false);
        if (foodSpell)
        {
            if (!bot->HasSpell(foodSpell))
                bot->learnSpell(foodSpell, false);
            if (!bot->HasAura(foodSpell))
            {
                if (!botAI->CastSpell(foodSpell, bot))
                    bot->AddAura(foodSpell, bot);  // 施放被拒时兜底直接附加进食光环
            }
            return true;
        }
        //End By leewheel

        bot->AddAura(25990, bot);
        return true;
    }

    return UseItemAction::Execute(event);
}

bool EatAction::isUseful() { return UseItemAction::isUseful() && AI_VALUE2(uint8, "health", "self target") < 100; }

bool EatAction::isPossible()
{
    if (bot->IsInCombat() || bot->IsMounted() || IsDisallowedShapeshiftForm(bot))
        return false;

    if (bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG) ||
        bot->HasAura(BG_EY_NETHERSTORM_FLAG_SPELL))
    {
        return false;
    }

    return botAI->HasCheat(BotCheatMask::food) || UseItemAction::isPossible();
}
