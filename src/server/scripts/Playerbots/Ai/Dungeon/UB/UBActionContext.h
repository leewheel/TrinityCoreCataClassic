/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBACTIONCONTEXT_H
#define PLAYERBOTS_UBACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "UBActions.h"

class TbcDungeonUnderbogActionContext : public NamedObjectContext<Action>
{
public:
    TbcDungeonUnderbogActionContext()
    {
        creators["ub retreat from foul spores"] = &TbcDungeonUnderbogActionContext::ub_retreat_from_foul_spores;
        creators["ub vacate spore cloud"] = &TbcDungeonUnderbogActionContext::ub_vacate_spore_cloud;
        creators["ub clear underbat back"] = &TbcDungeonUnderbogActionContext::ub_clear_underbat_back;
    }

private:
    static Action* ub_retreat_from_foul_spores(PlayerbotAI* botAI) { return new UBRetreatFromFoulSporesAction(botAI); }

    static Action* ub_vacate_spore_cloud(PlayerbotAI* botAI) { return new UBVacateSporeCloudAction(botAI); }

    static Action* ub_clear_underbat_back(PlayerbotAI* botAI) { return new UBClearUnderbatBackAction(botAI); }
};

#endif
