/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TalkToQuestGiverAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "ItemUsageValue.h"
#include "Object.h"
#include "Playerbots.h"
#include "QuestDef.h"
#include "StatsWeightCalculator.h"
#include "WorldPacket.h"
#include "BroadcastHelper.h"

bool TalkToQuestGiverAction::ProcessQuest(Quest const* quest, Object* questGiver)
{
    bool isCompleted = false;
    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "任务 ";
    //End By leewheel

    QuestStatus status = bot->GetQuestStatus(quest->GetQuestId());
    Player* master = GetMaster();

    if (sPlayerbotAIConfig.syncQuestForPlayer && master)
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(master);
        if (!masterBotAI || masterBotAI->IsRealPlayer())
        {
            QuestStatus masterStatus = master->GetQuestStatus(quest->GetQuestId());
            if (masterStatus == QUEST_STATUS_INCOMPLETE || masterStatus == QUEST_STATUS_FAILED)
                isCompleted |= CompleteQuest(master, quest->GetQuestId());
        }
    }

    if (sPlayerbotAIConfig.syncQuestWithPlayer)
    {
        if (master && master->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE &&
            (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_FAILED))
        {
            isCompleted |= CompleteQuest(bot, quest->GetQuestId());
            status = bot->GetQuestStatus(quest->GetQuestId());
        }
    }

    switch (status)
    {
    case QUEST_STATUS_COMPLETE:
        isCompleted |= TurnInQuest(quest, questGiver, out);
        break;
    case QUEST_STATUS_INCOMPLETE:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffff0000未完成|r";
        //End By leewheel
        break;
    case QUEST_STATUS_NONE:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cff00ff00可接取|r";
        //End By leewheel
        break;
    case QUEST_STATUS_FAILED:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffff0000已失败|r";
        //End By leewheel
        break;
    default:
        break;
    }

    out << ": " << chat->FormatQuest(quest);
    botAI->TellMaster(out);

    return isCompleted;
}

bool TalkToQuestGiverAction::TurnInQuest(Quest const* quest, Object* questGiver, std::ostringstream& out)
{
    uint32 questID = quest->GetQuestId();

    if (bot->GetQuestRewardStatus(questID))
        return false;

    bot->PlayDistanceSound(621);

    if (quest->GetRewChoiceItemsCount() == 0)
        RewardNoItem(quest, questGiver, out);
    else if (quest->GetRewChoiceItemsCount() == 1)
        RewardSingleItem(quest, questGiver, out);
    else
    {
        RewardMultipleItem(quest, questGiver, out);
    }

    if (botAI->HasStrategy("debug quest", BotState::BOT_STATE_NON_COMBAT) || botAI->HasStrategy("debug rpg", BotState::BOT_STATE_COMBAT))
    {
        const Quest* pQuest = sObjectMgr->GetQuestTemplate(questID);
        const std::string text_quest = ChatHelper::FormatQuest(pQuest);
        TC_LOG_INFO("playerbots", "{} => Quest [ {} ] completed", bot->GetName(), pQuest->GetTitle());
        //By leewheel 2026-08-01: 玩家可见文本中文化
        bot->Say("任务 [ " + text_quest + " ] 已完成", LANG_UNIVERSAL);
        //End By leewheel
    }

    return true;
}

void TalkToQuestGiverAction::RewardNoItem(Quest const* quest, Object* questGiver, std::ostringstream& out)
{
    std::map<std::string, std::string> args;
    args["%quest"] = chat->FormatQuest(quest);

    if (bot->CanRewardQuest(quest, false))
    {
        out << PlayerbotTextMgr::instance().GetBotText("quest_status_completed", args);
        BroadcastHelper::BroadcastQuestTurnedIn(botAI, bot, quest);

        //By leewheel 2026-07-10: TC的RewardQuest签名需要LootItemType和rewardId参数
        bot->RewardQuest(quest, LootItemType(0), 0, questGiver, false);
        //End By leewheel
    }
    else
    {
        out << PlayerbotTextMgr::instance().GetBotText("quest_status_unable_to_complete", args);
    }
}

void TalkToQuestGiverAction::RewardSingleItem(Quest const* quest, Object* questGiver, std::ostringstream& out)
{
    int index = 0;
    ItemTemplate const* item = sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[index]);
    std::map<std::string, std::string> args;
    args["%quest"] = chat->FormatQuest(quest);
    args["%item"] = chat->FormatItem(item);

    //By leewheel 2025-07-10
    // TC使用2参数或4参数版本CanRewardQuest，index作为rewardId传入LootItemType::Item
    if (bot->CanRewardQuest(quest, LootItemType::Item, quest->RewardChoiceItemId[index], false))
    {
        out << PlayerbotTextMgr::instance().GetBotText("quest_status_complete_single_reward", args);
        BroadcastHelper::BroadcastQuestTurnedIn(botAI, bot, quest);
        bot->RewardQuest(quest, LootItemType::Item, quest->RewardChoiceItemId[index], questGiver, true);
    }
    //End By leewheel
    else
    {
        out << PlayerbotTextMgr::instance().GetBotText("quest_status_unable_to_complete", args);
    }
}

ItemIds TalkToQuestGiverAction::BestRewards(Quest const* quest)
{
    ItemIds returnIds;
    ItemUsage bestUsage = ITEM_USAGE_NONE;
    if (quest->GetRewChoiceItemsCount() == 0)
        return returnIds;
    else if (quest->GetRewChoiceItemsCount() == 1)
        return {0};
    else
    {
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
            if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE)
                bestUsage = ITEM_USAGE_EQUIP;
            else if (usage == ITEM_USAGE_BAD_EQUIP && bestUsage != ITEM_USAGE_EQUIP)
                bestUsage = usage;
            else if (usage != ITEM_USAGE_NONE && bestUsage == ITEM_USAGE_NONE)
                bestUsage = usage;
        }
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
            if (usage == bestUsage || usage == ITEM_USAGE_REPLACE)
                returnIds.insert(i);
        }
        return returnIds;
    }
}

void TalkToQuestGiverAction::RewardMultipleItem(Quest const* quest, Object* questGiver, std::ostringstream& out)
{
    std::set<uint32> bestIds;

    std::ostringstream outid;
    //By leewheel 2026-08-01: 按上游(4fb82ed0)统一命名约定，IsAlt改为IsAltBot
    if (!botAI->IsAltBot() || sPlayerbotAIConfig.autoPickReward == "yes")
    {
        bestIds = BestRewards(quest);
        if (!bestIds.empty())
        {
            StatsWeightCalculator calc(bot);
            uint32 best = 0;
            float bestScore = 0;
            for (uint32 id : bestIds)
            {
                float score = calc.CalculateItem(quest->RewardChoiceItemId[id]);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = id;
                }
            }
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[best]);
            //By leewheel 2025-07-10
            // TC使用LootItemType::Item
            bot->RewardQuest(quest, LootItemType::Item, quest->RewardChoiceItemId[best], questGiver, true);
            //End By leewheel
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "已奖励 " << ChatHelper::FormatItem(item);
            //End By leewheel
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "找不到合适的奖励，寻求帮助....";
            //End By leewheel
            AskToSelectReward(quest, out, true);
        }
    }
    else if (sPlayerbotAIConfig.autoPickReward == "no")
    {
        // Old functionality, list rewards.
        AskToSelectReward(quest, out, false);
    }
    else
    {
        // Try to pick the usable item. If multiple, list usable rewards.
        bestIds = BestRewards(quest);

        if (bestIds.size() > 1)
            AskToSelectReward(quest, out, true);

        else if (!bestIds.empty())
        {
            // Pick the first item
            uint32 firstId = *bestIds.begin();
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[firstId]);
            //By leewheel 2025-07-10
            // TC使用LootItemType::Item
            bot->RewardQuest(quest, LootItemType::Item, quest->RewardChoiceItemId[firstId], questGiver, true);
            //End By leewheel

            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "已奖励 " << ChatHelper::FormatItem(item);
            //End By leewheel
        }
    }
}

void TalkToQuestGiverAction::AskToSelectReward(Quest const* quest, std::ostringstream& out, bool forEquip)
{
    std::ostringstream msg;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    msg << "请选择奖励: ";
    //End By leewheel

    for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[i]);

        if (!forEquip || BestRewards(quest).count(i) > 0)
        {
            msg << chat->FormatItem(item);
        }
    }

    botAI->TellMaster(msg);
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "等待选择奖励";
    //End By leewheel
}

bool TurnInQueryQuestAction::Execute(Event event)
{
    WorldPacket packet = event.getPacket();
    //By leewheel 2026-08-04: 修复——TC的SMSG_QUESTGIVER_OFFER_REWARD格式与AC不同
    //AC格式: guid(8) + questId(4) = 12字节
    //TC格式(QuestGiverOfferReward): QuestGiverGUID(8) + QuestGiverCreatureID(4) + QuestID(4) + ...
    //原代码 pakcet >> guid >> questId 读12字节, questId读到的是QuestGiverCreatureID而非QuestID
    //导致后续 object->hasQuest(questId) 永远查不到任务, TurnInQueryQuest功能静默失效
    //修复: 按TC格式跳过QuestGiverCreatureID, 正确读取QuestID
    //包体最小: 8(QuestGiverGUID) + 4(QuestGiverCreatureID) + 4(QuestID) = 16字节
    if (packet.wpos() < packet.rpos() || (packet.wpos() - packet.rpos()) < 16)
        return false;

    ObjectGuid guid;
    uint32 questGiverCreatureId;  // 跳过, 业务不需要
    uint32 questId;
    packet >> guid >> questGiverCreatureId >> questId;
    //End By leewheel
    Object* object =
        ObjectAccessor::GetObjectByTypeMask(*bot, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
    if (!object || (!object->hasQuest(questId) && !object->hasInvolvedQuest(questId)))
    {
        return false;
    }
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    QuestStatus status = bot->GetQuestStatus(quest->GetQuestId());
    Player* master = GetMaster();

    if (sPlayerbotAIConfig.syncQuestForPlayer && master)
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(master);
        if (!masterBotAI || masterBotAI->IsRealPlayer())
        {
            QuestStatus masterStatus = master->GetQuestStatus(quest->GetQuestId());
            if (masterStatus == QUEST_STATUS_INCOMPLETE || masterStatus == QUEST_STATUS_FAILED)
                CompleteQuest(master, quest->GetQuestId());
        }
    }

    if (sPlayerbotAIConfig.syncQuestWithPlayer)
    {
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_FAILED)
        {
            CompleteQuest(bot, quest->GetQuestId());
            status = bot->GetQuestStatus(quest->GetQuestId());
        }
    }
    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "任务 ";
    //End By leewheel
    switch (status)
    {
    case QUEST_STATUS_COMPLETE:
        TurnInQuest(quest, object, out);
        break;
    case QUEST_STATUS_INCOMPLETE:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffff0000未完成|r";
        //End By leewheel
        break;
    case QUEST_STATUS_NONE:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cff00ff00可接取|r";
        //End By leewheel
        break;
    case QUEST_STATUS_FAILED:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffff0000已失败|r";
        //End By leewheel
        break;
    case QUEST_STATUS_REWARDED:
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffff0000已奖励|r";
        //End By leewheel
        break;
    default:
        break;
    }

    out << ": " << chat->FormatQuest(quest);
    botAI->TellMaster(out);
    return true;
}
