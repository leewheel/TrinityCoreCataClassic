/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBTRIGGERS_H
#define PLAYERBOTS_UBTRIGGERS_H

#include "Trigger.h"

class UBFoulSporesTrigger : public Trigger
{
public:
    UBFoulSporesTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ub foul spores") {}
    bool IsActive() override;
};

class UBSporeCloudDangerTrigger : public Trigger
{
public:
    UBSporeCloudDangerTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ub spore cloud danger") {}
    bool IsActive() override;
};

class UBUnderbatLashTrigger : public Trigger
{
public:
    UBUnderbatLashTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ub underbat lash") {}
    bool IsActive() override;
};

#endif
