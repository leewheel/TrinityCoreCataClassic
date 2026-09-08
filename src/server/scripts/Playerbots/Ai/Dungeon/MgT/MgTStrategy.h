/*
 * 魔导师平台 机器人策略
 */
//By leewheel 2026-09-05: 移植来源 AC mod-playerbots MgTStrategy.h 移植适配 TC 框架(上游 feat tbc-mgt #2663)
//业务对标: AC azerothcore-wotlk modules/mod-playerbots src/Ai/Dungeon/MgT/MgTStrategy.h
//End By leewheel

#ifndef PLAYERBOTS_MGTSTRATEGY_H
#define PLAYERBOTS_MGTSTRATEGY_H

#include "AiObjectContext.h"
#include "Multiplier.h"
#include "Strategy.h"

class TbcDungeonMagistersTerraceStrategy : public Strategy
{
public:
    TbcDungeonMagistersTerraceStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "tbc-mgt"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;

    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
};

#endif
