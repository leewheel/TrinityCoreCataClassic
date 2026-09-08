/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_NEARESTUNITSVALUE_H
#define PLAYERBOTS_NEARESTUNITSVALUE_H

#include "PlayerbotAIConfig.h"
#include "Unit.h"
#include "Value.h"

class PlayerbotAI;

class NearestUnitsValue : public ObjectGuidListCalculatedValue
{
public:
    //By leewheel 2026-07-20: checkInterval从1改为1000ms，1=无缓存每次Get()都全网格扫描，500bot时严重卡顿
    NearestUnitsValue(PlayerbotAI* botAI, std::string const name = "nearest units",
                      float range = sPlayerbotAIConfig.sightDistance, bool ignoreLos = false, uint32 checkInterval = 1000)
        : ObjectGuidListCalculatedValue(botAI, name, checkInterval), range(range), ignoreLos(ignoreLos)
    //End By leewheel
    {
    }

    GuidVector Calculate() override;

protected:
    virtual void FindUnits(std::list<Unit*>& targets) = 0;
    virtual bool AcceptUnit(Unit* unit) = 0;

    float range;
    bool ignoreLos;
};

#endif
