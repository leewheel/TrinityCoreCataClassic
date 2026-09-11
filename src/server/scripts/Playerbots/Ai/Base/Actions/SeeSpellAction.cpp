/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "SeeSpellAction.h"

#include "Event.h"
#include "Formations.h"
#include "Playerbots.h"
#include "RTSCValues.h"
#include "RtscAction.h"
#include "PositionValue.h"
#include "ByteBuffer.h"
#include "SpellPackets.h" // By leewheel 2026-08-03: 使用核心CastSpell包解析

std::set<uint32> const FISHING_SPELLS = {7620, 7731, 7732, 18248, 33095, 51294};

Creature* SeeSpellAction::CreateWps(Player* wpOwner, float x, float y, float z, float o, uint32 entry, Creature* /*lastWp*/,
                                    bool important)
{
    float dist = wpOwner->GetDistance(x, y, z);
    float delay = 1000.0f * dist / wpOwner->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;

    if (!important)
        delay *= 0.25;

    Creature* wpCreature = wpOwner->SummonCreature(entry, x, y, z - 1, o, TEMPSUMMON_TIMED_DESPAWN, Milliseconds(static_cast<int64_t>(delay)));
    if (!important)
        wpCreature->SetObjectScale(0.2f);

    return wpCreature;
}

bool SeeSpellAction::Execute(Event event)
{
    // RTSC packet data
    WorldPacket p(event.getPacket());
    if (p.empty())
        return false;

    Player* master = botAI->GetMaster();
    if (!master)
        return false;

    //By leewheel 2026-08-03: 修复——客户端包经WorldSocket读opcode后rpos=2(opcode已消费)，
    //且CMSG_CAST_SPELL为bit-packed复杂格式(SpellCastRequest)，原扁平读(p>>castCount>>spellId>>castFlags
    //+手动targetMask解析)与TC 3.4.3格式完全不符，改用核心包解析，取SpellID与目标位置。
    WorldPackets::Spells::CastSpell castSpell(std::move(p));
    castSpell.Read();
    uint32 spellId = uint32(castSpell.Cast.SpellID);

    if (FISHING_SPELLS.find(spellId) != FISHING_SPELLS.end())
    {
        if (AI_VALUE(bool, "can fish") && sPlayerbotAIConfig.enableFishingWithMaster)
        {
            botAI->ChangeStrategy("+master fishing", BOT_STATE_NON_COMBAT);
            return true;
        }
        return false;
    }

    if (spellId != RTSC_MOVE_SPELL)
        return false;

    //By leewheel 2026-08-03: 目标位置从核心SpellTargetData.DstLocation读取(替代原手动targetMask解析)
    Position dstPos;
    bool hasDst = false;
    if (castSpell.Cast.Target.DstLocation)
    {
        dstPos.Relocate(castSpell.Cast.Target.DstLocation->Location.Pos);
        hasDst = true;
    }

    if (!hasDst)
    {
        LOG_WARN("playerbots", "SeeSpellAction: (malformed) RTSC payload does not contain full targets data");
        return false;
    }

    WorldPosition spellPosition(master->GetMapId(), dstPos);
    SET_AI_VALUE(WorldPosition, "see spell location", spellPosition);

    bool selected = AI_VALUE(bool, "RTSC selected");
    bool inRange = spellPosition.distance(bot) <= 10;
    std::string const nextAction = AI_VALUE(std::string, "RTSC next spell action");

    if (nextAction.empty())
    {
        if (!inRange && selected)
            master->SendPlaySpellVisualKit(6372, 0, 0); //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit
        else if (inRange && !selected)
            master->SendPlaySpellVisualKit(5036, 0, 0); //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit

        SET_AI_VALUE(bool, "RTSC selected", inRange);

        if (selected)
            return MoveToSpell(spellPosition);

        return inRange;
    }
    else if (nextAction == "move")
    {
        return MoveToSpell(spellPosition);
    }
    else if (nextAction.find("save ") != std::string::npos)
    {
        std::string locationName;
        if (nextAction.find("save selected ") != std::string::npos)
        {
            if (!selected)
                return false;

            locationName = nextAction.substr(14);
        }
        else
            locationName = nextAction.substr(5);

        SetFormationOffset(spellPosition);

        SET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName, spellPosition);

        Creature* wpCreature =
            bot->SummonCreature(15631, spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(),
                                spellPosition.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, Milliseconds(2000));
        wpCreature->SetObjectScale(0.5f);
        RESET_AI_VALUE(std::string, "RTSC next spell action");

        return true;
    }

    return false;
}

bool SeeSpellAction::SelectSpell(WorldPosition& spellPosition)
{
    Player* master = botAI->GetMaster();
    if (spellPosition.distance(bot) <= 5 || AI_VALUE(bool, "RTSC selected"))
    {
        SET_AI_VALUE(bool, "RTSC selected", true);
        //By leewheel 2025-01-16
        // TC中SendPlaySpellVisual需要更多参数
        master->SendPlaySpellVisual(bot, 5036, 0, 0, 0.0f, false);
        //End By leewheel 2025-01-16
    }

    return true;
}

bool SeeSpellAction::MoveToSpell(WorldPosition& spellPosition, bool inFormation)
{
    if (inFormation)
        SetFormationOffset(spellPosition);

    if (botAI->HasStrategy("stay", botAI->GetState()))
    {
        PositionMap& posMap = AI_VALUE(PositionMap&, "position");
        PositionInfo stayPosition = posMap["stay"];

        stayPosition.Set(spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), spellPosition.GetMapId());
        posMap["stay"] = stayPosition;
    }

    if (bot->IsWithinLOS(spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ()))
        return MoveNear(spellPosition.GetMapId(), spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), 0);

    return MoveTo(spellPosition.GetMapId(), spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), false,
                  false);
}

void SeeSpellAction::SetFormationOffset(WorldPosition& spellPosition)
{
    Player* master = botAI->GetMaster();

    Formation* formation = AI_VALUE(Formation*, "formation");

    WorldLocation formationLocation = formation->GetLocation();

    if (formationLocation.GetPositionX() != 0 || formationLocation.GetPositionY() != 0)
    {
        spellPosition -= WorldPosition(master);
        spellPosition += formationLocation;
    }
}
