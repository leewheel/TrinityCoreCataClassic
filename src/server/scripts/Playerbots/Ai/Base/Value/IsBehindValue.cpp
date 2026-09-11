/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "IsBehindValue.h"
#include <cmath>

#include "Playerbots.h"

bool IsBehindValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);
    if (!target)
        return false;

    float targetOrientation = target->GetOrientation();

    //By leewheel 2026-09-03 修复C4305警告：M_PI/M_PI_2为double字面量，统一改float常量
    float deltaAngle = Position::NormalizeOrientation(targetOrientation - target->GetAngle(bot));
    if (deltaAngle > float(M_PI))
        deltaAngle -= 2.0f * float(M_PI); // -PI..PI

    return fabs(deltaAngle) > float(M_PI_2);
    //End By leewheel
}
