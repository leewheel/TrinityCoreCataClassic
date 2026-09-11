/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PvpTriggers.h"

#include "BattleGroundTactics.h"
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata核心移除各战场Zone类(BattlegroundWS/EY/AV.h)
//旗帜携带状态在Cata以玩家光环标记(BG_WS_SPELL_WARSONG_FLAG等)，常量取自Compat/BattlegroundAVCompat.h
//End By leewheel
#include "BattlegroundMgr.h"
//By leewheel 2026-09-09: AV节点状态查询兼容接口(Cata的AV战场脚本注册表)，用于替代BattlegroundAV::GetAVNodeInfo
#include "alterac_valley_state_compat.h"
//End By leewheel
#include "Playerbots.h"
#include "ServerFacade.h"

bool EnemyPlayerNear::IsActive() { return AI_VALUE(Unit*, "enemy player target"); }

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata数据驱动战场没有BattlegroundWS类，旗帜携带者以光环标记：
//部落旗(战歌)由联盟玩家携带时带光环BG_WS_SPELL_WARSONG_FLAG，联盟旗(银翼)对应BG_WS_SPELL_SILVERWING_FLAG
bool PlayerHasNoFlag::IsActive()
{
    if (botAI->GetBot()->InBattleground())
    {
        if (botAI->GetBot()->GetBattlegroundTypeId() == BattlegroundTypeId::BATTLEGROUND_WS)
        {
            // bot身上没有任何旗帜光环，说明bot没有携带旗帜
            if (bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG))
                return false;
            return true;
        }
        return false;
    }

    return false;
}
//End By leewheel

bool PlayerIsInBattleground::IsActive() { return botAI->GetBot()->InBattleground(); }

bool BgWaitingTrigger::IsActive()
{
    if (bot->InBattleground())
    {
        if (bot->GetBattleground() && bot->GetBattleground()->GetStatus() == STATUS_WAIT_JOIN)
            return true;
    }

    return false;
}

bool BgActiveTrigger::IsActive()
{
    if (bot->InBattleground())
    {
        if (bot->GetBattleground() && bot->GetBattleground()->GetStatus() == STATUS_IN_PROGRESS)
            return true;
    }

    return false;
}

bool BgInviteActiveTrigger::IsActive()
{
    if (bot->InBattleground() || !bot->InBattlegroundQueue())
    {
        return false;
    }

    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattlegroundQueueTypeId queueTypeId = bot->GetBattlegroundQueueTypeId(i);
        if (queueTypeId == BATTLEGROUND_QUEUE_NONE)
            continue;

        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(queueTypeId);

        GroupQueueInfo ginfo;
        if (bgQueue.GetPlayerGroupInfoData(bot->GetGUID(), &ginfo))
        {
            if (ginfo.IsInvitedToBGInstanceGUID && ginfo.RemoveInviteTime)
            {
                TC_LOG_INFO("playerbots", "Bot {} <{}> ({} {}) : Invited to BG but not in BG",
                         bot->GetGUID().ToString().c_str(), bot->GetName(), bot->GetLevel(),
                         bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H");
                return true;
            }
        }
    }

    return false;
}

bool InsideBGTrigger::IsActive() { return bot->InBattleground() && bot->GetBattleground(); }

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，旗帜状态改为光环判断
bool PlayerIsInBattlegroundWithoutFlag::IsActive()
{
    if (botAI->GetBot()->InBattleground())
    {
        if (botAI->GetBot()->GetBattlegroundTypeId() == BattlegroundTypeId::BATTLEGROUND_WS)
        {
            // bot身上有任意旗帜光环即视为携带旗帜
            if (bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG))
                return false;
        }

        return true;
    }

    return false;
}
//End By leewheel

bool PlayerHasFlag::IsActive()
{
    return IsCapturingFlag(bot);
}

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata没有BattlegroundWS/BattlegroundEY类可查FlagPickerGUID/占点信息：
//bot携带旗帜改用光环判断；WS交旗距离判断简化为携带旗帜即视为可交
bool PlayerHasFlag::IsCapturingFlag(Player* bot)
{
    if (bot->InBattleground())
    {
        if (bot->GetBattlegroundTypeId() == BATTLEGROUND_WS)
        {
            // bot携带任一旗帜光环即视为持旗(战歌旗=部落旗被联盟拾取,银翼旗=联盟旗被部落拾取)
            return bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG);
        }

        if (bot->GetBattlegroundTypeId() == BATTLEGROUND_EY)
        {
            // 风暴之眼：bot携带风暴之眼旗帜光环即视为持旗
            return bot->HasAura(BG_EY_NETHERSTORM_FLAG_SPELL);
        }

        return false;
    }

    return false;
}
//End By leewheel

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata没有FlagPickerGUID接口，改为遍历战场玩家光环查找旗帜携带者
//辅助函数：在战场玩家列表中查找携带指定旗帜光环的玩家
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

bool TeamHasFlag::IsActive()
{
    if (!botAI->GetBot()->InBattleground())
        return false;

    if (botAI->GetBot()->GetBattlegroundTypeId() != BattlegroundTypeId::BATTLEGROUND_WS)
        return false;

    Battleground* bg = botAI->GetBot()->GetBattleground();

    ObjectGuid botGuid = bot->GetGUID();
    TeamId teamId = bot->GetTeamId();
    TeamId enemyTeamId = (teamId == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE);

    // If the bot is carrying any flag, don't activate
    if (bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) || bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG))
        return false;

    //By leewheel 2026-09-06: 移植到TrinityCore-Cata
    //己方携带敌方旗：敌方(联盟)旗光环在己方(部落)玩家身上=银翼旗；敌方(部落)旗光环=战歌旗
    Player* enemyFlagCarrier = (enemyTeamId == TEAM_ALLIANCE)
        ? FindFlagCarrierInBattleground(bg, BG_WS_SPELL_SILVERWING_FLAG)   // 联盟方携带的银翼旗
        : FindFlagCarrierInBattleground(bg, BG_WS_SPELL_WARSONG_FLAG);     // 部落方携带的战歌旗
    Player* ownFlagCarrier = (teamId == TEAM_ALLIANCE)
        ? FindFlagCarrierInBattleground(bg, BG_WS_SPELL_SILVERWING_FLAG)   // 己方联盟旗被谁携带
        : FindFlagCarrierInBattleground(bg, BG_WS_SPELL_WARSONG_FLAG);     // 己方部落旗被谁携带
    //End By leewheel

    bool ownTeamHasFlag = enemyFlagCarrier != nullptr;
    bool enemyTeamHasFlag = ownFlagCarrier != nullptr && ownFlagCarrier->GetTeamId() == enemyTeamId;

    return ownTeamHasFlag && !enemyTeamHasFlag;
}

bool EnemyTeamHasFlag::IsActive()
{
    if (botAI->GetBot()->InBattleground())
    {
        if (botAI->GetBot()->GetBattlegroundTypeId() == BattlegroundTypeId::BATTLEGROUND_WS)
        {
            Battleground* bg = botAI->GetBot()->GetBattleground();

            if (bot->GetTeamId() == TEAM_HORDE)
            {
                // 己方部落：己方旗(战歌旗)被任何玩家携带且携带者不属于敌方? 原逻辑=部落旗被部落方(己方)携带即敌队无旗
                // 原WotLK逻辑: GetFlagPickerGUID(TEAM_HORDE)非空=己方旗被携带(部落旗归联盟方拾取),即敌队(联盟)有旗
                if (Player* carrier = FindFlagCarrierInBattleground(bg, BG_WS_SPELL_WARSONG_FLAG))
                    if (carrier->GetTeamId() == TEAM_ALLIANCE)
                        return true;
            }
            else
            {
                // 己方联盟：己方旗(银翼旗)被部落方携带=敌队有旗
                if (Player* carrier = FindFlagCarrierInBattleground(bg, BG_WS_SPELL_SILVERWING_FLAG))
                    if (carrier->GetTeamId() == TEAM_HORDE)
                        return true;
            }
        }

        return false;
    }

    return false;
}

bool EnemyFlagCarrierNear::IsActive()
{
    Unit* carrier = AI_VALUE(Unit*, "enemy flag carrier");

    if (!carrier || !ServerFacade::instance().IsDistanceLessOrEqualThan(ServerFacade::instance().GetDistance2d(bot, carrier), 100.f))
        return false;

    // Check if there is another enemy player target closer than the FC
    Unit* nearbyEnemy = AI_VALUE(Unit*, "enemy player target");

    if (nearbyEnemy)
    {
        float distToFC = ServerFacade::instance().GetDistance2d(bot, carrier);
        float distToEnemy = ServerFacade::instance().GetDistance2d(bot, nearbyEnemy);

        // If the other enemy is significantly closer, don't pursue FC
        if (distToEnemy + 15.0f < distToFC) // Add small buffer
            return false;
    }

    return true;
}

bool TeamFlagCarrierNear::IsActive()
{
    if (bot->GetBattlegroundTypeId() == BATTLEGROUND_WS)
    {
        //By leewheel 2026-09-06: 移植到TrinityCore-Cata
        //原逻辑=两旗都不在基地时返回false；Cata下改用光环判断：两旗均被玩家携带即都不在基地
        Battleground* bg = bot->GetBattleground();
        if (bg && FindFlagCarrierInBattleground(bg, BG_WS_SPELL_WARSONG_FLAG) &&
            FindFlagCarrierInBattleground(bg, BG_WS_SPELL_SILVERWING_FLAG))
            return false;
        //End By leewheel
    }

    Unit* carrier = AI_VALUE(Unit*, "team flag carrier");
    return carrier && ServerFacade::instance().IsDistanceLessOrEqualThan(ServerFacade::instance().GetDistance2d(bot, carrier), 200.f);
}

bool PlayerWantsInBattlegroundTrigger::IsActive()
{
    if (bot->InBattleground())
        return false;

    if (bot->GetBattleground() && bot->GetBattleground()->GetStatus() == STATUS_WAIT_JOIN)
        return false;

    if (bot->GetBattleground() && bot->GetBattleground()->GetStatus() == STATUS_IN_PROGRESS)
        return false;

    if (bot->IsDeserter())
        return false;

    return true;
}

bool VehicleNearTrigger::IsActive()
{
    GuidVector npcs = AI_VALUE(GuidVector, "nearest vehicles");
    return npcs.size();
}

bool InVehicleTrigger::IsActive() { return botAI->IsInVehicle(); }

bool AllianceNoSnowfallGY::IsActive()
{
    if (!bot || bot->GetTeamId() != TEAM_ALLIANCE)
        return false;

    Battleground* bg = bot->GetBattleground();
    if (bg && BGTactics::GetBotStrategyForTeam(bg, TEAM_ALLIANCE) != AV_STRATEGY_BALANCED)
        return false;

    float botX = bot->GetPositionX();
    if (botX <= -562.0f)
        return false;

    if (bot->GetBattlegroundTypeId() != BATTLEGROUND_AV)
        return false;

    //By leewheel 2026-09-09: 移植到TrinityCore-Cata，Cata无BattlegroundAV类，改用AVCompatQueryNode查询节点归属
    uint8 snowState = 0;
    uint32 snowOwner = 0;
    bool snowTower = false;
    if (bg && AVCompatQueryNode(bg->GetInstanceID(), BG_AV_NODES_SNOWFALL_GRAVE, snowState, snowOwner, snowTower))
    {
        //雪落墓地不归联盟所有时触发(中立或部落控制均激活)
        return snowOwner != uint32(ALLIANCE);
    }

    return false;
}
