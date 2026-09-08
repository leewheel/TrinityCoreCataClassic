/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptResurrectAction.h"

#include "Event.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "MiscPackets.h"

bool AcceptResurrectAction::Execute(Event event)
{
    if (bot->IsAlive())
        return false;

    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
        return false;
    //End By leewheel
    ObjectGuid guid;
    p >> guid;

    WorldPacket packetData(CMSG_RESURRECT_RESPONSE);
    WorldPackets::Misc::ResurrectResponse resurrectResponse(std::move(packetData));
    resurrectResponse.Resurrecter = guid;
    // By leewheel 2026-08-07: 修复复活被拒——AC 语义 status==0 拒绝/非0接受，
    // TC 3.4.3 语义 Response!=0 拒绝(0=接受,1=拒绝,2=超时)，原代码按 AC 语义写 1，
    // 移植到 TC 后语义反转导致 bot 永远拒绝复活(不起来也不释放灵魂)
    resurrectResponse.Response = 0;
    bot->GetSession()->HandleResurrectResponse(resurrectResponse);

    return true;
}
