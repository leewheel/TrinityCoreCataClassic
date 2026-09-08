#include "NewRpgInfo.h"

#include <cmath>

#include "Timer.h"

void NewRpgInfo::ChangeToGoGrind(WorldPosition pos)
{
    startT = getMSTime();
    data = GoGrind{pos};
}

void NewRpgInfo::ChangeToGoCamp(WorldPosition pos)
{
    startT = getMSTime();
    data = GoCamp{pos};
}

void NewRpgInfo::ChangeToWanderNpc()
{
    startT = getMSTime();
    data = WanderNpc{};
}

void NewRpgInfo::ChangeToWanderRandom()
{
    startT = getMSTime();
    data = WanderRandom{};
}

void NewRpgInfo::ChangeToDoQuest(uint32 questId, const Quest* quest)
{
    startT = getMSTime();
    DoQuest do_quest;
    do_quest.questId = questId;
    do_quest.quest = quest;
    data = do_quest;
}

void NewRpgInfo::ChangeToTravelFlight(uint32 flightMasterEntry, WorldPosition flightMasterPos, std::vector<uint32> path)
{
    startT = getMSTime();
    TravelFlight flight;
    flight.flightMasterEntry = flightMasterEntry;
    flight.flightMasterPos = flightMasterPos;
    flight.path = std::move(path);
    flight.inFlight = false;
    data = flight;
}

void NewRpgInfo::ChangeToOutdoorPvp(ObjectGuid::LowType capturePointSpawnId)
{
    startT = getMSTime();
    OutdoorPvP pvp;
    pvp.capturePointSpawnId = capturePointSpawnId;
    data = pvp;
}

void NewRpgInfo::ChangeToRest()
{
    startT = getMSTime();
    data = Rest{};
}

void NewRpgInfo::ChangeToIdle()
{
    startT = getMSTime();
    data = Idle{};
}

bool NewRpgInfo::CanChangeTo(NewRpgStatus)
{
    return true;
}

void NewRpgInfo::Reset()
{
    data = Idle{};
    startT = getMSTime();
}

void NewRpgInfo::SetMoveFarTo(WorldPosition pos)
{
    nearestMoveFarDis = FLT_MAX;
    stuckTs = 0;
    stuckAttempts = 0;
    moveFarPos = pos;
}

NewRpgStatus NewRpgInfo::GetStatus()
{
    return std::visit([](auto&& arg) -> NewRpgStatus {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Idle>) return RPG_IDLE;
        if constexpr (std::is_same_v<T, GoGrind>) return RPG_GO_GRIND;
        if constexpr (std::is_same_v<T, GoCamp>) return RPG_GO_CAMP;
        if constexpr (std::is_same_v<T, WanderNpc>) return RPG_WANDER_NPC;
        if constexpr (std::is_same_v<T, WanderRandom>) return RPG_WANDER_RANDOM;
        if constexpr (std::is_same_v<T, Rest>) return RPG_REST;
        if constexpr (std::is_same_v<T, DoQuest>) return RPG_DO_QUEST;
        if constexpr (std::is_same_v<T, TravelFlight>) return RPG_TRAVEL_FLIGHT;
        if constexpr (std::is_same_v<T, OutdoorPvP>) return RPG_OUTDOOR_PVP;
        return RPG_IDLE;
    }, data);
}

std::string NewRpgInfo::ToString()
{
    std::stringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "状态: ";
    //End By leewheel
    std::visit([&out, this](auto&& arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, GoGrind>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "刷怪";
            out << "\n刷怪坐标: " << arg.pos.GetMapId() << " " << arg.pos.GetPositionX() << " "
                << arg.pos.GetPositionY() << " " << arg.pos.GetPositionZ();
            out << "\n上次刷怪: " << startT;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, GoCamp>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "蹲点";
            out << "\n蹲点坐标: " << arg.pos.GetMapId() << " " << arg.pos.GetPositionX() << " "
                << arg.pos.GetPositionY() << " " << arg.pos.GetPositionZ();
            out << "\n上次蹲点: " << startT;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, WanderNpc>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "闲逛";
            out << "\nNPC/物体条目: " << arg.npcOrGo.GetCounter();
            out << "\n上次闲逛: " << startT;
            out << "\n上次抵达NPC/物体: " << arg.lastReach;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, WanderRandom>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "随机游荡";
            out << "\n上次随机游荡: " << startT;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, Idle>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "空闲";
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, Rest>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "休息";
            out << "\n上次休息: " << startT;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, DoQuest>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "做任务";
            out << "\n任务ID: " << arg.questId;
            out << "\n目标序号: " << arg.objectiveIdx;
            out << "\n目标点坐标: " << arg.pos.GetMapId() << " " << arg.pos.GetPositionX() << " "
                << arg.pos.GetPositionY() << " " << arg.pos.GetPositionZ();
            out << "\n上次抵达目标点: " << (arg.lastReachPOI ? GetMSTimeDiffToNow(arg.lastReachPOI) : 0);
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, TravelFlight>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "飞行旅行";
            out << "\n飞行管理员条目: " << arg.flightMasterEntry;
            out << "\n起点节点: " << arg.path[0];
            out << "\n终点节点: " << arg.path[arg.path.size() - 1];
            out << "\n飞行中: " << arg.inFlight;
            //End By leewheel
        }
        else if constexpr (std::is_same_v<T, OutdoorPvP>)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "野外PVP";
            if (!arg.capturePointSpawnId)
                out << "\n未分配占领点。";
            else
                out << "\n占领点刷新ID: " << arg.capturePointSpawnId;
            //End By leewheel
        }
        else
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "未知";
            //End By leewheel
    }, data);
    return out.str();
}
