/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_CHATVALUE_H
#define PLAYERBOTS_CHATVALUE_H

#include "Value.h"

class PlayerbotAI;

//By leewheel 2026-07-09: TC的ChatMsg枚举基础类型是int32，不是uint32
enum ChatMsg : int32;
//End By leewheel

class ChatValue : public ManualSetValue<ChatMsg>
{
public:
    ChatValue(PlayerbotAI* botAI, std::string const name = "chat")
        : ManualSetValue<ChatMsg>(botAI, CHAT_MSG_WHISPER, name)
    {
    }
};

#endif
