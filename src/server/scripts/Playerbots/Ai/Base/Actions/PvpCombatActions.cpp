/* 副本机器人策略 */
//移植来源: 业务逻辑参考 Trickerer NPCBots (bot_ai.cpp BreakCC/PvP 战斗循环) 移植适配 Playerbots Action 框架
//业务对标: D:\WoWSourcedCode\AcoreSource\TrickererNbversion\AzerothCore-wotlk-with-NPCBots bot_ai.cpp
//By leewheel 2026-08-29 引入 NPCBots PVP 策略原理——解控/打断/风筝/逃跑动作
//End By leewheel
#include "PvpCombatActions.h"
#include "GenericSpellActions.h"
#include "Playerbots.h"
#include "Player.h"
#include "Unit.h"

// PVP 徽章解控: 复用 UseTrinketAction 触发装备栏徽章(如 PvP 徽章移除控制效果)
bool PvpBreakCrowdControlAction::Execute(Event event)
{
    // 直接执行 "use trinket" 动作逻辑, 由 UseTrinket 判断物品 CD 与可用性
    UseTrinketAction useTrinket(botAI);
    return useTrinket.Execute(event);
}

bool PvpBreakCrowdControlAction::isUseful()
{
    // 只有处于控制状态才需要解控
    return bot->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING |
        UNIT_STATE_DISTRACTED | UNIT_STATE_ROOT) || bot->isFrozen();
}

// 打断当前目标施法: 各职业打断技能名称不同, 按职业选择
bool PvpInterruptCastAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer() || !target->IsNonMeleeSpellCast(false))
        return false;

    std::string interruptSpell;
    switch (bot->getClass())
    {
        case CLASS_WARRIOR:     interruptSpell = "pummel"; break;
        case CLASS_PALADIN:     interruptSpell = "hammer of justice"; break;
        case CLASS_ROGUE:       interruptSpell = "kick"; break;
        case CLASS_PRIEST:      interruptSpell = "silence"; break;
        case CLASS_MAGE:        interruptSpell = "counterspell"; break;
        case CLASS_WARLOCK:     interruptSpell = "spell lock"; break;
        case CLASS_DRUID:       interruptSpell = "bash"; break;
        case CLASS_SHAMAN:      interruptSpell = "wind shear"; break;
        case CLASS_DEATH_KNIGHT: interruptSpell = "mind freeze"; break;
        case CLASS_HUNTER:      interruptSpell = "silencing shot"; break;
        default:                return false;
    }

    if (interruptSpell.empty())
        return false;

    if (botAI->CanCastSpell(interruptSpell, target))
        return botAI->CastSpell(interruptSpell, target);

    return false;
}

bool PvpInterruptCastAction::isUseful()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer())
        return false;

    // 目标在读条且未被控制时才需要打断
    return target->IsNonMeleeSpellCast(false) && !target->HasUnitState(UNIT_STATE_STUNNED);
}

// 风筝: 远程职业被近战贴脸时拉开距离
bool PvpKiteAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer())
        return false;

    // 先尝试施法减速/定身(如有瞬发控制), 再向后移动
    bot->CastStop();
    return MoveAway(target, sPlayerbotAIConfig.fleeDistance, true);
}

bool PvpKiteAction::isUseful()
{
    // 远程职业且近战敌人贴脸时才风筝
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer() || !target->IsAlive())
        return false;

    return target->IsPlayer() && PlayerbotAI::IsMelee(target->ToPlayer()) &&
        bot->GetDistance(target) <= 5.0f;
}

// 血量过低逃跑: 复用 FleeAction 的逃跑逻辑, 但只针对 PVP 战斗
bool PvpFleeAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && target->IsPlayer() && bot->GetHealthPct() <= 15.0f)
    {
        // 尝试移除减速/控制后向远离目标方向逃跑
        bot->CastStop();
        return MoveAway(target, sPlayerbotAIConfig.fleeDistance, true);
    }

    return FleeAction::Execute(event);
}

bool PvpFleeAction::isUseful()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && target->IsPlayer() && bot->GetHealthPct() <= 15.0f)
        return true;

    return FleeAction::isUseful();
}

// 打绷带回血: 敌人被控制或距离远时打绷带, 防止引导被中断
//By leewheel 2026-08-29: 老大需求——低血量PVP时先控制对手再打绷带回血
bool PvpBandageAction::Execute(Event /*event*/)
{
    Item* bandage = botAI->FindBandage();
    if (!bandage)
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "use_item_none_available", "没有可用的物品(或游戏对象)", {}));
        return false;
    }

    return UseItemAuto(bandage);
}

bool PvpBandageAction::isUseful()
{
    // 血量较高时不需要绷带
    if (bot->GetHealthPct() > 80.0f)
        return false;

    // 背包里没有绷带
    if (!botAI->FindBandage())
        return false;

    // 战斗中被攻击会打断绷带引导: 敌人被控制或距离较远才安全
    if (bot->IsInCombat())
    {
        Unit* target = bot->GetVictim();
        if (target && target->IsPlayer() && target->IsAlive())
        {
            bool targetCced = target->HasBreakableByDamageCrowdControlAura() ||
                target->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING) ||
                target->isFrozen() || target->IsCharmed();
            if (!targetCced && bot->GetDistance(target) <= 10.0f)
                return false;
        }
    }

    return true;
}
//End By leewheel