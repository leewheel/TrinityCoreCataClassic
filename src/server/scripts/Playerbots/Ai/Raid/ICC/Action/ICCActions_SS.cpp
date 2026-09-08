/*
 * 鍐板啝鍫″瀿 钀ㄩ瞾娉曞皵澶х帇 Boss 鍔ㄤ綔绛栫暐
 *
 * 浣滆€? leewheel
 */

#include "ICCActions.h"
#include "NearestNpcsValue.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "Vehicle.h"
#include "RtiValue.h"
#include "GenericSpellActions.h"
#include "ICCTriggers.h"
#include "Multiplier.h"

bool IccValkyreSpearAction::Execute(Event /*event*/)
{
    // Find the nearest spear
    Creature* spear = bot->FindNearestCreature(NPC_SPEAR, 100.0f);
    if (!spear)
        return false;

    // Move to the spear if not in range
    if (!spear->IsWithinDistInMap(bot, INTERACTION_DISTANCE))
        return MoveTo(spear, INTERACTION_DISTANCE);

    // Remove shapeshift forms
    botAI->RemoveShapeshift();

    // Stop movement and click the spear
    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    spear->HandleSpellClick(bot);

    // Dismount if mounted
    //By leewheel 2026-07-27: TC鐨凥andleCancelMountAuraOpcode鎺ユ敹CancelMountAura缁撴瀯浣擄紝鐢╓PPCompat鐨凞oCancelMountAura
    WPPCompat::DoCancelMountAura(bot->GetSession());
    //End By leewheel

    return false;
}

bool IccSisterSvalnaAction::Execute(Event /*event*/)
{
    Unit* svalna = AI_VALUE2(Unit*, "find target", "37126");
    if (!svalna || !svalna->HasAura(SPELL_AETHER_SHIELD))
        return false;

    // Check if bot has the spear item
    if (!botAI->HasItemInInventory(ITEM_SPEAR))
        return false;

    // Get all items from inventory
    std::vector<Item*> items = botAI->GetInventoryItems();
    for (Item* item : items)
    {
        if (item->GetEntry() == ITEM_SPEAR)
        {
            // Use spear on Svalna
            botAI->ImbueItem(item, svalna);
            return false;
        }
    }

    return false;
}
