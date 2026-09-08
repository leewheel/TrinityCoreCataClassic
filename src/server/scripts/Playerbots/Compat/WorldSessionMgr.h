#ifndef _ACWORLDSESSIONMGR_H_
#define _ACWORLDSESSIONMGR_H_

/*
 * AzerothCore的WorldSessionMgr是AC特有的会话管理器
 * TrinityCore使用World类管理会话
 * 这个垫片提供空接口定义，具体使用需要适配
 */

#include "World/World.h"

class WorldSessionMgr
{
public:
    static WorldSessionMgr* instance()
    {
        static WorldSessionMgr inst;
        return &inst;
    }

    // TC中实际使用sWorld管理会话，这里提供空壳
    WorldSession* FindSession(uint32 id) const;
    void UpdateSessions(uint32 diff);
};

#define sWorldSessionMgr WorldSessionMgr::instance()

#endif