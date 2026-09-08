/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TaxiAction.h"

#include "Event.h"
#include "LastMovementValue.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "PlayerbotAIConfig.h"
#include "Config.h"

bool TaxiAction::Execute(Event event)
{
    botAI->RemoveShapeshift();

    LastMovement& movement = context->GetValue<LastMovement&>("last taxi")->Get();

    //By leewheel 2026-09-03 修复C4189警告：按AC原版恢复opcode分支判断(TC opcode经Playerbots.h映射到
    //CMSG_ENABLE_TAXI_NODE/CMSG_TAXI_NODE_STATUS_QUERY)，p从死变量恢复实际引用
    WorldPacket& p = event.getPacket();
    std::string const param = event.getParam();
    if ((!p.empty() && (p.GetOpcode() == CMSG_TAXICLEARALLNODES || p.GetOpcode() == CMSG_TAXICLEARNODE)) ||
        param == "clear")
    //End By leewheel
    {
        movement.taxiNodes.clear();
        movement.Set(nullptr);
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "taxi_ready_next_flight", "我已经准备好开始下一段飞行了", {}));
        //End By leewheel
        return true;
    }

    GuidVector units = *context->GetValue<GuidVector>("nearest npcs");
    for (ObjectGuid const guid : units)
    {
        Creature* npc = ObjectAccessor::GetCreature(*bot, guid);
        if (!npc || !npc->IsAlive())
            continue;

        if (!(npc->GetNpcFlags() & UNIT_NPC_FLAG_FLIGHTMASTER))
            continue;

        if (bot->GetDistance(npc) > sPlayerbotAIConfig.farDistance)
            continue;

        bot->GetSession()->SendLearnNewTaxiNode(npc);
        uint32 curloc = sObjectMgr->GetNearestTaxiNode(npc->GetPositionX(), npc->GetPositionY(), npc->GetPositionZ(),
                                                       npc->GetMapId(), bot->GetTeamId());

        std::vector<uint32> nodes;
        for (uint32 i = 0; i < sTaxiPathStore.GetNumRows(); ++i)
        {
            if (TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(i))
                if (entry->from() == curloc)
                {
                    nodes.push_back(i);
                }
        }

        // Only for follower bots
        if (botAI->HasRealPlayerMaster())
        {
            uint32 index = botAI->GetGroupSlotIndex(bot);
            uint32 delay = sPlayerbotAIConfig.botTaxiDelayMin +
                          index * sPlayerbotAIConfig.botTaxiGapMs +
                          urand(0, sPlayerbotAIConfig.botTaxiGapJitterMs);

            delay = std::min(delay, sPlayerbotAIConfig.botTaxiDelayMax);

            // Store the NPC's GUID so we can re-acquire the pointer later
            ObjectGuid npcGuid = npc->GetGUID();

            // schedule the take-off
            botAI->AddTimedEvent(
                [bot = bot, &movement, npcGuid]() -> void
                {
                    if (Creature* npcPtr = ObjectAccessor::GetCreature(*bot, npcGuid))
                        if (!movement.taxiNodes.empty())
                            bot->ActivateTaxiPathTo(movement.taxiNodes, npcPtr, 0);
                },
                delay);
            botAI->SetNextCheckDelay(delay + 50);
            return true;
        }

        if (param == "?")
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMasterNoFacing("=== 飞行 ===");
            //End By leewheel

            uint32 index = 1;
            for (uint32 node : nodes)
            {
                TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(node);
                if (!entry)
                    continue;

                TaxiNodesEntry const* dest = sTaxiNodesStore.LookupEntry(entry->to());
                if (!dest)
                    continue;

                std::ostringstream out;
                //By leewheel 2026-07-10: TC的LocalizedString使用LocaleConstant而非int
                out << index++ << ": " << dest->Name[DEFAULT_LOCALE];
                //End By leewheel
                botAI->TellMasterNoFacing(out.str());
            }

            return true;
        }

        uint32 selected = atoi(param.c_str());
        if (selected)
        {
            uint32 path = nodes[selected - 1];
            TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(path);
            if (!entry)
                return false;

            return bot->ActivateTaxiPathTo({entry->from(), entry->to()}, npc, 0);
        }

        if (!movement.taxiNodes.empty() && !bot->ActivateTaxiPathTo(movement.taxiNodes, npc, 0))
        {
            movement.taxiNodes.clear();
            movement.Set(nullptr);
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "taxi_cant_fly_with_you", "我没法和你一起飞行", {}));
            //End By leewheel
            return false;
        }

        return true;
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "taxi_no_flightmaster_nearby", "附近找不到可以对话的飞行管理员", {}));
    //End By leewheel
    return false;
}
