/*
 * 外部事件辅助器实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "ExternalEventHelper.h"

#include "AiObjectContext.h"
#include "ChatHelper.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Trigger.h"

bool ExternalEventHelper::ParseChatCommand(std::string const command, Player* owner)
{
    if (HandleCommand(command, "", owner))
        return true;

    size_t i = std::string::npos;
    while (true)
    {
        size_t found = command.rfind(" ", i);
        if (found == std::string::npos || !found)
            break;

        std::string const name = command.substr(0, found);
        std::string const param = command.substr(found + 1);

        i = found - 1;

        if (HandleCommand(name, param, owner))
            return true;
    }

    if (!ChatHelper::parseableItem(command))
        return false;

    if (sPlayerbotAIConfig.enableAutoTradeOnItemMention)
    {
        HandleCommand("c", command, owner);
        HandleCommand("t", command, owner);
    }

    return true;
}

void ExternalEventHelper::HandlePacket(std::map<uint16, std::string>& handlers, WorldPacket const& packet,
                                       Player* owner)
{
    uint16 opcode = packet.GetOpcode();
    std::string const name = handlers[opcode];
    if (name.empty())
        return;

    Trigger* trigger = aiObjectContext->GetTrigger(name);
    if (!trigger)
        return;

    WorldPacket p(packet);
    trigger->ExternalEvent(p, owner);
}

bool ExternalEventHelper::HandleCommand(std::string const name, std::string const param, Player* owner)
{
    Trigger* trigger = aiObjectContext->GetTrigger(name);
    if (!trigger)
        return false;

    trigger->ExternalEvent(param, owner);

    return true;
}
