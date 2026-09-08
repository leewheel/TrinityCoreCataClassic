/* 太阳之井高地 机器人策略 */
//By leewheel 2026-09-04: 上游——新增 SunwellPlateauEncounterTrigger 中间基类(IsEncounterInProgress闸门),
//  全部BOSS触发器改继承并实现 IsActiveInEncounter; AuraToRemove触发器每秒节流
//End By leewheel
#ifndef PLAYERBOTS_SWPTRIGGERS_H
#define PLAYERBOTS_SWPTRIGGERS_H

//By leewheel 2026-09-04: 上游——中间基类需要 EncounterHelpers/SWPSharedConstants
#include "EncounterHelpers.h"
#include "SWPSharedConstants.h"
#include "Trigger.h"
#include <string>
//End By leewheel

// General

//By leewheel 2026-09-04: 上游——副本内触发器中间基类
class SunwellPlateauEncounterTrigger : public Trigger
{
public:
    SunwellPlateauEncounterTrigger(PlayerbotAI* botAI, std::string const name, int32 checkInterval = 1)
        : Trigger(botAI, name, checkInterval) {}

    bool IsActive() final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, SwpHelpers::SWP_MAP_ID) &&
            IsActiveInEncounter();
    }

protected:
    virtual bool IsActiveInEncounter() = 0;
};
//End By leewheel

class SunwellPlateauNoEncounterInProgressTrigger : public Trigger
{
public:
    SunwellPlateauNoEncounterInProgressTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "sunwell plateau no encounter in progress", 1000) {} //By leewheel 2026-09-04: 上游067fab59——无战斗触发器每秒节流一次(平时/清理无紧迫性)
    bool IsActive() override;
};
//End By leewheel

class SunwellPlateauBotHasAuraToRemoveTrigger : public Trigger
{
public:
    //By leewheel 2026-09-04: 上游——同样节流(战斗中也可能发生, 清冰 block/圣盾, 稍延迟更拟人)
    SunwellPlateauBotHasAuraToRemoveTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "sunwell plateau bot has aura to remove", 1000) {}
    //End By leewheel
    bool IsActive() override;
};

// Trash

class VolatileFiendSelfDestructsWhenNearTrigger : public Trigger
{
public:
    VolatileFiendSelfDestructsWhenNearTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "volatile fiend self destructs when near") {}
    bool IsActive() override;
};

class ApocalypseGuardProtectedByInfernalDefenseTrigger : public Trigger
{
public:
    ApocalypseGuardProtectedByInfernalDefenseTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "apocalypse guard protected by infernal defense") {}
    bool IsActive() override;
};

// Kalecgos

//By leewheel 2026-09-04: 上游——卡雷苟斯段触发器改继承 SunwellPlateauEncounterTrigger
class KalecgosShouldCommunicateBossHealthTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosShouldCommunicateBossHealthTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos should communicate boss health") {}

protected:
    bool IsActiveInEncounter() override;
};

//By leewheel 2026-08-21: 移植 brighton-chi b49a9cc2——卡雷苟斯拉怪阶段(猎人误导)触发器
class KalecgosPullingBossTrigger : public Trigger
{
public:
    KalecgosPullingBossTrigger(PlayerbotAI* botAI) : Trigger(botAI, "kalecgos pulling boss") {}
    bool IsActive() override;
};
//End By leewheel

class KalecgosRequiresTankRotationTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosRequiresTankRotationTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos requires tank rotation") {}

protected:
    bool IsActiveInEncounter() override;
};

class KalecgosSpectralRiftIsOpenTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosSpectralRiftIsOpenTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos spectral rift is open") {}

protected:
    bool IsActiveInEncounter() override;
};

class KalecgosBotsTakeSplashDamageTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosBotsTakeSplashDamageTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos bots take splash damage") {}

protected:
    bool IsActiveInEncounter() override;
};

class KalecgosTooManyArcaneBuffetStacksTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosTooManyArcaneBuffetStacksTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos too many arcane buffet stacks") {}

protected:
    bool IsActiveInEncounter() override;
};

class KalecgosHumanoidKalecTanksSathrovarrTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosHumanoidKalecTanksSathrovarrTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos humanoid kalec tanks sathrovarr") {}

protected:
    bool IsActiveInEncounter() override;
};

class KalecgosBotsDontObserveGravityTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KalecgosBotsDontObserveGravityTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kalecgos bots don't observe gravity") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

// Brutallus

class BrutallusPullingBossTrigger : public Trigger
{
public:
    BrutallusPullingBossTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "brutallus pulling boss") {}
    bool IsActive() override;
};

//By leewheel 2026-09-04: 上游——布鲁塔卢斯段触发器改继承 SunwellPlateauEncounterTrigger
class BrutallusRequiresTwoTanksTrigger : public SunwellPlateauEncounterTrigger
{
public:
    BrutallusRequiresTwoTanksTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "brutallus requires two tanks") {}

protected:
    bool IsActiveInEncounter() override;
};

class BrutallusMeleeShouldStandInPlaceTrigger : public SunwellPlateauEncounterTrigger
{
public:
    BrutallusMeleeShouldStandInPlaceTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "brutallus melee should stand in place") {}

protected:
    bool IsActiveInEncounter() override;
};

class BrutallusRangedShouldSoakMeteorSlashTrigger : public SunwellPlateauEncounterTrigger
{
public:
    BrutallusRangedShouldSoakMeteorSlashTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "brutallus ranged should soak meteor slash") {}

protected:
    bool IsActiveInEncounter() override;
};

class BrutallusBotIsBurningTrigger : public SunwellPlateauEncounterTrigger
{
public:
    BrutallusBotIsBurningTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "brutallus bot is burning") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

// Felmyst

class FelmystPullingBossTrigger : public Trigger
{
public:
    FelmystPullingBossTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "felmyst pulling boss") {}
    bool IsActive() override;
};

//By leewheel 2026-09-04: 上游——菲米丝段触发器改继承 SunwellPlateauEncounterTrigger
class FelmystGroundPhaseShouldBeTankedTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystGroundPhaseShouldBeTankedTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst ground phase should be tanked") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystRangedShouldPositionToDispelAndFleeTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystRangedShouldPositionToDispelAndFleeTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst ranged should position to dispel and flee") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystMeleeShouldStayTogetherTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystMeleeShouldStayTogetherTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst melee should stay together") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystBotIsEncapsulatedTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystBotIsEncapsulatedTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst bot is encapsulated") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystBotNearEncapsulatedPlayerTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystBotNearEncapsulatedPlayerTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst bot near encapsulated player") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystPlayerHasGasNovaTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystPlayerHasGasNovaTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst player has gas nova") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystShouldAvoidDemonicVaporTrailsTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystShouldAvoidDemonicVaporTrailsTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst should avoid demonic vapor trails") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystBotIsDemonicVaporTargetTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystBotIsDemonicVaporTargetTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst bot is demonic vapor target") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystFogOfCorruptionIsActiveTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystFogOfCorruptionIsActiveTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst fog of corruption is active") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystMeleeCannotReachFlyingBossTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystMeleeCannotReachFlyingBossTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst melee cannot reach flying boss") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystPlayerIsCharmedByFogTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystPlayerIsCharmedByFogTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst player is charmed by fog") {}

protected:
    bool IsActiveInEncounter() override;
};

class FelmystShouldHoldDpsWhileLandingTrigger : public SunwellPlateauEncounterTrigger
{
public:
    FelmystShouldHoldDpsWhileLandingTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "felmyst should hold dps while landing") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

// Eredar Twins

//By leewheel 2026-09-04: 上游——双子段触发器改继承 SunwellPlateauEncounterTrigger
class EredarTwinsMeleeIsAtBalconyTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsMeleeIsAtBalconyTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins melee is at balcony") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsPullingBossesTrigger : public Trigger
{
public:
    EredarTwinsPullingBossesTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "eredar twins pulling bosses") {}
    bool IsActive() override;
};

class EredarTwinsSacrolashRequiresTwoTanksTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsSacrolashRequiresTwoTanksTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins sacrolash requires two tanks") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsAlythessCastsBlazeOnTankTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsAlythessCastsBlazeOnTankTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins alythess casts blaze on tank") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsRangedNeedsLosTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsRangedNeedsLosTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins ranged needs los") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsOnlyAlythessRemainsTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsOnlyAlythessRemainsTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins only alythess remains") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsTooManyFlameTouchedStacksTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsTooManyFlameTouchedStacksTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins too many flame touched stacks") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsShouldFocusDpsTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsShouldFocusDpsTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins should focus dps") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsActiveConflagrationTargetTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsActiveConflagrationTargetTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins active conflagration target") {}

protected:
    bool IsActiveInEncounter() override;
};

class EredarTwinsSacrolashVictimHasConflagrationTrigger : public SunwellPlateauEncounterTrigger
{
public:
    EredarTwinsSacrolashVictimHasConflagrationTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "eredar twins sacrolash victim has conflagration") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

// M'uru

//By leewheel 2026-09-04: 上游——穆鲁段触发器改继承 SunwellPlateauEncounterTrigger
class MuruVoidSentinelOrEntropiusHasAppearedTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruVoidSentinelOrEntropiusHasAppearedTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru void sentinel or entropius has appeared") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruBossTransformedIntoEntropiusTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruBossTransformedIntoEntropiusTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru boss transformed into entropius") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruRangedShouldStackOrSpreadTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruRangedShouldStackOrSpreadTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru ranged should stack or spread") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruDeterminingDpsPriorityTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruDeterminingDpsPriorityTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru determining dps priority") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruVoidSentinelPulsesShadowTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruVoidSentinelPulsesShadowTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru void sentinel pulses shadow") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruAddsSpawnAtEntranceTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruAddsSpawnAtEntranceTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru adds spawn at entrance") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruDarkFiendsSpawnedTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruDarkFiendsSpawnedTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru dark fiends spawned") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruDarknessIsComingTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruDarknessIsComingTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru darkness is coming") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruBerserkerIsBuffedWithFlurryTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruBerserkerIsBuffedWithFlurryTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru berserker is buffed with flurry") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruFuryMageCastingFelFireballTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruFuryMageCastingFelFireballTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru fury mage casting fel fireball") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruFuryMageIsBuffedWithSpellFuryTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruFuryMageIsBuffedWithSpellFuryTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru fury mage is buffed with spell fury") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruVoidSpawnAvailableForEnslaveTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruVoidSpawnAvailableForEnslaveTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru void spawn available for enslave") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruWarlockHasEnslavedVoidSpawnTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruWarlockHasEnslavedVoidSpawnTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru warlock has enslaved void spawn") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruEntropiusDarknessPoolsSpawnDarkFiendsTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruEntropiusDarknessPoolsSpawnDarkFiendsTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru entropius darkness pools spawn dark fiends") {}

protected:
    bool IsActiveInEncounter() override;
};

class MuruTheSingularityIsNearTrigger : public SunwellPlateauEncounterTrigger
{
public:
    MuruTheSingularityIsNearTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "m'uru the singularity is near") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

// Kil'jaeden <The Deceiver>

class KiljaedenEncounterHasBegunTrigger : public Trigger
{
public:
    KiljaedenEncounterHasBegunTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "kil'jaeden encounter has begun") {}
    bool IsActive() override;
};

class KiljaedenHandsOfTheDeceiverAreActiveTrigger : public Trigger
{
public:
    KiljaedenHandsOfTheDeceiverAreActiveTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "kil'jaeden hands of the deceiver are active") {}
    bool IsActive() override;
};

//By leewheel 2026-09-04: 上游——KJ自身不下车不报IN_PROGRESS(控制器在第一只欺诈之手死后才置位),
//  前两个触发器不能用 SunwellPlateauEncounterTrigger; 之后的触发器都需要KJ本体(三只手全灭才出现)
//By leewheel 2026-09-04: 上游——基尔加丹段触发器改继承 SunwellPlateauEncounterTrigger
class KiljaedenTanksShouldHoldBossAndReflectionsTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenTanksShouldHoldBossAndReflectionsTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden tanks should hold boss and reflections") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenBossEngagedByMeleeTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenBossEngagedByMeleeTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden boss engaged by melee") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenBossEngagedByRangedTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenBossEngagedByRangedTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden boss engaged by ranged") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenBotHasFireBloomTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenBotHasFireBloomTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden bot has fire bloom") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenSaysChaosDestructionOblivionTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenSaysChaosDestructionOblivionTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden says: Chaos! Destruction! Oblivion!") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenDragonOrbIsActiveTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenDragonOrbIsActiveTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden dragon orb is active") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenBotHasStaleRootAfterDragonTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenBotHasStaleRootAfterDragonTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden bot has stale root after dragon") {}

protected:
    bool IsActiveInEncounter() override;
};

class KiljaedenBotControlsDragonTrigger : public SunwellPlateauEncounterTrigger
{
public:
    KiljaedenBotControlsDragonTrigger(PlayerbotAI* botAI)
        : SunwellPlateauEncounterTrigger(botAI, "kil'jaeden bot controls dragon") {}

protected:
    bool IsActiveInEncounter() override;
};
//End By leewheel

#endif
