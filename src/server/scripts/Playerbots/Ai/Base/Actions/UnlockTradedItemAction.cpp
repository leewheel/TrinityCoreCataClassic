#include "UnlockTradedItemAction.h"
#include "PlayerbotAI.h"
#include "TradeData.h"
#include "SpellInfo.h"
#include "Item.h"
#include "Playerbots.h"

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
    //By leewheel 2026-09-09: TC-Cata无SKILL_LOCKPICKING技能常量，盗贼自动获得开锁能力
    if (bot->getClass() != CLASS_ROGUE)
    //End By leewheel
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

    //By leewheel 2026-09-09: TC-Cata无SKILL_LOCKPICKING技能，盗贼开锁等级=等级*5
    uint32 botSkill = bot->GetLevel() * 5;
    //End By leewheel
    for (uint8 j = 0; j < 8; ++j)
    {
        //By leewheel 2026-09-09: TC-Cata用LOCKTYPE_LOCKPICKING直接判断，而非SkillByLockType
        if (lockInfo->Type[j] == LOCK_KEY_SKILL && lockInfo->Index[j] == LOCKTYPE_LOCKPICKING)
        //End By leewheel
        {
            uint32 requiredSkill = lockInfo->Skill[j];
            if (botSkill >= requiredSkill)
                return true;
            else
            {
                std::ostringstream out;
                out << "开锁技能不足 (" << botSkill << "/" << requiredSkill << ") 无法解锁: "
                    << ItemTemplate_GetName(item->GetTemplate());
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
        out << "正在解锁交易物品: " << ItemTemplate_GetName(item->GetTemplate());
        botAI->TellMaster(out.str());
    }
    else
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("开锁法术施放失败。");
        //End By leewheel
    }
}
