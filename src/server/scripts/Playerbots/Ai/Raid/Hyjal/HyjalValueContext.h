/*
 * 海加尔山 机器人值上下文(移植自brighton-chi the-lab)
 * By leewheel 2026-08-14
 */

#ifndef PLAYERBOTS_HYJALVALUECONTEXT_H
#define PLAYERBOTS_HYJALVALUECONTEXT_H

#include "HyjalHelpers.h"
#include "EncounterHelpers.h"
#include "NamedObjectContext.h"
#include "Value.h"
//By leewheel 2026-08-21: 移植 brighton-chi 8c9ab03d——补标准头, 不依赖间接包含
#include <string>
#include <vector>
//End By leewheel

using HyjalHelpers::HyjalSpells;
using namespace EncounterHelpers;

// Anetheron的地狱火GUID，最旧优先。缓存因为其背后的网格搜索不该由每个trigger/乘数
// 每tick对每个bot重复执行。
class HyjalInfernalsValue : public CalculatedValue<GuidVector>
{
public:
    HyjalInfernalsValue(PlayerbotAI* botAI) : CalculatedValue<GuidVector>(botAI, "hyjal infernals", 200) {}

protected:
    GuidVector Calculate() override { return HyjalHelpers::FindInfernalGuids(bot); }
};

// 地面危害位置的缓存，与地狱火同理由缓存：Engine每tick对队列里的每个动作应用所有乘数，
// 在乘数体内做网格搜索=每个动作付一次搜索费，远不如每bot一次。
//
// 每组按其各自的半径搜索（由最宽的使用方派生），helper 再进一步收窄
class HyjalHazardPositionsValue : public CalculatedValue<std::vector<Position>>
{
public:
    HyjalHazardPositionsValue(
        PlayerbotAI* botAI, std::string const& name, uint32 spellId, float searchRadius)
        : CalculatedValue<std::vector<Position>>(
              botAI, name, HyjalHelpers::HAZARD_CACHE_INTERVAL),
          _spellId(spellId), _searchRadius(searchRadius) {}

protected:
    std::vector<Position> Calculate() override
    {
        return GetDynamicObjectPositions(bot, _searchRadius, _spellId);
    }

private:
    uint32 const _spellId;
    float const _searchRadius;
};
//End By leewheel

class //By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext : public NamedObjectContext<UntypedValue>
{
public:
    //By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext()
    {
        creators["hyjal infernals"] = &//By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext::hyjal_infernals;
        creators["hyjal death and decay"] = &//By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext::hyjal_death_and_decay;
        creators["hyjal rain of fire"] = &//By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext::hyjal_rain_of_fire;
        creators["hyjal doomfire trail"] = &//By leewheel 2026-09-04: 上游——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
RaidHyjalValueContext::hyjal_doomfire_trail;
    }

private:
    static UntypedValue* hyjal_infernals(PlayerbotAI* botAI) { return new HyjalInfernalsValue(botAI); }
    static UntypedValue* hyjal_death_and_decay(PlayerbotAI* botAI) {
        return new HyjalHazardPositionsValue(
            botAI, "hyjal death and decay", HyjalHelpers::Id(HyjalSpells::SPELL_DEATH_AND_DECAY),
            HyjalHelpers::DEATH_AND_DECAY_SEARCH_RADIUS);
    }
    static UntypedValue* hyjal_rain_of_fire(PlayerbotAI* botAI) {
        return new HyjalHazardPositionsValue(
            botAI, "hyjal rain of fire", HyjalHelpers::Id(HyjalSpells::SPELL_RAIN_OF_FIRE),
            HyjalHelpers::RAIN_OF_FIRE_SEARCH_RADIUS);
    }
    static UntypedValue* hyjal_doomfire_trail(PlayerbotAI* botAI) {
        return new HyjalHazardPositionsValue(
            botAI, "hyjal doomfire trail", HyjalHelpers::Id(HyjalSpells::SPELL_DOOMFIRE_TRAIL),
            HyjalHelpers::DOOMFIRE_SEARCH_RADIUS);
    }
};

#endif
