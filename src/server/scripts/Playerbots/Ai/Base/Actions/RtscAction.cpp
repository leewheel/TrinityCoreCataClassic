/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RtscAction.h"

#include "Playerbots.h"
#include "RTSCValues.h"

bool RTSCAction::Execute(Event event)
{
    std::string const command = event.getParam();

    Player* master = botAI->GetMaster();

    if (!master)
        return false;

    if (command != "reset" && !master->HasSpell(RTSC_MOVE_SPELL))
    {
        master->learnSpell(RTSC_MOVE_SPELL, false);
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMasterNoFacing("RTS 控制已启用。");
        botAI->TellMasterNoFacing("已学会 Aedm（强力能量移动）技能。");
        //End By leewheel
    }
    else if (command == "reset")
    {
        if (master->HasSpell(RTSC_MOVE_SPELL))
        {
            //By leewheel 20260709: TC中Player无removeSpell，用RemoveSpell替代
            //By leewheel 2026-09-03 修复C4305警告：SPEC_MASK_ALL(int)截断为bool参数disabled，语义应为false；第三参true为learn_low_rank
            master->RemoveSpell(RTSC_MOVE_SPELL, false, true);
            //End By leewheel
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMasterNoFacing("RTS 控制技能已移除。");
            //End By leewheel
        }

        RESET_AI_VALUE(bool, "RTSC selected");
        RESET_AI_VALUE(std::string, "RTSC next spell action");

        for (auto value : botAI->GetAiObjectContext()->GetValues())
            if (value.find("RTSC saved location::") != std::string::npos)
                RESET_AI_VALUE(WorldPosition, value.c_str());

        return true;
    }

    bool selected = AI_VALUE(bool, "RTSC selected");

    if (command == "select" && !selected)
    {
        SET_AI_VALUE(bool, "RTSC selected", true);
        //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit，参数为(id, type, duration)
        master->SendPlaySpellVisualKit(5036, 0, 0);
        //End By leewheel
        return true;
    }
    else if (command == "cancel")
    {
        RESET_AI_VALUE(bool, "RTSC selected");
        RESET_AI_VALUE(std::string, "RTSC next spell action");
        if (selected)
            //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit
            master->SendPlaySpellVisualKit(6372, 0, 0);
            //End By leewheel
        return true;
    }
    else if (command == "toggle")
    {
        if (!selected)
        {
            SET_AI_VALUE(bool, "RTSC selected", true);
            //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit
            master->SendPlaySpellVisualKit(5036, 0, 0);
            //End By leewheel
        }
        else
        {
            SET_AI_VALUE(bool, "RTSC selected", false);
            //By leewheel 2026-07-10: TC使用SendPlaySpellVisualKit
            master->SendPlaySpellVisualKit(6372, 0, 0);
            //End By leewheel
        }

        return true;
    }
    else if (command.find("save here ") != std::string::npos)
    {
        std::string const locationName = command.substr(10);

        WorldPosition spellPosition(bot);
        SET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName, spellPosition);

        //By leewheel 2026-07-10: TC的SummonCreature使用Milliseconds而非float作为despawnTime
        Creature* wpCreature =
            bot->SummonCreature(15631, spellPosition.GetPositionX(), spellPosition.GetPositionY(),
                                spellPosition.GetPositionZ(), spellPosition.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN,
                                Milliseconds(2000));
        //End By leewheel
        wpCreature->SetObjectScale(0.5f);

        return true;
    }
    else if (command.find("unsave ") != std::string::npos)
    {
        std::string const locationName = command.substr(7);

        RESET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

        return true;
    }

    if (command.find("save ") != std::string::npos || command == "move")
    {
        SET_AI_VALUE(std::string, "RTSC next spell action", command);

        return true;
    }

    if (command.find("show ") != std::string::npos)
    {
        std::string const locationName = command.substr(5);
        WorldPosition spellPosition = AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

        if (spellPosition)
        {
            Creature* wpCreature =
                bot->SummonCreature(15631, spellPosition.GetPositionX(), spellPosition.GetPositionY(),
                                    spellPosition.GetPositionZ(), spellPosition.GetOrientation(),
                                    TEMPSUMMON_TIMED_DESPAWN, Milliseconds(2000));
            wpCreature->SetObjectScale(0.5f);
        }

        return true;
    }

    if (command.find("show") != std::string::npos)
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "已保存: ";
        //End By leewheel

        for (auto value : botAI->GetAiObjectContext()->GetValues())
            if (value.find("RTSC saved location::") != std::string::npos)
                if (AI_VALUE2(WorldPosition, "RTSC saved location", value.substr(21).c_str()))
                    out << value.substr(21).c_str() << ",";

        out.seekp(-1, out.cur);
        out << ".";

        botAI->TellMasterNoFacing(out);
    }

    if (command.find("go ") != std::string::npos)
    {
        std::string const locationName = command.substr(3);
        WorldPosition spellPosition = AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

        if (spellPosition)
            return MoveToSpell(spellPosition, false);

        return true;
    }
    else if (command == "last")
    {
        WorldPosition spellPosition = AI_VALUE(WorldPosition, "see spell location");
        if (spellPosition)
            return MoveToSpell(spellPosition);
    }

    return false;
}
