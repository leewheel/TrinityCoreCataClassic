/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  新增 ArchimondeControlDoomfireAvoidanceMultiplier(ab50f6a1/fdefbb16)。
  By leewheel 2026-09-04: 上游6d5d68cd——新增 HyjalSummitEncounterMultiplier 中间基类
  (IsEncounterInProgress 廉价闸门, GetValue() final + GetValueInEncounter() 虚函数),
  Boss 乘数改继承基类; HyjalSummitDelayDpsCooldownsMultiplier 保持直继 Multiplier。*/
#ifndef PLAYERBOTS_HYJALMULTIPLIERS_H
#define PLAYERBOTS_HYJALMULTIPLIERS_H

#include "EncounterHelpers.h"
#include "Multiplier.h"
#include "Util/HyjalHelpers.h"
#include <string>

class HyjalSummitEncounterMultiplier : public Multiplier
{
public:
    HyjalSummitEncounterMultiplier(PlayerbotAI* botAI, std::string const name)
        : Multiplier(botAI, name) {}

    float GetValue(Action* action) final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, HyjalHelpers::HYJAL_MAP_ID)
            ? GetValueInEncounter(action) : 1.0f;
    }

protected:
    virtual float GetValueInEncounter(Action* action) = 0;
};

class HyjalSummitDelayDpsCooldownsMultiplier : public Multiplier
{
public:
    HyjalSummitDelayDpsCooldownsMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "hyjal summit delay dps cooldowns") {}
    float GetValue(Action* action) override;
};

// Rage Winterchill

class RageWinterchillDisableCombatFormationMoveMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    RageWinterchillDisableCombatFormationMoveMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "rage winterchill disable combat formation move") {}
    float GetValueInEncounter(Action* action) override;
};

class RageWinterchillMeleeControlAvoidanceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    RageWinterchillMeleeControlAvoidanceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "rage winterchill melee control avoidance") {}
    float GetValueInEncounter(Action* action) override;
};

class RageWinterchillRangedControlAvoidanceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    RageWinterchillRangedControlAvoidanceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "rage winterchill ranged control avoidance") {}
    float GetValueInEncounter(Action* action) override;
};

// Anetheron

class AnetheronDisableAssistTargetingMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AnetheronDisableAssistTargetingMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "anetheron disable assist targeting") {}
    float GetValueInEncounter(Action* action) override;
};

class AnetheronAvoidAccidentalInfernalAggroMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AnetheronAvoidAccidentalInfernalAggroMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "anetheron avoid accidental infernal aggro") {}
    float GetValueInEncounter(Action* action) override;
};

class AnetheronInfernalTargetRunToPositionMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AnetheronInfernalTargetRunToPositionMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "anetheron infernal target run to position") {}
    float GetValueInEncounter(Action* action) override;
};

class AnetheronControlMovementMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AnetheronControlMovementMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "anetheron control movement") {}
    float GetValueInEncounter(Action* action) override;
};

class AnetheronControlMisdirectionMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AnetheronControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "anetheron control misdirection") {}
    float GetValueInEncounter(Action* action) override;
};

// Kaz'rogal

class KazrogalDisableDisperseAndTankFaceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    KazrogalDisableDisperseAndTankFaceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "kaz'rogal disable disperse and tank face") {}
    float GetValueInEncounter(Action* action) override;
};

class KazrogalControlLowManaMovementMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    KazrogalControlLowManaMovementMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "kaz'rogal control low mana movement") {}
    float GetValueInEncounter(Action* action) override;
};

class KazrogalKeepAspectOfTheViperActiveMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    KazrogalKeepAspectOfTheViperActiveMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "kaz'rogal keep aspect of the viper active") {}
    float GetValueInEncounter(Action* action) override;
};

// Azgalor

class AzgalorDisableAutoTargetingAndPositioningMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AzgalorDisableAutoTargetingAndPositioningMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "azgalor disable auto targeting and positioning") {}
    float GetValueInEncounter(Action* action) override;
};

class AzgalorDoomedBotPrioritizePositioningMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AzgalorDoomedBotPrioritizePositioningMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "azgalor doomed bot prioritize positioning") {}
    float GetValueInEncounter(Action* action) override;
};

class AzgalorMeleeDpsControlAvoidanceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AzgalorMeleeDpsControlAvoidanceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "azgalor melee dps control avoidance") {}
    float GetValueInEncounter(Action* action) override;
};

class AzgalorRangedControlAvoidanceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    AzgalorRangedControlAvoidanceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "azgalor ranged control avoidance") {}
    float GetValueInEncounter(Action* action) override;
};

// Archimonde

class ArchimondeDisableCombatFormationMoveMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    ArchimondeDisableCombatFormationMoveMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "archimonde disable combat formation move") {}
    float GetValueInEncounter(Action* action) override;
};

class ArchimondeSetTremorTotemMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    ArchimondeSetTremorTotemMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "archimonde set tremor totem") {}
    float GetValueInEncounter(Action* action) override;
};

class ArchimondeControlDoomfireAvoidanceMultiplier : public HyjalSummitEncounterMultiplier
{
public:
    ArchimondeControlDoomfireAvoidanceMultiplier(PlayerbotAI* botAI)
        : HyjalSummitEncounterMultiplier(botAI, "archimonde control doomfire avoidance") {}
    float GetValueInEncounter(Action* action) override;
};

#endif
