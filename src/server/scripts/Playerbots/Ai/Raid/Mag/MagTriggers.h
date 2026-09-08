/* 玛瑟里顿的巢穴 机器人策略 */
#ifndef PLAYERBOTS_MAGTRIGGERS_H
#define PLAYERBOTS_MAGTRIGGERS_H

#include "PlayerbotAI.h"
#include "Trigger.h"

//By leewheel 2026-09-04: 上游——NoEncounterInProgress 触发器移到文件头部(先于具体 Boss 触发器声明)
class MagtheridonNoEncounterInProgressTrigger : public Trigger
{
public:
    //By leewheel 2026-09-04: 上游067fab59——无战斗触发器每秒节流一次(平时/清理无紧迫性)
    MagtheridonNoEncounterInProgressTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon no encounter in progress", 1000) {}
    bool IsActive() override;
};
//End By leewheel

class MagtheridonFirstThreeChannelersEngagedByMainTankTrigger : public Trigger
{
public:
    MagtheridonFirstThreeChannelersEngagedByMainTankTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon first three channelers engaged by main tank") {}
    bool IsActive() override;
};

class MagtheridonLastTwoChannelersEngagedByAssistTanksTrigger : public Trigger
{
public:
    MagtheridonLastTwoChannelersEngagedByAssistTanksTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon last two channelers engaged by assist tanks") {}
    bool IsActive() override;
};

class MagtheridonPullingWestAndEastChannelersTrigger : public Trigger
{
public:
    MagtheridonPullingWestAndEastChannelersTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon pulling west and east channelers") {}
    bool IsActive() override;
};

class MagtheridonDeterminingKillOrderTrigger : public Trigger
{
public:
    MagtheridonDeterminingKillOrderTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon determining kill order") {}
    bool IsActive() override;
};

class MagtheridonBurningAbyssalSpawnedTrigger : public Trigger
{
public:
    MagtheridonBurningAbyssalSpawnedTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon burning abyssal spawned") {}
    bool IsActive() override;
};

class MagtheridonBossEngagedByMainTankTrigger : public Trigger
{
public:
    MagtheridonBossEngagedByMainTankTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon boss engaged by main tank") {}
    bool IsActive() override;
};

class MagtheridonBossEngagedByRangedTrigger : public Trigger
{
public:
    MagtheridonBossEngagedByRangedTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon boss engaged by ranged") {}
    bool IsActive() override;
};
class MagtheridonStandingInDebrisTrigger : public Trigger
{
public:
    MagtheridonStandingInDebrisTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon standing in debris") {};
    bool IsActive() override;
};

class MagtheridonIncomingBlastNovaTrigger : public Trigger
{
public:
    MagtheridonIncomingBlastNovaTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon incoming blast nova") {}
    bool IsActive() override;
};

class MagtheridonNeedToManageTimersAndAssignmentsTrigger : public Trigger
{
public:
    MagtheridonNeedToManageTimersAndAssignmentsTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "magtheridon need to manage timers and assignments") {}
    bool IsActive() override;
};

//(MagtheridonNoEncounterInProgressTrigger 已移至文件头部——By leewheel 2026-09-04 上游)

#endif
