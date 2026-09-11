/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PlayerbotSecurity.h"

#include "LFGMgr.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

PlayerbotSecurity::PlayerbotSecurity(Player* const bot) : bot(bot)
{
    if (bot)
        account = sCharacterCache->GetCharacterAccountIdByGuid(bot->GetGUID());
}

PlayerbotSecurityLevel PlayerbotSecurity::LevelFor(Player* from, DenyReason* reason, bool ignoreGroup)
{
    // Basic pointer validity checks
    if (!bot || !from || !from->GetSession())
    {
        if (reason)
            *reason = PLAYERBOT_DENY_NONE;

        return PLAYERBOT_SECURITY_DENY_ALL;
    }

    // GMs always have full access
    if (from->CanBeGameMaster())
        return PLAYERBOT_SECURITY_ALLOW_ALL;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        if (reason)
            *reason = PLAYERBOT_DENY_NONE;

        return PLAYERBOT_SECURITY_DENY_ALL;
    }

    if (botAI->IsOpposing(from))
    {
        if (reason)
            *reason = PLAYERBOT_DENY_OPPOSING;

        return PLAYERBOT_SECURITY_DENY_ALL;
    }

    if (sPlayerbotAIConfig.IsInRandomAccountList(account))
    {
        // (duplicate check in case of faction change)
        if (botAI->IsOpposing(from))
        {
            if (reason)
                *reason = PLAYERBOT_DENY_OPPOSING;

            return PLAYERBOT_SECURITY_DENY_ALL;
        }

        Group* fromGroup = from->GetGroup();
        Group* botGroup = bot->GetGroup();

        if (fromGroup && botGroup && fromGroup == botGroup && !ignoreGroup)
        {
            if (botAI->GetMaster() == from)
                return PLAYERBOT_SECURITY_ALLOW_ALL;

            //By leewheel 2026-07-24: 同组随机bot安全等级从TALK提升到INVITE
            //原TALK(1)导致HandleCommand第一道检查(需INVITE=2)静默失败，/p 集合等命令完全无响应
            if (reason)
                *reason = PLAYERBOT_DENY_NOT_YOURS;

            return PLAYERBOT_SECURITY_INVITE;
            //End By leewheel
        }

        if (sPlayerbotAIConfig.groupInvitationPermission <= 0)
        {
            if (reason)
                *reason = PLAYERBOT_DENY_NONE;

            return PLAYERBOT_SECURITY_TALK;
        }

        if (sPlayerbotAIConfig.groupInvitationPermission <= 1)
        {
            int32 levelDiff = int32(bot->GetLevel()) - int32(from->GetLevel());
            if (levelDiff > 5)
            {
                if (!bot->GetGuildId() || bot->GetGuildId() != from->GetGuildId())
                {
                    if (reason)
                        *reason = PLAYERBOT_DENY_LOW_LEVEL;

                    return PLAYERBOT_SECURITY_TALK;
                }
            }
        }

        int32 botGS = static_cast<int32>(botAI->GetEquipGearScore(bot));
        int32 fromGS = static_cast<int32>(botAI->GetEquipGearScore(from));

        if (sPlayerbotAIConfig.gearscorecheck && botGS && bot->GetLevel() > 15 && botGS > fromGS)
        {
            uint32 diffPct = uint32(100 * (botGS - fromGS) / botGS);
            uint32 reqPct = uint32(12 * sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) / from->GetLevel());

            if (diffPct >= reqPct)
            {
                if (reason)
                    *reason = PLAYERBOT_DENY_GEARSCORE;

                return PLAYERBOT_SECURITY_TALK;
            }
        }

        if (bot->InBattlegroundQueue())
        {
            if (!bot->GetGuildId() || bot->GetGuildId() != from->GetGuildId())
            {
                if (reason)
                    *reason = PLAYERBOT_DENY_BG;

                return PLAYERBOT_SECURITY_TALK;
            }
        }

        // If the bot is not in the group, we offer an invite
        botGroup = bot->GetGroup();
        if (!botGroup)
        {
            if (reason)
                *reason = PLAYERBOT_DENY_INVITE;

            return PLAYERBOT_SECURITY_INVITE;
        }

        if (!ignoreGroup && botGroup->IsFull())
        {
            if (reason)
                *reason = PLAYERBOT_DENY_FULL_GROUP;

            return PLAYERBOT_SECURITY_TALK;
        }

        if (!ignoreGroup && botGroup->GetLeaderGUID() != bot->GetGUID())
        {
            if (reason)
                *reason = PLAYERBOT_DENY_NOT_LEADER;

            return PLAYERBOT_SECURITY_TALK;
        }

        // The bot is the group leader, you can invite the initiator
        if (reason)
            *reason = PLAYERBOT_DENY_IS_LEADER;

        return PLAYERBOT_SECURITY_INVITE;
    }

    // Non-random bots: only their master has full access
    if (botAI->GetMaster() == from)
        return PLAYERBOT_SECURITY_ALLOW_ALL;

    if (reason)
        *reason = PLAYERBOT_DENY_NOT_YOURS;

    return PLAYERBOT_SECURITY_INVITE;
}

bool PlayerbotSecurity::CheckLevelFor(PlayerbotSecurityLevel level, bool silent, Player* from, bool ignoreGroup)
{
    // If something is wrong with the pointers, we silently refuse
    if (!bot || !from || !from->GetSession())
        return false;

    DenyReason reason = PLAYERBOT_DENY_NONE;
    PlayerbotSecurityLevel realLevel = LevelFor(from, &reason, ignoreGroup);

    if (realLevel >= level || from == bot)
        return true;

    PlayerbotAI* fromBotAI = GET_PLAYERBOT_AI(from);
    if (silent || (fromBotAI && !fromBotAI->IsRealPlayer()))
        return false;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return false;

    Player* master = botAI->GetMaster();
    if (master && botAI->IsOpposing(master))
        if (master->GetSession() && !master->CanBeGameMaster())
            return false;

    std::ostringstream out;

    switch (realLevel)
    {
        case PLAYERBOT_SECURITY_DENY_ALL:
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "我现在有点忙";
            //End By leewheel
            break;
        case PLAYERBOT_SECURITY_TALK:
            switch (reason)
            {
                case PLAYERBOT_DENY_NONE:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "稍后再做";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_LOW_LEVEL:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "你的等级太低了: |cffff0000" << uint32(from->GetLevel()) << "|cffffffff/|cff00ff00"
                        << uint32(bot->GetLevel());
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_GEARSCORE:
                {
                    int botGS = int(botAI->GetEquipGearScore(bot));
                    int fromGS = int(botAI->GetEquipGearScore(from));
                    int diff = (100 * (botGS - fromGS) / botGS);
                    int req = 12 * sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) / from->GetLevel();

                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "你的装备评分太低了: |cffff0000" << fromGS << "|cffffffff/|cff00ff00" << botGS
                        << " |cffff0000" << diff << "%|cffffffff/|cff00ff00" << req << "%";
                    //End By leewheel
                    break;
                }
                case PLAYERBOT_DENY_NOT_YOURS:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我已经有主人了";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_IS_BOT:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "你也是个机器人";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_OPPOSING:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "你是敌对阵营";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_DEAD:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我已经死了，稍后再做";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_INVITE:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "先邀请我进你的队伍";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_FAR:
                {
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "你得走近点才能邀请我进队。我现在在 ";
                    //End By leewheel
                    if (AreaTableEntry const* entry = sAreaTableStore.LookupEntry(bot->GetAreaId()))
                        //By leewheel 2026-09-09: TC-Cata中AreaTableEntry字段名是AreaName，类型是LocalizedString
                    out << " |cffffffff(|cffff0000" << entry->AreaName[LOCALE_enUS] << "|cffffffff)";
                    break;
                }
                case PLAYERBOT_DENY_FULL_GROUP:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我的队伍已满，稍后再做";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_IS_LEADER:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我正在带领队伍，如果需要我可以邀请你。";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_NOT_LEADER:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    if (Player* leader = botAI->GetGroupLeader())
                        out << "我正在和 " << leader->GetName() << " 组队。你可以找他邀请你。";
                    else
                        out << "我正在和别人组队。你可以找他邀请你。";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_BG:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我正在排战场队列，稍后再做";
                    //End By leewheel
                    break;
                case PLAYERBOT_DENY_LFG:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我正在排副本队列，稍后再做";
                    //End By leewheel
                    break;
                default:
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << "我做不到这件事";
                    //End By leewheel
                    break;
            }
            break;
        case PLAYERBOT_SECURITY_INVITE:
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "先邀请我进你的队伍";
            //End By leewheel
            break;
        default:
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "我做不到这件事";
            //End By leewheel
            break;
    }

    std::string const text = out.str();
    ObjectGuid guid = from->GetGUID();
    time_t lastSaid = whispers[guid][text];

    if (!lastSaid || (time(nullptr) - lastSaid) >= sPlayerbotAIConfig.repeatDelay / 1000)
    {
        whispers[guid][text] = time(nullptr);

        // Additional protection against crashes during logout
        if (bot->IsInWorld() && from->IsInWorld())
            bot->Whisper(text, LANG_UNIVERSAL, from);
    }

    return false;
}
