/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_XPGAINACTION_H
#define PLAYERBOTS_XPGAINACTION_H

#include "Action.h"

class PlayerbotAI;
class Unit;

class XpGainAction : public Action
{
public:
    XpGainAction(PlayerbotAI* botAI) : Action(botAI, "xp gain") {}

    bool Execute(Event event) override;

private:
    //By leewheel 2026-09-03 修复C4100警告：victim参数在TC移植中未使用，声明同步省略参数名
    void GiveXP(uint32 xp, Unit* /*victim*/);
    //End By leewheel
};

#endif
