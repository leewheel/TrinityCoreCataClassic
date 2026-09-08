/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RememberTaxiAction.h"

//By leewheel 2026-07-09: 确保CMSG_ACTIVATETAXI等opcode宏可见
#include "Playerbots.h"
//End By leewheel
#include "Event.h"
#include "LastMovementValue.h"
#include "AiObjectContext.h"

bool RememberTaxiAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-03: 修复——客户端包经WorldSocket读opcode后rpos=2，去掉rpos(0)防止把opcode当包体读

    switch (p.GetOpcode())
    {
        case CMSG_ACTIVATETAXI:
        {
            //By leewheel 2026-08-04: 长度检查防止伪造包越界崩溃
            //CMSG_ACTIVATETAXI包体: vendor(8)+node(4)+groundMount(4)+flyingMount(4)=20字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 20)
                return false;
            //End By leewheel

            LastMovement& movement = context->GetValue<LastMovement&>("last taxi")->Get();
            movement.taxiNodes.clear();
            //By leewheel 2026-08-03: TC 3.4.3的CMSG_ACTIVATETAXI为Vendor+Node+GroundMountID+FlyingMountID四字段
            //(AC 3.3.5为taxiMaster+from+to三字段)，按核心格式读取，taxiNodes只记录目标节点
            ObjectGuid vendor;
            uint32 node, groundMount, flyingMount;
            p >> vendor >> node >> groundMount >> flyingMount;
            movement.taxiMaster = vendor;
            movement.taxiNodes.push_back(node);
            return true;
        }
        case CMSG_ACTIVATETAXIEXPRESS:
        {
            //By leewheel 2026-08-04: 长度检查防止伪造包越界崩溃
            //CMSG_ACTIVATETAXIEXPRESS包体头: guid(8)+node_count(4)=12字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 12)
                return false;
            //End By leewheel

            ObjectGuid guid;
            uint32 node_count;
            p >> guid >> node_count;

            //By leewheel 2026-08-04: 限制node_count上界防止伪造大值导致越界读取或OOM
            //合法的飞行路线节点数不超过几百个,设上界256足够覆盖所有合法情况
            if (node_count > 256)
                return false;
            //剩余数据不足以容纳所有节点时也拒绝
            if ((p.wpos() - p.rpos()) < node_count * sizeof(uint32))
                return false;
            //End By leewheel

            LastMovement& movement = context->GetValue<LastMovement&>("last taxi")->Get();
            movement.taxiNodes.clear();
            for (uint32 i = 0; i < node_count; ++i)
            {
                uint32 node;
                p >> node;
                movement.taxiNodes.push_back(node);
            }

            return true;
        }
    }

    return false;
}
