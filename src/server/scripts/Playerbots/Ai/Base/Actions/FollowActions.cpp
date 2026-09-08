/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "FollowActions.h"

#include <algorithm>
#include <array> //By leewheel 2026-08-15: 渡船登船Z探针数组
#include <cmath> //By leewheel 2026-08-15: FindBoardingPointOnTransport的sqrt

#include "Event.h"
#include "Formations.h"
#include "LastMovementValue.h"
#include "MotionMaster.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "Transport.h"
#include "Map.h"

bool FollowAction::Execute(Event /*event*/)
{
    //By leewheel 2026-08-04: 非战斗时清除残留引导法术(如猎人Volley), 否isUseful放行但移动会被channel阻断
    if (!bot->IsInCombat() && bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        bot->CastStop();
    //End By leewheel

    Formation* formation = AI_VALUE(Formation*, "formation");
    std::string const target = formation->GetTargetName();

    // 运输工具处理（仅限船只/飞艇等动态运输工具）
    Player* master = botAI->GetMaster();
    if (master && master->IsInWorld() && bot->IsInWorld() && bot->GetMapId() == master->GetMapId())
    {
        Map* map = master->GetMap();
        uint32 const mapId = bot->GetMapId();
        Transport* transport = nullptr;
        bool masterOnTransport = false;

        //By leewheel 2026-08-15: 按 the-lab 移植 GetTransportForPosTolerant——从给定Z向下的
        //4个Z探针(z/z+0.5/z+1.5/z-0.5)逐一查Map::GetTransportForPos(TC版用PhaseShift)。
        //master在动态渡船/飞艇上时直接取其Transport；否则用探针判定是否站在某动态载具甲板上
        auto getTransportForPosTolerant = [](Map* m, WorldObject* ref, float x, float y, float z) -> Transport*
        {
            if (!m || !ref)
                return nullptr;

            std::array<float, 4> const probes = { z, z + 0.5f, z + 1.5f, z - 0.5f };
            for (float const pz : probes)
            {
                if (Transport* t = m->GetTransportForPos(ref->GetPhaseShift(), x, y, pz, ref))
                    return t;
            }

            return nullptr;
        };

        // 沿master→bot连线按0.75yd步进，返回最后一个仍被判定在期望载具上的点——
        // 保证bot走到船板而非水中央
        auto findBoardingPointOnTransport = [&getTransportForPosTolerant](Map* m, Transport* expected, WorldObject* ref,
            float masterX, float masterY, float masterZ, float botX, float botY, float botZ,
            float& outX, float& outY, float& outZ) -> bool
        {
            if (!m || !expected || !ref)
                return false;

            PhaseShift const& phaseShift = ref->GetPhaseShift();

            // master必须确实被判定在目标载具上
            if (getTransportForPosTolerant(m, ref, masterX, masterY, masterZ) != expected)
                return false;

            float const probeZ = std::max(masterZ, botZ);

            float const dx2 = botX - masterX;
            float const dy2 = botY - masterY;
            float const dist2d = std::sqrt(dx2 * dx2 + dy2 * dy2);
            int32 const steps = std::clamp(static_cast<int32>(dist2d / 0.75f), 10, 28);

            float const dx = (botX - masterX) / static_cast<float>(steps);
            float const dy = (botY - masterY) / static_cast<float>(steps);

            if (m->GetTransportForPos(phaseShift, masterX, masterY, probeZ, ref) != expected)
                return false;

            float lastX = masterX;
            float lastY = masterY;
            bool found = false;

            for (int32 i = 1; i <= steps; ++i)
            {
                float const px = masterX + dx * i;
                float const py = masterY + dy * i;

                Transport* const t = getTransportForPosTolerant(m, ref, px, py, probeZ);
                if (t != expected)
                    break;

                lastX = px;
                lastY = py;
                found = true;
            }

            if (!found)
                return false;

            outX = lastX;
            outY = lastY;
            outZ = masterZ; // 保持甲板高度，鼓励走上船板/平台
            return true;
        };
        //End By leewheel

        // 检查队长是否在动态运输工具上
        // TC中GetTransport()返回TransportBase*，需通过Map::GetTransport获取Transport*
        if (TransportBase* transportBase = master->GetTransport())
        {
            if (map)
                transport = map->GetTransport(transportBase->GetTransportGUID());
            masterOnTransport = (transport != nullptr);
        }
        else if (map)
        {
            transport = getTransportForPosTolerant(map, master,
                master->GetPositionX(), master->GetPositionY(), master->GetPositionZ());
            masterOnTransport = (transport != nullptr);
        }

        // 忽略静态运输工具(电梯/有轨电车)——TC中动态渡船/飞艇是GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT
        if (transport && transport->GetGOInfo()->type != GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT)
            transport = nullptr;

        if (transport && map && bot->GetTransport() != transport)
        {
            //By leewheel 2026-08-15: 按 the-lab 移植——先探针判定bot是否已站在该载具甲板上，
            //在则直接AddPassenger(TC单参版)；否则60y内辅助登船，沿线段探测船上最后可达点
            float const botProbeZ = std::max(bot->GetPositionZ(), transport->GetPositionZ());
            Transport* botSurfaceTransport = getTransportForPosTolerant(map, bot,
                bot->GetPositionX(), bot->GetPositionY(), botProbeZ);

            if (botSurfaceTransport == transport)
            {
                transport->AddPassenger(bot);
                bot->StopMovingOnCurrentPos();
                return true;
            }

            float const boardingAssistDistance = 60.0f;
            float const dist2d = ServerFacade::instance().GetDistance2d(bot, master);
            bool const inAssist = ServerFacade::instance().IsDistanceLessOrEqualThan(dist2d, boardingAssistDistance);

            if (inAssist)
            {
                float destX = masterOnTransport ? master->GetPositionX() : transport->GetPositionX();
                float destY = masterOnTransport ? master->GetPositionY() : transport->GetPositionY();
                float destZ = masterOnTransport ? master->GetPositionZ() : transport->GetPositionZ();
                float edgeX = 0.0f;
                float edgeY = 0.0f;
                float edgeZ = 0.0f;

                if (masterOnTransport &&
                    findBoardingPointOnTransport(map, transport, master,
                        master->GetPositionX(), master->GetPositionY(), master->GetPositionZ(),
                        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
                        edgeX, edgeY, edgeZ))
                {
                    destX = edgeX;
                    destY = edgeY;
                    destZ = edgeZ;
                }
                //End By leewheel

                MovementPriority const priority = botAI->GetState() == BOT_STATE_COMBAT
                    ? MovementPriority::MOVEMENT_COMBAT
                    : MovementPriority::MOVEMENT_NORMAL;

                bool const movingAllowed = IsMovingAllowed();
                bool const dupMove = IsDuplicateMove(destX, destY, destZ);
                bool const waiting = IsWaitingForLastMove(priority);

                if (movingAllowed && !dupMove && !waiting)
                {
                    if (bot->IsSitState())
                        bot->SetStandState(UNIT_STAND_STATE_STAND);

                    bot->CastStop();

                    if (MotionMaster* mm = bot->GetMotionMaster())
                    {
                        // TC的MovePoint签名: (id, x, y, z, generatePath, finalOrient, speed, ...)
                        mm->MovePoint(0, destX, destY, destZ, false);
                    }
                    else
                        return false;

                    float delay = 1000.0f * MoveDelay(bot->GetExactDist(destX, destY, destZ));
                    delay = std::clamp(delay, 0.0f, static_cast<float>(sPlayerbotAIConfig.maxWaitForMove));

                    AI_VALUE(LastMovement&, "last movement")
                        .Set(mapId, destX, destY, destZ, bot->GetOrientation(), delay, priority);
                    ClearIdleState();
                    return true;
                }
            }
        }
    }
    // 运输工具处理结束

    //By leewheel 2026-07-24: 跨地图自动传送 - master进入副本/不同地图时bot自动传送跟随
    //原代码在MovementAction::Follow中有类似逻辑但被注释掉("Default mechanics takes care of this now")
    //实际上TC没有任何替代机制，导致bot永远无法跟随master进入副本
    //By leewheel 2026-08-09: LFG随机本匹配的bot无玩家master, 用队伍leader(玩家队长)代替, 使其能传送进副本
    Player* crossMapMaster = botAI->GetMaster();
    if (!crossMapMaster && botAI->GetGroupLeader() && botAI->GetGroupLeader()->IsPlayer())
        crossMapMaster = botAI->GetGroupLeader()->ToPlayer();
    // End By leewheel
    if (crossMapMaster && bot->IsInWorld() && crossMapMaster->IsInWorld() &&
        bot->GetMapId() != crossMapMaster->GetMapId() && !bot->InBattleground() &&
        !(crossMapMaster->GetMap() && crossMapMaster->GetMap()->IsBattlegroundOrArena()))
    {
        if (bot->isDead())
        {
            bot->ResurrectPlayer(1.0f, false);
            bot->SpawnCorpseBones(false);
        }
        bot->CombatStop(true);
        bot->GetMotionMaster()->Clear();
        bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);
        // 重置副本锁定，确保bot能进入master所在的副本实例
        bot->ResetInstances(InstanceResetMethod::Manual);
        botAI->TellMasterNoFacing("我马上到！");
        bot->TeleportTo(crossMapMaster->GetMapId(), crossMapMaster->GetPositionX(), crossMapMaster->GetPositionY(),
                        crossMapMaster->GetPositionZ(), crossMapMaster->GetOrientation());
        return true;
    }
    //End By leewheel

    bool moved = false;
    if (!target.empty())
    {
        moved = Follow(AI_VALUE(Unit*, target));
    }
    else
    {
        WorldLocation loc = formation->GetLocation();
        //By leewheel 2026-08-02: 同isUseful修复——LFG bot无master时formation无位置,
        //用group leader(队伍队长=玩家)的位置作为跟随目标, 使副本内bot跟随队伍
        if (Formation::IsNullLocation(loc) || loc.GetMapId() == MAPID_INVALID)
        {
            Unit* leader = botAI->GetGroupLeader();
            if (!leader || leader->GetGUID() == bot->GetGUID() || leader->GetMapId() != bot->GetMapId())
                return false;
            loc = WorldLocation(leader->GetMapId(), leader->GetPositionX(), leader->GetPositionY(), leader->GetPositionZ());
        }
        //End By leewheel

        MovementPriority priority = botAI->GetState() == BOT_STATE_COMBAT ? MovementPriority::MOVEMENT_COMBAT : MovementPriority::MOVEMENT_NORMAL;
        moved = MoveTo(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), false, false, false,
                       true, priority, true);
    }

    // This section has been commented out because it was forcing the pet to
    // follow the bot on every "follow" action tick, overriding any attack or
    // stay commands that might have been issued by the player.
    // if (Pet* pet = bot->GetPet())
    // {
    //     botAI->PetFollow();
    // }
    // if (moved)
    // botAI->SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);

    return moved;
}

bool FollowAction::isUseful()
{
    // move from group takes priority over follow as it's added and removed automatically
    // (without removing/adding follow)
    if (botAI->HasStrategy("move from group", BOT_STATE_COMBAT) ||
        botAI->HasStrategy("move from group", BOT_STATE_NON_COMBAT))
        return false;

    //By leewheel 2026-08-04: 修复猎人Volley等引导法术残留导致bot永远不跟随的死循环
    //原逻辑: 有CURRENT_CHANNELED_SPELL就return false → Execute不执行 → CastStop不调用 → channel永不清除
    //修复: 战斗中保留原逻辑(不打断引导), 非战斗中允许Execute执行CastStop清除残留channel后跟随
    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr && bot->IsInCombat())
        return false;
    //End By leewheel

    Formation* formation = AI_VALUE(Formation*, "formation");
    if (!formation)
        return false;

    std::string const target = formation->GetTargetName();

    Unit* fTarget = nullptr;
    if (!target.empty())
        fTarget = AI_VALUE(Unit*, target);
    else if (botAI->GetMaster())
        fTarget = botAI->GetMaster();
    else
        fTarget = AI_VALUE(Unit*, "group leader");

    if (fTarget)
    {
        if (fTarget->HasUnitState(UNIT_STATE_IN_FLIGHT))
            return false;

        if (!CanDeadFollow(fTarget))
            return false;

        if (fTarget->GetGUID() == bot->GetGUID())
            return false;
    }

    //By leewheel 2026-07-24: 跨地图跟随时允许Execute处理传送
    //原逻辑在formation location为NullLocation(跨地图)时直接return false，导致Execute永远不被调用
    //直接用GetMaster()而非fTarget，因为AI_VALUE(Unit*, target)在跨地图时可能返回null
    //By leewheel 2026-08-09: LFG随机本匹配的bot(治疗/DPS)无玩家master, 用队伍leader(玩家队长)代替
    Player* crossMapTarget = botAI->GetMaster();
    if (!crossMapTarget && botAI->GetGroupLeader() && botAI->GetGroupLeader()->IsPlayer())
        crossMapTarget = botAI->GetGroupLeader()->ToPlayer();
    // End By leewheel
    if (crossMapTarget && crossMapTarget->IsInWorld() && bot->GetMapId() != crossMapTarget->GetMapId() &&
        !bot->InBattleground())
        return true;
    //End By leewheel

    float distance = 0.f;
    if (!target.empty())
    {
        distance = AI_VALUE2(float, "distance", target);
    }
    else
    {
        WorldLocation loc = formation->GetLocation();
        //By leewheel 2026-08-02: LFG随机本bot无master时formation无位置(NullLocation→不跟随)
        //LFG匹配进副本的bot(治疗/DPS)没有玩家master, 各类Formation::GetLocation在无master时
        //返回NullLocation导致isUseful=false→bot在副本内不跟随队伍。
        //修复: 无master时用group leader(队伍队长=玩家)的位置作为跟随目标。
        if (Formation::IsNullLocation(loc) || bot->GetMapId() != loc.GetMapId())
        {
            Unit* leader = botAI->GetGroupLeader();
            if (!leader || leader->GetGUID() == bot->GetGUID() || leader->GetMapId() != bot->GetMapId())
                return false;
            loc = WorldLocation(leader->GetMapId(), leader->GetPositionX(), leader->GetPositionY(), leader->GetPositionZ());
        }
        //End By leewheel

        distance = bot->GetDistance(loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ());
    }
    if (botAI->HasStrategy("master fishing", BOT_STATE_NON_COMBAT))
        return ServerFacade::instance().IsDistanceGreaterThan(distance, sPlayerbotAIConfig.fishingDistanceFromMaster);

    return ServerFacade::instance().IsDistanceGreaterThan(distance, formation->GetMaxDistance());
}

bool FollowAction::CanDeadFollow(Unit* target)
{
    // In battleground, wait for spirit healer
    if (bot->InBattleground() && !bot->IsAlive())
        return false;

    //By leewheel 2026-07-08 死亡时移动到尸体，当目标存活或不是灵魂状态时返回false
    // 使用getDeathState()==DEAD判断幽灵状态，与PLAYER_FLAGS_GHOST语义等价
    bool targetIsGhost = false;
    if (target->ToPlayer())
        targetIsGhost = (target->getDeathState() == DEAD);
    //End By leewheel

    if (!bot->IsAlive() && (target->IsAlive() || !targetIsGhost))
        return false;

    return true;
}

bool FleeToGroupLeaderAction::Execute(Event /*event*/)
{
    Unit* fTarget = AI_VALUE(Unit*, "group leader");
    bool canFollow = Follow(fTarget);
    if (!canFollow)
    {
        // botAI->SetNextCheckDelay(5000);
        return false;
    }

    WorldPosition targetPos(fTarget);
    WorldPosition bosPos(bot);
    float distance = bosPos.fDist(targetPos);

    if (distance < sPlayerbotAIConfig.reactDistance * 3)
    {
        if (!urand(0, 3))
            botAI->TellMaster("我快到了，等等我！");
    }
    else if (distance < 1000)
    {
        if (!urand(0, 10))
            botAI->TellMaster("我正在前往你的位置。");
    }
    else if (!urand(0, 20))
        botAI->TellMaster("我正在赶往你的位置。");

    botAI->SetNextCheckDelay(3000);

    return true;
}

bool FleeToGroupLeaderAction::isUseful()
{
    if (!botAI->GetGroupLeader())
        return false;

    if (botAI->GetGroupLeader() == bot)
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && botAI->GetGroupLeader()->GetTarget() == target->GetGUID())
        return false;

    if (!botAI->HasStrategy("follow", BOT_STATE_NON_COMBAT))
        return false;

    Unit* fTarget = AI_VALUE(Unit*, "group leader");

    if (!CanDeadFollow(fTarget))
        return false;

    return true;
}
