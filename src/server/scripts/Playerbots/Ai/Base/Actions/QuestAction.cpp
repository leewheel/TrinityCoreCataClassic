/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "QuestAction.h"
#include <sstream>
#include <algorithm>

#include "Chat.h"
#include "ChatHelper.h"
#include "Event.h"
#include "ItemTemplate.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "ReputationMgr.h"
#include "ServerFacade.h"
#include "BroadcastHelper.h"

bool QuestAction::Execute(Event event)
{
    ObjectGuid guid = event.getObject();
    Player* master = GetMaster();

    // Checks if the bot and botAI are valid
    if (!bot || !botAI)
        return false;

    // Sets guid based on bot or master target
    if (!guid)
    {
        if (!master)
        {
            guid = bot->GetTarget();
        }
        else
        {
            guid = master->GetTarget();
        }
    }

    if (guid)
    {
        return ProcessQuests(guid);
    }

    bool result = false;

    // Check the nearest NPCs
    GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
    for (auto const& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (unit && bot->GetDistance(unit) <= INTERACTION_DISTANCE)
        {
            result |= ProcessQuests(unit);
        }
    }

    // Checks the nearest game objects
    GuidVector gos = AI_VALUE(GuidVector, "nearest game objects");
    for (auto const& go : gos)
    {
        GameObject* gameobj = botAI->GetGameObject(go);
        if (gameobj && bot->GetDistance(gameobj) <= INTERACTION_DISTANCE)
        {
            result |= ProcessQuests(gameobj);
        }
    }

    return result;
}

bool QuestAction::CompleteQuest(Player* player, uint32 entry)
{
    Quest const* pQuest = sObjectMgr->GetQuestTemplate(entry);

    // If player doesn't have the quest
    if (!pQuest || player->GetQuestStatus(entry) == QUEST_STATUS_NONE)
    {
        return false;
    }

    // Add quest items for quests that require items
    for (uint8 x = 0; x < QUEST_ITEM_OBJECTIVES_COUNT; ++x)
    {
        uint32 id = pQuest->RequiredItemId[x];
        uint32 count = pQuest->RequiredItemCount[x];
        if (!id || !count)
        {
            continue;
        }

        uint32 curItemCount = player->GetItemCount(id, true);

        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count - curItemCount);
        if (msg == EQUIP_ERR_OK)
        {
            Item* item = player->StoreNewItem(dest, id, true);
            player->SendNewItem(item, count - curItemCount, true, false);
        }
    }

    // All creature/GO slain/casted (not required, but otherwise it will display "Creature slain 0/10")
    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        int32 creature = pQuest->RequiredNpcOrGo[i];
        uint32 creaturecount = pQuest->RequiredNpcOrGoCount[i];

        // TODO check if we need a REQSPELL condition, this methods and sql entry dosent seem implemented ?
        /*if (uint32 spell_id = pQuest->GetReqSpell[i])
        {
            for (uint16 z = 0; z < creaturecount; ++z)
            {
                player->CastedCreatureOrGO(creature, ObjectGuid(), spell_id);
            }
        }*/
        /*else*/
        if (creature > 0)
        {
            if (CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(creature))
                for (uint16 z = 0; z < creaturecount; ++z)
                {
                    //By leewheel 2026-09-09: TC-Cata的KilledMonster只接受Creature const*参数
                    //使用KilledMonsterCredit来给予击杀信用
                    player->KilledMonsterCredit(cInfo->Entry);
                }
        }
        else if (creature < 0)
        {
            for (uint16 z = 0; z < creaturecount; ++z)
            {
                player->KillCreditGO(-creature);
            }
        }
    }

    //By leewheel 2026-07-24: 使用TC的QuestObjective系统处理声望和金钱目标
    //替代AC的GetRepObjectiveFaction/GetRepObjectiveValue和GetRewOrReqMoney
    for (QuestObjective const& obj : pQuest->GetObjectives())
    {
        // 声望目标：MIN_REPUTATION(6)要求达到指定声望，INCREASE_REPUTATION(18)要求获得指定声望
        if (obj.Type == QUEST_OBJECTIVE_MIN_REPUTATION || obj.Type == QUEST_OBJECTIVE_INCREASE_REPUTATION)
        {
            uint32 factionId = static_cast<uint32>(obj.ObjectID);
            int32 repValue = obj.Amount;
            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId))
            {
                int32 curRep = player->GetReputationMgr().GetReputation(factionEntry);
                if (curRep < repValue)
                    player->GetReputationMgr().SetReputation(factionEntry, repValue);
            }
        }
        // 金钱目标：MONEY(8)要求玩家拥有一定金钱
        else if (obj.Type == QUEST_OBJECTIVE_MONEY)
        {
            uint32 requiredMoney = static_cast<uint32>(obj.Amount);
            if (player->GetMoney() < requiredMoney)
                player->ModifyMoney(static_cast<int32>(requiredMoney - player->GetMoney()));
        }
    }
    //End By leewheel

    const std::string text_quest = ChatHelper::FormatQuest(pQuest);
    if (botAI->HasStrategy("debug quest", BotState::BOT_STATE_NON_COMBAT) || botAI->HasStrategy("debug rpg", BotState::BOT_STATE_COMBAT))
    {
        TC_LOG_INFO("playerbots", "{} => Quest [ {} ] completed", bot->GetName(), pQuest->GetTitle());
        //By leewheel 2026-08-01: 玩家可见文本中文化
        bot->Say("任务 [ " + text_quest + " ] 已完成", LANG_UNIVERSAL);
    }
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMasterNoFacing("任务已完成 " + text_quest);

    player->CompleteQuest(entry);

    return true;
}

bool QuestAction::ProcessQuests(ObjectGuid questGiver)
{
    if (GameObject* gameObject = botAI->GetGameObject(questGiver))
        if (gameObject->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
            return ProcessQuests(gameObject);

    Creature* creature = botAI->GetCreature(questGiver);
    if (creature)
        return ProcessQuests(creature);

    return false;
}

//By leewheel 2026-07-24: 修复QuestMenu迭代失效bug - 与NewRpgBaseAction::InteractWithNpcOrGameObjectForQuest同理
//TC的handler在每次成功接/交任务后调用PrepareQuestMenu重建菜单，导致遍历中的引用失效
bool QuestAction::ProcessQuests(WorldObject* questGiver)
{
    ObjectGuid guid = questGiver->GetGUID();

    if (bot->GetDistance(questGiver) > INTERACTION_DISTANCE && !sPlayerbotAIConfig.syncQuestWithPlayer)
    {
        //if (botAI->HasStrategy("debug", BotState::BOT_STATE_COMBAT) || botAI->HasStrategy("debug", BotState::BOT_STATE_NON_COMBAT))

        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("无法与任务给予者交谈");
        return false;
    }

    if (!bot->HasInArc(CAST_ANGLE_IN_FRONT, questGiver, sPlayerbotAIConfig.sightDistance))
        bot->SetFacingToObject(questGiver);

    bot->SetTarget(guid);
    bot->PrepareQuestMenu(guid);

    QuestMenu& questMenu = bot->PlayerTalkClass->GetQuestMenu();
    // 拷贝任务ID到本地vector，避免ProcessQuest触发PrepareQuestMenu后menu失效
    std::vector<uint32> questIds;
    questIds.reserve(questMenu.GetMenuItemCount());
    for (uint32 i = 0; i < questMenu.GetMenuItemCount(); ++i)
        questIds.push_back(questMenu.GetItem(i).QuestId);

    for (uint32 questID : questIds)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questID);
        if (!quest)
            continue;

        ProcessQuest(quest, questGiver);
    }

    return true;
}
//End By leewheel

bool QuestAction::AcceptQuest(Quest const* quest, ObjectGuid questGiver)
{
    std::ostringstream out;

    uint32 questId = quest->GetQuestId();

    if (bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "已完成";
    else if (!botAI->IsAltBot() && !bot->CanTakeQuest(quest, false))
    {
        if (!bot->SatisfyQuestStatus(quest, false))
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "已在进行中";
        else
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "无法接取";
    }
    else if (!bot->SatisfyQuestLog(false))
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "任务日志已满";
    else if (!bot->CanAddQuest(quest, false))
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "背包已满";
    else
    {
        WorldPacket p(CMSG_QUESTGIVER_ACCEPT_QUEST);
        uint32 unk1 = 0;
        p << questGiver << questId << unk1;
        p.rpos(0);
        // By leewheel 2026-07-09: 使用WPPCompat兼容层
        WPPCompat::HandleQuestgiverAcceptQuestOpcode(bot->GetSession(), p);
        // End By leewheel

        if (bot->GetQuestStatus(questId) == QUEST_STATUS_NONE && sPlayerbotAIConfig.syncQuestWithPlayer)
        {
            Object* pObject = ObjectAccessor::GetObjectByTypeMask(*bot, questGiver,
                                                                  TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
            bot->AddQuest(quest, pObject);
        }

        if (bot->GetQuestStatus(questId) != QUEST_STATUS_NONE && bot->GetQuestStatus(questId) != QUEST_STATUS_REWARDED)
        {
            BroadcastHelper::BroadcastQuestAccepted(botAI, bot, quest);
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "已接取 " << chat->FormatQuest(quest);
            botAI->TellMaster(out);
            return true;
        }
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "无法接取";
    }

    out << " " << chat->FormatQuest(quest);
    botAI->TellMaster(out);

    return false;
}

bool QuestUpdateCompleteAction::Execute(Event event)
{
    // the action can hardly be triggered
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel

    uint32 questId = 0;
    p >> questId;

    p.print_storage();
    // TC_LOG_INFO("playerbots", "Packet: empty{} questId{}", p.empty(), questId);

    Quest const* qInfo = sObjectMgr->GetQuestTemplate(questId);
    if (qInfo)
    {
        // std::map<std::string, std::string> placeholders;
        // placeholders["%quest_link"] = format;

        // if (botAI->HasStrategy("debug quest", BotState::BOT_STATE_NON_COMBAT) || botAI->HasStrategy("debug rpg", BotState::BOT_STATE_COMBAT))
        // {
            //     TC_LOG_INFO("playerbots", "{} => Quest [ {} ] completed", bot->GetName(), qInfo->GetTitle());
            //     bot->Say("Quest [ " + format + " ] completed", LANG_UNIVERSAL);
            // }
        const auto format = ChatHelper::FormatQuest(qInfo);
        if (botAI->GetMaster())
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMasterNoFacing("任务已完成 " + format);
        BroadcastHelper::BroadcastQuestUpdateComplete(botAI, bot, qInfo);
        botAI->rpgStatistic.questCompleted++;
        // LOG_DEBUG("playerbots", "[New rpg] {} complete quest {}", bot->GetName(), qInfo->GetQuestId());
        // botAI->rpgStatistic.questCompleted++;
    }

    return true;
}

/*
* For creature or gameobject
*/
//By leewheel 2026-07-24: 适配TC的SMSG_QUEST_UPDATE_ADD_CREDIT包格式
//AC格式: questId(u32) >> entry(u32) >> available(u32) >> required(u32)
//TC格式: VictimGUID(ObjectGuid) >> QuestID(i32) >> ObjectID(i32) >> Count(u16) >> Required(u16) >> ObjectiveType(u8)
bool QuestUpdateAddKillAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)和长度检查
    //rpos(0)会把opcode当包体读导致字段错位; TC包体需8+4+4+2+2+1=21字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 21)
        return false;
    //End By leewheel

    ObjectGuid victimGuid;
    int32 questId, objectId;
    uint16 available, required;
    uint8 objectiveType;
    p >> victimGuid >> questId >> objectId >> available >> required >> objectiveType;

    const Quest* qInfo = sObjectMgr->GetQuestTemplate(questId);
    if (qInfo && objectId < 0)
    {
        // objectId为负数表示GO目标
        uint32 goEntry = static_cast<uint32>(-objectId);
        const GameObjectTemplate* info = sObjectMgr->GetGameObjectTemplate(goEntry);
        if (info)
        {
            std::string infoName = botAI->GetLocalizedGameObjectName(goEntry);
            BroadcastHelper::BroadcastQuestUpdateAddKill(botAI, bot, qInfo, available, required, infoName);
            if (botAI->GetMaster())
            {
                std::ostringstream out;
                out << infoName << " " << available << "/" << required << " " << ChatHelper::FormatQuest(qInfo);
                botAI->TellMasterNoFacing(out.str());
            }
        }
    }
    else if (qInfo && objectId > 0)
    {
        // objectId为正数表示生物目标
        uint32 creatureEntry = static_cast<uint32>(objectId);
        CreatureTemplate const* info = sObjectMgr->GetCreatureTemplate(creatureEntry);
        if (info)
        {
            std::string infoName = botAI->GetLocalizedCreatureName(creatureEntry);
            BroadcastHelper::BroadcastQuestUpdateAddKill(botAI, bot, qInfo, available, required, infoName);
            if (botAI->GetMaster())
            {
                std::ostringstream out;
                out << infoName << " " << available << "/" << required << " " << ChatHelper::FormatQuest(qInfo);
                botAI->TellMasterNoFacing(out.str());
            }
        }
    }
    return false;
}
//End By leewheel

bool QuestUpdateAddItemAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
        return false;
    //End By leewheel

    uint32 itemId, count;
    p >> itemId >> count;

    auto const* itemPrototype = sObjectMgr->GetItemTemplate(itemId);
    if (itemPrototype)
    {
        std::map<std::string, std::string> placeholders;
        placeholders["%item_link"] = botAI->GetChatHelper()->FormatItem(itemPrototype);
        uint32 availableItemsCount = botAI->GetInventoryItemsCountWithId(itemId);
        placeholders["%quest_obj_available"] = std::to_string(availableItemsCount);

        for (auto const& pair : botAI->GetCurrentQuestsRequiringItemId(itemId))
        {
            placeholders["%quest_link"] = chat->FormatQuest(pair.first);
            uint32 requiredItemsCount = pair.second;
            placeholders["%quest_obj_required"] = std::to_string(requiredItemsCount);
            if (botAI->HasStrategy("debug quest", BotState::BOT_STATE_COMBAT) || botAI->HasStrategy("debug quest", BotState::BOT_STATE_NON_COMBAT))
            {
                const auto text = PlayerbotTextMgr::instance().GetBotText("%quest_link - %item_link %quest_obj_available/%quest_obj_required", placeholders);
                botAI->Say(text);
                TC_LOG_INFO("playerbots", "{} => {}", bot->GetName(), text);
            }

            BroadcastHelper::BroadcastQuestUpdateAddItem(botAI, bot, pair.first, availableItemsCount, requiredItemsCount, itemPrototype);
        }
    }
    return false;
}

bool QuestItemPushResultAction::Execute(Event event)
{
    WorldPacket packet = event.getPacket();
    //By leewheel 2026-08-04: 修复——TC的SMSG_ITEM_PUSH_RESULT格式与AC完全不同
    //AC格式: playerGuid(8)+received(4)+created(4)+sendChatMessage(4)+bagSlot(1)+itemSlot(4)+itemEntry(4)
    //        +itemSuffixFactory(4)+itemRandomPropertyId(4)+count(4)+itemCount(4) = 45字节
    //TC格式: PlayerGUID(8)+Slot(1)+SlotInBag(4)+QuestLogItemID(4)+Quantity(4)+QuantityInInventory(4)
    //        +DungeonEncounterID(4)+BattlePetSpeciesID(4)+BattlePetBreedID(4)+BattlePetBreedQuality(4)
    //        +BattlePetLevel(4)+ItemGUID(8)+bits...+Item(ItemInstance)
    //原代码按AC格式读11字段45字节,在TC下全部错位,itemEntry读到的是Quantity等垃圾值
    //修复: 按TC格式读取,用ItemGUID反查itemEntry,用Quantity/QuantityInInventory作为count/itemCount
    //包体最小: 8+1+4*9+8 = 53字节(到ItemGUID结束)
    if (packet.wpos() < packet.rpos() || (packet.wpos() - packet.rpos()) < 53)
        return false;

    ObjectGuid guid;
    packet >> guid;  // PlayerGUID
    if (guid != bot->GetGUID())
        return false;

    uint8 slot;
    packet >> slot;  // 跳过 Slot
    int32 slotInBag, questLogItemId, quantity, quantityInInventory;
    int32 dungeonEncounterId, battlePetSpeciesId, battlePetBreedId, battlePetLevel;
    uint32 battlePetBreedQuality;
    packet >> slotInBag >> questLogItemId >> quantity >> quantityInInventory;
    packet >> dungeonEncounterId >> battlePetSpeciesId >> battlePetBreedId;
    packet >> battlePetBreedQuality >> battlePetLevel;

    ObjectGuid itemGuid;
    packet >> itemGuid;  // ItemGUID,用于反查itemEntry

    // 用ItemGUID反查Item,获取itemEntry
    uint32 itemEntry = 0;
    if (Item* item = bot->GetItemByGuid(itemGuid))
        itemEntry = item->GetEntry();
    //End By leewheel

    const ItemTemplate* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto)
        return false;

    //By leewheel 2026-08-04: TC字段映射 quantity=count(本次获得), quantityInInventory=itemCount(背包总量)
    uint32 count = quantity;
    uint32 itemCount = quantityInInventory;
    //End By leewheel

    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (!questId)
            continue;

        const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
        {
            uint32 itemId = quest->RequiredItemId[i];
            if (!itemId)
                continue;

            int32 previousCount = itemCount - count;
            if (itemId == itemEntry && uint32(previousCount) < quest->RequiredItemCount[i])
            {
                if (botAI->GetMaster())
                {
                    std::string itemLink = ChatHelper::FormatItem(proto);
                    std::ostringstream out;
                    int32 required = quest->RequiredItemCount[i];
                    int32 available = std::min((int32)itemCount, required);
                    out << itemLink << " " << available << "/" << required << " " << ChatHelper::FormatQuest(quest);
                    botAI->TellMasterNoFacing(out.str());
                }
            }
        }
    }

    return false;
}

bool QuestUpdateFailedAction::Execute(Event /*event*/)
{
    //opcode SMSG_QUESTUPDATE_FAILED is never sent...(yet?)
    return false;
}

bool QuestUpdateFailedTimerAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel

    uint32 questId;
    p >> questId;

    Quest const* qInfo = sObjectMgr->GetQuestTemplate(questId);

    if (qInfo)
    {
        std::map<std::string, std::string> placeholders;
        placeholders["%quest_link"] = botAI->GetChatHelper()->FormatQuest(qInfo);
        //By leewheel 2026-08-01: 玩家可见文本中文化(键名保留英文兼容DB，无DB文本时用中文默认)
        botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "Failed timer for %quest_link, abandoning", "任务 %quest_link 超时失败，已放弃", placeholders));
        //End By leewheel
        BroadcastHelper::BroadcastQuestUpdateFailedTimer(botAI, bot, qInfo);
    }
    else
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("任务失败计时到期: " + std::to_string(questId));
    }

    //drop quest
    bot->AbandonQuest(questId);

    return false;
}
