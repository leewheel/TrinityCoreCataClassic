/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PlayerbotAI.h"
#include "WipeAction.h"

bool WipeAction::Execute(Event event)
{
    Player* const owner = event.getOwner();
    Player* const master = this->botAI->GetMaster();

    if (owner != nullptr && master != nullptr && master->GetGUID() != owner->GetGUID())
        return false;

    //By leewheel 2026-09-04: 上游——已死亡的 bot 不再重复执行自尽(防对尸体重复 Kill)
    if (!bot->IsAlive())
        return false;

    bot->Kill(bot, bot);
    //End By leewheel

    return true;
}
