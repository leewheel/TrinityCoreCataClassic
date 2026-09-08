/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RepairAllAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "Playerbots.h"

bool RepairAllAction::Execute(Event /*event*/)
{
    GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
    for (ObjectGuid const guid : npcs)
    {
        Creature* unit = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_REPAIR, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
        if (!unit)
            continue;

        if (bot->HasUnitState(UNIT_STATE_DIED))
            bot->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

        bot->SetFacingToObject(unit);
        float discountMod = bot->GetReputationPriceDiscount(unit);

        uint32 botMoney = bot->GetMoney();
        if (botAI->HasCheat(BotCheatMask::gold))
        {
            bot->SetMoney(10000000);
        }

        // By leewheel 2026-07-08: TC的DurabilityRepair返回void且只有3个参数
        // AC返回uint32(修理费用)且有4个参数，TC不支持获取修理费用
        uint32 totalCost = 0;
        bot->DurabilityRepair(EQUIPMENT_SLOT_MAINHAND, true, discountMod);
        bot->DurabilityRepair(EQUIPMENT_SLOT_RANGED, true, discountMod);
        bot->DurabilityRepair(EQUIPMENT_SLOT_OFFHAND, true, discountMod);

        bot->DurabilityRepairAll(true, discountMod, false);
        // End By leewheel

        if (botAI->HasCheat(BotCheatMask::gold))
        {
            bot->SetMoney(botMoney);
        }

        if (totalCost > 0)
        {
            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "修理费: " << chat->formatMoney(totalCost) << " (" << unit->GetName() << ")";
            //End By leewheel
            botAI->TellMasterNoFacing(out.str());

            bot->PlayDistanceSound(1116);
        }

        context->GetValue<uint32>("death count")->Set(0);

        return true;
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellError("找不到可以修理的NPC");
    //End By leewheel
    return false;
}
