#ifndef _ACRACEMGR_H_
#define _ACRACEMGR_H_

/*
 * AzerothCore的RaceMgr是AC特有的种族管理器
 * TrinityCore使用DB2中的ChrRacesEntry
 */

#include "DataStores/DB2Structure.h"
#include "SharedDefines.h"

class RaceMgr
{
public:
    static RaceMgr* instance()
    {
        static RaceMgr inst;
        return &inst;
    }

    static uint8 GetMaxRaces() { return MAX_RACES; }
};

#define sRaceMgr RaceMgr::instance()

#endif