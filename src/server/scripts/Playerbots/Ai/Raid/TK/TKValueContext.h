/*
 * 风暴要塞 值上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_TKVALUECONTEXT_H
#define PLAYERBOTS_TKVALUECONTEXT_H

#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "TKHelpers.h"
#include "Value.h"

//By leewheel 2026-08-21: 移植 brighton-chi 77ff8ec4——凯尔萨斯神器武器死亡GUID缓存值(每200ms刷新)
class TKDeadLegendaryWeaponsValue : public CalculatedValue<GuidVector>
{
public:
    TKDeadLegendaryWeaponsValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(botAI, "tk dead legendary weapons", 200) {}

protected:
    GuidVector Calculate() override { return TkHelpers::FindDeadLegendaryWeaponGuids(bot); }
};

class RaidTempestKeepValueContext : public NamedObjectContext<UntypedValue>
{
public:
    RaidTempestKeepValueContext()
    {
        creators["tk dead legendary weapons"] =
            &RaidTempestKeepValueContext::tk_dead_legendary_weapons;
    }

private:
    static UntypedValue* tk_dead_legendary_weapons(PlayerbotAI* botAI) {
        return new TKDeadLegendaryWeaponsValue(botAI);
    }
};
//End By leewheel

#endif