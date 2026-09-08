/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AttackersValue.h"

#include "CellImpl.h"
#include "CombatManager.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Playerbots.h"
#include "ReputationMgr.h"
#include "ServerFacade.h"

GuidVector AttackersValue::Calculate()
{
    std::unordered_set<Unit*> targets;

    GuidVector result;
    if (!botAI->AllowActivity(ALL_ACTIVITY))
        return result;

    AddAttackersOf(bot, targets);

    if (Group* group = bot->GetGroup())
        AddAttackersOf(group, targets);

    RemoveNonThreating(targets);

    // prioritized target
    GuidVector prioritizedTargets = AI_VALUE(GuidVector, "prioritized targets");
    for (ObjectGuid target : prioritizedTargets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && IsValidTarget(unit, bot))
            targets.insert(unit);
    }
    if (Group* group = bot->GetGroup())
    {
        ObjectGuid skullGuid = group->GetTargetIcon(7);
        Unit* skullTarget = botAI->GetUnit(skullGuid);
        if (skullTarget && IsValidTarget(skullTarget, bot))
            targets.insert(skullTarget);
    }

    for (Unit* unit : targets)
        result.push_back(unit->GetGUID());

    if (bot->duel && bot->duel->Opponent)
        result.push_back(bot->duel->Opponent->GetGUID());

    // workaround for bots of same faction not fighting in arena
    if (bot->InArena())
    {
        GuidVector possibleTargets = AI_VALUE(GuidVector, "possible targets");
        for (ObjectGuid const guid : possibleTargets)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && unit->IsPlayer() && IsValidTarget(unit, bot))
                result.push_back(unit->GetGUID());
        }
    }

    return result;
}

void AttackersValue::AddAttackersOf(Group* group, std::unordered_set<Unit*>& targets)
{
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();

    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
    {
        Player* member = ObjectAccessor::FindPlayer(itr->guid);
        if (!member)
            continue;

        if (!member->IsAlive())
            continue;
        if (member == bot)
            continue;
        if (member->GetMapId() != bot->GetMapId())
            continue;
        float dist = ServerFacade::instance().GetDistance2d(bot, member);
        if (dist > sPlayerbotAIConfig.sightDistance)
            continue;

        AddAttackersOf(member, targets);
    }
}

struct AddGuardiansHelper
{
    explicit AddGuardiansHelper(std::vector<Unit*>& units) : units(units) {}

    void operator()(Unit* target) const { units.push_back(target); }

    std::vector<Unit*>& units;
};

void AttackersValue::AddAttackersOf(Player* player, std::unordered_set<Unit*>& targets)
{
    if (!player || !player->IsInWorld() || player->IsBeingTeleported())
        return;

    //By leewheel 2026-07-20: 使用TC的CombatManager API作为主要敌人来源
    //根因: TC中玩家EngageWithTarget时只调用SetInCombatWith()创建CombatReference
    //ThreatReference仅在生物端AddThreat时创建，再回填到玩家的_threatenedByMe
    //因此GetThreatenedByMeList()对玩家不可靠（CombatDiag证实: masterInCombat=true但attackers=0）
    //CombatManager.GetPvECombatRefs()在战斗开始时立即填充，是TC正确的"我在打谁"API
    for (auto const& [guid, ref] : player->GetCombatManager().GetPvECombatRefs())
    {
        if (!ref || ref->IsSuppressedFor(player))
            continue;

        Unit* enemy = ref->GetOther(player);
        if (!enemy)
            continue;

        if (player->GetMapId() != enemy->GetMapId() || !enemy->IsAlive() ||
            !enemy->IsInWorld() || !enemy->IsVisible() ||
            enemy->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE_2))
            continue;

        if (player->IsFriendlyTo(enemy))
            continue;

        float dist = player->GetDistance2d(enemy);
        if (dist >= sPlayerbotAIConfig.sightDistance)
            continue;

        targets.insert(enemy);
    }

    //PvP战斗引用（竞技场/战场中的敌对玩家）
    for (auto const& [guid, ref] : player->GetCombatManager().GetPvPCombatRefs())
    {
        if (!ref || ref->IsSuppressedFor(player))
            continue;

        Unit* enemy = ref->GetOther(player);
        if (!enemy)
            continue;

        if (player->GetMapId() != enemy->GetMapId() || !enemy->IsAlive() ||
            !enemy->IsInWorld() || !enemy->IsVisible())
            continue;

        if (player->IsFriendlyTo(enemy))
            continue;

        float dist = player->GetDistance2d(enemy);
        if (dist >= sPlayerbotAIConfig.sightDistance)
            continue;

        targets.insert(enemy);
    }
    //End By leewheel

    //By leewheel 2026-07-17: 威胁列表作为补充来源（覆盖CombatManager可能遗漏的情况）
    for (auto const& [guid, ref] : player->GetThreatMgr().GetThreatenedByMeList())
    {
        Unit* attacker = ref->GetOwner();
        if (!attacker)
            continue;

        if (player->GetMapId() != attacker->GetMapId() || !attacker->IsAlive() ||
            !attacker->IsInWorld() || !attacker->IsVisible() ||
            attacker->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE_2))
            continue;

        if (player->IsFriendlyTo(attacker))
            continue;

        float dist = player->GetDistance2d(attacker);
        if (dist >= sPlayerbotAIConfig.sightDistance)
            continue;

        targets.insert(attacker);
    }
    //End By leewheel
}

void AttackersValue::RemoveNonThreating(std::unordered_set<Unit*>& targets)
{
    //By leewheel 2026-07-20: 重写RemoveNonThreating，添加CombatManager旁路
    //根因: TC的威胁系统与Acore不同，hasRealThreat和IsValidTarget中的canAttack检查
    //都依赖威胁表数据，但TC中玩家EngageWithTarget只创建CombatReference不创建ThreatReference
    //修复: 如果队伍中有人正在与目标战斗(CombatManager.IsInCombatWith)，跳过严格检查
    for (std::unordered_set<Unit*>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        Unit* unit = *tIter;

        //基本有效性检查（始终执行，无论是否有CombatManager旁路）
        if (bot->GetMapId() != unit->GetMapId() || !unit->IsInWorld() || !unit->IsAlive())
        {
            std::unordered_set<Unit*>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
            continue;
        }

        //CombatManager旁路: 如果队伍中有人正在与此目标战斗，保留目标
        //只检查基本敌对关系，跳过canAttack/leaderHasThreat等依赖威胁表的检查
        bool groupInCombatWithTarget = false;
        if (bot->GetGroup())
        {
            for (GroupReference* gref = bot->GetGroup()->GetFirstMember(); gref; gref = gref->next())
            {
                Player* member = gref->GetSource();
                if (member && member != bot && member->IsInWorld() &&
                    member->GetCombatManager().IsInCombatWith(unit))
                {
                    groupInCombatWithTarget = true;
                    break;
                }
            }
        }

        if (groupInCombatWithTarget)
        {
            //By leewheel 2026-07-20: 队友正在打这个怪，只检查基本敌对关系
            //不使用CanSeeOrDetect: playerbot无真实客户端，TC可见性系统对bot不可靠
            //bot的移动AI会自行处理靠近和视野问题
            float distToTarget = bot->GetDistance2d(unit);
            if (!unit->IsFriendlyTo(bot) && distToTarget < 200.0f)
            {
                ++tIter;
                continue;
            }
            else
            {
                std::unordered_set<Unit*>::iterator tIter2 = tIter;
                ++tIter;
                targets.erase(tIter2);
                continue;
            }
            //End By leewheel
        }

        //原有严格检查（无CombatManager旁路时执行）
        if (!hasRealThreat(unit) || !IsValidTarget(unit, bot))
        {
            std::unordered_set<Unit*>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
        }
        else
            ++tIter;
    }
    //End By leewheel
}

bool AttackersValue::hasRealThreat(Unit* attacker)
{
    return attacker && attacker->IsInWorld() && attacker->IsAlive() && !attacker->IsPolymorphed() &&
           // !attacker->isInRoots() &&
           !attacker->IsFriendlyTo(bot);
}

bool AttackersValue::IsPossibleTarget(Unit* attacker, Player* bot, float /*range*/)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return false;

    // Basic check
    if (!attacker)
        return false;

    // bool inCannon = botAI->IsInVehicle(false, true);
    // bool enemy = botAI->GetAiObjectContext()->GetValue<Unit*>("enemy player target")->Get();

    // Validity checks
    if (!attacker->IsVisible() || !attacker->IsInWorld() || attacker->GetMapId() != bot->GetMapId())
        return false;

    if (attacker->isDead() || attacker->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
        return false;

    // Flag checks
    if (attacker->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE_2))
        return false;

    if (attacker->HasUnitFlag(UNIT_FLAG_IMMUNE_TO_PC) || attacker->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;

    // Skip targets that are immune to all damage (e.g., Ice Block, Divine Shield)
    if (attacker->IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL) &&
        attacker->IsImmunedToDamage(SPELL_SCHOOL_MASK_MAGIC))
        return false;

    // Relationship checks
    if (attacker->IsFriendlyTo(bot))
        return false;

    // Critter exception
    if (attacker->GetCreatureType() == CREATURE_TYPE_CRITTER && !attacker->IsInCombat())
        return false;

    //By leewheel 2026-07-20: 用IsWithinLOSInMap替代CanSeeOrDetect
    //playerbot无真实客户端，CanSeeOrDetect不可靠；IsWithinLOSInMap是服务端LOS检查
    if (!bot->IsWithinLOSInMap(attacker))
        return false;
    //End By leewheel

    // PvP prohibition checks (skip for duels)
    if ((attacker->GetGUID().IsPlayer() || attacker->GetGUID().IsPet()) &&
        (!bot->duel || bot->duel->Opponent != attacker) &&
        (sPlayerbotAIConfig.IsPvpProhibited(attacker->GetZoneId(), attacker->GetAreaId()) ||
        sPlayerbotAIConfig.IsPvpProhibited(bot->GetZoneId(), bot->GetAreaId())))
    {
        // This will stop aggresive pets from starting an attack.
        // This will stop currently attacking pets from continuing their attack.
        // This will first require the bot to change from a combat strat. It will
        // not be reached if the bot only switches targets, including NPC targets.
        for (Unit::ControlSet::const_iterator itr = bot->m_Controlled.begin();
            itr != bot->m_Controlled.end(); ++itr)
        {
            Creature* creature = dynamic_cast<Creature*>(*itr);
            if (creature && creature->GetVictim() == attacker)
            {
                creature->AttackStop();
                if (CharmInfo* charmInfo = creature->GetCharmInfo())
                    charmInfo->SetIsCommandAttack(false);
            }
        }

        return false;
    }

    // Unflagged player check
    if (attacker->IsPlayer() && !attacker->IsPvP() && !attacker->IsFFAPvP() &&
        (!bot->duel || bot->duel->Opponent != attacker))
        return false;

    // Creature-specific checks
    Creature* c = attacker->ToCreature();
    if (c)
    {
        if (c->IsInEvadeMode())
            return false;

        bool leaderHasThreat = false;
        if (bot->GetGroup() && botAI->GetMaster())
            leaderHasThreat = attacker->GetThreatMgr().GetThreat(botAI->GetMaster());

        // A player claims a creature by damaging it or landing a hostile spell on it, which is what
        // sets its loot recipient. The below code covers anti-kill-stealing mechanics.
        //
        // (1) Whatever the claim, the bot may attack if it (a) is in a raid/group and has a master
        //     holding nonzero threat on the creature, (b) has, or has a raid/group member that has,
        //     already tapped it, or (c) is already in combat with the creature.
        if (leaderHasThreat || c->isTappedBy(bot) || c->IsInCombatWith(bot))
            return true;

        // (2) Nobody has claimed the creature, so ask who it is attacking. If it is not attacking
        //     anything, or if it is attacking (a) something with no player behind it (e.g., a
        //     critter), (b) the bot, (c) the bot's master, or (d) a member of the bot's raid/group,
        //     then the victim is considered to belong to the bot and may be attacked. Clauses (b)
        //     through (d) also include a player's pets, guardians, totems, and charms.
        if (!c->hasLootRecipient())
        {
            Unit* victim = c->GetVictim();
            Player* victimOwner = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!victim || !victimOwner || victimOwner == bot || victimOwner == botAI->GetMaster() ||
                (bot->GetGroup() && bot->GetGroup() == victimOwner->GetGroup()))
            {
                return true;
            }
        }

        // (3) Last because it is applied automatically only in battlegrounds and arenas: the
        //     "attack tagged" strategy always allows for attacking of creatures.
        return botAI->HasStrategy("attack tagged", BOT_STATE_NON_COMBAT);
    }

    return true;
}

bool AttackersValue::IsValidTarget(Unit* attacker, Player* bot)
{
    return IsPossibleTarget(attacker, bot) && bot->IsWithinLOSInMap(attacker);
}

bool PossibleAddsValue::Calculate()
{
    GuidVector possible = botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get();
    GuidVector attackers = botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get();

    for (ObjectGuid const guid : possible)
    {
        if (find(attackers.begin(), attackers.end(), guid) != attackers.end())
            continue;
        Unit* add = botAI->GetUnit(guid);
        if (!add || !add->IsInWorld() || add->IsDuringRemoveFromWorld())
            continue;

        if (!add->GetTarget() && !add->GetThreatMgr().GetLastVictim() && add->IsHostileTo(bot))
        {
            for (ObjectGuid const attackerGUID : attackers)
            {
                Unit* attacker = botAI->GetUnit(attackerGUID);
                if (!attacker)
                    continue;

                float dist = ServerFacade::instance().GetDistance2d(attacker, add);
                if (ServerFacade::instance().IsDistanceLessOrEqualThan(dist, sPlayerbotAIConfig.aoeRadius * 1.5f))
                    continue;

                if (ServerFacade::instance().IsDistanceLessOrEqualThan(dist, sPlayerbotAIConfig.aggroDistance))
                    return true;
            }
        }
    }

    return false;
}
