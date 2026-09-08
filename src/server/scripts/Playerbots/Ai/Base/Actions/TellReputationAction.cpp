/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellReputationAction.h"

#include <algorithm>

#include "Event.h"
#include "PlayerbotAI.h"
#include "ReputationMgr.h"

#include "SharedDefines.h"

std::string TellReputationAction::BuildReputationLine(FactionEntry const* entry)
{
    ReputationMgr& repMgr = bot->GetReputationMgr();
    ReputationRank rank = repMgr.GetRank(entry);
    int32 reputation = repMgr.GetReputation(entry->ID);

    std::ostringstream out;
    //By leewheel 20260709: TC的LocalizedString::operator[]接受LocaleConstant而非int
    out << entry->Name[DEFAULT_LOCALE] << ": |cff";
    //End By leewheel

    switch (rank)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        case REP_HATED:
            out << "cc2222憎恨";
            break;
        case REP_HOSTILE:
            out << "ff0000敌对";
            break;
        case REP_UNFRIENDLY:
            out << "ee6622冷淡";
            break;
        case REP_NEUTRAL:
            out << "ffff00中立";
            break;
        case REP_FRIENDLY:
            out << "00ff00友善";
            break;
        case REP_HONORED:
            out << "00ff88尊敬";
            break;
        case REP_REVERED:
            out << "00ffcc崇敬";
            break;
        case REP_EXALTED:
            out << "00ffff崇拜";
            break;
        default:
            out << "808080未知";
            break;
        //End By leewheel
    }

    out << "|cffffffff";

    int32 base = ReputationMgr::Reputation_Cap + 1;
    for (int32 i = MAX_REPUTATION_RANK - 1; i >= rank; --i)
        base -= ReputationMgr::PointsInRank[i];

    out << " (" << (reputation - base) << "/" << ReputationMgr::PointsInRank[rank] << ")";
    return out.str();
}

bool TellReputationAction::Execute(Event event)
{
    std::string const param = event.getParam();
    if (param == "all")
    {
        ReputationMgr& repMgr = bot->GetReputationMgr();
        std::vector<std::string> lines;

        FactionStateList const& stateList = repMgr.GetStateList();
        lines.reserve(stateList.size());

        for (auto const& itr : stateList)
        {
            FactionState const& faction = itr.second;
            //By leewheel 2026-07-10: TC使用ReputationFlags枚举类
            if (!faction.Flags.HasFlag(ReputationFlags::Visible))
                continue;

            if ((faction.Flags.HasFlag(ReputationFlags::Hidden) || faction.Flags.HasFlag(ReputationFlags::Header)) &&
                !faction.Flags.HasFlag(ReputationFlags::ShowPropagated))
                continue;
            //End By leewheel

            FactionEntry const* entry = sFactionStore.LookupEntry(faction.ID);
            if (!entry)
                continue;

            lines.push_back(BuildReputationLine(entry));
        }

        std::sort(lines.begin(), lines.end());

        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("=== 声望 ===");
        //End By leewheel
        for (auto const& line : lines)
            botAI->TellMaster(line);

        return true;
    }

    Player* master = GetMaster();
    if (!master)
        return false;

    ObjectGuid selection = master->GetTarget();
    if (selection.IsEmpty())
        return false;

    Unit* unit = ObjectAccessor::GetUnit(*master, selection);
    if (!unit)
        return false;

    FactionTemplateEntry const* factionTemplate = unit->GetFactionTemplateEntry();

    //By leewheel 2025-07-10
    // TC中FactionTemplateEntry的faction是兼容方法（返回Faction成员），需要加()调用
    FactionEntry const* entry = sFactionStore.LookupEntry(factionTemplate->faction());
    //End By leewheel
    if (!entry)
        return false;

    botAI->TellMaster(BuildReputationLine(entry));

    return true;
}
