/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "LootAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "GuildMgr.h"
//By leewheel 2026-07-14: 需要ObjectAccessor访问Loot对象，Creature/GameObject/LootMgr
#include "ObjectAccessor.h"
#include "LootMgr.h"
//End By leewheel
#include "GuildTaskMgr.h"
#include "ItemUsageValue.h"
#include "LootObjectStack.h"
#include "LootStrategyValue.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "GuildMgr.h"
#include "BroadcastHelper.h"

bool LootAction::Execute(Event /*event*/)
{
    if (!AI_VALUE(bool, "has available loot"))
        return false;

    LootObject prevLoot = AI_VALUE(LootObject, "loot target");
    LootObject const& lootObject =
        AI_VALUE(LootObjectStack*, "available loot")->GetLoot(sPlayerbotAIConfig.lootDistance);

    if (!prevLoot.IsEmpty() && prevLoot.guid != lootObject.guid)
    {
        WorldPacket* packet = new WorldPacket(CMSG_LOOT_RELEASE, 8);
        *packet << prevLoot.guid;
        bot->GetSession()->QueuePacket(packet);
        // bot->GetSession()->HandleLootReleaseOpcode(packet);
    }

    // Provide a system to check if the game object id is disallowed in the user configurable list or not.
    //By leewheel 2026-08-09: 移植上游718aa919(#2606)——禁用列表只对gameobject生效，
    //否则item/creature的entry撞上禁用列表也会被错误拦截
    if (lootObject.guid.IsGameObject() &&
        sPlayerbotAIConfig.disallowedGameObjects.contains(lootObject.guid.GetEntry()))
    //End By leewheel
    {
        return false;  // Game object ID is disallowed, so do not proceed
    }
    else
    {
        context->GetValue<LootObject>("loot target")->Set(lootObject);
        return true;
    }
}

bool LootAction::isUseful()
{
    //By leewheel 2026-08-01: 移植selfbot FFA拾取增强(2533)——即使全局禁用FreeMethodLoot，
    //selfbot(主人即玩家自身的机器人)也允许在FFA拾取下拾取；其他bot仍受全局配置约束。
    //仍可通过移除selfbot的loot策略(nc -loot)来禁止其拾取。
    //By leewheel 2026-08-07: 择优移植 brighton-chi/the-lab 采集支持——FFA/宽松条件下，
    //允许采集类节点(草药/采矿/剥皮 skillId != SKILL_NONE)的拾取。
    if (sPlayerbotAIConfig.freeMethodLoot || !bot->GetGroup() ||
        bot->GetGroup()->GetLootMethod() != FREE_FOR_ALL || botAI->IsRealPlayer())
    {
        return true;
    }

    LootObjectStack* lootStack = AI_VALUE(LootObjectStack*, "available loot");
    if (!lootStack)
        return false;

    LootObject nearest = lootStack->GetLoot(sPlayerbotAIConfig.lootDistance);
    return !nearest.IsEmpty() && nearest.skillId != SKILL_NONE;
    //End By leewheel
}

enum ProfessionSpells
{
    ALCHEMY = 2259,
    BLACKSMITHING = 2018,
    COOKING = 2550,
    ENCHANTING = 7411,
    ENGINEERING = 49383,
    FIRST_AID = 3273,
    FISHING = 7620,
    HERB_GATHERING = 2366,
    INSCRIPTION = 45357,
    JEWELCRAFTING = 25229,
    MINING = 2575,
    SKINNING = 8613,
    TAILORING = 3908
};

bool OpenLootAction::Execute(Event /*event*/)
{
    LootObject lootObject = AI_VALUE(LootObject, "loot target");
    bool result = DoLoot(lootObject);
    if (result)
    {
        AI_VALUE(LootObjectStack*, "available loot")->Remove(lootObject.guid);
        context->GetValue<LootObject>("loot target")->Set(LootObject());
    }
    //By leewheel 2026-07-18: 拾取失败时必须清理 loot target 并从 available loot 中移除，否则下一 tick 会再次尝试同一目标
    else if (!lootObject.IsEmpty())
    {
        AI_VALUE(LootObjectStack*, "available loot")->Remove(lootObject.guid);
        context->GetValue<LootObject>("loot target")->Set(LootObject());
    }
    //End By leewheel
    return result;
}

bool OpenLootAction::DoLoot(LootObject& lootObject)
{
    if (lootObject.IsEmpty())
        return false;

    Creature* creature = botAI->GetCreature(lootObject.guid);
    if (creature && bot->GetDistance(creature) > INTERACTION_DISTANCE - 2.0f)
        return false;

    // Dismount if the bot is mounted
    if (bot->IsMounted())
    {
        bot->Dismount();
        botAI->SetNextCheckDelay(sPlayerbotAIConfig.lootDelay); // Small delay to avoid animation issues
    }

    if (creature && creature->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE))
    {
        //By leewheel 2026-09-09: TC-Cata的SendLoot接受Loot&而非guid+type
        Loot* loot = creature->GetLootForPlayer(bot);
        if (!loot)
            return false;
        bot->SendLoot(*loot);

        // 拾取金币
        if (loot->gold > 0)
        {
            bot->ModifyMoney(loot->gold);
            loot->gold = 0;
        }

        // 直接调用StoreLootItem拾取每个物品，slotIndex是items数组的0-based下标
        // StoreLootItem内部已有CanStoreNewItem背包空间检查，无需额外检查
        for (uint8 slotIndex = 0; slotIndex < loot->items.size(); ++slotIndex)
        {
            LootItem& lootItem = loot->items[slotIndex];
            if (lootItem.is_looted || lootItem.is_blocked)
                continue;

            if (!lootItem.AllowedForPlayer(bot, loot))
                continue;

            if (!StoreLootAction::IsLootAllowed(lootItem.itemid, botAI))
                continue;

            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(lootItem.itemid);
            if (!proto)
                continue;

            // 直接调用StoreLootItem，slotIndex是0-based（与LootItemInSlot一致）
            bot->StoreLootItem(lootObject.guid, slotIndex, loot);

            BroadcastHelper::BroadcastLootingItem(botAI, bot, proto);
        }

        // 释放loot
        bot->GetSession()->DoLootRelease(loot);

        botAI->SetNextCheckDelay(sPlayerbotAIConfig.lootPickupDelay); //By leewheel 2026-08-07: 拾取完成改用短延迟，提速捡尸
        //End By leewheel
        return true;
    }

    //By leewheel 2026-09-05: 上游bf6190b2——移动中停止后立即施法会被AI拒绝(CastSpell要求站立),
    //本tick直接放弃, 延迟到下一tick(站立后)再开, 避免同一秒内连续7次被拒的无效尝试
    if (bot->isMoving())
    {
        bot->StopMoving();
        botAI->SetNextCheckDelay(sPlayerbotAIConfig.lootDelay);
        return false;
    }
    //End By leewheel

    if (creature)
    {
        //By leewheel 2026-09-09: TC-Cata的CreatureTemplate无GetRequiredLootSkill()，使用lootObject.skillId
        uint32 skill = lootObject.skillId;
        //End By leewheel
        if (!CanOpenLock(skill, lootObject.reqSkillValue))
            return false;

        switch (skill)
        {
            case SKILL_ENGINEERING:
                return botAI->HasSkill(SKILL_ENGINEERING) ? botAI->CastSpell(ENGINEERING, creature) : false;
            case SKILL_HERBALISM:
                return botAI->HasSkill(SKILL_HERBALISM) ? botAI->CastSpell(32605, creature) : false;
            case SKILL_MINING:
                return botAI->HasSkill(SKILL_MINING) ? botAI->CastSpell(32606, creature) : false;
            default:
                return botAI->HasSkill(SKILL_SKINNING) ? botAI->CastSpell(SKINNING, creature) : false;
        }
    }

    GameObject* go = botAI->GetGameObject(lootObject.guid);
    if (go && bot->GetDistance(go) > INTERACTION_DISTANCE - 2.0f)
        return false;

    if (go && (go->GetGoState() != GO_STATE_READY))
        return false;

    // This prevents dungeon chests like Tribunal Chest (Halls of Stone) from being ninja'd by the bots.
    // Quest objects carry the same flag but are gated on quest state, which ActivateToQuest answers.
    //By leewheel 2026-08-09: 移植上游83830c6e——带GO_FLAG_INTERACT_COND的任务宝箱/任务GO由ActivateToQuest按任务状态放行，
    //避免bot漏拾任务物品(如任务宝箱、任务采集点)
    if (go && go->HasFlag(GO_FLAG_INTERACT_COND) && !go->ActivateToQuest(bot))
        return false;
    //End By leewheel

    // This prevents raid chests like Gunship Armory (ICC) from being ninja'd by the bots
    if (go && go->HasFlag(GO_FLAG_NOT_SELECTABLE))
        return false;

    if (lootObject.skillId == SKILL_MINING)
        //By leewheel 2026-08-14: 施法采集前兜底检查工具
        return botAI->HasSkill(SKILL_MINING) && HasGatheringTool(bot, SKILL_MINING)
            ? botAI->CastSpell(MINING, bot) : false;
        //End By leewheel

    if (lootObject.skillId == SKILL_HERBALISM)
        //By leewheel 2026-08-14: 施法采集前兜底检查工具
        return botAI->HasSkill(SKILL_HERBALISM) && HasGatheringTool(bot, SKILL_HERBALISM)
            ? botAI->CastSpell(HERB_GATHERING, bot) : false;
        //End By leewheel

    uint32 spellId = GetOpeningSpell(lootObject);
    if (!spellId)
        return false;

    return botAI->CastSpell(spellId, bot);
}

uint32 OpenLootAction::GetOpeningSpell(LootObject& lootObject)
{
    if (GameObject* go = botAI->GetGameObject(lootObject.guid))
        if (go->isSpawned())
            return GetOpeningSpell(lootObject, go);

    return 0;
}

uint32 OpenLootAction::GetOpeningSpell(LootObject& lootObject, GameObject* go)
{
    for (PlayerSpellMap::iterator itr = bot->GetSpellMap().begin(); itr != bot->GetSpellMap().end(); ++itr)
    {
        uint32 spellId = itr->first;

        //By leewheel 2025-07-10
        // TC中PlayerSpell成员为小写state和active
        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
        //End By leewheel
            continue;

        if (spellId == MINING || spellId == HERB_GATHERING)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        if (CanOpenLock(lootObject, spellInfo, go))
            return spellId;
    }

    for (uint32 spellId = 0; spellId < SpellMgr_GetSpellInfoStoreSize(); spellId++)
    {
        if (spellId == MINING || spellId == HERB_GATHERING)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        if (CanOpenLock(lootObject, spellInfo, go))
            return spellId;
    }

    return sPlayerbotAIConfig.openGoSpell;
}

bool OpenLootAction::CanOpenLock(LootObject& /*lootObject*/, SpellInfo const* spellInfo, GameObject* go)
{
    for (uint8 effIndex = 0; effIndex <= EFFECT_2; effIndex++)
    {
        if (spellInfo->GetEffects()[effIndex].Effect != SPELL_EFFECT_OPEN_LOCK &&
            spellInfo->GetEffects()[effIndex].Effect != SPELL_EFFECT_SKINNING)
            return false;

        uint32 lockId = go->GetGOInfo()->GetLockId();
        if (!lockId)
            return false;

        LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);
        if (!lockInfo)
            return false;

        for (uint8 j = 0; j < 8; ++j)
        {
            switch (lockInfo->Type[j])
            {
                /*
                case LOCK_KEY_ITEM:
                    return true;
                */
                case LOCK_KEY_SKILL:
                {
                    //By leewheel 2026-09-03 修复C4389警告：LockEntry::Index为int32数组，与uint32的MiscValue比较产生有符号/无符号不匹配；锁索引为正数，显式转换
                    if (uint32(spellInfo->GetEffects()[effIndex].MiscValue) != static_cast<uint32>(lockInfo->Index[j]))
                        continue;
                    //End By leewheel

                    uint32 skillId = SkillByLockType(LockType(lockInfo->Index[j]));
                    if (skillId == SKILL_NONE)
                        return true;

                    if (CanOpenLock(skillId, lockInfo->Skill[j]))
                        return true;
                }
            }
        }
    }

    return false;
}

bool OpenLootAction::CanOpenLock(uint32 skillId, uint32 reqSkillValue)
{
    //By leewheel 2026-08-14: 采集类技能必须已学且带工具，堵住reqSkillValue==0时无技能放行的漏洞
    if (reqSkillValue && bot->GetSkillValue(skillId) < reqSkillValue)
        return false;

    if (skillId == SKILL_MINING || skillId == SKILL_HERBALISM || skillId == SKILL_SKINNING)
        return botAI->HasSkill((SkillType)skillId) && HasGatheringTool(bot, skillId);
    //End By leewheel

    return true;
}

/*
uint32 StoreLootAction::RoundPrice(double price)
{
    if (price < 100)
    {
        return (uint32)price;
    }

    if (price < 10000)
    {
        return (uint32)(price / 100.0) * 100;
    }

    if (price < 100000)
    {
        return (uint32)(price / 1000.0) * 1000;
    }

    return (uint32)(price / 10000.0) * 10000;
}

bool StoreLootAction::AuctionItem(uint32 itemId)
{
    ItemTemplate const* proto = sItemStorage.LookupEntry<ItemTemplate>(itemId);
    if (!proto)
        return false;

    if (!proto || proto->GetBonding() == BIND_WHEN_PICKED_UP || proto->GetBonding() == BIND_QUEST_ITEM)
        return false;

    Item* oldItem = bot->GetItemByEntry(itemId);
    if (!oldItem)
        return false;

    AuctionHouseEntry const* ahEntry = AuctionHouseMgr::GetAuctionHouseEntry(unit->getFaction());
    if (!ahEntry)
        return false;

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(ahEntry);

    uint32 price = oldItem->GetCount() * proto->GetBuyPrice() * sRandomPlayerbotMgr.GetBuyMultiplier(bot);

uint32 stackCount = urand(1, proto->GetMaxStackSize());
    if (!price || !stackCount)
        return false;

    if (!stackCount)
        stackCount = 1;

    if (urand(0, 100) <= sAhBotConfig.underPriceProbability * 100)
        price = price * 100 / urand(100, 200);

    uint32 bidPrice = RoundPrice(stackCount * price);
    uint32 buyoutPrice = RoundPrice(stackCount * urand(price, 4 * price / 3));

    Item* item = Item::CreateItem(proto->GetId(), stackCount);
    if (!item)
        return false;

    uint32 auction_time = uint32(urand(8, 24) * HOUR * sWorld->getRate(RATE_AUCTION_TIME));

    AuctionEntry* auctionEntry = new AuctionEntry;
    auctionEntry->Id = sObjectMgr->GenerateAuctionID();
    auctionEntry->itemGuidLow = item->GetGUID().GetCounter();
    auctionEntry->itemTemplate = item->GetEntry();
    auctionEntry->itemCount = item->GetCount();
    auctionEntry->itemRandomPropertyId = item->GetItemRandomPropertyId();
    auctionEntry->owner = bot->GetGUID().GetCounter();
    auctionEntry->startbid = bidPrice;
    auctionEntry->bidder = 0;
    auctionEntry->bid = 0;
    auctionEntry->buyout = buyoutPrice;
    auctionEntry->expireTime = time(nullptr) + auction_time;
    //auctionEntry->moneyDeliveryTime = 0;
    auctionEntry->deposit = 0;
    auctionEntry->auctionHouseEntry = ahEntry;

    auctionHouse->AddAuction(auctionEntry);

    sAuctionMgr.AddAItem(item);

    item->SaveToDB();
    auctionEntry->SaveToDB();

    TC_LOG_ERROR("playerbots", "AhBot {} added {} of {} to auction {} for {}..{}", bot->GetName().c_str(), stackCount,
proto->GetName(DEFAULT_LOCALE), 1, bidPrice, buyoutPrice);

    if (oldItem->GetCount() > stackCount)
        oldItem->SetCount(oldItem->GetCount() - stackCount);
    else
        bot->RemoveItem(item->GetBagSlot(), item->GetSlot(), true);

    return true;
}
*/

//By leewheel 2026-07-14: 重写StoreLootAction::Execute
// TC的SMSG_LOOT_RESPONSE包格式与AC完全不同，直接解析会导致ByteBufferException崩溃
// 改为从event包中读取loot GUID，然后直接访问WorldObject的Loot对象
//By leewheel 2026-07-20: 修复核心BUG - SMSG_LOOT_RESPONSE中Owner=生物GUID, LootObj=HighGuid::LootObject
//原代码用LootObj GUID查找生物/设置SetLootGUID/发送CMSG_LOOT_RELEASE，全部失败
//TC的HandleAutostoreLootItemOpcode/HandleLootMoneyOpcode/HandleLootReleaseOpcode
//均通过player->GetLootGUID()查找目标，SetLootGUID必须设为生物GUID(owner)
bool StoreLootAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());

    // TC SMSG_LOOT_RESPONSE格式: Owner (ObjectGuid) + LootObj (ObjectGuid) + ...
    // Owner = 生物/GO/Item的GUID，用于查找WorldObject和SetLootGUID
    // LootObj = HighGuid::LootObject类型GUID，仅用于CMSG_LOOT_ITEM的Object字段
    //By leewheel 2026-08-04: SMSG包rpos已是0,移除多余rpos(0); 加长度检查防止空包/短包崩溃
    //TC的LootResponse前16字节是Owner+LootObj(扁平,非bit-packed),原解析字段顺序正确
    //但缺少长度检查,空包或伪造包会导致ByteBufferException崩溃
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 16)
        return false;
    //End By leewheel
    ObjectGuid owner;
    ObjectGuid lootObjGuid;
    p >> owner;       // 生物/GO/Item的GUID
    p >> lootObjGuid; // LootObject GUID (HighGuid::LootObject)

    Loot* loot = nullptr;

    //By leewheel 2026-07-20: 用owner(生物GUID)查找WorldObject，而非lootObjGuid(LootObject GUID)
    // 尝试从 Creature 获取 loot
    if (Creature* creature = ObjectAccessor::GetCreature(*bot, owner))
        loot = creature->GetLootForPlayer(bot);
    // 尝试从 GameObject 获取 loot
    else if (GameObject* go = ObjectAccessor::GetGameObject(*bot, owner))
        loot = go->GetLootForPlayer(bot);
    // 尝试从 Item 获取 loot (容器)
    else if (Item* item = bot->GetItemByGuid(owner))
        loot = item->GetLootForPlayer(bot);
    //End By leewheel

    if (!loot)
        return false;

    uint32 gold = loot->gold;

    //By leewheel 2026-07-20: SetLootGUID必须设为owner(生物GUID)
    //TC的HandleLootMoneyOpcode/HandleAutostoreLootItemOpcode/HandleLootReleaseOpcode
    //均通过player->GetLootGUID()查找目标，若设为LootObject GUID则查找失败
    bot->SetLootGUID(owner);
    //End By leewheel

    if (gold > 0)
    {
        //By leewheel 2026-07-20: TC的LootMoney::Read()用ReadBit()读取IsSoftInteract
        WorldPacket* packet = new WorldPacket(CMSG_LOOT_MONEY);
        packet->WriteBit(false);  // IsSoftInteract=false
        packet->FlushBits();
        bot->GetSession()->QueuePacket(packet);
        //End By leewheel
    }

    uint8 lootSlotCount = 0;
    for (LootItem& lootItem : loot->items)
    {
        if (lootItem.is_looted || lootItem.is_blocked)
            continue;

        //By leewheel 2026-08-15: 修复——原代码把拾取体包在(freeforall||follow_loot_rules||is_underthreshold)内，
        //TC中普通掉落(矿石/草药/宝箱物品)三个标志全为false → 普通物品永不拾取(采矿/采药产出为零)，
        //这是"采矿、采集一直有问题"的直接根因。改为跳过特殊标志物品(由队伍分配等机制处理)，普通物品正常拾取
        if (lootItem.freeforall || lootItem.follow_loot_rules || lootItem.is_underthreshold)
            continue;
        //End By leewheel

        if (!lootItem.AllowedForPlayer(bot, loot))
            continue;

        uint32 itemid = lootItem.itemid;
        uint32 itemcount = lootItem.count;
        uint8 itemindex = lootItem.LootListId;

        if (!IsLootAllowed(itemid, botAI))
            continue;

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemid);
        if (!proto)
            continue;

        if (!botAI->HasActivePlayerMaster() && AI_VALUE(uint8, "bag space") > 80)
        {
            uint32 maxStack = proto->GetMaxStackSize();
            if (maxStack == 1)
                continue;

            std::vector<Item*> found = parseItems(chat->FormatItem(proto));

            bool hasFreeStack = false;

            for (auto stack : found)
            {
                if (stack->GetCount() + itemcount < maxStack)
                {
                    hasFreeStack = true;
                    break;
                }
            }

            if (!hasFreeStack)
                continue;
        }

        Player* master = botAI->GetMaster();
        if (sRandomPlayerbotMgr.IsRandomBot(bot) && master)
        {
            uint32 price = itemcount * proto->GetBuyPrice() * sRandomPlayerbotMgr.GetBuyMultiplier(bot) + gold;
            if (price)
                sRandomPlayerbotMgr.AddTradeDiscount(bot, master, price);

            if (Group* group = bot->GetGroup())
                for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                    if (ref->GetSource() != bot)
                        GuildTaskMgr::instance().CheckItemTask(itemid, itemcount, ref->GetSource(), bot);
        }

        // 使用 TC 的 CMSG_LOOT_ITEM 格式，QueuePacket让HandleBotPackets处理
        //By leewheel 2026-07-20: TC的LootItem::Read()格式：uint32 Count + (ObjectGuid + uint8 LootListID)*Count + bool IsSoftInteract(bit)
        //Object字段用lootObjGuid(LootObject GUID)，但HandleAutostoreLootItemOpcode实际用GetLootGUID()查找
        //By leewheel 2026-08-15: 修复——TC的HandleAutostoreLootItemOpcode执行LootListID-1(期望1-based)，
        //原发送0-based的itemIndex导致首件下溢255失败/错位拾取/末件漏拾，改为+1
        WorldPacket* packet = new WorldPacket(CMSG_LOOT_ITEM);
        *packet << uint32(1);  // Count
        *packet << lootObjGuid;  // LootObject GUID
        *packet << uint8(itemindex + 1);  // LootListID (1-based)
        packet->WriteBit(false);  // IsSoftInteract=false
        packet->FlushBits();
        bot->GetSession()->QueuePacket(packet);
        //End By leewheel
        botAI->SetNextCheckDelay(sPlayerbotAIConfig.lootPickupDelay); //By leewheel 2026-08-07: 物品间拾取改用短延迟，提速
        lootSlotCount++;

        if (proto->GetQuality() > ITEM_QUALITY_NORMAL && !urand(0, 50) && botAI->HasStrategy("emote", BOT_STATE_NON_COMBAT) && sPlayerbotAIConfig.randomBotEmote)
            botAI->PlayEmote(TEXT_EMOTE_CHEER);

        if (proto->GetQuality() >= ITEM_QUALITY_RARE && !urand(0, 1) && botAI->HasStrategy("emote", BOT_STATE_NON_COMBAT) && sPlayerbotAIConfig.randomBotEmote)
            botAI->PlayEmote(TEXT_EMOTE_CHEER);

        BroadcastHelper::BroadcastLootingItem(botAI, bot, proto);

        //By leewheel 2026-07-20: 每次loot后重新获取Loot对象指针，用owner(生物GUID)查找
        if (Creature* creature = ObjectAccessor::GetCreature(*bot, owner))
            loot = creature->GetLootForPlayer(bot);
        else if (GameObject* go = ObjectAccessor::GetGameObject(*bot, owner))
            loot = go->GetLootForPlayer(bot);
        else
            break;
        //End By leewheel
    }

    if (lootSlotCount == 0)
        AI_VALUE(LootObjectStack*, "available loot")->Remove(owner);

    //By leewheel 2026-07-20: release loot - 用owner(生物GUID)，与GetLootGUID()匹配
    WorldPacket* releasePacket = new WorldPacket(CMSG_LOOT_RELEASE, 16);
    *releasePacket << owner;
    bot->GetSession()->QueuePacket(releasePacket);
    //End By leewheel
    return true;
}
//End By leewheel 2026-07-14

bool StoreLootAction::IsLootAllowed(uint32 itemid, PlayerbotAI* botAI)
{
    AiObjectContext* context = botAI->GetAiObjectContext();
    LootStrategy* lootStrategy = AI_VALUE(LootStrategy*, "loot strategy");

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemid);
    if (!proto)
        return false;

    std::set<uint32>& lootItems = AI_VALUE(std::set<uint32>&, "always loot list");
    if (lootItems.find(itemid) != lootItems.end())
        return true;

    //By leewheel 2026-08-02: 修复移植bug - AC原版用proto->MaxCount(物品持有上限,唯一物品=1,默认0=不检查)
    //原代码误用GetMaxStackSize()(堆叠上限),导致背包已有满堆叠物品时bot不再拾取同类物品
    //(如已有20个面包就不捡第21个),漏拾取可堆叠材料/消耗品。TC 343的GetMaxCount()与AC的MaxCount语义相同(ItemSparse.MaxCount)
    uint32 max = proto->GetMaxCount();
    if (max > 0 && botAI->GetBot()->HasItemCount(itemid, max, true))
        return false;
    //End By leewheel

    if (proto->GetStartQuest())
    {
        return true;
    }

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 entry = botAI->GetBot()->GetQuestSlotQuestId(slot);
        Quest const* quest = sObjectMgr->GetQuestTemplate(entry);
        if (!quest)
            continue;

        //By leewheel 2026-07-23: TC 343使用GetObjectives()系统，不再用RequiredItemId数组
        for (QuestObjective const& objective : quest->GetObjectives())
        {
            //By leewheel 2026-09-03 修复C4389警告：QuestObjective::ObjectID为int32，itemid为uint32，显式转换对齐
            if (objective.Type == QUEST_OBJECTIVE_ITEM && static_cast<uint32>(objective.ObjectID) == itemid)
                return true;
        }
        //End By leewheel
    }

    // if (proto->GetBonding() == BIND_QUEST_ITEM ||  //Still testing if it works ok without these lines.
    //     proto->GetBonding() == BIND_QUEST_ITEM1 || //Eventually this has to be removed.
    //     CreatureTemplate_GetClass(proto) == ITEM_CLASS_QUEST)
    //{

    bool canLoot = lootStrategy->CanLoot(proto, context);
    // if (canLoot && proto->GetBonding() == BIND_WHEN_PICKED_UP && botAI->HasActivePlayerMaster())
    // canLoot = sPlayerbotAIConfig.IsInRandomAccountList(botAI->GetBot()->GetSession()->GetAccountId());

    return canLoot;
}

bool ReleaseLootAction::Execute(Event /*event*/)
{
    GuidVector gos = context->GetValue<GuidVector>("nearest game objects")->Get();
    for (ObjectGuid const guid : gos)
    {
        WorldPacket* packet = new WorldPacket(CMSG_LOOT_RELEASE, 8);
        *packet << guid;
        bot->GetSession()->QueuePacket(packet);
    }

    GuidVector corpses = context->GetValue<GuidVector>("nearest corpses")->Get();
    for (ObjectGuid const guid : corpses)
    {
        WorldPacket* packet = new WorldPacket(CMSG_LOOT_RELEASE, 8);
        *packet << guid;
        bot->GetSession()->QueuePacket(packet);
    }

    return true;
}
