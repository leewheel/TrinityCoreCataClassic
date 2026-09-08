/*
 * AI 引擎实现
 * �?AzerothCore mod-playerbots 移植�?TrinityCore
 * 适配: LOG_ERROR -> TC_LOG_ERROR, LOG_DEBUG -> TC_LOG_DEBUG
 */

#include "Engine.h"

#include "Action.h"
#include "AiObjectContext.h"
#include "ByteBuffer.h" // By leewheel 2026-08-03: try-catch捕获ByteBufferException
#include "Event.h"
#include "PerfMonitor.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Queue.h"
#include "Strategy.h"
#include "Timer.h"
#include <cstdarg>
#include "Log.h"

Engine::Engine(PlayerbotAI* botAI, AiObjectContext* factory) : PlayerbotAIAware(botAI), aiObjectContext(factory)
{
    lastRelevance = 0.0f;
    testMode = false;
}

bool ActionExecutionListeners::Before(Action* action, Event event)
{
    bool result = true;
    for (std::list<ActionExecutionListener*>::iterator i = listeners.begin(); i != listeners.end(); i++)
    {
        result &= (*i)->Before(action, event);
    }

    return result;
}

void ActionExecutionListeners::After(Action* action, bool executed, Event event)
{
    for (std::list<ActionExecutionListener*>::iterator i = listeners.begin(); i != listeners.end(); i++)
    {
        (*i)->After(action, executed, event);
    }
}

bool ActionExecutionListeners::OverrideResult(Action* action, bool executed, Event event)
{
    bool result = executed;
    for (std::list<ActionExecutionListener*>::iterator i = listeners.begin(); i != listeners.end(); i++)
    {
        result = (*i)->OverrideResult(action, result, event);
    }

    return result;
}

bool ActionExecutionListeners::AllowExecution(Action* action, Event event)
{
    bool result = true;
    for (std::list<ActionExecutionListener*>::iterator i = listeners.begin(); i != listeners.end(); i++)
    {
        result &= (*i)->AllowExecution(action, event);
    }

    return result;
}

ActionExecutionListeners::~ActionExecutionListeners()
{
    for (std::list<ActionExecutionListener*>::iterator i = listeners.begin(); i != listeners.end(); i++)
    {
        delete *i;
    }

    listeners.clear();
}

Engine::~Engine(void)
{
    Reset();
    strategies.clear();
}

void Engine::Reset()
{
    strategyTypeMask = 0;

    ActionNode* action = nullptr;

    while ((action = queue.Pop()) != nullptr)
    {
        delete action;
    }

    for (TriggerNode* trigger : triggers)
    {
        delete trigger;
    }

    triggers.clear();

    for (Multiplier* multiplier : multipliers)
    {
        delete multiplier;
    }

    multipliers.clear();

    actionNodeFactories.creators.clear();
}

void Engine::Init()
{
    Reset();

    //By leewheel 2026-07-26: 重置并重新聚合目标排除标志。
    hasTargetExclusions = false;
    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); i++)
    {
        Strategy* strategy = i->second;
        strategyTypeMask |= strategy->GetType();
        hasTargetExclusions |= strategy->HasTargetExclusions();
        //End By leewheel
        strategy->InitMultipliers(multipliers);
        strategy->InitTriggers(triggers);
        for (auto &iter : strategy->actionNodeFactories.creators)
        {
            actionNodeFactories.creators[iter.first] = iter.second;
        }
    }

    if (testMode)
    {
        FILE* file = fopen("test.log", "w");
        fprintf(file, "\n");
        fclose(file);
    }
}

bool Engine::DoNextAction(Unit* /*unit*/, uint32 /*depth*/, bool minimal)
{
    LogAction("--- AI Tick ---");

    if (sPlayerbotAIConfig.logValuesPerTick)
        LogValues();

    bool actionExecuted = false;
    ActionBasket* basket = nullptr;
    time_t currentTime = time(nullptr);

    if (!minimal)
        botAI->forceRebuff.RollBuffPendingCycle();

    // Update triggers and push default actions
    ProcessTriggers(minimal);
    PushDefaultActions();

    uint32 iterations = 0;
    uint32 iterationsPerTick = queue.Size() * (minimal ? 2 : sPlayerbotAIConfig.iterationsPerTick);

    //By leewheel 2026-07-17: 引擎队列状态日志已禁用（刷屏严重）
    //End By leewheel

    while (++iterations <= iterationsPerTick)
    {
        basket = queue.Peek();
        if (!basket)
            break;

        float relevance = basket->getRelevance();
        bool skipPrerequisites = basket->isSkipPrerequisites();

        if (minimal && (relevance < 100))
            continue;

        Event event = basket->getEvent();
        ActionNode* actionNode = queue.Pop();
        Action* action = InitializeAction(actionNode);

        if (!action)
        {
            LogAction("A:%s - 未知", actionNode->getName().c_str());
            //By leewheel 2026-07-14: 诊断日志 - 动作初始化失�?

            //End By leewheel
        }
        else if (action->isUseful())
        {
            for (Multiplier* multiplier : multipliers)
            {
                relevance *= multiplier->GetValue(action);
                action->setRelevance(relevance);

                if (relevance <= 0)
                {
                    LogAction("倍率�?%s 使动�?%s 无用", multiplier->getName().c_str(), action->getName().c_str());
                    break;
                }
            }

            if (action->isPossible() && relevance > 0)
            {
                if (!skipPrerequisites)
                {
                    LogAction("A:%s - 前置条件", action->getName().c_str());

                    if (MultiplyAndPush(actionNode->getPrerequisites(), relevance + 0.002f, false, event, "前置"))
                    {
                        PushAgain(actionNode, relevance + 0.001f, event);
                        continue;
                    }
                }

                //By leewheel 2026-07-17: 动作结果日志已禁用（刷屏严重�?                //End By leewheel
                PerfMonitorOperation* pmo = sPerfMonitor.start(PERF_MON_ACTION, action->getName(), &aiObjectContext->performanceStack);
                actionExecuted = ListenAndExecute(action, event);
                if (pmo)
                    pmo->finish();

                if (actionExecuted)
                {
                    LogAction("A:%s - 成功", action->getName().c_str());
                    //By leewheel 2026-07-17: 成功日志已禁�?                    //End By leewheel
                    MultiplyAndPush(actionNode->getContinuers(), relevance, false, event, "后续");
                    lastRelevance = relevance;
                    delete actionNode;
                    break;
                }
                else
                {
                    LogAction("A:%s - 失败", action->getName().c_str());
                    //By leewheel 2026-07-17: 失败日志已禁用
                    //End By leewheel
                    MultiplyAndPush(actionNode->getAlternatives(), relevance + 0.003f, false, event, "替代");
                }
            }
            else
            {
                LogAction("A:%s - 不可能", action->getName().c_str());
                //By leewheel 2026-07-17: 不可能日志已禁用
                //End By leewheel
                MultiplyAndPush(actionNode->getAlternatives(), relevance + 0.003f, false, event, "替代");
            }
        }
        else
        {
            LogAction("A:%s - 无用", action->getName().c_str());
            //By leewheel 2026-07-17: 无用日志已禁用
            //End By leewheel
            lastRelevance = relevance;
        }

        delete actionNode;
    }

    if (time(nullptr) - currentTime > 1)
    {
        LogAction("执行时间超过1秒");
    }

    //By leewheel 2026-07-17: 没有执行任何动作的日志已禁用
    //End By leewheel

    queue.RemoveExpired();

    return actionExecuted;
}

ActionNode* Engine::CreateActionNode(std::string const name)
{
    ActionNode* node = actionNodeFactories.GetContextObject(name, botAI);
    if (node)
        return node;

    return new ActionNode(name, {}, {}, {});
}

bool Engine::MultiplyAndPush(
    std::vector<NextAction> actions,
    float forceRelevance,
    bool skipPrerequisites,
    Event event,
    char const* pushType
)
{
    bool pushed = false;

    for (NextAction nextAction : actions)
    {
        ActionNode* action = this->CreateActionNode(nextAction.getName());

        this->InitializeAction(action);

        float k = nextAction.getRelevance();

        if (forceRelevance > 0.0f)
        {
            k = forceRelevance;
        }

        if (k > 0)
        {
            this->LogAction("PUSH:%s - %f (%s)", action->getName().c_str(), k, pushType);
            queue.Push(new ActionBasket(action, k, skipPrerequisites, event));
            pushed = true;

            continue;
        }

        delete action;

    }

    return pushed;
}

ActionResult Engine::ExecuteAction(std::string const name, Event event, std::string const qualifier)
{
    bool result = false;

    ActionNode* actionNode = CreateActionNode(name);
    if (!actionNode)
        return ACTION_RESULT_UNKNOWN;

    Action* action = InitializeAction(actionNode);
    if (!action)
    {
        delete actionNode;
        return ACTION_RESULT_UNKNOWN;
    }

    if (!qualifier.empty())
    {
        if (Qualified* q = dynamic_cast<Qualified*>(action))
            q->Qualify(qualifier);
    }

    if (!action->isUseful())
    {
        delete actionNode;
        return ACTION_RESULT_USELESS;
    }

    if (!action->isPossible())
    {
        delete actionNode;
        return ACTION_RESULT_IMPOSSIBLE;
    }

    action->MakeVerbose();

    result = ListenAndExecute(action, event);
    MultiplyAndPush(action->getContinuers(), 0.0f, false, event, "默认");

    delete actionNode;

    return result ? ACTION_RESULT_OK : ACTION_RESULT_FAILED;
}

void Engine::addStrategy(std::string const name, bool init)
{
    removeStrategy(name, init);

    if (Strategy* strategy = aiObjectContext->GetStrategy(name))
    {
        std::set<std::string> siblings = aiObjectContext->GetSiblingStrategy(name);
        for (std::set<std::string>::iterator i = siblings.begin(); i != siblings.end(); i++)
            removeStrategy(*i, init);

        //By leewheel 2026-07-17: addStrategy日志已禁用（刷屏严重�?
        //End By leewheel
        LogAction("S:+%s", strategy->getName().c_str());
        strategies[strategy->getName()] = strategy;
    }
    if (init)
        Init();
}

void Engine::addStrategies(std::string first, ...)
{
    addStrategy(first, false);

    va_list vl;
    va_start(vl, first);

    const char* cur;
    do
    {
        cur = va_arg(vl, const char*);
        if (cur)
            addStrategy(cur, false);
    } while (cur);

    Init();

    va_end(vl);
}

void Engine::addStrategiesNoInit(std::string first, ...)
{
    addStrategy(first, false);

    va_list vl;
    va_start(vl, first);

    const char* cur;
    do
    {
        cur = va_arg(vl, const char*);
        if (cur)
            addStrategy(cur, false);
    } while (cur);

    va_end(vl);
}

bool Engine::removeStrategy(std::string const name, bool init)
{
    std::map<std::string, Strategy*>::iterator i = strategies.find(name);
    if (i == strategies.end())
        return false;

    LogAction("S:-%s", name.c_str());
    strategies.erase(i);
    if (init)
        Init();

    return true;
}

void Engine::removeAllStrategies()
{
    strategies.clear();
    Init();
}

void Engine::toggleStrategy(std::string const name)
{
    if (!removeStrategy(name))
        addStrategy(name);
}

bool Engine::HasStrategy(std::string const name) { return strategies.find(name) != strategies.end(); }

Strategy* Engine::GetStrategy(std::string const name)
{
    std::map<std::string, Strategy*>::iterator i = strategies.find(name);
    return i != strategies.end() ? i->second : nullptr;
}

void Engine::ProcessTriggers(bool minimal)
{
    std::unordered_map<Trigger*, Event> fires;
    uint32 now = getMSTime();
    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); i++)
    {
        TriggerNode* node = *i;
        if (!node)
            continue;

        Trigger* trigger = node->getTrigger();
        if (!trigger)
        {
            trigger = aiObjectContext->GetTrigger(node->getName());
            node->setTrigger(trigger);
        }

        if (!trigger)
            continue;

        if (fires.find(trigger) != fires.end())
            continue;

        if (testMode || trigger->needCheck(now))
        {
            if (minimal && node->getFirstRelevance() < 100)
                continue;

            PerfMonitorOperation* pmo =
                sPerfMonitor.start(PERF_MON_TRIGGER, trigger->getName(), &aiObjectContext->performanceStack);
            Event event = trigger->Check();
            if (pmo)
                pmo->finish();

            if (!event)
                continue;

            if (trigger->IsBuffTrigger() && !trigger->IsDebuffTrigger())
                botAI->forceRebuff.NoteBuffProposed();

            fires[trigger] = event;
            LogAction("T:%s", trigger->getName().c_str());
        }
    }

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); i++)
    {
        TriggerNode* node = *i;
        Trigger* trigger = node->getTrigger();
        if (fires.find(trigger) == fires.end())
            continue;

        Event event = fires[trigger];
        MultiplyAndPush(node->getHandlers(), 0.0f, false, event, "触发");
    }

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); i++)
    {
        if (Trigger* trigger = (*i)->getTrigger())
            trigger->Reset();
    }
}

void Engine::PushDefaultActions()
{
    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); i++)
    {
        Strategy* strategy = i->second;
        Event emptyEvent;
        MultiplyAndPush(strategy->getDefaultActions(), 0.0f, false, emptyEvent, "默认");
    }
}

std::string const Engine::ListStrategies()
{
    std::string s = "策略: ";

    if (strategies.empty())
        return s;

    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); i++)
    {
        s.append(i->first);
        s.append(", ");
    }

    return s.substr(0, s.length() - 2);
}

std::vector<std::string> Engine::GetStrategies()
{
    std::vector<std::string> result;
    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); i++)
    {
        result.push_back(i->first);
    }

    return result;
}

void Engine::PushAgain(ActionNode* actionNode, float relevance, Event event)
{
    std::vector<NextAction> nextAction = { NextAction(actionNode->getName(), relevance) };

    MultiplyAndPush(nextAction, relevance, true, event, "重推");

    delete actionNode;
}

bool Engine::ContainsStrategy(StrategyType type)
{
    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); i++)
    {
        if (i->second->GetType() & type)
            return true;
    }
    return false;
}

Action* Engine::InitializeAction(ActionNode* actionNode)
{
    Action* action = actionNode->getAction();
    if (!action)
    {
        action = aiObjectContext->GetAction(actionNode->getName());
        actionNode->setAction(action);
    }

    return action;
}

bool Engine::ListenAndExecute(Action* action, Event event)
{
    bool actionExecuted = false;

    if (action == nullptr)
    {
        TC_LOG_ERROR("playerbots", "动作为空指针");

        return actionExecuted;
    }

    if (actionExecutionListeners.Before(action, event))
    {
        //By leewheel 2026-08-03: 添加try-catch系统保护——客户端包解析(如PartyUninvite/DFTeleport等)
        //可能因异常/伪造包抛ByteBuffer异常，若无保护将直接abort导致worldserver崩溃(玩家崩溃日志)
        try
        {
            actionExecuted = actionExecutionListeners.AllowExecution(action, event) ? action->Execute(event) : true;
        }
        catch (ByteBufferException const& e)
        {
            TC_LOG_ERROR("playerbots", "[Engine] 动作 {} 执行时字节缓冲解析异常: {}", action->getName(), e.what());
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("playerbots", "[Engine] 动作 {} 执行时发生异常: {}", action->getName(), e.what());
        }
        //End By leewheel
    }

    if (botAI->HasStrategy("debug", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        out << "do: ";
        out << action->getName();

        if (actionExecuted)
            out << " 1 (";
        else
            out << " 0 (";

        out << action->getRelevance() << ")";

        if (!event.GetSource().empty())
            out << " [" << event.GetSource() << "]";

        botAI->TellMasterNoFacing(out);
    }

    actionExecuted = actionExecutionListeners.OverrideResult(action, actionExecuted, event);
    actionExecutionListeners.After(action, actionExecuted, event);
    return actionExecuted;
}

void Engine::LogAction(char const* format, ...)
{
    Player* bot = botAI->GetBot();
    if (sPlayerbotAIConfig.logInGroupOnly && (!bot->GetGroup() || !botAI->HasRealPlayerMaster()) && !testMode)
        return;

    char buf[1024];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    lastAction += "|";
    lastAction += buf;
    if (lastAction.size() > 512)
    {
        lastAction = lastAction.substr(512);
        size_t pos = lastAction.find("|");
        lastAction = (pos == std::string::npos ? "" : lastAction.substr(pos));
    }

    if (testMode)
    {
        FILE* file = fopen("test.log", "a");
        fprintf(file, "'%s'", buf);
        fprintf(file, "\n");
        fclose(file);
    }
    else
    {
        // TC_LOG_DEBUG("playerbots", "{} {}", bot->GetName().c_str(), buf);
    }
}

void Engine::ChangeStrategy(std::string const names)
{
    std::vector<std::string> splitted = split(names, ',');
    for (std::vector<std::string>::iterator i = splitted.begin(); i != splitted.end(); i++)
    {
        char const* name = i->c_str();
        switch (name[0])
        {
            case '+':
                addStrategy(name + 1);
                break;
            case '-':
                removeStrategy(name + 1);
                break;
            case '~':
                toggleStrategy(name + 1);
                break;
            case '?':
                botAI->TellMaster(ListStrategies());
                break;
        }
    }
}

void Engine::LogValues()
{
    if (testMode)
        return;

    Player* bot = botAI->GetBot();
    if (sPlayerbotAIConfig.logInGroupOnly && (!bot->GetGroup() || !botAI->HasRealPlayerMaster()))
        return;

    std::string const text = botAI->GetAiObjectContext()->FormatValues();
    // TC_LOG_DEBUG("playerbots", "机器�?{} 的�? {}", bot->GetName().c_str(), text.c_str());
}
