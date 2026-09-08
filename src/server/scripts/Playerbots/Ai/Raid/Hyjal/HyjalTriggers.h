/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 47abcff2(压缩: 拉怪/主坦接战触发收敛为通用 HyjalPullingBossTrigger/
  HyjalBossShouldBeTankedTrigger, 带接战血量参数)。*/
//By leewheel 2026-09-04: 上游6d5d68cd——新增 HyjalSummitEncounterTrigger 中间基类(IsEncounterInProgress
//  廉价闸门, IsActive() final + IsActiveInEncounter() 虚函数), Boss 触发器改继承基类;
//  无战斗触发器补每秒节流
//End By leewheel
#ifndef PLAYERBOTS_HYJALTRIGGERS_H
#define PLAYERBOTS_HYJALTRIGGERS_H

#include "EncounterHelpers.h"
#include "Util/HyjalHelpers.h"
#include "Trigger.h"
#include <string>

// General

class HyjalSummitEncounterTrigger : public Trigger
{
public:
    HyjalSummitEncounterTrigger(PlayerbotAI* botAI, std::string const name, int32 checkInterval = 1)
        : Trigger(botAI, name, checkInterval) {}

    bool IsActive() final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, HyjalHelpers::HYJAL_MAP_ID) &&
            IsActiveInEncounter();
    }

protected:
    virtual bool IsActiveInEncounter() = 0;
};

class HyjalSummitNoEncounterInProgress : public Trigger
{
public:
    //By leewheel 2026-09-04: 上游067fab59——无战斗触发器每秒节流一次(平时/清理无紧迫性)
    HyjalSummitNoEncounterInProgress(PlayerbotAI* botAI)
        : Trigger(botAI, "hyjal summit no encounter in progress", 1000) {}
    bool IsActive() override;
};

// A hunter opening the fight, which is any hunter looking at a boss still on full health. Four of
// the five encounters are pulled exactly this way and only the boss differs, so they share one
// class and name it at registration. Anetheron has its own: there the hunter keeps misdirecting
// all fight to hand over Infernals, so it never gates on health at all
class HyjalPullingBossTrigger : public Trigger
{
public:
    HyjalPullingBossTrigger(
        PlayerbotAI* botAI, std::string const& name, std::string const& bossName)
        : Trigger(botAI, name), _bossName(bossName) {}
    bool IsActive() override;

private:
    std::string const _bossName;
};

// This covers all five MT actions, and activeAboveHealthPct is used for Archimonde only.
// Anetheron, Kaz'rogal, and Azgalor need their offtanks free for the Infernals, the
// Malevolent Cleave split, and the Doomguards, respectively, so those three are main tank only.
//By leewheel 2026-08-30: 补回 mainTankOnly 参数（上游有此参数，本地遗漏导致 HyjalTriggerContext.h 5参调用报 C2661）
class HyjalBossShouldBeTankedTrigger : public HyjalSummitEncounterTrigger
{
public:
    HyjalBossShouldBeTankedTrigger(
        PlayerbotAI* botAI, std::string const& name, std::string const& bossName,
        float activeAboveHealthPct = 0.0f, bool mainTankOnly = true)
        : HyjalSummitEncounterTrigger(botAI, name), _bossName(bossName),
          _activeAboveHealthPct(activeAboveHealthPct), _mainTankOnly(mainTankOnly) {}

protected:
    bool IsActiveInEncounter() override;

private:
    std::string const _bossName;
    float const _activeAboveHealthPct;
    bool const _mainTankOnly;
};
//End By leewheel

// Rage Winterchill

class RageWinterchillRangedShouldSpreadTrigger : public HyjalSummitEncounterTrigger
{
public:
    RageWinterchillRangedShouldSpreadTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "rage winterchill ranged should spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class RageWinterchillMeleeNearDeathAndDecayTrigger : public HyjalSummitEncounterTrigger
{
public:
    RageWinterchillMeleeNearDeathAndDecayTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "rage winterchill melee near death and decay") {}

protected:
    bool IsActiveInEncounter() override;
};

class RageWinterchillRangedInDeathAndDecayTrigger : public HyjalSummitEncounterTrigger
{
public:
    RageWinterchillRangedInDeathAndDecayTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "rage winterchill ranged in death and decay") {}

protected:
    bool IsActiveInEncounter() override;
};

// Anetheron

class AnetheronPullingBossOrInfernalTrigger : public Trigger
{
public:
    AnetheronPullingBossOrInfernalTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "anetheron pulling boss or infernal") {}
    bool IsActive() override;
};

class AnetheronRangedShouldSpreadTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronRangedShouldSpreadTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron ranged should spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class AnetheronBotIsNearInfernoTargetTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronBotIsNearInfernoTargetTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron bot is near inferno target") {}

protected:
    bool IsActiveInEncounter() override;
};

class AnetheronBotIsTargetedByInfernalTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronBotIsTargetedByInfernalTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron bot is targeted by infernal") {}

protected:
    bool IsActiveInEncounter() override;
};

class AnetheronInfernalsPulseImmolationTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronInfernalsPulseImmolationTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron infernals pulse immolation") {}

protected:
    bool IsActiveInEncounter() override;
};

class AnetheronInfernalsShouldBeTankedAwayTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronInfernalsShouldBeTankedAwayTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron infernals should be tanked away") {}

protected:
    bool IsActiveInEncounter() override;
};

class AnetheronShouldDivideDpsTrigger : public HyjalSummitEncounterTrigger
{
public:
    AnetheronShouldDivideDpsTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "anetheron should divide dps") {}

protected:
    bool IsActiveInEncounter() override;
};

// Kaz'rogal

class KazrogalCanSplitMalevolentCleaveDamageTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalCanSplitMalevolentCleaveDamageTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal can split malevolent cleave damage") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalRangedShouldAvoidWarStompTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalRangedShouldAvoidWarStompTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal ranged should avoid war stomp") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalBotIsLowOnManaTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalBotIsLowOnManaTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal bot is low on mana") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalHunterShouldPreserveManaTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalHunterShouldPreserveManaTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal hunter should preserve mana") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalMarkOnMageOrPaladinTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalMarkOnMageOrPaladinTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal mark on mage or paladin") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalImmunityNoLongerNeededTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalImmunityNoLongerNeededTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal immunity no longer needed") {}

protected:
    bool IsActiveInEncounter() override;
};

class KazrogalWarlockShouldManageManaTrigger : public HyjalSummitEncounterTrigger
{
public:
    KazrogalWarlockShouldManageManaTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "kaz'rogal warlock should manage mana") {}

protected:
    bool IsActiveInEncounter() override;
};

// Azgalor

class AzgalorRangedShouldSpreadTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorRangedShouldSpreadTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor ranged should spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class AzgalorMeleeNearRainOfFireTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorMeleeNearRainOfFireTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor melee near rain of fire") {}

protected:
    bool IsActiveInEncounter() override;
};

class AzgalorRangedInRainOfFireTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorRangedInRainOfFireTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor ranged in rain of fire") {}

protected:
    bool IsActiveInEncounter() override;
};

class AzgalorBotIsDoomedTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorBotIsDoomedTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor bot is doomed") {}

protected:
    bool IsActiveInEncounter() override;
};

class AzgalorShouldControlDoomguardsTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorShouldControlDoomguardsTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor should control doomguards") {}

protected:
    bool IsActiveInEncounter() override;
};

class AzgalorShouldDivideDpsTrigger : public HyjalSummitEncounterTrigger
{
public:
    AzgalorShouldDivideDpsTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "azgalor should divide dps") {}

protected:
    bool IsActiveInEncounter() override;
};

// Archimonde

class ArchimondeBossCastsFearTrigger : public HyjalSummitEncounterTrigger
{
public:
    ArchimondeBossCastsFearTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "archimonde boss casts fear") {}

protected:
    bool IsActiveInEncounter() override;
};

class ArchimondeBossCastingAirBurstTrigger : public HyjalSummitEncounterTrigger
{
public:
    ArchimondeBossCastingAirBurstTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "archimonde boss casting air burst") {}

protected:
    bool IsActiveInEncounter() override;
};

class ArchimondeRangedShouldSpreadTrigger : public HyjalSummitEncounterTrigger
{
public:
    ArchimondeRangedShouldSpreadTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "archimonde ranged should spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class ArchimondeBotIsNearDoomfireTrigger : public HyjalSummitEncounterTrigger
{
public:
    ArchimondeBotIsNearDoomfireTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "archimonde bot is near doomfire") {}

protected:
    bool IsActiveInEncounter() override;
};

class ArchimondeBotStoodInDoomfireTrigger : public HyjalSummitEncounterTrigger
{
public:
    ArchimondeBotStoodInDoomfireTrigger(PlayerbotAI* botAI)
        : HyjalSummitEncounterTrigger(botAI, "archimonde bot stood in doomfire") {}

protected:
    bool IsActiveInEncounter() override;
};

#endif
