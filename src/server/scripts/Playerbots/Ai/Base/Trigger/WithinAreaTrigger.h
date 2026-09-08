/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option) any later version.
 */

#ifndef PLAYERBOTS_WITHINAREATRIGGER_H
#define PLAYERBOTS_WITHINAREATRIGGER_H

#include "Trigger.h"

class PlayerbotAI;

//By leewheel 2026-07-09: AC使用AreaTrigger结构体，TC使用AreaTriggerEntry(DBC条目)
struct AreaTriggerEntry;
//End By leewheel

class WithinAreaTrigger : public Trigger
{
public:
    WithinAreaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "within area trigger") {}

    bool IsActive() override;

private:
    //By leewheel 2026-07-09: 参数类型从AreaTrigger改为AreaTriggerEntry
    bool IsPointInAreaTriggerZone(AreaTriggerEntry const* atEntry, uint32 mapid, float x, float y, float z, float delta);
    //End By leewheel
};

#endif
