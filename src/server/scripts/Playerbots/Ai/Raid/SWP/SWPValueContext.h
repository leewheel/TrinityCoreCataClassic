/* 太阳之井高地 机器人策略 */
/* 移植来源: brighton-chi mod-playerbots the-lab SWPValueContext.h 移植适配 TC 框架 */
//By leewheel 2026-08-21: 移植 brighton-chi cbfc7ea0——SWP值上下文(双子烈焰位置缓存)
#ifndef PLAYERBOTS_SWPVALUECONTEXT_H
#define PLAYERBOTS_SWPVALUECONTEXT_H

#include "NamedObjectContext.h"
#include "SWPActions.h"
#include "SWPEncounter_KJ.h"
#include "SWPEncounter_Kalec.h"
#include "SWPEncounter_Muru.h"
#include "SWPEncounter_Twins.h"
#include "Value.h"
#include "Position.h"
#include <vector>

class EredarTwinsBlazePositionsValue : public CalculatedValue<std::vector<Position>>
{
public:
    EredarTwinsBlazePositionsValue(PlayerbotAI* botAI)
        : CalculatedValue<std::vector<Position>>(
              botAI, "eredar twins blaze", SwpHelpers::EREDAR_TWINS_BLAZE_CACHE_INTERVAL_MS) {}

protected:
    std::vector<Position> Calculate() override
    {
        return SwpHelpers::FindEredarTwinsBlazePositions(bot);
    }
};

// Collapses four per-tick sweeps of "possible targets no los"
class MuruEncounterTargetsValue : public CalculatedValue<SwpHelpers::MuruEncounterGuids>
{
public:
    MuruEncounterTargetsValue(PlayerbotAI* botAI)
        : CalculatedValue<SwpHelpers::MuruEncounterGuids>(
              botAI, "muru encounter targets",
              SwpHelpers::MURU_ENCOUNTER_TARGETS_CACHE_INTERVAL_MS) {}

protected:
    SwpHelpers::MuruEncounterGuids Calculate() override
    {
        return SwpHelpers::FindMuruEncounterGuids(botAI);
    }
};

// The four values below replace grid searches that are otherwise run by corresponding triggers and
// actions each tick.
class MuruVoidZonesValue : public CalculatedValue<GuidVector>
{
public:
    MuruVoidZonesValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(
              botAI, "muru void zones", SwpHelpers::VOID_ZONE_CACHE_INTERVAL_MS) {}

protected:
    GuidVector Calculate() override { return SwpHelpers::FindMuruVoidZoneGuids(bot); }
};
//End By leewheel

class SwpVolatileFiendValue : public CalculatedValue<ObjectGuid>
{
public:
    SwpVolatileFiendValue(PlayerbotAI* botAI)
        : CalculatedValue<ObjectGuid>(
              botAI, "swp volatile fiend", SwpHelpers::VOLATILE_FIEND_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return SwpHelpers::FindSwpVolatileFiendGuid(bot); }
};

class KalecgosSpectralRiftValue : public CalculatedValue<ObjectGuid>
{
public:
    KalecgosSpectralRiftValue(PlayerbotAI* botAI)
        : CalculatedValue<ObjectGuid>(
              botAI, "kalecgos spectral rift", SwpHelpers::SPECTRAL_RIFT_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return SwpHelpers::FindKalecgosSpectralRiftGuid(bot); }
};

class MuruSingularityValue : public CalculatedValue<ObjectGuid>
{
public:
    MuruSingularityValue(PlayerbotAI* botAI)
        : CalculatedValue<ObjectGuid>(
              botAI, "muru singularity", SwpHelpers::SINGULARITY_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return SwpHelpers::FindMuruSingularityGuid(bot); }
};

class KiljaedenDragonOrbsValue : public CalculatedValue<GuidVector>
{
public:
    KiljaedenDragonOrbsValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(
              botAI, "kiljaeden dragon orbs", SwpHelpers::DRAGON_ORB_CACHE_INTERVAL_MS) {}

protected:
    GuidVector Calculate() override { return SwpHelpers::FindKiljaedenDragonOrbGuids(bot); }
};

//By leewheel 2026-08-29: 引入 mod-playerbots 新提交——KJ 手部 GUID 缓存值
class KiljaedenHandsValue : public CalculatedValue<GuidVector>
{
public:
    KiljaedenHandsValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(
              botAI, "kiljaeden hands", SwpHelpers::HAND_CACHE_INTERVAL_MS) {}

protected:
    GuidVector Calculate() override { return SwpHelpers::FindKiljaedenHandGuids(bot); }
};
//End By leewheel

//By leewheel 2026-09-04: 上游——RaidSunwellValueContext 改名 RaidSwpValueContext
class RaidSwpValueContext : public NamedObjectContext<UntypedValue>
{
public:
    RaidSwpValueContext()
    {
        creators["eredar twins blaze"] = &RaidSwpValueContext::eredar_twins_blaze;
        creators["muru encounter targets"] = &RaidSwpValueContext::muru_encounter_targets;
        creators["muru void zones"] = &RaidSwpValueContext::muru_void_zones;
        creators["swp volatile fiend"] = &RaidSwpValueContext::swp_volatile_fiend;
        creators["kalecgos spectral rift"] = &RaidSwpValueContext::kalecgos_spectral_rift;
        creators["muru singularity"] = &RaidSwpValueContext::muru_singularity;
        creators["kiljaeden dragon orbs"] = &RaidSwpValueContext::kiljaeden_dragon_orbs;
        creators["kiljaeden hands"] = &RaidSwpValueContext::kiljaeden_hands;
    }

private:
    static UntypedValue* eredar_twins_blaze(PlayerbotAI* botAI) {
        return new EredarTwinsBlazePositionsValue(botAI);
    }
    static UntypedValue* muru_encounter_targets(PlayerbotAI* botAI) {
        return new MuruEncounterTargetsValue(botAI);
    }
    static UntypedValue* muru_void_zones(PlayerbotAI* botAI) {
        return new MuruVoidZonesValue(botAI);
    }
    static UntypedValue* swp_volatile_fiend(PlayerbotAI* botAI) {
        return new SwpVolatileFiendValue(botAI);
    }
    static UntypedValue* kalecgos_spectral_rift(PlayerbotAI* botAI) {
        return new KalecgosSpectralRiftValue(botAI);
    }
    static UntypedValue* muru_singularity(PlayerbotAI* botAI) {
        return new MuruSingularityValue(botAI);
    }
    static UntypedValue* kiljaeden_dragon_orbs(PlayerbotAI* botAI) {
        return new KiljaedenDragonOrbsValue(botAI);
    }
    static UntypedValue* kiljaeden_hands(PlayerbotAI* botAI) {
        return new KiljaedenHandsValue(botAI);
    }
};
//End By leewheel

#endif
