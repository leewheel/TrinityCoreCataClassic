/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "IsMovingValue.h"

#include "Playerbots.h"

bool IsMovingValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);

    if (!target)
        return false;

    return target->isMoving();
}

bool IsSwimmingValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);

    if (!target)
        return false;

    //By leewheel 20260709: TC的Unit使用GetLiquidStatus而非GetLiquidData().Status
    ZLiquidStatus targetInLiquidState = target->GetLiquidStatus();

    return targetInLiquidState & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER);
}
