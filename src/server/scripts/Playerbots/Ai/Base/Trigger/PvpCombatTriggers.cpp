/* 副本机器人策略 */
//移植来源: 业务逻辑参考 Trickerer NPCBots (bot_ai.cpp CCed/BreakCC/爆发时机判定) 移植适配 Playerbots Trigger 框架
//业务对标: D:\WoWSourcedCode\AcoreSource\TrickererNbversion\AzerothCore-wotlk-with-NPCBots bot_ai.cpp
//By leewheel 2026-08-29 引入 NPCBots PVP 策略原理——控制/爆发/走位/保命触发条件
//End By leewheel
#include "PvpCombatTriggers.h"
#include "Playerbots.h"
#include "Player.h"
#include "Unit.h"

// 自己处于被控制状态(昏迷/恐惧/迷惑/定身/冰冻)
bool PvpBotCcedTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer())
        return false;

    // 定身/冰冻也算需要解控的情况(远程职业被定身时无法拉开距离)
    return bot->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING |
        UNIT_STATE_DISTRACTED | UNIT_STATE_ROOT) || bot->isFrozen();
}

// 当前目标正在读条施法 → 打断时机
bool PvpTargetCastingTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer() || !target->IsAlive())
        return false;

    // 目标在读条(非瞬发)且打断技能可用时触发
    return target->IsNonMeleeSpellCast(false) && !target->HasUnitState(UNIT_STATE_STUNNED);
}

// 敌方目标是治疗职业(牧师/圣骑士/萨满/德鲁伊)或正在施放治疗法术
bool PvpTargetIsHealerTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer() || !target->IsAlive())
        return false;

    Player* player = target->ToPlayer();
    switch (player->getClass())
    {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
            return true;
        default:
            return false;
    }
}

// 自己血量 ≤30% 且处于PVP战斗 → 保命/逃跑窗口
bool PvpBotLowHealthTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer())
        return false;

    return bot->GetHealthPct() <= 30.0f;
}

// 低血量(≤40%)且处于PVP战斗 → 控制对手→打绷带→逃跑窗口
//By leewheel 2026-08-29: 老大需求——低血量时先控制对手再打绷带回血, 控制技能CD中则逃跑
//By leewheel 2026-08-30: 强化——加入"打不过才跑"判定: 目标血量高于自己(硬拼会死)或自己被集火时才触发控制逃遁,
//  打得过则继续输出, 避免无谓逃跑
//End By leewheel
bool PvpLowHealthCCTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer())
        return false;

    // 自己血量高于 40% 不触发
    if (bot->GetHealthPct() > 40.0f)
        return false;

    // 打得过就不跑: 目标血量比自己还低 → 继续输出
    if (target->GetHealthPct() < bot->GetHealthPct())
        return false;

    // 被 ≥2 个敌方玩家围攻 → 必跑
    uint32 playerAttackers = 0;
    for (Unit* attacker : bot->getAttackers())
        if (attacker && attacker->IsPlayer() && attacker->IsAlive())
            ++playerAttackers;

    return playerAttackers >= 2 || target->GetHealthPct() > bot->GetHealthPct();
}
//End By leewheel

// 自己被 ≥2 个敌方玩家攻击(被集火) → 走位/保命
bool PvpBotFocusedTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    uint32 playerAttackers = 0;
    for (Unit* attacker : bot->getAttackers())
    {
        if (attacker && attacker->IsPlayer() && attacker->IsAlive())
            ++playerAttackers;
    }

    return playerAttackers >= 2;
}

// 远程职业被近战敌人贴身(≤5码) → 需要拉开距离
bool PvpKiteTargetTrigger::IsActive()
{
    // 只有远程职业才需要风筝
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !target->IsPlayer() || !target->IsAlive())
        return false;

    // 近战职业贴脸
    if (target->IsPlayer() && PlayerbotAI::IsMelee(target->ToPlayer()))
        return bot->GetDistance(target) <= 5.0f;

    return false;
}

// 发现敌方玩家在附近(视野范围) → 准备PVP
bool PvpEnemyPlayerSeenTrigger::IsActive()
{
    if (bot->IsInCombat())
        return false;

    // 复用已有的 "enemy player target" 值判断附近敌方玩家
    return AI_VALUE(Unit*, "enemy player target") != nullptr;
}

// 爆发技能可用(检测通用爆发类长CD技能冷却) → 触发爆发动作由职业策略执行
bool PvpBurstAvailableTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer() || !target->IsAlive())
        return false;

    // 由各职业策略通过 PvpBurstAvailable 值自行判定具体爆发技能,
    // 此触发器仅确认处于PVP战斗且目标存活
    return true;
}

// 血量 <50% 且战斗中没有治疗者 → 自疗
bool PvpNeedHealTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer())
        return false;

    return bot->GetHealthPct() < 50.0f;
}

// 血量 ≤15% 且仍在PVP战斗 → 逃跑脱离
bool PvpFleeTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    Unit* target = bot->GetVictim();
    if (!target || !target->IsPlayer())
        return false;

    return bot->GetHealthPct() <= 15.0f;
}

//By leewheel 2026-08-30: 强化控场循环——"控制→远遁→绷带→等CD→再来一轮"闭环的最后一环:
//  脱战后血量恢复到安全线以上、职业主控制技能冷却就绪、附近仍有敌方玩家时, 重新投入战斗
bool PvpReengageTrigger::IsActive()
{
    // 已脱战(逃跑/消失/假死后)才考虑重新进场
    if (bot->IsInCombat())
        return false;

    // 血量恢复到安全线(>70%)才回来, 避免残血送死
    if (bot->GetHealthPct() <= 70.0f)
        return false;

    // 附近必须有敌方玩家才值得回来
    Unit* enemy = AI_VALUE(Unit*, "enemy player target");
    if (!enemy || !enemy->IsAlive())
        return false;

    // 职业主控制技能冷却就绪才回来——保证回来就能控住对方再打一轮
    std::string controlSpell;
    switch (bot->getClass())
    {
        case CLASS_MAGE:      controlSpell = "polymorph"; break;
        case CLASS_ROGUE:     controlSpell = "kidney shot"; break;
        case CLASS_WARRIOR:   controlSpell = "intimidating shout"; break;
        case CLASS_PRIEST:    controlSpell = "psychic scream"; break;
        case CLASS_PALADIN:   controlSpell = "hammer of justice"; break;
        case CLASS_WARLOCK:   controlSpell = "fear"; break;
        case CLASS_HUNTER:    controlSpell = "wyvern sting"; break;
        case CLASS_SHAMAN:    controlSpell = "frost shock"; break;
        case CLASS_DRUID:     controlSpell = "cyclone"; break;
        case CLASS_DEATH_KNIGHT: controlSpell = "strangulate"; break;
        default:              return false;
    }

    if (controlSpell.empty())
        return false;

    // 控制技能CD就绪(未在CD)才触发重新进场
    return botAI->CanCastSpell(controlSpell, enemy);
}
//End By leewheel