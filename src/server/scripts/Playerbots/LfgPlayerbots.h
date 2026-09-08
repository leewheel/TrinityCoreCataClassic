/*
 * LFG 类型定义 - 为 Playerbots 模块提供 AzerothCore 兼容的 Lfg5Guids 类型
 * TrinityCore 没有此类型，这里创建一个兼容版本
 */

#ifndef LFG_PLAYERBOTS_H
#define LFG_PLAYERBOTS_H

#include "ObjectGuid.h"
#include <array>
#include <map>

namespace lfg
{
    typedef std::map<ObjectGuid, uint8> LfgRolesMap;

    class Lfg5Guids
    {
    public:
        std::array<ObjectGuid, 5> guids = { };
        LfgRolesMap* roles;

        Lfg5Guids()
        {
            guids.fill(ObjectGuid::Empty);
            roles = nullptr;
        }

        Lfg5Guids(ObjectGuid g)
        {
            guids.fill(ObjectGuid::Empty);
            guids[0] = g;
            roles = nullptr;
        }

        bool IsEmpty() const
        {
            for (ObjectGuid const& g : guids)
                if (!g.IsEmpty())
                    return false;
            return true;
        }

        bool operator<(Lfg5Guids const& x) const
        {
            for (uint8 i = 0; i < 5; ++i)
            {
                if (guids[i] < x.guids[i])
                    return true;
                else if (guids[i] > x.guids[i])
                    return false;
            }
            return false; // 等于
        }
    };
}

#endif // LFG_PLAYERBOTS_H
