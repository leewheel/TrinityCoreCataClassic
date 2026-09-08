/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "ChangeChatAction.h"

#include "AiObjectContext.h"
#include "ChatHelper.h"
#include "Event.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

bool ChangeChatAction::Execute(Event event)
{
    std::string const text = event.getParam();
    ChatMsg parsed = chat->parseChat(text);
    if (parsed == CHAT_MSG_SYSTEM)
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "当前聊天方式是 " << chat->FormatChat(*context->GetValue<ChatMsg>("chat"));
        //End By leewheel
        botAI->TellMaster(out);
    }
    else
    {
        context->GetValue<ChatMsg>("chat")->Set(parsed);

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "聊天方式已设置为 " << chat->FormatChat(parsed);
        //End By leewheel
        botAI->TellMaster(out);
    }

    return true;
}
