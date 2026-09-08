/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_GENERICBUFFUTILS_H
#define PLAYERBOTS_GENERICBUFFUTILS_H

#include <string>
#include <unordered_map>

//By leewheel 2026-08-23: 合并 the-lab(#2571) —— 保留 HEAD 的 Common.h(TC 需要完整类型), 补充 Aura 前置声明
#include "Common.h"
class Aura;
//End By leewheel
class Player;
class PlayerbotAI;
class Unit;

namespace ai::buff
{

typedef std::unordered_map<std::string, uint32> MissingBuffReagentNoticeMap;

// True when the buff should be (re)cast: topped off toward full duration during an
// out-of-combat force-rebuff, below baseBeforeDuration ms remaining otherwise.
bool BuffBelowRefreshTarget(PlayerbotAI* botAI, Aura* aura, uint32 baseBeforeDuration);

bool IsGroupVariantEnabled(Player* bot, std::string const& name);

std::string MakeAuraQualifierForBuff(std::string const& name);

std::string GroupVariantFor(std::string const& name);

bool NeedsPostLoginBuffGrace(std::string const& name);

bool ShouldDeferPartyBuffEvaluationForRecentLogin(
    Player* bot,
    Unit* target,
    std::string const& spell);

bool ShouldDeferGreaterBlessingAssignmentForRecentLogin(Player* bot);

bool HasRequiredReagents(Player* bot, uint32 spellId);

void ClearMissingBuffReagentNotice(PlayerbotAI* botAI, std::string const& groupName);

bool TryAnnounceMissingBuffReagents(
    PlayerbotAI* botAI, std::string const& baseName, std::string const& groupName);

std::string UpgradeToGroupIfAppropriate(
    Player* bot,
    PlayerbotAI* botAI,
    std::string const& baseName,
    std::string* outMissingReagentGroupName = nullptr);

}

namespace ai::spell
{
    bool HasSpellOrCategoryCooldown(Player* bot, uint32 spellId);
}

#endif
