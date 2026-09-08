#include "Playerbots.h"
#include "PoSTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"

bool IckAndKrickTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "36476");
    if (!boss)
        return false;

    return true;
}

bool TyrannusTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "36794");
    if (!boss)
        return false;

    return true;
}
