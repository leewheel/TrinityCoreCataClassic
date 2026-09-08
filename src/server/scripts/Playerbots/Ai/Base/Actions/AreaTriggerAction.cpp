/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AreaTriggerAction.h"

#include "AreaTriggerPackets.h"
#include "DB2Stores.h"
#include "Event.h"
#include "LastMovementValue.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "Transport.h"

bool ReachAreaTriggerAction::Execute(Event event)
{
    if (botAI->IsRealPlayer())  // Do not trigger own area trigger.
        return false;

    uint32 triggerId;
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，去掉rpos(0)防止把opcode当包体读
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel
    p >> triggerId;

    //By leewheel 2026-09-06: 移植到TrinityCore-Cata
    //Cata把传送型区域触发器数据改为AreaTriggerTeleport(即WorldSafeLocsEntry)，
    //字段为Loc(WorldLocation)，替代WotLK的target_mapId/target_X等成员
    AreaTriggerTeleport const* at = sObjectMgr->GetAreaTrigger(triggerId);
    if (!at)
        return false;

    // 如果不是传送型区域触发器，直接处理传送
    if (at->Loc.GetMapId() == 0 && at->Loc.GetPositionX() == 0 && at->Loc.GetPositionY() == 0 && at->Loc.GetPositionZ() == 0)
    {
        WorldPacket packetData(CMSG_AREA_TRIGGER);
        WorldPackets::AreaTrigger::AreaTrigger packet(std::move(packetData));
        packet.AreaTriggerID = triggerId;
        packet.Entered = true;
        packet.FromClient = false;
        bot->GetSession()->HandleAreaTriggerOpcode(packet);

        return true;
    }

    if (bot->GetMapId() != at->Loc.GetMapId())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "area_trigger_follow_too_far_error", "我不跟随：太远了", {}));
        return true;
    }

    bot->GetMotionMaster()->MovePoint(
        triggerId,
        at->Loc.GetPositionX(), at->Loc.GetPositionY(), at->Loc.GetPositionZ(),
        /*generatePath*/ true);
    //End By leewheel

    // 从DBC获取区域触发器位置用于距离计算
    AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(triggerId);
    if (!atEntry)
        return true;

    float distance = bot->GetDistance(atEntry->Pos.X, atEntry->Pos.Y, atEntry->Pos.Z);
    float delay = 1000.0f * distance / bot->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;
    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "area_trigger_wait_for_me", "Wait for me", {}));
    botAI->SetNextCheckDelay(delay);
    context->GetValue<LastMovement&>("last area trigger")->Get().lastAreaTrigger = triggerId;

    return true;
}

bool PlayerbotAreaTriggerAction::Execute(Event event)
{
    Player* bot = botAI->GetBot();
    if (!bot || !event.getObject())
        return false;

    WorldPacket p(event.getPacket());
    uint32 triggerId;
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel
    p >> triggerId;

    if (!sObjectMgr->GetAreaTrigger(triggerId))
        return false;

    if (!sObjectMgr->GetAreaTrigger(triggerId))
        return false;

    WorldPacket packetData(CMSG_AREA_TRIGGER);
    WorldPackets::AreaTrigger::AreaTrigger packet(std::move(packetData));
    packet.AreaTriggerID = triggerId;
    packet.Entered = true;
    packet.FromClient = false;
    bot->GetSession()->HandleAreaTriggerOpcode(packet);

    botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault("hello", "你好", {}));
    return true;
}
