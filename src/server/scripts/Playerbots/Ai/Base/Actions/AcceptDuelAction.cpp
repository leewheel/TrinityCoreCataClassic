/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptDuelAction.h"

#include "Event.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "AiObjectContext.h"

bool AcceptDuelAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 长度检查防止越界崩溃, SMSG_DUEL_REQUESTED需2个ObjectGuid=16字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 16)
        return false;
    //End By leewheel

    ObjectGuid flagGuid;
    p >> flagGuid;
    ObjectGuid playerGuid;
    p >> playerGuid;

    // do not auto duel with low hp
    if ((!botAI->HasRealPlayerMaster() || (botAI->GetMaster() && botAI->GetMaster()->GetGUID() != playerGuid)) &&
        AI_VALUE2(uint8, "health", "self target") < 90)
    {
        bot->GetSession()->HandleDuelCancelled();
        return false;
    }

    bot->GetSession()->HandleDuelAccepted(playerGuid);

    botAI->ResetStrategies();
    return true;
}
