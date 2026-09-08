/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GoAction.h"

#include "ChooseTravelTargetAction.h"
#include "Event.h"
#include "Formations.h"
#include "PathGenerator.h"
#include "Playerbots.h"
#include "PositionValue.h"
#include "ServerFacade.h"

std::vector<std::string> split(std::string const s, char delim);
char* strstri(char const* haystack, char const* needle);

bool GoAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    std::string const param = event.getParam();
    if (param == "?")
    {
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        Map2ZoneCoordinates(x, y, bot->GetZoneId());

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "我在 " << x << "," << y;
        //End By leewheel
        botAI->TellMaster(out.str());
        return true;
    }

    if (param.find("travel") != std::string::npos && param.size() > 7)
    {
        WorldPosition botPos(bot);

        std::string const destination = param.substr(7);

        TravelTarget* target = context->GetValue<TravelTarget*>("travel target")->Get();

        if (TravelDestination* dest = ChooseTravelTargetAction::FindDestination(bot, destination))
        {
            std::vector<WorldPosition*> points = dest->nextPoint(const_cast<WorldPosition*>(&botPos), true);
            if (points.empty())
                return false;

            target->setTarget(dest, points.front());
            target->setForced(true);

            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "正在前往 " << dest->getTitle();
            //End By leewheel
            botAI->TellMasterNoFacing(out.str());

            return true;
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMasterNoFacing("已清除旅行目标");
            //End By leewheel
            target->setTarget(TravelMgr::instance().nullTravelDestination, TravelMgr::instance().nullWorldPosition);
            target->setForced(false);
            return true;
        }
    }

    GuidVector gos = ChatHelper::parseGameobjects(param);
    if (!gos.empty())
    {
        for (ObjectGuid const guid : gos)
        {
            if (GameObject* go = botAI->GetGameObject(guid))
                if (go->isSpawned())
                {
                    if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, go),
                                                             sPlayerbotAIConfig.reactDistance))
                    {
                        //By leewheel 2026-08-01: 玩家可见文本中文化
                        botAI->TellError("太远了");
                        //End By leewheel
                        return false;
                    }

                    std::ostringstream out;
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "正在移动至 " << ChatHelper::FormatGameobject(go);
                    //End By leewheel
                    botAI->TellMasterNoFacing(out.str());
                    return MoveNear(bot->GetMapId(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ() + 0.5f,
                                    sPlayerbotAIConfig.followDistance);
                }
        }
        return false;
    }

    GuidVector units;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
    units.insert(units.end(), npcs.begin(), npcs.end());
    GuidVector players = AI_VALUE(GuidVector, "nearest friendly players");
    units.insert(units.end(), players.begin(), players.end());
    for (ObjectGuid const guid : units)
    {
        if (Unit* unit = botAI->GetUnit(guid))
            if (strstri(unit->GetName().c_str(), param.c_str()))
            {
                std::ostringstream out;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << "正在移动至 " << unit->GetName();
                //End By leewheel
                botAI->TellMasterNoFacing(out.str());
                return MoveNear(bot->GetMapId(), unit->GetPositionX(), unit->GetPositionY(),
                                unit->GetPositionZ() + 0.5f, sPlayerbotAIConfig.followDistance);
            }
    }

    if (param.find(";") != std::string::npos)
    {
        std::vector<std::string> coords = split(param, ';');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        float z;
        if (coords.size() > 2)
            z = atof(coords[2].c_str());
        else
            z = bot->GetPositionZ();

        if (botAI->HasStrategy("debug move", BOT_STATE_NON_COMBAT))
        {
            PathGenerator path(bot);

            path.CalculatePath(x, y, z, false);

            Movement::Vector3 end = path.GetEndPosition();
            Movement::Vector3 aend = path.GetActualEndPosition();

            Movement::PointsArray const& points = path.GetPath();
            PathType type = path.GetPathType();

            std::ostringstream out;

            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << x << ";" << y << ";" << z << " =";

            out << "路径为：";

            out << type;

            out << "，长度 ";

            out << points.size();

            out << "，偏移 ";

            out << (end - aend).length();
            //End By leewheel

            for (auto i : points)
            {
                CreateWp(bot, i.x, i.y, i.z, 0.f, 11144);
            }

            botAI->TellMaster(out);
        }

        if (bot->IsWithinLOS(x, y, z))
            return MoveNear(bot->GetMapId(), x, y, z, 0);
        else
            return MoveTo(bot->GetMapId(), x, y, z, false, false);

        return true;
    }

    if (param.find(",") != std::string::npos)
    {
        std::vector<std::string> coords = split(param, ',');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        Zone2MapCoordinates(x, y, bot->GetZoneId());

        Map* map = bot->GetMap();
        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);

        if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, x, y),
                                                 sPlayerbotAIConfig.reactDistance))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMaster("太远了");
            //End By leewheel
            return false;
        }

        if (map->IsInWater(bot->GetPhaseShift(), x, y, z, nullptr, bot->GetCollisionHeight()))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("它在水中");
            //End By leewheel
            return false;
        }

        //By leewheel 2025-01-16
        // TC中GetHeight需要PhaseShift参数
        float ground = map->GetHeight(bot->GetPhaseShift(), x, y, z + 0.5f);
        //End By leewheel 2025-01-16
        if (ground <= INVALID_HEIGHT)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("我去不了那里");
            //End By leewheel
            return false;
        }

        float x1 = x, y1 = y;
        Map2ZoneCoordinates(x1, y1, bot->GetZoneId());

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在移动至 " << x1 << "," << y1;
        //End By leewheel
        botAI->TellMasterNoFacing(out.str());

        return MoveNear(bot->GetMapId(), x, y, z + 0.5f, sPlayerbotAIConfig.followDistance);
    }

    PositionInfo pos = context->GetValue<PositionMap&>("position")->Get()[param];
    if (pos.isSet())
    {
        if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, pos.x, pos.y),
                                                 sPlayerbotAIConfig.reactDistance))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("太远了");
            //End By leewheel
            return false;
        }

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在移动至位置 " << param;
        //End By leewheel
        botAI->TellMasterNoFacing(out.str());
        return MoveNear(bot->GetMapId(), pos.x, pos.y, pos.z + 0.5f, sPlayerbotAIConfig.followDistance);
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("私聊 'go x,y'、'go [游戏对象]'、'go 单位' 或 'go 位置'，我就会前往那里");
    //End By leewheel
    return false;
}
