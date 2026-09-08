/*
 * 外部事件辅助器
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_EXTERNALEVENTHELPER_H
#define PLAYERBOTS_EXTERNALEVENTHELPER_H

#include <map>
#include <string>

class AiObjectContext;
class Player;
class WorldPacket;

class ExternalEventHelper
{
public:
    ExternalEventHelper(AiObjectContext* aiObjectContext) : aiObjectContext(aiObjectContext) {}

    bool ParseChatCommand(std::string const command, Player* owner = nullptr);
    void HandlePacket(std::map<uint16, std::string>& handlers, WorldPacket const& packet, Player* owner = nullptr);
    bool HandleCommand(std::string const name, std::string const param, Player* owner = nullptr);

private:
    AiObjectContext* aiObjectContext;
};

#endif // PLAYERBOTS_EXTERNALEVENTHELPER_H
