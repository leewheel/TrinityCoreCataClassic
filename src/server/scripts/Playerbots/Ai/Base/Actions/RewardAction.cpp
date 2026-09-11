/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RewardAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "LootItemType.h"
#include "Playerbots.h"

bool RewardAction::Execute(Event event)
{
    std::string const link = event.getParam();

    ItemIds itemIds = chat->parseItems(link);
    if (itemIds.empty())
        return false;

    uint32 itemId = *itemIds.begin();

    GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
    for (ObjectGuid const guid : npcs)
    {
        if (Unit* npc = botAI->GetUnit(guid))
            if (Reward(itemId, npc))
                return true;
    }

    GuidVector gos = AI_VALUE(GuidVector, "nearest game objects");
    for (ObjectGuid const guid : gos)
    {
        if (GameObject* go = botAI->GetGameObject(guid))
            if (Reward(itemId, go))
                return true;
    }

    Unit* groupLeaderUnit = AI_VALUE(Unit*, "group leader");
    if (groupLeaderUnit && Reward(itemId, groupLeaderUnit))
        return true;

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellError("无法与任务给予者交谈");
    //End By leewheel
    return false;
}

bool RewardAction::Reward(uint32 itemId, Object* questGiver)
{
    QuestMenu& questMenu = bot->PlayerTalkClass->GetQuestMenu();
    for (uint32 iI = 0; iI < questMenu.GetMenuItemCount(); ++iI)
    {
        QuestMenuItem const& qItem = questMenu.GetItem(iI);

        uint32 questID = qItem.QuestId;
        Quest const* pQuest = sObjectMgr->GetQuestTemplate(questID);
        QuestStatus status = bot->GetQuestStatus(questID);

        // if quest is complete, turn it in
        if (status == QUEST_STATUS_COMPLETE && !bot->GetQuestRewardStatus(questID) &&
            pQuest->GetRewChoiceItemsCount() > 1 && bot->CanRewardQuest(pQuest, false))
        {
            for (uint8 rewardIdx = 0; rewardIdx < pQuest->GetRewChoiceItemsCount(); ++rewardIdx)
            {
                ItemTemplate const* pRewardItem = sObjectMgr->GetItemTemplate(pQuest->RewardChoiceItemId[rewardIdx]);
                if (itemId == pRewardItem->GetId())
                {
                    //By leewheel 2026-07-10: TC的RewardQuest需要LootItemType参数
                    bot->RewardQuest(pQuest, LootItemType::Item, rewardIdx, questGiver, false);
                    //End By leewheel

                    std::string const questTitle = pQuest->GetTitle();
                    //By leewheel 2026-09-09: TC-Cata的ItemTemplate::GetName需要locale参数
                    std::string const itemName = pRewardItem->GetName(LOCALE_enUS);
                    //End By leewheel

                    std::ostringstream out;
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << chat->FormatItem(pRewardItem) << " 已奖励";
                    //End By leewheel
                    botAI->TellMaster(out);

                    return true;
                }
            }
        }
    }

    return false;
}
