/*
 * PlayerbotAI 感知基类
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_PLAYERBOTAIAWARE_H
#define PLAYERBOTS_PLAYERBOTAIAWARE_H

class PlayerbotAI;

class PlayerbotAIAware
{
public:
    PlayerbotAIAware(PlayerbotAI* botAI) : botAI(botAI) {}

protected:
    PlayerbotAI* botAI;
};

#endif // PLAYERBOTS_PLAYERBOTAIAWARE_H
