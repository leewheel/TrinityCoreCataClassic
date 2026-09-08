/* 副本机器人策略 */
//By leewheel 2026-09-04: 上游067fab59/8baf63da——新增 GruulsLairEncounterTrigger 中间基类(IsEncounterInProgress闸门),
//  全部BOSS触发器改继承并实现 IsActiveInEncounter; NoEncounterInProgress触发器改标准命名+每秒节流
//End By leewheel
#ifndef PLAYERBOTS_GRUULTRIGGERS_H
#define PLAYERBOTS_GRUULTRIGGERS_H

//By leewheel 2026-09-04: 上游——中间基类需要 EncounterHelpers/GruulHelpers
#include "EncounterHelpers.h"
#include "GruulHelpers.h"
#include "Trigger.h"
#include <string>
//End By leewheel

//By leewheel 2026-09-04: 上游——副本内触发器中间基类
// General

class GruulsLairEncounterTrigger : public Trigger
{
public:
    GruulsLairEncounterTrigger(PlayerbotAI* botAI, std::string const name, int32 checkInterval = 1)
        : Trigger(botAI, name, checkInterval) {}

    bool IsActive() final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, GruulHelpers::GRUUL_MAP_ID) &&
            IsActiveInEncounter();
    }

protected:
    virtual bool IsActiveInEncounter() = 0;
};
//End By leewheel

class GruulsLairNoEncounterInProgressTrigger : public Trigger
{
public:
    //By leewheel 2026-09-04: 上游067fab59——无战斗触发器每秒节流一次(平时/清理无紧迫性); 类名对齐上游加Trigger后缀
    GruulsLairNoEncounterInProgressTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "gruul's lair no encounter in progress", 1000) {}
    //End By leewheel
    bool IsActive() override;
};

//By leewheel 2026-09-04: 上游——注释分段
// High King Maulgar <Lord of the Ogres>

class HighKingMaulgarThreeOgresNeedMeleeTanksTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarThreeOgresNeedMeleeTanksTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar three ogres need melee tanks") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarKroshNeedsMageTankTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarKroshNeedsMageTankTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar krosh needs mage tank") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarKigglerNeedsMoonkinTankTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarKigglerNeedsMoonkinTankTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar kiggler needs moonkin tank") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarDeterminingKillOrderTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarDeterminingKillOrderTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar determining kill order") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarBossChannelingWhirlwindTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarBossChannelingWhirlwindTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar boss channeling whirlwind") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarKroshCastsBlastWaveTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarKroshCastsBlastWaveTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar krosh casts blast wave") {}

protected:
    bool IsActiveInEncounter() override;
};

class HighKingMaulgarWildFelStalkerSpawnedTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarWildFelStalkerSpawnedTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar wild fel stalker spawned") {}

protected:
    bool IsActiveInEncounter() override;
};

//By leewheel 2026-09-04 对齐上游: 恢复 GruulsLairEncounterTrigger 基类——
//原裸 Trigger+IsActive() 无副本状态闸门, 开怪前/灭团重整时误导触发器会激活白耗 2 分钟 CD
class HighKingMaulgarPullingOgreCouncilTrigger : public GruulsLairEncounterTrigger
{
public:
    HighKingMaulgarPullingOgreCouncilTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "high king maulgar pulling ogre council") {}

protected:
    bool IsActiveInEncounter() override;
};

//By leewheel 2026-09-04: 对齐上游b8304144——fear ward改由通用牧师策略处理，删除恐吓咆哮触发类
//End By leewheel

//By leewheel 2026-09-04: 上游——注释分段
// Gruul the Dragonkiller

class GruulTheDragonkillerShouldBeTankedTrigger : public GruulsLairEncounterTrigger
{
public:
    GruulTheDragonkillerShouldBeTankedTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "gruul the dragonkiller should be tanked") {}

protected:
    bool IsActiveInEncounter() override;
};

class GruulTheDragonkillerRangedShouldSpreadTrigger : public GruulsLairEncounterTrigger
{
public:
    GruulTheDragonkillerRangedShouldSpreadTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "gruul the dragonkiller ranged should spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class GruulTheDragonkillerIncomingShatterTrigger : public GruulsLairEncounterTrigger
{
public:
    GruulTheDragonkillerIncomingShatterTrigger(PlayerbotAI* botAI)
        : GruulsLairEncounterTrigger(botAI, "gruul the dragonkiller incoming shatter") {}

protected:
    bool IsActiveInEncounter() override;
};

#endif
