#ifndef PLAYERBOTS_NEWRPGOUTDOORPVP_H
#define PLAYERBOTS_NEWRPGOUTDOORPVP_H

#include "NewRpgBaseAction.h"
#include "OutdoorPvP.h"

class NewRpgOutdoorPvpAction : public NewRpgBaseAction
{
public:
    NewRpgOutdoorPvpAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg outdoor pvp") {}

    virtual bool Execute(Event event) override;
    OPvPCapturePoint* SelectNewObjective(OutdoorPvP::OPvPCapturePointMap const& capturePointMap);

private:
    // By leewheel 2026-07-08: 改为使用WorldPosition替代GameObject*
    bool PatrolCapturePoint(WorldPosition const& targetPos, float radius);
    // End By leewheel
};

#endif
