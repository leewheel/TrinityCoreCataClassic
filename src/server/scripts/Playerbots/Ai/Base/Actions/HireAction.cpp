/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "HireAction.h"

#include "Event.h"
#include "RandomPlayerbotMgr.h"
#include "PlayerbotAI.h"

bool HireAction::Execute(Event /*event*/)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    if (!RandomPlayerbotMgr::instance().IsRandomBot(bot))
        return false;

    uint32 account = master->GetSession()->GetAccountId();
    QueryResult results = CharacterDatabase.PQuery("SELECT COUNT(*) FROM characters WHERE account = {}", account);

    uint32 charCount = 10;
    if (results)
    {
        Field* fields = results->Fetch();
        charCount = uint32(fields[0].Get<uint64>());
    }

    if (charCount >= 10)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("你已经拥有最大数量的角色");
        //End By leewheel
        return false;
    }

    if (bot->GetLevel() > master->GetLevel())
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("你不能雇佣比你等级高的角色");
        //End By leewheel
        return false;
    }

    uint32 discount = RandomPlayerbotMgr::instance().GetTradeDiscount(bot, master);
    uint32 m = 1 + (bot->GetLevel() / 10);
    uint32 moneyReq = m * 5000 * bot->GetLevel();
    if (discount < moneyReq)
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "你不能雇佣我——我们还不熟。请确保你有至少 " << chat->formatMoney(moneyReq)
            << " 的交易折扣";
        //End By leewheel
        botAI->TellMaster(out.str());
        return false;
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("下次你重新登录时我将加入你");
    //End By leewheel

    bot->SetMoney(moneyReq);
    RandomPlayerbotMgr::instance().Remove(bot);
    CharacterDatabase.PExecute("UPDATE characters SET account = {} WHERE guid = {}", account,
                              bot->GetGUID().GetCounter());

    return true;
}
