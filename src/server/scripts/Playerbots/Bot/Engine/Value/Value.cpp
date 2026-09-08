/*
 * 值基类实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 适配: LOG_DEBUG -> TC_LOG_DEBUG
 */

#include "Value.h"

#include "AiObjectContext.h"
#include "PerfMonitor.h"
#include "Playerbots.h"
#include "Timer.h"
#include "ObjectMgr.h"
#include "CreatureData.h"

UnitCalculatedValue::UnitCalculatedValue(PlayerbotAI* botAI, std::string const name, int32 checkInterval)
    : CalculatedValue<Unit*>(botAI, name, checkInterval)
{
}

std::string const UnitCalculatedValue::Format()
{
    Unit* unit = Calculate();
    return unit ? unit->GetName() : "<无>";
}

std::string const UnitManualSetValue::Format()
{
    Unit* unit = Get();
    return unit ? unit->GetName() : "<无>";
}

std::string const Uint8CalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

std::string const Uint32CalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

std::string const FloatCalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

CDPairCalculatedValue::CDPairCalculatedValue(PlayerbotAI* botAI, std::string const name, int32 checkInterval)
    : CalculatedValue<CreatureData const*>(botAI, name, checkInterval)
{
}

std::string const CDPairCalculatedValue::Format()
{
    CreatureData const* creatureData = Calculate();
    if (creatureData)
    {
        CreatureTemplate const* bmTemplate = sObjectMgr->GetCreatureTemplate(creatureData->id);
        return bmTemplate ? bmTemplate->Name : "<无>";
    }

    return "<无>";
}

CDPairListCalculatedValue::CDPairListCalculatedValue(PlayerbotAI* botAI, std::string const name, int32 checkInterval)
    : CalculatedValue<std::vector<CreatureData const*>>(botAI, name, checkInterval)
{
}

std::string const CDPairListCalculatedValue::Format()
{
    std::ostringstream out;
    out << "{";
    std::vector<CreatureData const*> cdPairs = Calculate();
    for (CreatureData const* cdPair : cdPairs)
    {
        out << cdPair->id << ",";
    }

    out << "}";
    return out.str();
}

ObjectGuidCalculatedValue::ObjectGuidCalculatedValue(PlayerbotAI* botAI, std::string const name, int32 checkInterval)
    : CalculatedValue<ObjectGuid>(botAI, name, checkInterval)
{
}

std::string const ObjectGuidCalculatedValue::Format()
{
    ObjectGuid guid = Calculate();
    return !guid.IsEmpty() ? guid.ToString() : "<无>";
}

ObjectGuidListCalculatedValue::ObjectGuidListCalculatedValue(PlayerbotAI* botAI, std::string const name,
                                                             int32 checkInterval)
    : CalculatedValue<GuidVector>(botAI, name, checkInterval)
{
}

std::string const ObjectGuidListCalculatedValue::Format()
{
    std::ostringstream out;
    out << "{";

    GuidVector guids = Calculate();
    for (GuidVector::iterator i = guids.begin(); i != guids.end(); ++i)
    {
        ObjectGuid guid = *i;
        out << guid.ToString() << ",";
    }
    out << "}";

    return out.str();
}

Unit* UnitCalculatedValue::Get()
{
    if (checkInterval < 2)
    {
        PerfMonitorOperation* pmo = sPerfMonitor.start(
            PERF_MON_VALUE, this->getName(), this->context ? &this->context->performanceStack : nullptr);
        value = Calculate();
        if (pmo)
            pmo->finish();
    }
    else
    {
        time_t now = getMSTime();
        if (!lastCheckTime || now - lastCheckTime >= checkInterval)
        {
            lastCheckTime = now;
            PerfMonitorOperation* pmo = sPerfMonitor.start(
                PERF_MON_VALUE, this->getName(), this->context ? &this->context->performanceStack : nullptr);
            value = Calculate();
            if (pmo)
                pmo->finish();
        }
    }
    // 防止崩溃：检查是否在世界中
    if (value && value->IsInWorld())
        return value;
    return nullptr;
}

Unit* UnitManualSetValue::Get()
{
    // 防止崩溃：检查是否在世界中
    if (value && value->IsInWorld())
        return value;
    return nullptr;
}
