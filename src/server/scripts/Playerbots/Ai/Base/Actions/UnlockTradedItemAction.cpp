#include "UnlockTradedItemAction.h"
#include "PlayerbotAI.h"
#include "TradeData.h"
#include "SpellInfo.h"
//By leewheel 2026-07-10: 需要包含Item.h以使用Item类型
#include "Item.h"
//End By leewheel

inline constexpr uint32_t PICK_LOCK_SPELL_ID = 1804;

bool UnlockTradedItemAction::Execute(Event /*event*/)
{
    Player* trader = bot->GetTrader();
    if (!trader)
        return false; // No active trade session

    TradeData* tradeData = trader->GetTradeData();
    if (!tradeData)
        return false; // No trade data available

    Item* lockbox = tradeData->GetItem(TRADE_SLOT_NONTRADED);
    if (!lockbox)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("不可交易栏中没有物品。");
        //End By leewheel
        return false;
    }

    if (!CanUnlockItem(lockbox))
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("无法解锁该物品。");
        //End By leewheel
        return false;
    }

    UnlockItem(lockbox);
    return true;
}

bool UnlockTradedItemAction::CanUnlockItem(Item* item)
{
    if (!item)
        return false;

    ItemTemplate const* itemTemplate = item->GetTemplate();
    if (!itemTemplate)
        return false;

    // Ensure the bot is a rogue and has Lockpicking skill
    if (bot->getClass() != CLASS_ROGUE || !botAI->HasSkill(SKILL_LOCKPICKING))
        return false;

    // Ensure the item is actually locked
    //By leewheel 2026-07-10: TC中LockID是方法而非成员变量
    if (itemTemplate->GetLockID() == 0 || !item->IsLocked())
    //End By leewheel
        return false;

    // Check if the bot's Lockpicking skill is high enough
    //By leewheel 2026-07-10: TC中LockID是方法
    uint32 lockId = itemTemplate->GetLockID();
    //End By leewheel
    LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);
    if (!lockInfo)
        return false;

    uint32 botSkill = bot->GetSkillValue(SKILL_LOCKPICKING);
    for (uint8 j = 0; j < 8; ++j)
    {
        if (lockInfo->Type[j] == LOCK_KEY_SKILL && SkillByLockType(LockType(lockInfo->Index[j])) == SKILL_LOCKPICKING)
        {
            uint32 requiredSkill = lockInfo->Skill[j];
            if (botSkill >= requiredSkill)
                return true;
            else
            {
                std::ostringstream out;
                out << "开锁技能不足 (" << botSkill << "/" << requiredSkill << ") 无法解锁: "
                    << item->GetTemplate()->GetName();
                botAI->TellMaster(out.str());
            }
        }
    }

    return false;
}

void UnlockTradedItemAction::UnlockItem(Item* item)
{
    if (!bot->HasSpell(PICK_LOCK_SPELL_ID))
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("无法解锁,缺少开锁技能。");
        //End By leewheel
        return;
    }

    // Use CastSpell to unlock the item
    if (botAI->CastSpell(PICK_LOCK_SPELL_ID, bot->GetTrader(), item)) // Unit target is trader
    {
        std::ostringstream out;
        out << "正在解锁交易物品: " << item->GetTemplate()->GetName();
        botAI->TellMaster(out.str());
    }
    else
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("开锁法术施放失败。");
        //End By leewheel
    }
}
