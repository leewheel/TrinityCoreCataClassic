/* 太阳之井高地 机器人策略 */
//By leewheel 2026-09-04: 上游——新增 SunwellPlateauEncounterMultiplier 中间基类(IsEncounterInProgress闸门),
//  全部BOSS乘数改继承并实现 GetValueInEncounter; 新增 VolatileFiendRestrictApproachMultiplier(内敛恶魔犬接近抑制)
//End By leewheel
#ifndef PLAYERBOTS_SWPMULTIPLIERS_H
#define PLAYERBOTS_SWPMULTIPLIERS_H

//By leewheel 2026-09-04: 上游——中间基类需要 EncounterHelpers/SWPSharedConstants
#include "EncounterHelpers.h"
#include "Multiplier.h"
#include "SWPSharedConstants.h"
#include <string>
//End By leewheel

// General

//By leewheel 2026-09-04: 上游——副本内乘数中间基类
class SunwellPlateauEncounterMultiplier : public Multiplier
{
public:
    SunwellPlateauEncounterMultiplier(PlayerbotAI* botAI, std::string const name)
        : Multiplier(botAI, name) {}

    float GetValue(Action* action) final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, SwpHelpers::SWP_MAP_ID)
            ? GetValueInEncounter(action) : 1.0f;
    }

protected:
    virtual float GetValueInEncounter(Action* action) = 0;
};
//End By leewheel

class SunwellPlateauNoEncounterDrinkingMultiplier : public Multiplier
{
public:
    SunwellPlateauNoEncounterDrinkingMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "sunwell plateau no encounter drinking") {}
    float GetValue(Action* action) override;
};

//By leewheel 2026-09-04: 上游70808114——新增Trash段(内敛恶魔犬自爆接近抑制乘数)
// Trash

class VolatileFiendRestrictApproachMultiplier : public Multiplier
{
public:
    VolatileFiendRestrictApproachMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "volatile fiend restrict approach") {}
    float GetValue(Action* action) override;
};

// Kalecgos

class KalecgosControlMisdirectionMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosWaitToDecurseMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosWaitToDecurseMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos wait to decurse") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosControlMovementMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosControlMovementMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosRestrictTauntMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosRestrictTauntMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos restrict taunt") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosSuppressAssistTankPullThreatMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosSuppressAssistTankPullThreatMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos suppress assist tank pull threat") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosEnterSpectralRiftMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosEnterSpectralRiftMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos enter spectral rift") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KalecgosDelayCooldownsForSathrovarrMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KalecgosDelayCooldownsForSathrovarrMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kalecgos delay cooldowns for sathrovarr") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Brutallus

class BrutallusControlMisdirectionMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    BrutallusControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "brutallus control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class BrutallusControlMovementMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    BrutallusControlMovementMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "brutallus control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class BrutallusNoKillingSpreeWhenNearbyBurnMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    BrutallusNoKillingSpreeWhenNearbyBurnMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "brutallus no killing spree when nearby burn") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class BrutallusRestrictTauntMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    BrutallusRestrictTauntMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "brutallus restrict taunt") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class BrutallusDelayCooldownsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    BrutallusDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "brutallus delay cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Felmyst

class FelmystControlMovementMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystControlMovementMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystWaitForLandingDpsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystWaitForLandingDpsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst wait for landing dps") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystPrioritizeEncapsulateAvoidanceMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystPrioritizeEncapsulateAvoidanceMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst prioritize encapsulate avoidance") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystPrioritizeFogAvoidanceMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystPrioritizeFogAvoidanceMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst prioritize fog avoidance") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystPrioritizeDemonicVaporAvoidanceMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystPrioritizeDemonicVaporAvoidanceMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst prioritize demonic vapor avoidance") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystFocusAttacksOnCharmedPlayerMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystFocusAttacksOnCharmedPlayerMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst focus attacks on charmed player") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystDontDotAddsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystDontDotAddsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst don't dot adds") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class FelmystDelayCooldownsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    FelmystDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "felmyst delay cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Eredar Twins

class EredarTwinsDisableAutomaticTargetingMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsDisableAutomaticTargetingMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins disable automatic targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsControlMisdirectionMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins misdirect bosses to tanks") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsHoldDpsAtStartMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsHoldDpsAtStartMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins hold dps at start") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsControlThreatMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsControlThreatMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins control threat") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsControlMovementMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsControlMovementMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsIsolateConflagrationMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsIsolateConflagrationMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins isolate conflagration") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class EredarTwinsDelayCooldownsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    EredarTwinsDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "eredar twins delay cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// M'uru

class MuruDisableDefaultTargetingMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    MuruDisableDefaultTargetingMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "m'uru disable default targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class MuruControlMisdirectionMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    MuruControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "m'uru control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class MuruControlMovementMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    MuruControlMovementMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "m'uru control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class MuruDelayCooldownsMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    MuruDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "m'uru delay cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Kil'jaeden <The Deceiver>

//By leewheel 2026-09-04: 上游——前两个乘数不加闸门(见 SWPTriggers.h 注释: 欺诈之手阶段先于 IN_PROGRESS,
//  二者都是压制型乘数, 闸门默认1.0恰恰放行它们要禁止的东西)
class KiljaedenDelayCooldownsMultiplier : public Multiplier
{
public:
    KiljaedenDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "kil'jaeden delay cooldowns") {}
    float GetValue(Action* action) override;
};

//By leewheel 2026-09-05: 对齐 the-lab c9774e50——坦克/DPS 双乘数合并为单目标之手乘数
//  (原 TanksFocusAssignedHandOnly 删除, DpsFocusAssignedHandOnly 改名并重写为通用压制型乘数)
class KiljaedenSingleTargetHandsMultiplier : public Multiplier
{
public:
    KiljaedenSingleTargetHandsMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "kil'jaeden single target hands") {}
    float GetValue(Action* action) override;
};
//End By leewheel

class KiljaedenControlMovementAndTargetingMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KiljaedenControlMovementAndTargetingMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kil'jaeden control movement and targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KiljaedenPrioritizeDarknessProtectionMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KiljaedenPrioritizeDarknessProtectionMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kil'jaeden prioritize darkness protection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KiljaedenControlDragonMultiplier : public SunwellPlateauEncounterMultiplier
{
public:
    KiljaedenControlDragonMultiplier(PlayerbotAI* botAI)
        : SunwellPlateauEncounterMultiplier(botAI, "kil'jaeden dragon buff and protect raid") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

#endif
