/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PvpValues.h"

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata核心移除BattlegroundWS/EY类
//旗帜携带者改用战场玩家光环扫描(战歌旗23333/银翼旗23335/风暴之眼旗34976)
#include "BattlegroundMgr.h"
#include "Playerbots.h"
#include "ServerFacade.h"

// 辅助函数：在战场玩家列表中查找携带指定旗帜光环的玩家
static Player* FindFlagCarrierInBattleground(Battleground* bg, uint32 flagSpellId)
{
    if (!bg)
        return nullptr;

    for (auto const& [guid, bgPlayer] : bg->GetPlayers())
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (player && player->HasAura(flagSpellId))
            return player;
    }
    return nullptr;
}
//End By leewheel

Unit* FlagCarrierValue::Calculate()
{
    Unit* carrier = nullptr;

    if (botAI->GetBot()->InBattleground())
    {
        if (botAI->GetBot()->GetBattlegroundTypeId() == BattlegroundTypeId::BATTLEGROUND_WS)
        {
            Battleground* bg = botAI->GetBot()->GetBattleground();

            if (!bg)
                return nullptr;

            //By leewheel 2026-09-06: 移植到TrinityCore-Cata
            //战歌旗(TEAM_HORDE)携带者与银翼旗(TEAM_ALLIANCE)携带者用光环扫描获取
            Player* hordeFlagCarrier = FindFlagCarrierInBattleground(bg, BG_WS_SPELL_WARSONG_FLAG);    // 战歌旗(部落旗)携带者
            Player* allianceFlagCarrier = FindFlagCarrierInBattleground(bg, BG_WS_SPELL_SILVERWING_FLAG); // 银翼旗(联盟旗)携带者
            //End By leewheel

            if (((!sameTeam && bot->GetTeamId() == TEAM_HORDE) || (sameTeam && bot->GetTeamId() == TEAM_ALLIANCE)) &&
                hordeFlagCarrier)
                carrier = hordeFlagCarrier;

            if (((!sameTeam && bot->GetTeamId() == TEAM_ALLIANCE) || (sameTeam && bot->GetTeamId() == TEAM_HORDE)) &&
                allianceFlagCarrier)
                carrier = allianceFlagCarrier;

            if (carrier)
            {
                if (ignoreRange || bot->IsWithinDistInMap(carrier, sPlayerbotAIConfig.sightDistance))
                {
                    return carrier;
                }
                else
                    return nullptr;
            }
        }

        if (botAI->GetBot()->GetBattlegroundTypeId() == BATTLEGROUND_EY)
        {
            Battleground* bg = botAI->GetBot()->GetBattleground();

            if (!bg)
                return nullptr;

            //By leewheel 2026-09-06: 移植到TrinityCore-Cata，风暴之眼旗手用光环扫描
            Player* fc = FindFlagCarrierInBattleground(bg, BG_EY_NETHERSTORM_FLAG_SPELL);
            if (!fc)
                return nullptr;
            //End By leewheel

            if (!sameTeam && (fc->GetTeamId() != bot->GetTeamId()))
                carrier = fc;

            if (sameTeam && (fc->GetTeamId() == bot->GetTeamId()))
                carrier = fc;

            if (carrier)
            {
                if (ignoreRange || bot->IsWithinDistInMap(carrier, sPlayerbotAIConfig.sightDistance))
                {
                    return carrier;
                }
                else
                    return nullptr;
            }
        }
    }

    return carrier;
}

std::vector<CreatureData const*> BgMastersValue::Calculate()
{
    BattlegroundTypeId bgTypeId = (BattlegroundTypeId)stoi(qualifier);

    std::vector<uint32> entries;
    std::map<TeamId, std::map<BattlegroundTypeId, std::vector<uint32>>> battleMastersCache =
        sRandomPlayerbotMgr.getBattleMastersCache();
    entries.insert(entries.end(), battleMastersCache[TEAM_NEUTRAL][bgTypeId].begin(),
                   battleMastersCache[TEAM_NEUTRAL][bgTypeId].end());
    entries.insert(entries.end(), battleMastersCache[TEAM_ALLIANCE][bgTypeId].begin(),
                   battleMastersCache[TEAM_ALLIANCE][bgTypeId].end());
    entries.insert(entries.end(), battleMastersCache[TEAM_HORDE][bgTypeId].begin(),
                   battleMastersCache[TEAM_HORDE][bgTypeId].end());

    std::vector<CreatureData const*> bmGuids;

    for (auto entry : entries)
    {
        for (auto creaturePair : WorldPosition().getCreaturesNear(0, entry))
        {
            bmGuids.push_back(creaturePair);
        }
    }

    return bmGuids;
}

CreatureData const* BgMasterValue::Calculate()
{
    CreatureData const* bmPair = NearestBm(false);
    if (!bmPair)
        bmPair = NearestBm(true);

    return bmPair;
}

CreatureData const* BgMasterValue::NearestBm(bool allowDead)
{
    WorldPosition botPos(bot);

    std::vector<CreatureData const*> bmPairs = AI_VALUE2(std::vector<CreatureData const*>, "bg masters", qualifier);

    float rDist = 0.0f;
    CreatureData const* rbmPair = nullptr;

    for (auto& bmPair : bmPairs)
    {
        if (!bmPair)
            continue;

        WorldPosition bmPos(bmPair->mapid(), bmPair->posX(), bmPair->posY(), bmPair->posZ(), bmPair->orientation());

        float dist = botPos.distance(bmPos);  // This is the aproximate travel distance.

        // Did we already find a closer unit that is not dead?
        if (rbmPair && rDist <= dist)
            continue;

        CreatureTemplate const* bmTemplate = sObjectMgr->GetCreatureTemplate(bmPair->id);
        if (!bmTemplate)
            continue;

        FactionTemplateEntry const* bmFactionEntry = sFactionTemplateStore.LookupEntry(bmTemplate->faction);

        // Is the unit hostile?
        //By leewheel 2025-07-10
        // TC中GetFactionReactionTo是静态方法,需要两个参数: FactionTemplateEntry和WorldObject
        if (WorldObject::GetFactionReactionTo(bmFactionEntry, bot) < REP_NEUTRAL)
        //End By leewheel 2025-07-10
            continue;

        AreaTableEntry const* area = bmPos.getArea();

        if (!area)
            continue;

        // Is the area hostile?
        if (area->team() == 4 && bot->GetTeamId() == TEAM_ALLIANCE)
            continue;
        if (area->team() == 2 && bot->GetTeamId() == TEAM_HORDE)
            continue;

        if (!allowDead)
        {
            //By leewheel 2025-07-10
            // TC中CreatureData没有ObjectGuid,需要用ObjectGuid::Create创建
            ObjectGuid unitGuid = ObjectGuid::Create<HighGuid::Creature>(bmPair->mapId, bmPair->id, bmPair->spawnId);
            Unit* unit = botAI->GetUnit(unitGuid);
            //End By leewheel 2025-07-10

            if (!unit)
                continue;

            // Is the unit dead?
            //By leewheel 2025-01-16
            // TC中使用isDead()方法而不是DeathState::Dead
            if (unit->isDead())
            //End By leewheel 2025-01-16
                continue;
        }

        rbmPair = bmPair;
        rDist = dist;
    }

    return rbmPair;
}

BattlegroundTypeId RpgBgTypeValue::Calculate()
{
    GuidPosition guidPosition = AI_VALUE(GuidPosition, "rpg target");

    if (guidPosition)
        for (uint32 i = 1; i < MAX_BATTLEGROUND_QUEUE_TYPES; i++)
        {
            BattlegroundQueueTypeId queueTypeId = (BattlegroundQueueTypeId)i;

            //By leewheel 2025-07-10
            // TC中BGTemplateId返回uint32,需要显式转换为BattlegroundTypeId
            BattlegroundTypeId bgTypeId = (BattlegroundTypeId)sBattlegroundMgr->BGTemplateId((BattlegroundQueueTypeId)i);
            //End By leewheel 2025-07-10

            //By leewheel 2026-07-11: TC使用GetBattlegroundTemplate返回BattlegroundTemplate*
            BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
            if (!bgTemplate)
                continue;

            if (bot->GetLevel() < bgTemplate->GetMinLevel())
                continue;
            //End By leewheel

            // check if already in queue
            //By leewheel 2026-07-11: TC的InBattlegroundQueueForBattlegroundQueueType接受BattlegroundQueueTypeId参数
            if (bot->InBattlegroundQueueForBattlegroundQueueType(queueTypeId))
                continue;
            //End By leewheel

            std::map<TeamId, std::map<BattlegroundTypeId, std::vector<uint32>>> battleMastersCache =
                sRandomPlayerbotMgr.getBattleMastersCache();

            for (auto& entry : battleMastersCache[TEAM_NEUTRAL][bgTypeId])
                if (entry == guidPosition.GetEntry())
                    return bgTypeId;

            for (auto& entry : battleMastersCache[bot->GetTeamId()][bgTypeId])
                if (entry == guidPosition.GetEntry())
                    return bgTypeId;
        }

    return BATTLEGROUND_TYPE_NONE;
}
