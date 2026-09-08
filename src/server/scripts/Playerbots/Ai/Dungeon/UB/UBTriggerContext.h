/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBTRIGGERCONTEXT_H
#define PLAYERBOTS_UBTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "TriggerContext.h"
#include "UBTriggers.h"

class TbcDungeonUnderbogTriggerContext : public NamedObjectContext<Trigger>
{
public:
    TbcDungeonUnderbogTriggerContext()
    {
        creators["ub foul spores"] = &TbcDungeonUnderbogTriggerContext::ub_foul_spores;
        creators["ub spore cloud danger"] = &TbcDungeonUnderbogTriggerContext::ub_spore_cloud_danger;
        creators["ub underbat lash"] = &TbcDungeonUnderbogTriggerContext::ub_underbat_lash;
    }

private:
    static Trigger* ub_foul_spores(PlayerbotAI* botAI) { return new UBFoulSporesTrigger(botAI); }

    static Trigger* ub_spore_cloud_danger(PlayerbotAI* botAI) { return new UBSporeCloudDangerTrigger(botAI); }

    static Trigger* ub_underbat_lash(PlayerbotAI* botAI) { return new UBUnderbatLashTrigger(botAI); }
};

#endif
