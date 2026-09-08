/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "DruidShapeshiftActions.h"

#include "Playerbots.h"

// By leewheel 2026-08-07: Druid 形态 spell ID fallback, 防止 locale 匹配失败导致形态判断错误
namespace {
constexpr uint32 SPELL_DIRE_BEAR_FORM = 9634;
constexpr uint32 SPELL_BEAR_FORM = 5487;
constexpr uint32 SPELL_CAT_FORM = 768;
constexpr uint32 SPELL_TRAVEL_FORM = 783;
constexpr uint32 SPELL_AQUATIC_FORM = 1066;
constexpr uint32 SPELL_FLIGHT_FORM = 33943;
constexpr uint32 SPELL_SWIFT_FLIGHT_FORM = 40120;
constexpr uint32 SPELL_MOONKIN_FORM = 24858;
constexpr uint32 SPELL_DASH = 1850;

// 所有德鲁伊形态 spell ID (用于 CastCasterFormAction 检查是否在任意形态中)
constexpr uint32 ALL_DRUID_FORM_SPELL_IDS[] = {
    SPELL_DIRE_BEAR_FORM, SPELL_BEAR_FORM, SPELL_CAT_FORM, SPELL_TRAVEL_FORM,
    SPELL_AQUATIC_FORM, SPELL_FLIGHT_FORM, SPELL_SWIFT_FLIGHT_FORM, SPELL_MOONKIN_FORM
};

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace

bool CastBearFormAction::isUseful()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放熊形态
    return CastBuffSpellAction::isUseful() && !botAI->HasAura("dire bear form", GetTarget()) && !GetTarget()->HasAura(SPELL_DIRE_BEAR_FORM);
}

bool CastBearFormAction::isPossible()
{
    // By leewheel 2026-08-07: 同上
    return CastBuffSpellAction::isPossible() && !botAI->HasAura("dire bear form", GetTarget()) && !GetTarget()->HasAura(SPELL_DIRE_BEAR_FORM);
}

std::vector<NextAction> CastDireBearFormAction::getAlternatives()
{
    return NextAction::merge({NextAction("bear form")}, CastSpellAction::getAlternatives());
}

bool CastTravelFormAction::isUseful()
{
    bool firstmount = bot->GetLevel() >= 20;

    // useful if no mount or with wsg flag
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放旅行形态
    return !bot->IsMounted() && (!firstmount || (bot->HasAura(23333) || bot->HasAura(23335) || bot->HasAura(34976))) &&
           !botAI->HasAura("dash", bot) && !bot->HasAura(SPELL_DASH);
}

bool CastCasterFormAction::Execute(Event /*event*/)
{
    botAI->RemoveShapeshift();
    return true;
}

bool CastCasterFormAction::isUseful()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致无法识别形态而不切换回人形
    bool hasFormByName = botAI->HasAnyAuraOf(GetTarget(), "dire bear form", "bear form", "cat form", "travel form", "aquatic form",
                               "flight form", "swift flight form", "moonkin form", nullptr);
    bool hasFormById = HasAuraFromList(GetTarget(), ALL_DRUID_FORM_SPELL_IDS, std::size(ALL_DRUID_FORM_SPELL_IDS));
    return (hasFormByName || hasFormById) &&
           AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.mediumHealth;
}

bool CastCancelDruidAction::Execute(Event /*event*/)
{
    botAI->RemoveAura(auraName);
    return true;
}

bool CastCancelDruidAction::isUseful() { return bot->HasAura(auraId); }

bool CastTreeFormAction::isUseful()
{
    constexpr uint32 SPELL_TREE_OF_LIFE = 33891;
    return GetTarget() && CastSpellAction::isUseful() && !bot->HasAura(SPELL_TREE_OF_LIFE);
}
