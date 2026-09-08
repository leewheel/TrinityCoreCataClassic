#ifndef PLAYERBOTS_DUNGEONSTRATEGYUTILS_H
#define PLAYERBOTS_DUNGEONSTRATEGYUTILS_H

//By leewheel 2026-07-09: 添加Playerbots.h包含以获取DUNGEON_DIFFICULTY兼容宏
#include "Playerbots.h"
//End By leewheel

template<class T> inline
const T& DUNGEON_MODE(Player* bot, const T& normal5, const T& heroic10)
{
    //By leewheel 2026-09-06: 移植到TrinityCore-Cata，Map::GetDifficulty改名为GetDifficultyID
    switch (bot->GetMap()->GetDifficultyID())
    //End By leewheel
    {
        case DUNGEON_DIFFICULTY_NORMAL:
            return normal5;
        case DUNGEON_DIFFICULTY_HEROIC:
            return heroic10;
        default:
            break;
    }

    return heroic10;
}

#endif
