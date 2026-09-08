/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "IsFacingValue.h"
#include <cmath>

#include "Playerbots.h"

bool IsFacingValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);
    if (!target)
        return false;

    //By leewheel 2026-09-03 修复C4305警告：M_PI_2为double字面量，HasInArc参数为float，显式转换
    return bot->HasInArc(float(M_PI_2), target);
    //End By leewheel
}
