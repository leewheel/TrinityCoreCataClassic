/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "FlagAction.h"

#include "Event.h"
#include "Playerbots.h"

bool FlagAction::TellUsage()
{
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellError("用法：flag cloak/helm/pvp on/set/off/clear/toggle/?");
    //End By leewheel
    return false;
}

bool FlagAction::Execute(Event event)
{
    std::string const cmd = event.getParam();
    std::vector<std::string> ss = split(cmd, ' ');
    if (ss.size() != 2)
        return TellUsage();

    bool setFlag = (ss[1] == "set" || ss[1] == "on");
    bool clearFlag = (ss[1] == "clear" || ss[1] == "off");
    bool toggleFlag = (ss[1] == "toggle");
    if (ss[0] == "pvp")
    {
        if (setFlag)
            bot->SetPvP(true);
        else if (clearFlag)
            bot->SetPvP(false);
        else if (toggleFlag)
            bot->SetPvP(!bot->IsPvP());

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << ss[0] << " 标记为 " << chat->FormatBoolean(bot->IsPvP());
        //End By leewheel
        botAI->TellMaster(out.str());
        return true;
    }

    uint32 playerFlags;
    if (ss[0] == "cloak")
        playerFlags = PLAYER_FLAGS_HIDE_CLOAK;

    if (ss[0] == "helm")
        playerFlags = PLAYER_FLAGS_HIDE_HELM;

    if (clearFlag)
        bot->SetPlayerFlag(static_cast<PlayerFlags>(playerFlags));
    else if (setFlag)
        bot->RemovePlayerFlag(static_cast<PlayerFlags>(playerFlags));
    else if (toggleFlag && bot->HasPlayerFlag(static_cast<PlayerFlags>(playerFlags)))
        bot->RemovePlayerFlag(static_cast<PlayerFlags>(playerFlags));
    else if (toggleFlag && !bot->HasPlayerFlag(static_cast<PlayerFlags>(playerFlags)))
        bot->SetPlayerFlag(static_cast<PlayerFlags>(playerFlags));

    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << ss[0] << " 标记为 " << chat->FormatBoolean(!bot->HasPlayerFlag(static_cast<PlayerFlags>(playerFlags)));
    //End By leewheel
    botAI->TellMaster(out.str());
    return true;
}
