/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BattleGroundTactics.h"
#include "Chat.h"
#include "GuildTaskMgr.h"
#include "PerfMonitor.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
//By leewheel 2026-07-10: 需要包含RBAC.h以使用rbac::RBAC_PERM_COMMAND_PLAYER_BOT
#include "RBAC.h"
//End By leewheel

using namespace Trinity::ChatCommands;

class playerbots_commandscript : public CommandScript
{
public:
    playerbots_commandscript() : CommandScript("playerbots_commandscript") {}

    //By leewheel 2026-09-08: TC-Cata返回std::span而非ChatCommandTable(数组类型不能作为函数返回值)
    std::span<ChatCommandBuilder const> GetCommands() const override
    {
//By leewheel 2026-07-10: TC使用ChatCommandTable和RBAC权限，不使用SEC_*安全等级
        static ChatCommandTable playerbotsDebugCommandTable = {
            {"bg", HandleDebugBGCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No}
        };

        static ChatCommandTable playerbotsAccountCommandTable = {
            {"setKey", HandleSetSecurityKeyCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
            {"link", HandleLinkAccountCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
            {"linkedAccounts", HandleViewLinkedAccountsCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
            {"unlink", HandleUnlinkAccountCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
        };

        static ChatCommandTable playerbotsCommandTable = {
            {"bot", HandlePlayerbotCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
            {"gtask", HandleGuildTaskCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::Yes},
            {"pmon", HandlePerfMonCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::Yes},
            {"rndbot", HandleRandomPlayerbotCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::Yes},
            {"debug", playerbotsDebugCommandTable},
            {"account", playerbotsAccountCommandTable},
        };

        static ChatCommandTable commandTable = {
            {"playerbots", playerbotsCommandTable},
            //By leewheel 2026-07-22: 添加playerbot和bot别名，对齐参考代码
            {"playerbot", playerbotsCommandTable},
            {"bot", HandlePlayerbotCommand, rbac::RBAC_PERM_COMMAND_PLAYER_BOT, Console::No},
            //End By leewheel
        };

        return commandTable;
    }

    static bool HandlePlayerbotCommand(ChatHandler* handler, char const* args)
    {
        return PlayerbotMgr::HandlePlayerbotMgrCommand(handler, args);
    }

    static bool HandleRandomPlayerbotCommand(ChatHandler* handler, char const* args)
    {
        return RandomPlayerbotMgr::HandlePlayerbotConsoleCommand(handler, args);
    }

    static bool HandleGuildTaskCommand(ChatHandler* handler, char const* args)
    {
        return GuildTaskMgr::HandleConsoleCommand(handler, args);
    }

    static bool HandlePerfMonCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!strcmp(args, "reset"))
        {
            sPerfMonitor.Reset();
            return true;
        }

        if (!strcmp(args, "tick"))
        {
            sPerfMonitor.PrintStats(true, false);
            return true;
        }

        if (!strcmp(args, "stack"))
        {
            sPerfMonitor.PrintStats(false, true);
            return true;
        }

        if (!strcmp(args, "toggle"))
        {
            sPlayerbotAIConfig.perfMonEnabled = !sPlayerbotAIConfig.perfMonEnabled;
            if (sPlayerbotAIConfig.perfMonEnabled)
                TC_LOG_INFO("playerbots", "Performance monitor enabled");
            else
                TC_LOG_INFO("playerbots", "Performance monitor disabled");
            return true;
        }

        sPerfMonitor.PrintStats();
        return true;
    }

    static bool HandleDebugBGCommand(ChatHandler* handler, char const* args)
    {
        return BGTactics::HandleConsoleCommand(handler, args);
    }

    static bool HandleSetSecurityKeyCommand(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
        {
            handler->PSendSysMessage("用法：.playerbots account setKey <安全密钥>");
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        std::string key = args;

        PlayerbotMgr* mgr = PlayerbotsMgr::instance().GetPlayerbotMgr(player);
        if (mgr)
        {
        //By leewheel 2026-08-15: 移植the-lab账号信任链——setKey已实现
        mgr->HandleSetSecurityKeyCommand(player, key);
        //End By leewheel
            return true;
        }
        else
        {
            handler->PSendSysMessage("未找到 PlayerbotMgr 实例。");
            return false;
        }
    }

    static bool HandleLinkAccountCommand(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
            return false;

        char* accountName = strtok((char*)args, " ");
        char* key = strtok(nullptr, " ");

        if (!accountName || !key)
        {
            handler->PSendSysMessage("用法：.playerbots account link <账号名> <安全密钥>");
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        PlayerbotMgr* mgr = PlayerbotsMgr::instance().GetPlayerbotMgr(player);
        if (mgr)
        {
            //By leewheel 2026-08-15: 移植the-lab账号信任链——link已实现
            mgr->HandleLinkAccountCommand(player, accountName, key);
            //End By leewheel
            return true;
        }
        else
        {
            handler->PSendSysMessage("未找到 PlayerbotMgr 实例。");
            return false;
        }
    }

    static bool HandleViewLinkedAccountsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        PlayerbotMgr* mgr = PlayerbotsMgr::instance().GetPlayerbotMgr(player);
        if (mgr)
        {
            //By leewheel 2026-08-15: 移植the-lab账号信任链——linkedAccounts已实现
            mgr->HandleViewLinkedAccountsCommand(player);
            //End By leewheel
            return true;
        }
        else
        {
            handler->PSendSysMessage("未找到 PlayerbotMgr 实例。");
            return false;
        }
    }

    static bool HandleUnlinkAccountCommand(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
            return false;

        char* accountName = strtok((char*)args, " ");
        if (!accountName)
        {
            handler->PSendSysMessage("用法：.playerbots account unlink <账号名>");
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        PlayerbotMgr* mgr = PlayerbotsMgr::instance().GetPlayerbotMgr(player);
        if (mgr)
        {
            //By leewheel 2026-08-15: 移植the-lab账号信任链——unlink已实现
            mgr->HandleUnlinkAccountCommand(player, accountName);
            //End By leewheel
            return true;
        }
        else
        {
            handler->PSendSysMessage("未找到 PlayerbotMgr 实例。");
            return false;
        }
    }
};

void AddPlayerbotsCommandscripts() { new playerbots_commandscript(); }
