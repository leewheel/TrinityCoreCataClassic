/* 副本机器人策略 */
//By leewheel 2026-09-04: 上游14413ee2——新增 ZulAmanEncounterMultiplier 中间基类(IsEncounterInProgress首道闸门),
//  各乘数改继承并实现 GetValueInEncounter; 新增 HexLordMalacrassStayAwayFromFreezingTrapMultiplier;
//  Halazzi 主坦禁嘲讽山猫之灵逻辑并入统一乘数(本地原有按boss拆分乘数保留兼容 ZAActionContext 注册)
//End By leewheel
#ifndef PLAYERBOTS_ZAMULTIPLIERS_H
#define PLAYERBOTS_ZAMULTIPLIERS_H

//By leewheel 2026-09-04: 上游——中间基类需要 EncounterHelpers/ZAHelpers
#include "EncounterHelpers.h"
#include "Multiplier.h"
#include "ZAHelpers.h"
#include <string>
//End By leewheel

//By leewheel 2026-09-04: 上游——副本内乘数中间基类: 非战斗期(垃圾桶位/重置期)一律放行1.0
class ZulAmanEncounterMultiplier : public Multiplier
{
public:
    ZulAmanEncounterMultiplier(PlayerbotAI* botAI, std::string const name)
        : Multiplier(botAI, name) {}

    float GetValue(Action* action) final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, ZaHelpers::ZA_MAP_ID)
            ? GetValueInEncounter(action) : 1.0f;
    }

protected:
    virtual float GetValueInEncounter(Action* action) = 0;
};
//End By leewheel

// General

class ZulAmanDelayDpsCooldownsMultiplier : public ZulAmanEncounterMultiplier
{
public:
    ZulAmanDelayDpsCooldownsMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "zul'aman delay dps cooldowns") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class ZulAmanDisableCombatFormationMoveMultiplier : public ZulAmanEncounterMultiplier
{
public:
    ZulAmanDisableCombatFormationMoveMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "zul'aman disable combat formation move") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class ZulAmanAvoidWhirlwindMultiplier : public ZulAmanEncounterMultiplier
{
public:
    ZulAmanAvoidWhirlwindMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "zul'aman avoid whirlwind") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Akil'zon <Eagle Avatar>

class AkilzonStayInEyeOfTheStormMultiplier : public ZulAmanEncounterMultiplier
{
public:
    AkilzonStayInEyeOfTheStormMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "akil'zon stay in eye of the storm") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Nalorakk <Bear Avatar>

//By leewheel 2026-09-04: 上游14413ee2——按boss拆分的本地乘数并入统一乘数, 类保留但改为继承中间基类
//  (语义对齐上游 ZulAmanDisableTankActionsMultiplier/ControlMisdirection 的 Nalorakk 分支)
//End By leewheel
class NalorakkDisableTankActionsMultiplier : public ZulAmanEncounterMultiplier
{
public:
    NalorakkDisableTankActionsMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "nalorakk disable tank actions") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class NalorakkControlMisdirectionMultiplier : public ZulAmanEncounterMultiplier
{
public:
    NalorakkControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "nalorakk control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Jan'alai <Dragonhawk Avatar>

class JanalaiDisableTankActionsMultiplier : public ZulAmanEncounterMultiplier
{
public:
    JanalaiDisableTankActionsMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "jan'alai disable tank actions") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class JanalaiStayAwayFromFireBombsMultiplier : public ZulAmanEncounterMultiplier
{
public:
    JanalaiStayAwayFromFireBombsMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "jan'alai stay away from fire bombs") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class JanalaiDoNotCrowdControlHatchersMultiplier : public ZulAmanEncounterMultiplier
{
public:
    JanalaiDoNotCrowdControlHatchersMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "jan'alai do not crowd control hatchers") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Halazzi <Lynx Avatar>

class HalazziDisableTankActionsMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HalazziDisableTankActionsMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "halazzi disable tank actions") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class HalazziControlMisdirectionMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HalazziControlMisdirectionMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "halazzi control misdirection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class HalazziDisableAutoDpsTargetingMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HalazziDisableAutoDpsTargetingMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "halazzi disable auto dps targeting") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

// Hex Lord Malacrass

class HexLordMalacrassUnstableAfflictionMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HexLordMalacrassUnstableAfflictionMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "hex lord malacrass unstable affliction") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class HexLordMalacrassSpellReflectionMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HexLordMalacrassSpellReflectionMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "hex lord malacrass spell reflection") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

//By leewheel 2026-09-04: 上游14413ee2——新增冰霜陷阱驻留乘数(接近类动作在陷阱附近一律暂停)
class HexLordMalacrassStayAwayFromFreezingTrapMultiplier : public ZulAmanEncounterMultiplier
{
public:
    HexLordMalacrassStayAwayFromFreezingTrapMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "hex lord malacrass stay away from freezing trap") {}

protected:
    float GetValueInEncounter(Action* action) override;
};
//End By leewheel

// Zul'jin

class ZuljinDisableTankFaceMultiplier : public ZulAmanEncounterMultiplier
{
public:
    ZuljinDisableTankFaceMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "zul'jin disable tank face") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

class ZuljinEagleDisableAvoidAoeMultiplier : public ZulAmanEncounterMultiplier
{
public:
    ZuljinEagleDisableAvoidAoeMultiplier(PlayerbotAI* botAI)
        : ZulAmanEncounterMultiplier(botAI, "zul'jin eagle disable avoid aoe") {}

protected:
    float GetValueInEncounter(Action* action) override;
};

#endif
