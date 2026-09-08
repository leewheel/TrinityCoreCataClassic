/* 副本机器人策略 */
//移植来源: AC mod-playerbots GruulMultipliers.cpp 移植适配 TC 框架
//业务对标: AC azerothcore-wotlk mod-playerbots src/Ai/Raid/Gruul/GruulMultipliers.cpp
//By leewheel 2026-08-29 引入 mod-playerbots 新提交: Gruul 策略更新——
//  限制嘲讽/HoldWhileSnared/破碎后保持分散等新乘数器
//End By leewheel
#include "GruulMultipliers.h"
#include "ChooseTargetActions.h"
#include "EncounterHelpers.h"
#include "GruulActions.h"
#include "GruulHelpers.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"
#include "ShamanActions.h"

using namespace GruulHelpers;
using namespace EncounterHelpers;

//By leewheel 2026-09-04: 上游8baf63da——全部乘数实现改 GetValueInEncounter(中间基类已闸门副本战斗判定)

// General

float GruulsLairDelayDpsCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* gruul = AI_VALUE2(Unit*, "find target", "19044");
    if (gruul && gruul->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
        return 0.0f;

    Unit* blindeye = AI_VALUE2(Unit*, "find target", "18836");
    return blindeye && blindeye->GetHealthPct() > BLINDEYE_ENGAGED_HEALTH_PCT ? 0.0f : 1.0f;
}

// High King Maulgar <Lord of the Ogres>

float HighKingMaulgarControlTankActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<TankAssistAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action))
    {
        return 1.0f;
    }

    return AI_VALUE2(Unit*, "find target", "18831") ? 0.0f : 1.0f;
}

float HighKingMaulgarRestrictTauntingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    bool const isAoeThreat = IsAoeThreatAction(bot, action);
    if (!isAoeThreat && !IsTauntAction(bot, action))
        return 1.0f;

    // 主坦全程拉着玛尔加, 可以随意行动
    if (PlayerbotAI::IsMainTank(bot))
        return 1.0f;

    // 盲眼与奥尔由不同坦克相邻拉怪; 盲眼未死前不要使用AOE仇恨技能
    if (isAoeThreat && AI_VALUE2(Unit*, "find target", "18836"))
        return 0.0f;

    // 奇格勒是唯一嘲讽有问题的食人魔, 因为他是唯一(1)由非传统坦克拉怪
    // 且(2)被传统坦克(盲眼与奥尔倒下后)指定攻击的目标
    Unit* kiggler = AI_VALUE2(Unit*, "find target", "18835");
    if (!kiggler)
        return 1.0f;

    if (!GetKigglerMoonkinTank(bot))
        return 1.0f;

    return AI_VALUE(Unit*, "current target") == kiggler ? 0.0f : 1.0f;
}

float HighKingMaulgarDisableDpsAssistMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "18831") ? 0.0f : 1.0f;
}

float HighKingMaulgarAvoidWhirlwindMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<HighKingMaulgarRunAwayFromWhirlwindAction*>(action))
        return 1.0f;

    Unit* maulgar = AI_VALUE2(Unit*, "find target", "18831");
    if (!maulgar || !maulgar->HasAura(Id(GruulSpells::SPELL_WHIRLWIND)))
        return 1.0f;

    if (PlayerbotAI::IsMainTank(bot))
        return 1.0f;

    //By leewheel 2026-09-04: 上游8baf63da——驻留距离常量化(15y=逃跑12y+3y余量), 距离判定改精确
    return bot->GetExactDist2d(maulgar) < MAULGAR_WHIRLWIND_HOLD_DISTANCE ? 0.0f : 1.0f;
    //End By leewheel
}

float HighKingMaulgarControlHunterActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    bool const isMainTankMisdirect = dynamic_cast<CastMisdirectionOnMainTankAction*>(action);
    if (!isMainTankMisdirect && !dynamic_cast<CastArcaneShotAction*>(action))
        return 1.0f;

    // Krosh/Kiggler will be the last to die before Maulgar
    // When only Maulgar is left, the standard Misdirection strategy is fine
    if (isMainTankMisdirect &&
        ((AI_VALUE2(Unit*, "find target", "18832")) ||
         (AI_VALUE2(Unit*, "find target", "18835"))))
    {
        return 0.0f;
    }

    // Arcane Shot removes Spell Shield, which the mage tank needs to survive
    Unit* krosh = AI_VALUE2(Unit*, "find target", "18832");
    return krosh && action->GetTarget() == krosh ? 0.0f : 1.0f;
}

float HighKingMaulgarControlMageTankActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_MAGE)
        return 1.0f;

    if (action->getThreatType() != Action::ActionThreatType::Aoe &&
        !dynamic_cast<CastIceBlockAction*>(action) &&
        !dynamic_cast<CastInvisibilityAction*>(action))
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "18832"))
        return 1.0f;

    return GetKroshMageTank(bot) == bot ? 0.0f : 1.0f;
}

// Gruul the Dragonkiller

float GruulTheDragonkillerControlTankMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<AvoidAoeAction*>(action))
    {
        return 1.0f;
    }

    Unit* gruul = AI_VALUE2(Unit*, "find target", "19044");
    return gruul && gruul->GetVictim() == bot ? 0.0f : 1.0f;
}

float GruulTheDragonkillerStaySpreadForShatterMultiplier::GetValueInEncounter(Action* action)
{
    if (!HasGroundSlam(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    return dynamic_cast<GruulTheDragonkillerShatterSpreadAction*>(action) ? 1.0f : 0.0f;
}

// MoveTo does not check speed, and thus even with a snare of -100% or more, it starts a spline
// and calculates IsWaitingForLastMove from distance / speed, which is infinite in that case and
// clamps to MaxWaitForMove (5s), blocking all movements for that duration. This multiplier is
// needed to solve the issue for Gruul because the snare he applies (Gronn Lord's Grasp) persists
// 300ms beyond the Shatter sequence, meaning that bots would otherwise be unable to move for 5s
// after the Shatter sequence, even though no in-game factors would prevent their movement.
float GruulTheDragonkillerHoldWhileSnaredMultiplier::GetValueInEncounter(Action* action)
{
    if (bot->GetSpeed(MOVE_RUN) > 0.0f)
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "19044"))
        return 1.0f;

    return dynamic_cast<MovementAction*>(action) ? 0.0f : 1.0f;
}