/*
 * AI 对象上下文实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObjectContext.h"
#include "Helpers.h"

//By leewheel 2026-07-14: 添加职业特定的AiObjectContext头文件
#include "Ai/Class/Dk/DKAiObjectContext.h"
#include "Ai/Class/Druid/DruidAiObjectContext.h"
#include "Ai/Class/Hunter/HunterAiObjectContext.h"
#include "Ai/Class/Mage/MageAiObjectContext.h"
#include "Ai/Class/Paladin/PaladinAiObjectContext.h"
#include "Ai/Class/Priest/PriestAiObjectContext.h"
#include "Ai/Class/Rogue/RogueAiObjectContext.h"
#include "Ai/Class/Shaman/ShamanAiObjectContext.h"
#include "Ai/Class/Warlock/WarlockAiObjectContext.h"
#include "Ai/Class/Warrior/WarriorAiObjectContext.h"
//End By leewheel

// BuildShared*Contexts 实现在独立的 BuildShared*Contexts.cpp 文件中

SharedNamedObjectContextList<Strategy> AiObjectContext::sharedStrategyContexts;
SharedNamedObjectContextList<Action> AiObjectContext::sharedActionContexts;
SharedNamedObjectContextList<Trigger> AiObjectContext::sharedTriggerContexts;
SharedNamedObjectContextList<UntypedValue> AiObjectContext::sharedValueContexts;

AiObjectContext::AiObjectContext(PlayerbotAI* botAI, SharedNamedObjectContextList<Strategy>& sharedStrategyContext,
                                 SharedNamedObjectContextList<Action>& sharedActionContext,
                                 SharedNamedObjectContextList<Trigger>& sharedTriggerContext,
                                 SharedNamedObjectContextList<UntypedValue>& sharedValueContext)
    : PlayerbotAIAware(botAI),
      strategyContexts(sharedStrategyContext),
      actionContexts(sharedActionContext),
      triggerContexts(sharedTriggerContext),
      valueContexts(sharedValueContext)
{
}

void AiObjectContext::BuildAllSharedContexts()
{
    AiObjectContext::BuildSharedContexts();
    //By leewheel 2026-07-14: 调用所有职业特定的BuildSharedContexts初始化策略上下文
    //职业特定的策略（如"new rpg"）需要在各自的上下文中初始化
    WarriorAiObjectContext::BuildSharedContexts();
    MageAiObjectContext::BuildSharedContexts();
    PriestAiObjectContext::BuildSharedContexts();
    WarlockAiObjectContext::BuildSharedContexts();
    PaladinAiObjectContext::BuildSharedContexts();
    HunterAiObjectContext::BuildSharedContexts();
    RogueAiObjectContext::BuildSharedContexts();
    ShamanAiObjectContext::BuildSharedContexts();
    DruidAiObjectContext::BuildSharedContexts();
    DKAiObjectContext::BuildSharedContexts();
    //End By leewheel
}

void AiObjectContext::BuildSharedContexts()
{
    BuildSharedStrategyContexts(sharedStrategyContexts);
    BuildSharedActionContexts(sharedActionContexts);
    BuildSharedTriggerContexts(sharedTriggerContexts);
    BuildSharedValueContexts(sharedValueContexts);
}

std::vector<std::string> AiObjectContext::Save()
{
    std::vector<std::string> result;

    std::set<std::string> names = valueContexts.GetCreated();
    for (std::set<std::string>::iterator i = names.begin(); i != names.end(); ++i)
    {
        UntypedValue* value = GetUntypedValue(*i);
        if (!value)
            continue;

        std::string const data = value->Save();
        if (data == "?")
            continue;

        std::string const name = *i;
        std::ostringstream out;
        out << name;

        out << ">" << data;
        result.push_back(out.str());
    }

    return result;
}

void AiObjectContext::Load(std::vector<std::string> data)
{
    for (std::vector<std::string>::iterator i = data.begin(); i != data.end(); ++i)
    {
        std::string const row = *i;
        std::vector<std::string> parts = split(row, '>');
        if (parts.size() != 2)
            continue;

        std::string const name = parts[0];
        std::string const text = parts[1];

        UntypedValue* value = GetUntypedValue(name);
        if (!value)
            continue;

        value->Load(text);
    }
}

Strategy* AiObjectContext::GetStrategy(std::string const name)
{
    return strategyContexts.GetContextObject(name, botAI);
}

std::set<std::string> AiObjectContext::GetSiblingStrategy(std::string const name)
{
    return strategyContexts.GetSiblings(name);
}

Trigger* AiObjectContext::GetTrigger(std::string const name) { return triggerContexts.GetContextObject(name, botAI); }

Action* AiObjectContext::GetAction(std::string const name) { return actionContexts.GetContextObject(name, botAI); }

UntypedValue* AiObjectContext::GetUntypedValue(std::string const name)
{
    return valueContexts.GetContextObject(name, botAI);
}

std::set<std::string> AiObjectContext::GetValues() { return valueContexts.GetCreated(); }

std::set<std::string> AiObjectContext::GetSupportedStrategies() { return strategyContexts.supports(); }

std::set<std::string> AiObjectContext::GetSupportedActions() { return actionContexts.supports(); }

std::string const AiObjectContext::FormatValues()
{
    std::ostringstream out;
    std::set<std::string> names = valueContexts.GetCreated();
    for (std::set<std::string>::iterator i = names.begin(); i != names.end(); ++i, out << "|")
    {
        UntypedValue* value = GetUntypedValue(*i);
        if (!value)
            continue;

        std::string const text = value->Format();
        if (text == "?")
            continue;

        out << "{" << *i << "=" << text << "}";
    }

    return out.str();
}
