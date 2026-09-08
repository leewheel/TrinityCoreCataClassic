/*
 * 幽暗沼泽 机器人策略
 */

#include "UBMultipliers.h"
#include "AttackAction.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"
#include "UBActions.h"
#include "UBShared.h"

using namespace UnderbogHungarfen;

float HungarfenFoulSporesMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "17770");
    if (!boss || !boss->HasAura(SPELL_FOUL_SPORES))
        return 1.0f;

    if (dynamic_cast<UBRetreatFromFoulSporesAction*>(action) || dynamic_cast<UBVacateSporeCloudAction*>(action) ||
        dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<MovementAction*>(action) || dynamic_cast<CastReachTargetSpellAction*>(action))
        return 0.0f;

    return 1.0f;
}

float HungarfenMushroomIgnoreMultiplier::GetValue(Action* action)
{
    //By leewheel 2026-09-04: 恢复上游三分支语义——
    //①AnyMushroomAlive门控: 无活蘑菇时放行一切(原写法蘑菇清完后DPS的AoE持续被压0瘫痪);
    //②非AoE的AttackAnythingAction也纳入抑制(grind目标选择类攻击);
    //③grind target恰为活蘑菇时压制攻击动作(IsMushroom/AnyMushroomAlive本地UBShared已实现但此前未被调用)
    bool const aoe = action->getThreatType() == Action::ActionThreatType::Aoe;
    if (!aoe && !dynamic_cast<AttackAnythingAction*>(action))
        return 1.0f;

    if (aoe && (dynamic_cast<CastHealingSpellAction*>(action) || !PlayerbotAI::IsDps(bot)))
        return 1.0f;

    auto const& mushrooms = AI_VALUE_REF(GuidVector, "ub mushrooms");
    if (!AnyMushroomAlive(bot, mushrooms))
        return 1.0f;

    if (!aoe)
        return IsMushroom(AI_VALUE(Unit*, "grind target")) ? 0.0f : 1.0f;

    return 0.0f;
}

//By leewheel 2026-09-04: 补回UnderbatFacingMultiplier::GetValue实现——
//类声明与策略注册均在但实现缺失, worldserver链接阶段必报unresolved external(移植遗漏);
//语义: 幽暗蝙蝠进入鞭击范围时压制SetBehindTargetAction(转身背对蝙蝠会挨鞭击)
float UnderbatFacingMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    auto const& attackers = AI_VALUE_REF(GuidVector, "attackers");
    return AnyUnderbatInLashRange(bot, attackers) ? 0.0f : 1.0f;
}
