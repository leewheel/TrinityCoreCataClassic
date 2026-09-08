/*
 * 策略基类
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_STRATEGY_H
#define PLAYERBOTS_STRATEGY_H

#include "Action.h"
#include "Multiplier.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "PlayerbotAIAware.h"
#include "Trigger.h"

//By leewheel 2026-07-26: 移植参考项目的目标排除类型枚举，供策略在选目标时声明要排除的目标集合。
enum class TargetValueExclusionType : uint8
{
    None = 0,
    Tank,
    Dps,
    Attacker
};
//End By leewheel

enum StrategyType : uint32
{
    STRATEGY_TYPE_GENERIC = 0,
    STRATEGY_TYPE_COMBAT = 1,
    STRATEGY_TYPE_NONCOMBAT = 2,
    STRATEGY_TYPE_TANK = 4,
    STRATEGY_TYPE_DPS = 8,
    STRATEGY_TYPE_HEAL = 16,
    STRATEGY_TYPE_RANGED = 32,
    STRATEGY_TYPE_MELEE = 64
};

// 动作优先级常量
static float ACTION_IDLE = 0.0f;
static float ACTION_BG = 1.0f;
static float ACTION_DEFAULT = 5.0f;
static float ACTION_NORMAL = 10.0f;
static float ACTION_HIGH = 20.0f;
static float ACTION_MOVE = 30.0f;
static float ACTION_INTERRUPT = 40.0f;
static float ACTION_DISPEL = 50.0f;
static float ACTION_RAID = 60.0f;
static float ACTION_LIGHT_HEAL = 10.0f;
static float ACTION_MEDIUM_HEAL = 20.0f;
static float ACTION_CRITICAL_HEAL = 30.0f;
static float ACTION_EMERGENCY = 90.0f;

class Strategy : public PlayerbotAIAware
{
public:
    Strategy(PlayerbotAI* botAI);
    virtual ~Strategy() {}

    virtual std::vector<NextAction> getDefaultActions() { return {}; }
    virtual void InitTriggers([[maybe_unused]] std::vector<TriggerNode*>& triggers) {}
    virtual void InitMultipliers([[maybe_unused]] std::vector<Multiplier*>& multipliers) {}
    //By leewheel 2026-07-26: 移植目标排除接口。基类默认不排除任何目标(HasTargetExclusions返回false)，
    //因此除非有子类重写，整套机制保持休眠、行为与移植前完全一致。
    virtual void AppendTargetExclusions([[maybe_unused]] GuidSet& exclusions,
                                        [[maybe_unused]] TargetValueExclusionType type) {}
    virtual bool HasTargetExclusions() const { return false; }
    //End By leewheel
    virtual std::string const getName() = 0;
    virtual uint32 GetType() const { return STRATEGY_TYPE_GENERIC; }
    virtual ActionNode* GetAction(std::string const name);
    void Update() {}
    void Reset() {}

public:
    NamedObjectFactoryList<ActionNode> actionNodeFactories;
};

#endif // PLAYERBOTS_STRATEGY_H
