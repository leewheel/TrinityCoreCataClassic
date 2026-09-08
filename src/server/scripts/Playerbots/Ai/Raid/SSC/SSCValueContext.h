/* 毒蛇神殿 机器人策略 */
//By leewheel 2026-09-04: 上游d86674ef——新建 SSC 值上下文: 毒水池危害位置缓存值(500ms),
//  供 UnderbogColossusEscapeToxicPoolAction / SSC 触发器经 SscHelpers 查询
#ifndef PLAYERBOTS_SSCVALUECONTEXT_H
#define PLAYERBOTS_SSCVALUECONTEXT_H

#include "SSCHelpers.h"
#include "EncounterHelpers.h"
#include "NamedObjectContext.h"
#include "Value.h"
#include <string>
#include <vector>

using SscHelpers::SscSpells;
using namespace EncounterHelpers;

class SscHazardPositionsValue : public CalculatedValue<std::vector<Position>>
{
public:
    SscHazardPositionsValue(
        PlayerbotAI* botAI, std::string const& name, uint32 spellId, float searchRadius)
        : CalculatedValue<std::vector<Position>>(
              botAI, name, SscHelpers::HAZARD_CACHE_INTERVAL),
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

class RaidSscValueContext : public NamedObjectContext<UntypedValue>
{
public:
    RaidSscValueContext()
    {
        creators["ssc toxic pool"] = &RaidSscValueContext::ssc_toxic_pool;
    }

private:
    static UntypedValue* ssc_toxic_pool(PlayerbotAI* botAI) {
        return new SscHazardPositionsValue(
            botAI, "ssc toxic pool", SscHelpers::Id(SscSpells::SPELL_TOXIC_POOL),
            SscHelpers::TOXIC_POOL_SEARCH_RADIUS);
    }
};

#endif
