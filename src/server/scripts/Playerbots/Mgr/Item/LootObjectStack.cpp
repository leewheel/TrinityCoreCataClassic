/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootObjectStack.h"

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Loot.h"
#include "LootMgr.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "Unit.h"

#define MAX_LOOT_OBJECT_COUNT 200

LootTarget::LootTarget(ObjectGuid guid) : guid(guid), asOfTime(time(nullptr)) {}

LootTarget::LootTarget(LootTarget const& other)
{
    guid = other.guid;
    asOfTime = other.asOfTime;
}

LootTarget& LootTarget::operator=(LootTarget const& other)
{
    if ((void*)this == (void*)&other)
        return *this;

    guid = other.guid;
    asOfTime = other.asOfTime;

    return *this;
}

bool LootTarget::operator<(LootTarget const& other) const { return guid < other.guid; }

void LootTargetList::shrink(time_t fromTime)
{
    for (std::set<LootTarget>::iterator i = begin(); i != end();)
    {
        if (i->asOfTime <= fromTime)
            erase(i++);
        else
            ++i;
    }
}

LootObject::LootObject(Player* bot, ObjectGuid guid) : guid(), skillId(SKILL_NONE), reqSkillValue(0), reqItem(0)
{
    Refresh(bot, guid);
}

void LootObject::Refresh(Player* bot, ObjectGuid lootGUID)
{
    skillId = SKILL_NONE;
    reqSkillValue = 0;
    reqItem = 0;
    guid.Clear();

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        return;
    }
    Creature* creature = botAI->GetCreature(lootGUID);
    //By leewheel 2026-07-10: TC的DeathState是非作用域枚举，值是大写CORPSE
    if (creature && creature->getDeathState() == CORPSE)
    //End By leewheel
    {
        if (creature->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE))
        {
            //By leewheel 2026-08-07: 尸体仍LOOTABLE但已无可拾取物品时，不标记为拾取目标。
            //场景：物品已被玩家拾取并进入需求roll等待(is_underthreshold/follow_loot_rules)，
            //或金币已取、物品全部is_looted——此时每个机器人反复摸尸只会浪费AI节拍。
            //仅当存在未拾取且非roll等待的物品(且非他人FFA独占)时才标记。
            bool hasPickableLoot = false;
            Loot& loot = creature->loot;
            for (LootItem const& lootItem : loot.items)
            {
                if (lootItem.is_looted || lootItem.is_blocked)
                    continue;
                if (lootItem.is_underthreshold || lootItem.follow_loot_rules)
                    continue;
                if (lootItem.freeforall && !lootItem.AllowedForPlayer(bot))
                    continue;
                hasPickableLoot = true;
                break;
            }
            for (LootItem const& lootItem : loot.quest_items)
            {
                if (lootItem.is_looted || lootItem.is_blocked)
                    continue;
                if (lootItem.is_underthreshold || lootItem.follow_loot_rules)
                    continue;
                if (lootItem.freeforall && !lootItem.AllowedForPlayer(bot))
                    continue;
                hasPickableLoot = true;
                break;
            }
            if (hasPickableLoot || loot.gold > 0)
                guid = lootGUID;
            //End By leewheel
        }

        //By leewheel 2026-07-30: 剥皮必须在拾取尸体后才能进行
        //原代码仅检查UNIT_FLAG_SKINNABLE就标记为剥皮目标，但该flag在生物死亡时就设置
        //导致bot在未拾取尸体的情况下就尝试剥皮。必须额外检查尸体已被拾取(非LOOTABLE)或是小动物
        if (creature->HasUnitFlag(UNIT_FLAG_SKINNABLE) &&
            (creature->IsCritter() || !creature->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE)))
        {
            skillId = creature->GetCreatureTemplate()->GetRequiredLootSkill();
            uint32 targetLevel = creature->GetLevel();
            reqSkillValue = targetLevel < 10 ? 1 : targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5;
            if (botAI->HasSkill((SkillType)skillId) && bot->GetSkillValue(skillId) >= reqSkillValue)
                guid = lootGUID;
        }
        //End By leewheel

        return;
    }

    GameObject* go = botAI->GetGameObject(lootGUID);
    if (go && go->isSpawned() && go->GetGoState() == GO_STATE_READY)
    {
        bool onlyHasQuestItems = true;
        bool hasAnyQuestItems = false;
        //By leewheel 2026-07-23: 恢复acore的neededQuestItem标记逻辑
        //原移植代码在此处直接return，跳过了后续lock处理，导致skillId永远为SKILL_NONE
        //采集节点含任务物品时（如贫瘠之地草药掉根样本），bot无法识别需要采矿/采药技能
        bool neededQuestItem = false;
        //End By leewheel

        GameObjectQuestItemList const* items = sObjectMgr->GetGameObjectQuestItemList(go->GetEntry());
        for (size_t i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; i++)
        {
            if (!items || i >= items->size())
                break;

            uint32 itemId = uint32((*items)[i]);
            if (!itemId)
                continue;

            hasAnyQuestItems = true;

            if (IsNeededForQuest(bot, itemId))
            {
                //By leewheel 2026-07-23: 设标记继续执行，不return，确保后续lock处理能设置skillId
                this->guid = lootGUID;
                neededQuestItem = true;
                //End By leewheel
            }

            const ItemTemplate* proto = sObjectMgr->GetItemTemplate(itemId);
            if (!proto)
                continue;

            if (proto->GetClass() != ITEM_CLASS_QUEST)
            {
                onlyHasQuestItems = false;
            }
        }

        // Retrieve the correct loot table entry
        uint32 lootEntry = go->GetGOInfo()->GetLootId();
        if (lootEntry == 0)
            return;

        // Check the main loot template
        if (const LootTemplate* lootTemplate = LootTemplates_Gameobject.GetLootFor(lootEntry))
        {
            //By leewheel 2026-07-10: TC的Process签名是(Loot&, bool, uint16, uint8)，不同于AC
            Loot loot;
            lootTemplate->Process(loot, false, 1, 0);
            //End By leewheel

            for (const LootItem& item : loot.items)
            {
                uint32 itemId = item.itemid;
                if (!itemId)
                    continue;

                const ItemTemplate* proto = sObjectMgr->GetItemTemplate(itemId);
                if (!proto)
                    continue;

                if (proto->GetClass() != ITEM_CLASS_QUEST)
                {
                    onlyHasQuestItems = false;
                    break;
                }

                // If this item references another loot table, process it
                if (const LootTemplate* refLootTemplate = LootTemplates_Reference.GetLootFor(itemId))
                {
                    //By leewheel 2026-07-10: TC的Process签名是(Loot&, bool, uint16, uint8)
                    Loot refLoot;
                    refLootTemplate->Process(refLoot, false, 1, 0);
                    //End By leewheel

                    for (const LootItem& refItem : refLoot.items)
                    {
                        uint32 refItemId = refItem.itemid;
                        if (!refItemId)
                            continue;

                        const ItemTemplate* refProto = sObjectMgr->GetItemTemplate(refItemId);
                        if (!refProto)
                            continue;

                        if (refProto->GetClass() != ITEM_CLASS_QUEST)
                        {
                            onlyHasQuestItems = false;
                            break;
                        }
                    }
                }
            }
        }

        // If gameobject has only quest items that bot doesn’t need, skip it.
        //By leewheel 2026-07-23: 加上!neededQuestItem条件，bot需要的任务物品节点不跳过
        if (!neededQuestItem && hasAnyQuestItems && onlyHasQuestItems)
        //End By leewheel
            return;

        // Otherwise, loot it.
        guid = lootGUID;

        uint32 goId = go->GetEntry();
        uint32 lockId = go->GetGOInfo()->GetLockId();
        LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);
        if (!lockInfo)
            return;

        for (uint8 i = 0; i < 8; ++i)
        {
            switch (lockInfo->Type[i])
            {
                case LOCK_KEY_ITEM:
                    if (lockInfo->Index[i] > 0)
                    {
                        reqItem = lockInfo->Index[i];
                        guid = lootGUID;
                    }
                    break;

                case LOCK_KEY_SKILL:
                    //By leewheel 2026-08-14: 采集物(矿脉/草药)必须已学对应技能且等级足够且有工具，
                    //否则不设为loot——否则bot会跑过去才因技能/工具不足失败，白白浪费时间。
                    if (goId == 13891 || goId == 19535)  // Serpentbloom
                    {
                        if (botAI->HasSkill(SKILL_HERBALISM) &&
                            HasGatheringTool(bot, SKILL_HERBALISM))
                        {
                            this->guid = lootGUID;
                        }
                    }
                    else if (SkillByLockType(LockType(lockInfo->Index[i])) > 0)
                    {
                        skillId = SkillByLockType(LockType(lockInfo->Index[i]));
                        reqSkillValue = std::max((uint32)1, (uint32)lockInfo->Skill[i]); // By leewheel 2026-07-09
                        if (botAI->HasSkill((SkillType)skillId) &&
                            bot->GetSkillValue(skillId) >= reqSkillValue &&
                            HasGatheringTool(bot, skillId))
                        {
                            guid = lootGUID;
                        }
                    }
                    //End By leewheel
                    break;

                case LOCK_KEY_NONE:
                    guid = lootGUID;
                    break;
            }
        }
    }
}

//By leewheel 2026-07-23: 增加已收集数量检查，避免bot反复采集已完成的任务物品
bool LootObject::IsNeededForQuest(Player* bot, uint32 itemId)
{
    for (int qs = 0; qs < MAX_QUEST_LOG_SIZE; ++qs)
    {
        uint32 questId = bot->GetQuestSlotQuestId(qs);
        if (questId == 0)
            continue;

        QuestStatusData& qData = bot->getQuestStatusMap()[questId];
        if (qData.Status != QUEST_STATUS_INCOMPLETE)
            continue;

        Quest const* qInfo = sObjectMgr->GetQuestTemplate(questId);
        if (!qInfo)
            continue;

        for (QuestObjective const& objective : qInfo->GetObjectives())
        {
            if (objective.Type != QUEST_OBJECTIVE_ITEM)
                continue;

            //By leewheel 2026-09-03 修复C4389警告：ObjectID为int32，itemId为uint32，比较前显式转换
            if (static_cast<uint32>(objective.ObjectID) != itemId)
                continue;
            //End By leewheel

            // 检查背包中该物品数量是否已满足任务需求
            uint32 haveCount = bot->GetItemCount(itemId);
            if (haveCount >= (uint32)objective.Amount)
                continue;  // 已收集够，不再需要

            return true;
        }
    }

    return false;
}
//End By leewheel

WorldObject* LootObject::GetWorldObject(Player* bot)
{
    Refresh(bot, guid);

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        return nullptr;
    }
    Creature* creature = botAI->GetCreature(guid);
    //By leewheel 2026-07-10: TC的DeathState是非作用域枚举，值是大写CORPSE
    if (creature && creature->getDeathState() == CORPSE && creature->IsInWorld())
    //End By leewheel
        return creature;

    GameObject* go = botAI->GetGameObject(guid);
    if (go && go->isSpawned() && go->IsInWorld())
        return go;

    return nullptr;
}

LootObject::LootObject(LootObject const& other)
{
    guid = other.guid;
    skillId = other.skillId;
    reqSkillValue = other.reqSkillValue;
    reqItem = other.reqItem;
}

bool LootObject::IsLootPossible(Player* bot)
{
    if (IsEmpty() || !bot)
        return false;

    WorldObject* worldObj = GetWorldObject(bot);  // Store result to avoid multiple calls
    if (!worldObj)
        return false;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        return false;
    }
    if (reqItem && !bot->HasItemCount(reqItem, 1))
        return false;

    if (abs(worldObj->GetPositionZ() - bot->GetPositionZ()) > INTERACTION_DISTANCE - 2.0f)
        return false;

    Creature* creature = botAI->GetCreature(guid);
    //By leewheel 2026-07-10: TC的DeathState是非作用域枚举，值是大写CORPSE
    if (creature && creature->getDeathState() == CORPSE)
    //End By leewheel
    {
        if (!bot->isAllowedToLoot(creature) && skillId != SKILL_SKINNING)
            return false;

        //By leewheel 2026-08-07: 防ADD——非战斗拾取时，若尸体附近有未参战的敌对怪，
        //bot走过去拾取会引到怪。此处检查尸体周围(默认10码)是否有敌对且不在战斗中的怪，
        //有则视该尸体为危险目标，不前往拾取。
        //注意：仅当bot自身不在战斗中时启用(战斗中由战斗引擎处理仇恨，不受此限制)。
        if (!bot->IsInCombat() && creature->IsInWorld())
        {
            float const lootSafeRadius = 10.0f;
            std::list<Unit*> targets;
            Trinity::AnyUnitInObjectRangeCheck u_check(creature, lootSafeRadius);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(creature, targets, u_check);
            Cell::VisitGridObjects(creature, searcher, lootSafeRadius);
            for (Unit* unit : targets)
            {
                if (!unit || unit == bot || unit == creature)
                    continue;
                if (!unit->IsAlive() || unit->IsPlayer() || unit->IsCritter())
                    continue;
                if (!unit->IsHostileTo(bot))
                    continue;
                // 已在战斗中的怪不触发ADD(它已有目标，不会因bot靠近而新增仇恨)
                if (unit->IsInCombat())
                    continue;
                return false;
            }
        }
        //End By leewheel
    }

    // Prevent bot from running to chests that are unlootable (e.g. Gunship Armory before completing the event) or on
    // respawn time
    GameObject* go = botAI->GetGameObject(guid);
    if (go && (go->HasFlag(GO_FLAG_NOT_SELECTABLE) || !go->isSpawned()))
        return false;

    // Conditional objects (quest chests, goobers, ...) are gated client-side on quest state.
    // A bot has no client, so make the same call the server makes for one.
    //By leewheel 2026-08-09: 移植上游83830c6e——INTERACT_COND任务GO由ActivateToQuest按任务状态放行，
    //避免bot漏拾任务宝箱/任务GO(原代码与NOT_SELECTABLE合并判断，把任务GO也一刀切拦掉了)
    if (go && go->HasFlag(GO_FLAG_INTERACT_COND) && !go->ActivateToQuest(bot))
        return false;
    //End By leewheel

    if (skillId == SKILL_NONE)
        return true;

    if (skillId == SKILL_FISHING)
        return false;

    if (!botAI->HasSkill((SkillType)skillId))
        return false;

    if (!reqSkillValue)
        return true;

    uint32 skillValue = uint32(bot->GetSkillValue(skillId));
    if (reqSkillValue > skillValue)
        return false;

    //By leewheel 2026-08-14: 工具检查统一收敛到HasGatheringTool(原散落的采矿镐/剥皮刀判断)
    if (!HasGatheringTool(bot, skillId))
        return false;
    //End By leewheel

    return true;
}

//By leewheel 2026-08-14: 采集工具检查——采矿需要矿锄、剥皮需要剥皮刀，采药无需工具。
//工具物品ID列表与核心开锁判定保持一致。
bool HasGatheringTool(Player* bot, uint32 skillId)
{
    switch (skillId)
    {
        case SKILL_MINING:
            return bot->HasItemCount(756, 1) || bot->HasItemCount(778, 1) ||
                bot->HasItemCount(1819, 1) || bot->HasItemCount(1893, 1) ||
                bot->HasItemCount(1959, 1) || bot->HasItemCount(2901, 1) ||
                bot->HasItemCount(9465, 1) || bot->HasItemCount(20723, 1) ||
                bot->HasItemCount(40772, 1) || bot->HasItemCount(40892, 1) ||
                bot->HasItemCount(40893, 1);
        case SKILL_SKINNING:
            return bot->HasItemCount(7005, 1) || bot->HasItemCount(40772, 1) ||
                bot->HasItemCount(40893, 1) || bot->HasItemCount(12709, 1) ||
                bot->HasItemCount(19901, 1);
        default:
            return true;  // 采药/钓鱼等无工具需求
    }
}
//End By leewheel

bool LootObjectStack::Add(ObjectGuid guid)
{
    if (availableLoot.size() >= MAX_LOOT_OBJECT_COUNT)
    {
        availableLoot.shrink(time(nullptr) - 30);
    }

    if (availableLoot.size() >= MAX_LOOT_OBJECT_COUNT)
    {
        availableLoot.clear();
    }

    if (!availableLoot.insert(guid).second)
        return false;

    return true;
}

void LootObjectStack::Remove(ObjectGuid guid)
{
    LootTargetList::iterator i = availableLoot.find(guid);
    if (i != availableLoot.end())
        availableLoot.erase(i);
}

void LootObjectStack::Clear() { availableLoot.clear(); }

bool LootObjectStack::CanLoot(float maxDistance)
{
    LootObject nearest = GetNearest(maxDistance);
    return !nearest.IsEmpty();
}

LootObject LootObjectStack::GetLoot(float maxDistance)
{
    LootObject nearest = GetNearest(maxDistance);
    return nearest.IsEmpty() ? LootObject() : nearest;
}

LootObject LootObjectStack::GetNearest(float maxDistance)
{
    availableLoot.shrink(time(nullptr) - 30);

    LootObject nearest;
    float nearestDistance = std::numeric_limits<float>::max();

    LootTargetList safeCopy(availableLoot);
    for (LootTargetList::iterator i = safeCopy.begin(); i != safeCopy.end(); i++)
    {
        ObjectGuid guid = i->guid;

        WorldObject* worldObj = ObjectAccessor::GetWorldObject(*bot, guid);
        if (!worldObj)
            continue;

        float distance = bot->GetDistance(worldObj);

        if (distance >= nearestDistance || (maxDistance && distance > maxDistance))
            continue;

        LootObject lootObject(bot, guid);

        if (!lootObject.IsLootPossible(bot))
            continue;

        nearestDistance = distance;
        nearest = lootObject;
    }

    return nearest;
}
