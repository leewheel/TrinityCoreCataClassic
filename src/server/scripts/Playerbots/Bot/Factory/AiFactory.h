/*
 * AI工厂
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 创建AI上下文和引擎实例
 */

#ifndef PLAYERBOTS_AIFACTORY_H
#define PLAYERBOTS_AIFACTORY_H

#include <map>
//By leewheel 20260709: 包含PlayerbotAI.h以获取BotRoles枚举定义
#include "PlayerbotAI.h"
//End By leewheel
#include "Common.h"

class AiObjectContext;
class Engine;
class Player;
class PlayerbotAI;

class AiFactory
{
public:
    static AiObjectContext* createAiObjectContext(Player* player, PlayerbotAI* botAI);
    static Engine* createCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* aiObjectContext);
    static Engine* createNonCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* aiObjectContext);
    static Engine* createDeadEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* aiObjectContext);
    static void AddDefaultNonCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* nonCombatEngine);
    static void AddDefaultDeadStrategies(Player* player, PlayerbotAI* const facade, Engine* deadEngine);
    static void AddDefaultCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* engine);

    static uint8 GetPlayerSpecTab(Player* player);
    static std::map<uint8, uint32> GetPlayerSpecTabs(Player* player);
    //By leewheel 20260709: TC的GetPlayerRoles返回BotRoles(uint8)而非uint32
static BotRoles GetPlayerRoles(Player* player);
//End By leewheel
    static std::string GetPlayerSpecName(Player* player);
};

#endif
