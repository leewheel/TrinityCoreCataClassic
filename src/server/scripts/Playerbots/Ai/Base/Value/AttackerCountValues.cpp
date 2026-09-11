/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AttackerCountValues.h"

#include "Playerbots.h"
#include "SharedDefines.h"

uint8 MyAttackerCountValue::Calculate() { return bot->getAttackers().size(); }

bool HasAggroValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return true;
    }
    Unit* victim = target->GetVictim();
    if (!victim)
    {
        //By leewheel 2026-08-07: 目标无当前仇恨目标(开怪瞬间怪物刚被拉、尚未攻击任何人)时，
        //原逻辑一律返回true(视为"已有仇恨")，导致坦克的lose aggro→taunt触发器不激活，
        //坦克开怪后不嘲讽、仇恨建立极慢、DPS先手抢仇。
        //改为：bot在目标仇恨列表中有实际仇恨才算有aggro；开怪瞬间bot尚无仇恨→返回false→坦克主动嘲讽。
        return target->GetThreatManager().GetThreat(bot) > 0.0f;
    }
    bool isMT = botAI->IsExplicitMainTank(bot);
    if (victim &&
        (victim->GetGUID() == bot->GetGUID() || (!isMT && victim->ToPlayer() && botAI->IsTank(victim->ToPlayer()))))
    {
        return true;
    }
    return false;
}

uint8 AttackerCountValue::Calculate()
{
    uint32 count = 0;
    float range = sPlayerbotAIConfig.sightDistance;

    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        float distance = bot->GetDistance(unit);
        if (distance <= range)
            ++count;
    }

    return count;
}

uint8 BalancePercentValue::Calculate()
{
    float playerLevel = 0, attackerLevel = 0;

    if (Group* group = bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* player = ObjectAccessor::FindPlayer(itr->guid);
            if (!player || !player->IsAlive())
                continue;

            playerLevel += player->GetLevel();
        }
        uint32 memberCount = group->GetMembersCount();
        playerLevel /= memberCount;
        if (memberCount <= 10)
            playerLevel *= memberCount;
        else
            playerLevel *= 10;
    }
    GuidVector v = context->GetValue<GuidVector>("attackers")->Get();
    for (ObjectGuid const guid : v)
    {
        Creature* creature = botAI->GetCreature((guid));
        if (!creature || !creature->IsAlive())
            continue;

        uint32 level = creature->GetLevel();

        //By leewheel 2026-09-06: 移植到TrinityCore-Cata，rank字段更名为Classification
        switch (creature->GetCreatureTemplate()->Classification)
        {
            case CREATURE_ELITE_RARE:
                level *= 2;
                break;
            case CREATURE_ELITE_ELITE:
                level *= 3;
                break;
            case CREATURE_ELITE_RAREELITE:
                level *= 3;
                break;
            case CREATURE_ELITE_WORLDBOSS:
                level *= 20;
                break;
        }

        attackerLevel += level;
    }

    if (!attackerLevel)
        return 100;

    float percent = playerLevel * 100 / attackerLevel;
    return percent <= 200 ? (uint8)percent : 200;
}

Unit* AttackerCountValue::GetTarget()
{
    AiObjectContext* ctx = AiObject::context;
    return ctx->GetValue<Unit*>(qualifier)->Get();
}

Unit* MyAttackerCountValue::GetTarget()
{
    AiObjectContext* ctx = AiObject::context;
    return ctx->GetValue<Unit*>(qualifier)->Get();
}

Unit* HasAggroValue::GetTarget()
{
    AiObjectContext* ctx = AiObject::context;
    return ctx->GetValue<Unit*>(qualifier)->Get();
}

Unit* BalancePercentValue::GetTarget()
{
    AiObjectContext* ctx = AiObject::context;
    return ctx->GetValue<Unit*>(qualifier)->Get();
}
