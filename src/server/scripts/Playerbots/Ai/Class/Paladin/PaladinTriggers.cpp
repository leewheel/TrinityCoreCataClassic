/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PaladinTriggers.h"

#include "GenericBuffUtils.h"
#include "PaladinGreaterBlessingAction.h"
#include "PaladinActions.h"
#include "PaladinHelper.h"
#include "Playerbots.h"

//By leewheel 2026-08-06: SealTrigger添加spell ID fallback
//原代码用7个法术名检查,locale匹配失败时所有HasAura返回false,IsActive永远true导致重复切Seal
//修复: 名称检查认为没有Seal时,用spell ID确认是否真的没有Seal
bool SealTrigger::IsActive()
{
    Unit* target = GetTarget();

    // 名称匹配检查
    bool hasSealByName = botAI->HasAura("seal of justice", target) || botAI->HasAura("seal of command", target) ||
                         botAI->HasAura("seal of vengeance", target) || botAI->HasAura("seal of corruption", target) ||
                         botAI->HasAura("seal of righteousness", target) || botAI->HasAura("seal of light", target) ||
                         botAI->HasAura("seal of wisdom", target);

    if (hasSealByName)
    {
        // 名称匹配认为有Seal,检查是否是智慧圣印且蓝量>70%(需要切换)
        bool hasWisdomByName = botAI->HasAura("seal of wisdom", target);
        if (hasWisdomByName && AI_VALUE2(uint8, "mana", "self target") > 70)
            return true;  // 有智慧圣印但蓝量充足,触发切换
        return false;  // 有其他Seal,不触发
    }

    // 名称匹配认为没有Seal,用spell ID确认(防止locale匹配失败)
    if (ai::paladin::HasAnySeal(target))
    {
        // spell ID确认有Seal,检查是否是智慧圣印且蓝量>70%(需要切换)
        if (ai::paladin::HasSealOfWisdom(target) && AI_VALUE2(uint8, "mana", "self target") > 70)
            return true;  // 有智慧圣印但蓝量充足,触发切换
        return false;  // 有其他Seal,不触发
    }

    return true;  // 确实没有Seal,触发
}
//End By leewheel

bool CrusaderAuraTrigger::IsActive()
{
    Unit* target = GetTarget();
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致骑乘时无法切换十字军光环
    // Crusader Aura 全等级只有 32223 (玩家可学)
    if (!AI_VALUE2(bool, "mounted", "self target"))
        return false;
    return !botAI->HasAura("crusader aura", target) && !target->HasAura(32223);
}

//By leewheel 2026-08-06: BlessingOnPartyTrigger添加spell ID fallback
//BuffTrigger::IsActive()已修复逗号分隔问题,但locale匹配失败时GetAura仍返回nullptr
//这里在基类判断"需要加buff"后,用spell ID确认目标是否真的没有祝福
bool BlessingOnPartyTrigger::IsActive()
{
    // 先用基类逻辑(已修复逗号分隔的法术名问题)
    if (!BuffOnPartyTrigger::IsActive())
        return false;

    // 基类认为需要加buff,但可能是locale匹配失败导致的误判
    // 用spell ID fallback确认目标是否真的没有任何祝福
    Unit* target = GetTarget();
    if (!target)
        return true;

    if (ai::paladin::HasAnyBlessing(target))
        return false;  // 目标已有祝福(通过spell ID确认),不触发

    return true;  // 确实没有祝福,触发
}
//End By leewheel

//By leewheel 2026-08-06: 添加spell ID fallback,防止locale匹配失败导致重复加BUFF
bool BlessingTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!SpellTrigger::IsActive())
        return false;

    // 名称匹配检查
    if (!botAI->HasAnyAuraOf(target, "blessing of might", "blessing of wisdom",
                             "blessing of kings", "blessing of sanctuary", nullptr))
    {
        // 名称匹配说没有祝福,再用spell ID确认(防止locale匹配失败)
        if (!ai::paladin::HasAnyBlessing(target))
            return true; // 确实没有祝福,触发
    }
    return false;
}
//End By leewheel

bool DivineShieldLowHealthTrigger::IsActive()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致圣盾术判断错误
    // Divine Shield 全等级: 642(玩家主) + 13874/29382/33581/41367/54322/67251(NPC/物品版本)
    static constexpr uint32 DIVINE_SHIELD_SPELL_IDS[] = { 642, 13874, 29382, 33581, 41367, 54322, 63148, 67251 };
    bool hasDivineShield = botAI->HasAura("divine shield", bot);
    if (!hasDivineShield)
    {
        for (uint32 id : DIVINE_SHIELD_SPELL_IDS)
        {
            if (bot->HasAura(id)) { hasDivineShield = true; break; }
        }
    }
    return hasDivineShield && AI_VALUE2(uint8, "health", "self target") < 80;
}

Unit* HandOfFreedomOnPartyTrigger::GetTarget()
{
    bool const selfImpaired = botAI->IsMovementImpaired(bot);
    bool const hasSelfHand = selfImpaired && ai::paladin::HasAnyPaladinHandFromCaster(bot, bot);

    if (!bot->GetGroup())
    {
        if (selfImpaired && !hasSelfHand)
            return bot;

        return nullptr;
    }

    if (selfImpaired && !hasSelfHand)
        return bot;

    return Trigger::GetTarget();
}

bool HandOfFreedomOnPartyTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    if (target != bot &&
        bot->GetExactDist2dSq(target->GetPositionX(), target->GetPositionY()) > 30.0f * 30.0f)
        return false;

    if (!botAI->CanCastSpell("hand of freedom", target))
        return false;

    return !ai::paladin::HasAnyPaladinHandFromCaster(target, bot) && botAI->IsMovementImpaired(target);
}

bool NotSensingUndeadTrigger::IsActive()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放感知亡灵
    // Sense Undead: 5502 (玩家唯一版本)
    return !botAI->HasAura("sense undead", bot) && !bot->HasAura(5502);
}

bool GreaterBlessingNeededTrigger::IsActive()
{
    if (!ai::gbless::IsEligibleGroupForAutoBlessings(bot->GetGroup()))
        return false;

    if (ai::buff::ShouldDeferGreaterBlessingAssignmentForRecentLogin(bot))
        return false;

    Group* group = bot->GetGroup();
    uint32 const groupKey = group ? group->GetLeaderGUID().GetCounter() : 0;

    Value<ai::gbless::CachedPendingBlessingAssignment>* pendingValue =
        context->GetValue<ai::gbless::CachedPendingBlessingAssignment>("greater blessing pending assignment");
    if (!pendingValue)
        return false;

    ai::gbless::CachedPendingBlessingAssignment pendingAssignment = pendingValue->Get();
    if (pendingAssignment.groupKey != groupKey)
    {
        pendingValue->Reset();
        pendingAssignment = pendingValue->Get();
    }

    return pendingAssignment.valid && pendingAssignment.groupKey == groupKey;
}
