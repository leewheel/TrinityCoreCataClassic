/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PriestActions.h"

#include "Event.h"
#include "Playerbots.h"

// By leewheel 2026-08-07: Priest Actions spell ID fallback, 防止 locale 匹配失败导致技能判断错误
namespace {
// Shadowform (玩家唯一版本)
constexpr uint32 SHADOWFORM_SPELL_IDS[] = { 15473 };
// Weakened Soul debuff (玩家唯一版本)
constexpr uint32 WEAKENED_SOUL_SPELL_IDS[] = { 6788 };
//By leewheel 2026-08-15: 修复——原数组含错误ID 48040(实为心灵之火Inner Fire,与INNER_FIRE数组自相矛盾)
//与48041(实为Summon the Brewmaiden召唤酿酒侍女,非盾法术),且漏掉真实顶级盾48065(79级rank13)/48066(80级rank14)。
//后果:对任何带心灵之火的牧师目标(含治疗bot自身)HasWeakenedSoulOrShield恒返回true→预防性真言术:盾永不施放;
//非enUS下真实盾aura(48065/48066)识别不到→已套盾目标被误判无盾反复施放。经TC核心PlayerAI.cpp:193
//(SPELL_POWER_WORD_SHIELD=48066)与客户端SpellName.db2三方佐证
// Power Word: Shield 全等级 (玩家可学版本)
constexpr uint32 POWER_WORD_SHIELD_SPELL_IDS[] = {
    17, 592, 600, 3747, 6065, 6066, 10898, 10899, 10900, 10901,
    25217, 25218, 48065, 48066
};
//End By leewheel

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace

bool CastRemoveShadowformAction::Execute(Event /*event*/)
{
    botAI->RemoveAura("shadowform");
    return true;
}

// By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致无法识别暗影形态
bool CastRemoveShadowformAction::isUseful()
{
    Unit* self = AI_VALUE(Unit*, "self target");
    if (!self) return false;
    return botAI->HasAura("shadowform", self) || HasAuraFromList(self, SHADOWFORM_SPELL_IDS, std::size(SHADOWFORM_SPELL_IDS));
}

// By leewheel 2026-08-07: 检查目标是否有 Weakened Soul 或 Power Word: Shield (名称 + spell ID fallback)
static bool HasWeakenedSoulOrShield(PlayerbotAI* ai, Unit* unit)
{
    if (ai->HasAnyAuraOf(unit, "weakened soul", "power word: shield", nullptr))
        return true;
    return HasAuraFromList(unit, WEAKENED_SOUL_SPELL_IDS, std::size(WEAKENED_SOUL_SPELL_IDS)) ||
           HasAuraFromList(unit, POWER_WORD_SHIELD_SPELL_IDS, std::size(POWER_WORD_SHIELD_SPELL_IDS));
}

Unit* CastPowerWordShieldOnAlmostFullHealthBelowAction::GetTarget()
{
    Group* group = bot->GetGroup();
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->GetSource();
        if (!player)
            continue;
        if (player->isDead())
        {
            continue;
        }
        if (player->GetHealthPct() > sPlayerbotAIConfig.almostFullHealth)
        {
            continue;
        }
        if (player->GetDistance2d(bot) > sPlayerbotAIConfig.spellDistance)
        {
            continue;
        }
        if (HasWeakenedSoulOrShield(botAI, player))
        {
            continue;
        }
        return player;
    }
    return nullptr;
}

bool CastPowerWordShieldOnAlmostFullHealthBelowAction::isUseful()
{
    Group* group = bot->GetGroup();
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->GetSource();
        if (!player)
            continue;
        if (player->isDead())
        {
            continue;
        }
        if (player->GetHealthPct() > sPlayerbotAIConfig.almostFullHealth)
        {
            continue;
        }
        if (player->GetDistance2d(bot) > sPlayerbotAIConfig.spellDistance)
        {
            continue;
        }
        if (HasWeakenedSoulOrShield(botAI, player))
        {
            continue;
        }
        return true;
    }
    return false;
}

Unit* CastPowerWordShieldOnNotFullAction::GetTarget()
{
    Group* group = bot->GetGroup();
    MinValueCalculator calc(100);
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->GetSource();
        if (!player)
            continue;
        if (player->isDead() || player->IsFullHealth())
        {
            continue;
        }
        if (player->GetDistance2d(bot) > sPlayerbotAIConfig.spellDistance)
        {
            continue;
        }
        if (HasWeakenedSoulOrShield(botAI, player))
        {
            continue;
        }
        calc.probe(player->GetHealthPct(), player);
    }
    return (Unit*)calc.param;
}

bool CastPowerWordShieldOnNotFullAction::isUseful()
{
    return GetTarget();
}
