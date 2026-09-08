/*
 * 服务器外观模式
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_SERVERFACADE_H
#define PLAYERBOTS_SERVERFACADE_H

class Player;
class Unit;
class WorldObject;
class WorldPacket;

/**
 * 服务器引擎操作的外观模式接口
 * 为机器人系统提供集中化的工具方法
 */
class ServerFacade
{
public:
    ServerFacade() {}
    virtual ~ServerFacade() {}

    static ServerFacade& instance()
    {
        static ServerFacade instance;
        return instance;
    }

public:
    float GetDistance2d(Unit* unit, WorldObject* wo);
    float GetDistance2d(Unit* unit, float x, float y);
    bool IsDistanceLessThan(float dist1, float dist2);
    bool IsDistanceGreaterThan(float dist1, float dist2);
    bool IsDistanceGreaterOrEqualThan(float dist1, float dist2);
    bool IsDistanceLessOrEqualThan(float dist1, float dist2);
    void SetFacingTo(Player* bot, WorldObject* wo, bool force = false);
    Unit* GetChaseTarget(Unit* target);
    void SendPacket(Player* player, WorldPacket* packet);
};

#define sServerFacade ServerFacade::instance()

#endif // PLAYERBOTS_SERVERFACADE_H
