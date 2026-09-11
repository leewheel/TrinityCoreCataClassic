#include "UnlockItemAction.h"
//By leewheel 20260709: 添加Item.h解决C2027未定义类型Item
#include "Item.h"
//End By leewheel
#include "PlayerbotAI.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"

inline constexpr uint32_t PICK_LOCK_SPELL_ID = 1804;

bool UnlockItemAction::Execute(Event /*event*/)
{
    bool foundLockedItem = false;

    Item* item = botAI->FindLockedItem();
    if (item)
    {
        UnlockItem(item);
        foundLockedItem = true;
    }

    return foundLockedItem;
}

void UnlockItemAction::UnlockItem(Item* item)
{
    // Use CastSpell to unlock the item
    if (botAI->CastSpell(PICK_LOCK_SPELL_ID, bot, item))
    {
        std::ostringstream out;
        //By leewheel 2026-09-09: TC使用GetName(locale)方法，ItemTemplate_GetName是AC函数
        out << "对 " << item->GetTemplate()->GetName(LOCALE_enUS) << " 使用了开锁";
        botAI->TellMaster(out.str());
    }
    else
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("开锁法术施放失败。");
        //End By leewheel
}
