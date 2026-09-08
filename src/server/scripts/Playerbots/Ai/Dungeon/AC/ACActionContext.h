#ifndef PLAYERBOTS_ACACTIONCONTEXT_H
#define PLAYERBOTS_ACACTIONCONTEXT_H

#include "AiObjectContext.h"
#include "Action.h"
#include "ACActions.h"
//By leewheel 2026-08-24: the-lab 引入 NamedObjectContext.h
#include "NamedObjectContext.h"
//End By leewheel

class TbcDungeonAuchenaiCryptsActionContext : public NamedObjectContext<Action>
{
public:
    TbcDungeonAuchenaiCryptsActionContext()
    {
        creators["shirrak tank position boss"] =
            &TbcDungeonAuchenaiCryptsActionContext::shirrak_tank_position_boss;

        creators["shirrak flee focus fire"] =
            &TbcDungeonAuchenaiCryptsActionContext::shirrak_flee_focus_fire;

        creators["shirrak ranged keep distance"] =
            &TbcDungeonAuchenaiCryptsActionContext::shirrak_ranged_keep_distance;
    }
private:
    static Action* shirrak_tank_position_boss(
        PlayerbotAI* botAI) { return new ShirrakTankPositionBossAction(botAI); }

    static Action* shirrak_flee_focus_fire(
        PlayerbotAI* botAI) { return new ShirrakFleeFocusFireAction(botAI); }

    static Action* shirrak_ranged_keep_distance(
        PlayerbotAI* botAI) { return new ShirrakRangedKeepDistanceAction(botAI); }
};

#endif
