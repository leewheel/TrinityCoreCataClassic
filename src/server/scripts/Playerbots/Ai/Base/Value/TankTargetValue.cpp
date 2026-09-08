/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TankTargetValue.h"

#include "AiObjectContext.h"
#include "AttackersValue.h"
#include "Group.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
//By leewheel 2026-07-26: 引入Strategy.h以使用TargetValueExclusionType。
#include "Strategy.h"
//End By leewheel

class FindTargetForTankStrategy : public FindNonCcTargetStrategy
{
public:
    FindTargetForTankStrategy(PlayerbotAI* botAI) : FindNonCcTargetStrategy(botAI), minThreat(0) {}

    void CheckAttacker(Unit* creature, ThreatManager* threatMgr) override
    {
        if (!creature || !creature->IsAlive())
            return;

        Player* bot = botAI->GetBot();
        float threat = threatMgr->GetThreat(bot);
        if (!result)
        {
            minThreat = threat;
            result = creature;
        }
        // neglect if victim is main tank, or no victim (for untauntable target)
        if (Unit* victim = threatMgr->GetCurrentVictim())
        {
            if (victim->ToPlayer() && botAI->IsMainTank(victim->ToPlayer()))
                return;
        }
        if (minThreat >= threat)
        {
            minThreat = threat;
            result = creature;
        }
    }

protected:
    float minThreat;
};

class FindTankTargetSmartStrategy : public FindTargetStrategy
{
public:
    FindTankTargetSmartStrategy(PlayerbotAI* botAI) : FindTargetStrategy(botAI) {}

    //By leewheel 2026-07-26: 移植排除类型(Tank)。
    TargetValueExclusionType GetExclusionType() override { return TargetValueExclusionType::Tank; }
    //End By leewheel

    void CheckAttacker(Unit* attacker, ThreatManager* /*threatMgr*/) override
    {
        if (Group* group = botAI->GetBot()->GetGroup())
        {
            ObjectGuid guid = group->GetTargetIcon(4);
            if (guid && attacker->GetGUID() == guid)
                return;
        }
        if (!attacker->IsAlive())
            return;

        if (!result || IsBetter(attacker, result))
            result = attacker;
    }
    bool IsBetter(Unit* new_unit, Unit* old_unit)
    {
        Player* bot = botAI->GetBot();
        // if group has multiple tanks, explicit main tank just focus on the current target
        Unit* currentTarget = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
        if (currentTarget && botAI->IsExplicitMainTank(bot) && botAI->GetGroupTankNum(bot) > 1)
        {
            if (old_unit == currentTarget)
                return false;

            if (new_unit == currentTarget)
                return true;
        }
        float new_threat = new_unit->GetThreatMgr().GetThreat(bot);
        float old_threat = old_unit->GetThreatMgr().GetThreat(bot);
        float new_dis = bot->GetDistance(new_unit);
        float old_dis = bot->GetDistance(old_unit);
        // hasAggro? -> withinMelee? -> threat
        if (GetIntervalLevel(new_unit) != GetIntervalLevel(old_unit))
            return GetIntervalLevel(new_unit) > GetIntervalLevel(old_unit);

        int32_t interval = GetIntervalLevel(new_unit);
        if (interval == 2)
            return new_dis < old_dis;

        return new_threat < old_threat;
    }
    int32_t GetIntervalLevel(Unit* unit)
    {
        if (!botAI->HasAggro(unit))
            return 2;

        if (botAI->GetBot()->IsWithinMeleeRange(unit))
            return 1;

        return 0;
    }
};

Unit* TankTargetValue::Calculate()
{
    std::string const rti = botAI->GetAiObjectContext()->GetValue<std::string>("rti")->Get();
    Unit* rtiTarget = RtiTargetValue::Calculate();
    if (rtiTarget)
    {
        //By leewheel 2026-08-07: 开怪瞬间(怪物还在路上、尚未攻击任何人)坦克应立即以RTI目标为接怪目标，
        //否则必须等怪物攻击某人才有目标，导致坦克发呆数秒、DPS仇恨过高。
        //原逻辑仅当rti目标已攻击非坦克成员时才返回，此时怪物可能已被DPS抢仇恨。
        //用IsPossibleTarget(不要求目标已进入战斗)而非IsValidAttackTarget。
        if (rtiTarget->IsAlive() && AttackersValue::IsPossibleTarget(rtiTarget, bot))
            return rtiTarget;
        //End By leewheel

        Unit* victim = rtiTarget->GetVictim();

        if (victim && victim != bot)
        {
            if (Player* victimPlayer = victim->ToPlayer())
            {
                // rti target is attacking a non-tank player
                if (!PlayerbotAI::IsTank(victimPlayer))
                    return rtiTarget;
                // rti target is attacking a tank player, check if the tank is a bot and has the same rti setting
                PlayerbotAI* victimBotAI = GET_PLAYERBOT_AI(victimPlayer);
                if (!victimBotAI || victimBotAI->GetAiObjectContext()->GetValue<std::string>("rti")->Get() != rti)
                    return rtiTarget;
            }
        }
    }

        // FindTargetForTankStrategy strategy(botAI);
    //By leewheel 2026-08-07: 无RTI标记或RTI不可接时，回退到真人队长的选中目标(玩家直接点怪拉怪)。
    //By leewheel 2026-08-09: 严重修复——玩家仅"选中"目标(未攻击/未进战斗)时, 不得作为坦克目标。
    //必须玩家实际攻击(进战斗且玩家在目标攻击者列表)才允许, 否则坦克只打RTI标记(明确指令)
    if (!rtiTarget)
    {
        if (Player* master = botAI->GetMaster())
        {
            if (master->IsInCombat() && !master->GetTarget().IsEmpty())
            {
                ObjectGuid masterTargetGuid = master->GetTarget();
                Unit* masterTarget = botAI->GetUnit(masterTargetGuid);
                if (masterTarget && masterTarget->IsAlive() && masterTarget != bot &&
                    AttackersValue::IsPossibleTarget(masterTarget, bot))
                {
                    // 玩家是否实际在攻击该目标(TC CombatManager, 与 AttackersValue 一致)
                    for (auto const& [guid, ref] : master->GetCombatManager().GetPvECombatRefs())
                    {
                        if (!ref || ref->IsSuppressedFor(master))
                            continue;
                        if (ref->GetOther(master) == masterTarget)
                            return masterTarget;
                    }
                }
            }
        }
    }
    //End By leewheel
    FindTankTargetSmartStrategy strategy(botAI);
    return FindTarget(&strategy);
}
