/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "UseMeetingStoneAction.h"

#include "CellImpl.h"
#include "Event.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "NearestGameObjects.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "PositionValue.h"

bool UseMeetingStoneAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，去掉rpos(0)防止把opcode当包体读
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
        return false;
    //End By leewheel
    ObjectGuid guid;
    p >> guid;

    if (master->GetTarget() && master->GetTarget() != bot->GetGUID())
        return false;

    if (!master->GetTarget() && master->GetGroup() != bot->GetGroup())
        return false;

    if (master->IsBeingTeleported())
        return false;

    if (bot->IsInCombat())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "meeting_stone_in_combat", "I am in combat", {}));
        return false;
    }

    Map* map = master->GetMap();
    if (!map)
        return false;

    GameObject* gameObject = map->GetGameObject(guid);
    if (!gameObject)
        return false;

    GameObjectTemplate const* goInfo = gameObject->GetGOInfo();
    if (!goInfo || goInfo->entry != 179944)
        return false;

    return Teleport(master, bot, false);
}

bool SummonAction::Execute(Event event)
{
    Player* master = GetMaster();
    //By leewheel 2026-08-02: LFG随机本匹配进来的bot没有玩家master(GetMaster()=nullptr)
    //导致 /p summon、/p 集合 命令无效(bot无法被拉过来)
    //修复: 无master时用命令发送者(event.getOwner(), 与InviteToGroupAction同模式)
    if (!master)
        master = event.getOwner();
    if (!master)
        return false;
    //End By leewheel

    //By leewheel 2026-08-02: 诊断日志——确认集合命令执行 + 失败点
    TC_LOG_INFO("playerbots", "[集合诊断] SummonAction 执行 bot={} summoner={} mapId={} 战斗中={} 载具={}",
        bot->GetName(), master->GetName(), bot->GetMapId(), bot->IsInCombat() ? 1 : 0, bot->GetVehicle() ? 1 : 0);
    //End By leewheel

    if (bot->GetPet())
        botAI->PetFollow();

    if (master->GetSession()->GetSecurity() >= SEC_PLAYER)
    {
        // botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({});
        AI_VALUE(std::list<FleeInfo>&, "recently flee info").clear();
        return Teleport(master, bot, true);
    }

    if (SummonUsingGos(master, bot, true) || SummonUsingNpcs(master, bot, true))
    {
        botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "hello", "Hello!", {}));
        return true;
    }

    if (SummonUsingGos(bot, master, true) || SummonUsingNpcs(bot, master, true))
    {
        botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "meeting_stone_welcome", "Welcome!", {}));
        return true;
    }

    return false;
}

bool SummonAction::SummonUsingGos(Player* summoner, Player* player, bool preserveAuras)
{
    std::list<GameObject*> targets;
    AnyGameObjectInObjectRangeCheck u_check(summoner, sPlayerbotAIConfig.sightDistance);
    Trinity::GameObjectListSearcher<AnyGameObjectInObjectRangeCheck> searcher(summoner, targets, u_check);
    Cell::VisitGridObjects(summoner, searcher, sPlayerbotAIConfig.sightDistance);

    for (GameObject* go : targets)
    {
        if (go->isSpawned() && go->GetGoType() == GAMEOBJECT_TYPE_MEETINGSTONE)
            return Teleport(summoner, player, preserveAuras);
    }

    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        summoner == bot ? "meeting_stone_none_nearby" : "meeting_stone_none_near_you",
        summoner == bot ? "There is no meeting stone nearby" : "There is no meeting stone near you",
        {}));
    return false;
}

bool SummonAction::SummonUsingNpcs(Player* summoner, Player* player, bool preserveAuras)
{
    if (!sPlayerbotAIConfig.summonAtInnkeepersEnabled)
        return false;

    std::list<Unit*> targets;
    Trinity::AnyUnitInObjectRangeCheck u_check(summoner, sPlayerbotAIConfig.sightDistance);
    Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(summoner, targets, u_check);
    Cell::VisitGridObjects(summoner, searcher, sPlayerbotAIConfig.sightDistance);

    for (Unit* unit : targets)
    {
        if (unit && unit->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER))
        {
            if (!player->HasItemCount(6948, 1, false))
            {
                botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    player == bot ? "meeting_stone_no_hearthstone_self" : "meeting_stone_no_hearthstone_you",
                    player == bot ? "I have no hearthstone" : "You have no hearthstone",
                    {}));
                return false;
            }

            if (player->HasSpellCooldown(8690))
            {
                botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    player == bot ? "meeting_stone_hearthstone_not_ready_self" : "meeting_stone_hearthstone_not_ready_you",
                    player == bot ? "My hearthstone is not ready" : "Your hearthstone is not ready",
                    {}));
                return false;
            }

            // Trigger cooldown
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(8690);
            if (!spellInfo)
                return false;

            Spell spell(player, spellInfo, TRIGGERED_NONE);
            spell.SendSpellCooldown();

            return Teleport(summoner, player, preserveAuras);
        }
    }

    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        summoner == bot ? "meeting_stone_no_innkeepers_nearby" : "meeting_stone_no_innkeepers_near_you",
        summoner == bot ? "There are no innkeepers nearby" : "There are no innkeepers near you",
        {}));
    return false;
}

bool SummonAction::Teleport(Player* summoner, Player* player, bool preserveAuras)
{
    // Player* master = GetMaster();
    if (!summoner || summoner == player)
        return false;

    if (player->GetVehicle())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "meeting_stone_cannot_summon_vehicle", "You cannot summon me while I'm on a vehicle", {}));
        return false;
    }

    if (!summoner->IsBeingTeleported() && !player->IsBeingTeleported())
    {
        //By leewheel 2026-07-24: 跨地图召唤时先重置副本锁定，确保bot能进入summoner所在的副本
        if (player->GetMapId() != summoner->GetMapId())
            player->ResetInstances(InstanceResetMethod::Manual);
        //End By leewheel

        float followAngle = GetFollowAngle();
        //By leewheel 2026-09-03 修复C4305警告：M_PI为double字面量，float循环变量改float常量
        for (float angle = followAngle - float(M_PI); angle <= followAngle + float(M_PI); angle += float(M_PI) / 4)
        //End By leewheel
        {
            uint32 mapId = summoner->GetMapId();
            float x = summoner->GetPositionX() + cos(angle) * sPlayerbotAIConfig.followDistance;
            float y = summoner->GetPositionY() + sin(angle) * sPlayerbotAIConfig.followDistance;
            float z = summoner->GetPositionZ();

            if (summoner->IsWithinLOS(x, y, z))
            {
                if (sPlayerbotAIConfig.botRepairWhenSummon)  // .conf option to repair bot gear when summoned 0 = off, 1 = on
                    bot->DurabilityRepairAll(false, 1.0f, false);

                if (summoner->IsInCombat() && !sPlayerbotAIConfig.allowSummonInCombat)
                {
                    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "meeting_stone_cannot_summon_master_in_combat",
                        "You cannot summon me while you're in combat",
                        {}));
                    return false;
                }

                if (!summoner->IsAlive() && !sPlayerbotAIConfig.allowSummonWhenMasterIsDead)
                {
                    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "meeting_stone_cannot_summon_master_dead",
                        "You cannot summon me while you're dead",
                        {}));
                    return false;
                }

                if (bot->isDead() && !bot->HasPlayerFlag(PLAYER_FLAGS_GHOST) &&
                    !sPlayerbotAIConfig.allowSummonWhenBotIsDead)
                {
                    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "meeting_stone_cannot_summon_bot_dead",
                        "You cannot summon me while I'm dead, you need to release my spirit first",
                        {}));
                    return false;
                }

                bool revive =
                    sPlayerbotAIConfig.reviveBotWhenSummoned == 2 ||
                    (sPlayerbotAIConfig.reviveBotWhenSummoned == 1 && !summoner->IsInCombat() && summoner->IsAlive());

                if (bot->isDead() && revive)
                {
                    bot->ResurrectPlayer(1.0f, false);
                    bot->SpawnCorpseBones();
                    botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "meeting_stone_revived", "I live, again!", {}));
                    botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Reset();
                }

                player->GetMotionMaster()->Clear();
                AI_VALUE(LastMovement&, "last movement").clear();

                if (!preserveAuras)
                    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED |
                                                          AURA_INTERRUPT_FLAG_CHANGE_MAP);
                player->TeleportTo(mapId, x, y, z, 0);
                if (player->GetPet())
                    player->GetPet()->NearTeleportTo(x, y, z, player->GetOrientation());
                if (player->GetGuardianPet())
                    player->GetGuardianPet()->NearTeleportTo(x, y, z, player->GetOrientation());
                if (botAI->HasStrategy("stay", botAI->GetState()))
                {
                    PositionMap& posMap = AI_VALUE(PositionMap&, "position");
                    PositionInfo stayPosition = posMap["stay"];

                    stayPosition.Set(x,y, z, mapId);
                    posMap["stay"] = stayPosition;
                }

                return true;
            }
        }

        //By leewheel 2026-07-24: 副本内LOS检查全部失败时的兜底传送
        //副本走廊狭窄，8个方向的IsWithinLOS检查可能全部失败
        //此时直接传送到summoner位置，TC的Map::AddToMap会自动调整到合法位置
        if (summoner->GetMap() && summoner->GetMap()->IsDungeon())
        {
            if (sPlayerbotAIConfig.botRepairWhenSummon)
                bot->DurabilityRepairAll(false, 1.0f, false);

            bool revive =
                sPlayerbotAIConfig.reviveBotWhenSummoned == 2 ||
                (sPlayerbotAIConfig.reviveBotWhenSummoned == 1 && !summoner->IsInCombat() && summoner->IsAlive());
            if (bot->isDead() && revive)
            {
                bot->ResurrectPlayer(1.0f, false);
                bot->SpawnCorpseBones();
            }

            player->GetMotionMaster()->Clear();
            AI_VALUE(LastMovement&, "last movement").clear();
            if (!preserveAuras)
                player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);
            player->TeleportTo(summoner->GetMapId(), summoner->GetPositionX(), summoner->GetPositionY(),
                               summoner->GetPositionZ(), summoner->GetOrientation());
            return true;
        }
        //End By leewheel
    }

    if (summoner != player)
         botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
             "meeting_stone_not_enough_space", "Not enough place to summon", {}));
    return false;
}
