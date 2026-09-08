/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBACTIONS_H
#define PLAYERBOTS_UBACTIONS_H

#include "MovementActions.h"

class UBRetreatFromFoulSporesAction : public MovementAction
{
public:
    UBRetreatFromFoulSporesAction(PlayerbotAI* botAI) : MovementAction(botAI, "ub retreat from foul spores") {}
    bool Execute(Event event) override;
};

class UBVacateSporeCloudAction : public MovementAction
{
public:
    UBVacateSporeCloudAction(PlayerbotAI* botAI) : MovementAction(botAI, "ub vacate spore cloud") {}
    bool Execute(Event event) override;
};

class UBClearUnderbatBackAction : public MovementAction
{
public:
    UBClearUnderbatBackAction(PlayerbotAI* botAI) : MovementAction(botAI, "ub clear underbat back") {}
    bool Execute(Event event) override;

private:
    uint32 _lastReposition = 0;
};

#endif
