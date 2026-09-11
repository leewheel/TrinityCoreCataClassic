#include "OpenItemAction.h"
#include "PlayerbotAI.h"
//By leewheel 20260709: 添加Item.h解决C2027未定义类型Item
#include "Item.h"
//End By leewheel
#include "ItemTemplate.h"
#include "WorldPacket.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "LootObjectStack.h"
#include "AiObjectContext.h"
#include "SpellPackets.h" // By leewheel 2026-07-09

bool OpenItemAction::Execute(Event /*event*/)
{
    bool foundOpenable = false;

    Item* item = botAI->FindOpenableItem();
    if (item)
    {
        uint8 bag = item->GetBagSlot();  // Retrieves the bag slot (0 for main inventory)
        uint8 slot = item->GetSlot();    // Retrieves the actual slot inside the bag

        OpenItem(item, bag, slot);
        foundOpenable = true;
    }

    return foundOpenable;
}

void OpenItemAction::OpenItem(Item* item, uint8 bag, uint8 slot)
{
    // By leewheel 2026-07-09: TC的HandleOpenItemOpcode需要WorldPackets::Spells::OpenItem而不是WorldPacket
    // By leewheel 2026-07-09: 修复最令人头疼的解析(Most Vexing Parse)，使用花括号初始化
    WorldPackets::Spells::OpenItem openItem{WorldPacket(CMSG_OPEN_ITEM)};
    // End By leewheel
    openItem.PackSlot = bag;
    openItem.Slot = slot;
    bot->GetSession()->HandleOpenItemOpcode(openItem);
    // End By leewheel

    // Store the item GUID as the loot target
    LootObject lootObject;
    lootObject.guid = item->GetGUID();
    botAI->GetAiObjectContext()->GetValue<LootObject>("loot target")->Set(lootObject);

    std::ostringstream out;
    //By leewheel 2026-09-09: TC使用GetName(locale)方法，ItemTemplate_GetName是AC函数
    out << "打开物品: " << item->GetTemplate()->GetName(LOCALE_enUS);
    botAI->TellMaster(out.str());
}
