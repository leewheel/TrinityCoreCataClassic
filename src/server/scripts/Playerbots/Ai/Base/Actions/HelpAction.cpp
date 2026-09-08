/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "HelpAction.h"

#include "ChatActionContext.h"
#include "Event.h"
#include "AiObjectContext.h"

HelpAction::HelpAction(PlayerbotAI* botAI) : Action(botAI, "help") { chatContext = new ChatActionContext(); }

HelpAction::~HelpAction() { delete chatContext; }

bool HelpAction::Execute(Event /*event*/)
{
    TellChatCommands();
    TellStrategies();
    return true;
}

void HelpAction::TellChatCommands()
{
    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "私聊任意命令: ";
    //End By leewheel
    out << CombineSupported(chatContext->supports());
    out << ", [物品]、[任务] 或 [对象] 链接";
    botAI->TellError(out.str());
}

void HelpAction::TellStrategies()
{
    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "可用策略 (co/nc/dead 命令): ";
    //End By leewheel
    out << CombineSupported(botAI->GetAiObjectContext()->GetSupportedStrategies());
    botAI->TellError(out.str());
}

std::string const HelpAction::CombineSupported(std::set<std::string> commands)
{
    std::ostringstream out;

    for (std::set<std::string>::iterator i = commands.begin(); i != commands.end();)
    {
        out << *i;
        if (++i != commands.end())
            out << ", ";
    }

    return out.str();
}
