/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "SecurityCheckAction.h"

#include "Event.h"
#include "Playerbots.h"

bool SecurityCheckAction::isUseful()
{
    return RandomPlayerbotMgr::instance().IsRandomBot(bot)
        && botAI->GetMaster()
        && !botAI->GetMaster()->CanBeGameMaster()
        && !GET_PLAYERBOT_AI(botAI->GetMaster());
}

bool SecurityCheckAction::Execute(Event /*event*/)
{
    if (Group* group = bot->GetGroup())
    {
        LootMethod method = group->GetLootMethod();
        ItemQualities threshold = group->GetLootThreshold();
        if (method == MASTER_LOOT || method == FREE_FOR_ALL || threshold > ITEM_QUALITY_UNCOMMON)
        {
            if ((botAI->GetGroupLeader()->GetSession()->GetSecurity() == SEC_PLAYER) &&
                (!bot->GetGuildId() || bot->GetGuildId() != botAI->GetGroupLeader()->GetGuildId()))
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellError("只有我在你的公会里，我才会接受这种分配方式来战斗:/");
                //End By leewheel
                botAI->ChangeStrategy("+passive,+stay", BOT_STATE_NON_COMBAT);
                botAI->ChangeStrategy("+passive,+stay", BOT_STATE_COMBAT);
            }
            return true;
        }
    }

    return false;
}
