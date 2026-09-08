/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellMasterAction.h"

#include "Event.h"
#include "Playerbots.h"

bool TellMasterAction::Execute(Event /*event*/)
{
    botAI->TellMaster(text);
    return true;
}

bool OutOfReactRangeAction::Execute(Event /*event*/)
{
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("等等我！");
    //End By leewheel
    return true;
}

bool OutOfReactRangeAction::isUseful()
{
    bool canFollow = Follow(AI_VALUE(Unit*, "group leader"));
    if (!canFollow)
    {
        return false;
    }

    return true;
}
