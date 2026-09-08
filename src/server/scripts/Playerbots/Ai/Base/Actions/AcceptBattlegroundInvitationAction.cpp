/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptBattlegroundInvitationAction.h"

#include "Event.h"
#include "PlayerbotAI.h"
#include "BattlegroundPackets.h"
#include "Battleground.h"

bool AcceptBgInvitationAction::Execute(Event /*event*/)
{
    // 获取机器人的战场队列信息
    uint32 queueSlot = 0;
    BattlegroundQueueTypeId bgQueueTypeId = BATTLEGROUND_QUEUE_NONE;
    for (uint32 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        bgQueueTypeId = bot->GetBattlegroundQueueTypeId(i);
        if (bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
            continue;

        queueSlot = i;
        break;
    }

    // 构造战场端口数据包（使用花括号初始化避免C++最令人苦恼的解析）
    WorldPackets::Battleground::BattlefieldPort battlefieldPort{WorldPacket(CMSG_BATTLEFIELD_PORT)};
    battlefieldPort.Ticket.Id = queueSlot;
    battlefieldPort.Ticket.Type = WorldPackets::LFG::RideType::Battlegrounds;
    battlefieldPort.Ticket.Time = bot->GetBattlegroundQueueJoinTime(bgQueueTypeId);
    battlefieldPort.AcceptedInvite = true;
    bot->GetSession()->HandleBattleFieldPortOpcode(battlefieldPort);

    botAI->ResetStrategies();

    return true;
}
