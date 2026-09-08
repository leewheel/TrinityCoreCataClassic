/*
 * 风暴要塞 机器人乘数声明
 *
 * 作者: leewheel
 * 移植来源: AC [TKMultipliers.h] 移植适配 TC 框架
 * 业务对标: AC mod-playerbots src/Ai/Raid/TK/TKMultipliers.h
 * By leewheel 2026-09-04: 上游4f9815d1——新增 TempestKeepEncounterMultiplier 中间基类(IsEncounterInProgress
 *   廉价闸门, GetValue() final + GetValueInEncounter() 虚函数), 全部乘数改继承基类
 * End By leewheel
 */

#ifndef PLAYERBOTS_TKMULTIPLIERS_H
#define PLAYERBOTS_TKMULTIPLIERS_H

#include "EncounterHelpers.h"
#include "Multiplier.h"
#include "TKHelpers.h"
#include <string>

// General

class TempestKeepEncounterMultiplier : public Multiplier
{
public:
    TempestKeepEncounterMultiplier(PlayerbotAI* botAI, std::string const name)
        : Multiplier(botAI, name) {}

    float GetValue(Action* action) final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, TkHelpers::TK_MAP_ID)
            ? GetValueInEncounter(action) : 1.0f;
    }

protected:
    virtual float GetValueInEncounter(Action* action) = 0;
};

// Al'ar <Phoenix God>

class AlarSuppressGapClosersMultiplier : public TempestKeepEncounterMultiplier
{
public:
    AlarSuppressGapClosersMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "al'ar suppress gap closers") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class AlarControlMovementMultiplier : public TempestKeepEncounterMultiplier
{
public:
    AlarControlMovementMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "al'ar control movement") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class AlarDisableAutomaticTargetingMultiplier : public TempestKeepEncounterMultiplier
{
public:
    AlarDisableAutomaticTargetingMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "al'ar disable automatic targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class AlarStayAwayFromRebirthMultiplier : public TempestKeepEncounterMultiplier
{
public:
    AlarStayAwayFromRebirthMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "al'ar stay away from rebirth") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class AlarControlTauntingMultiplier : public TempestKeepEncounterMultiplier
{
public:
    AlarControlTauntingMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "al'ar control taunting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Void Reaver

class VoidReaverMaintainPositionsMultiplier : public TempestKeepEncounterMultiplier
{
public:
    VoidReaverMaintainPositionsMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "void reaver maintain positions") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// High Astromancer Solarian

class HighAstromancerSolarianDisableMeleeTargetingMultiplier : public TempestKeepEncounterMultiplier
{
public:
    HighAstromancerSolarianDisableMeleeTargetingMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "high astromancer solarian disable melee targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class HighAstromancerSolarianWrathStayAwayMultiplier : public TempestKeepEncounterMultiplier
{
public:
    HighAstromancerSolarianWrathStayAwayMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "high astromancer solarian wrath stay away") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Kael'thas Sunstrider <Lord of the Blood Elves>

class KaelthasSunstriderWaitForDpsMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderWaitForDpsMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider wait for dps") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderKiteThaladredMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderKiteThaladredMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider kiting thaladred") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderControlMisdirectionMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

//By leewheel 2026-08-21: 移植 brighton-chi 5232cb9d——卡波妮娅术士坦禁用灵魂碎裂
class KaelthasSunstriderDisableWarlockTankSoulshatterMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderDisableWarlockTankSoulshatterMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider disable warlock tank soulshatter") {}

protected:
    float GetValueInEncounter(Action* action) override;
};
//End By leewheel

class KaelthasSunstriderKeepDistanceFromCapernianMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderKeepDistanceFromCapernianMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider keep distance from capernian") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderManageWeaponTankingMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderManageWeaponTankingMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider manage weapon tanking") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderSuppressEquipUpgradeMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderSuppressEquipUpgradeMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider suppress equip upgrade") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderManageAutomaticTargetingMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderManageAutomaticTargetingMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider manage automatic targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderDisableDisperseMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderDisableDisperseMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider disable disperse") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderPrepareForPhase3Multiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderPrepareForPhase3Multiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider prepare for phase 3") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderDelayCooldownsMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderDelayCooldownsMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider delay cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class KaelthasSunstriderStaySpreadDuringGravityLapseMultiplier : public TempestKeepEncounterMultiplier
{
public:
    KaelthasSunstriderStaySpreadDuringGravityLapseMultiplier(PlayerbotAI* botAI)
        : TempestKeepEncounterMultiplier(botAI, "kael'thas sunstrider stay spread during gravity lapse") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

#endif
