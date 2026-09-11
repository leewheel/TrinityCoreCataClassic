/*
* Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
* and/or modify it under version 3 of the License, or (at your option), any later version.
*/

//By leewheel 2026-07-10: 添加TC需要的头文件
//By leewheel 2026-08-01: 日志清理——注释掉机器人/账户创建过程的DEBUG日志，仅保留错误与启动流程反馈
//End By leewheel

#include "RandomPlayerbotFactory.h"

//By leewheel 2026-08-23: 合并 the-lab(#2637) —— AssignBotToArenaTeamInternal 使用 std::array
#include <array>
//End By leewheel
#include <algorithm>
#include <random>
#include <unordered_set>

#include "AccountMgr.h"
//By leewheel 2026-08-23: 合并 the-lab(#2637) —— AssignBotToArenaTeam 经 PlayerbotWorldThreadProcessor 异步入队; ArenaTeamAssignOperation 定义在 PlayerbotOperations.h
#include "PlayerbotWorldThreadProcessor.h"
#include "PlayerbotOperations.h"
//End By leewheel
#include "ArenaTeamMgr.h"
#include "CharacterPackets.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "PlayerbotAI.h"
#include "RaceMgr.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "Timer.h"
#include "Log.h"
#include "World.h"
//End By leewheel

constexpr RandomPlayerbotFactory::NameRaceAndGender RandomPlayerbotFactory::CombineRaceAndGender(uint8 race,
                                                                                                uint8 gender)
{
    NameRaceAndGender baseIndex;
    switch (race)
    {
        case RACE_ORC:        baseIndex = NameRaceAndGender::OrcMale; break;
        case RACE_DWARF:      baseIndex = NameRaceAndGender::DwarfMale; break;
        case RACE_NIGHTELF:   baseIndex = NameRaceAndGender::NightelfMale; break;
        case RACE_TAUREN:     baseIndex = NameRaceAndGender::TaurenMale; break;
        case RACE_GNOME:      baseIndex = NameRaceAndGender::GnomeMale; break;
        case RACE_TROLL:      baseIndex = NameRaceAndGender::TrollMale; break;
        case RACE_BLOODELF:   baseIndex = NameRaceAndGender::BloodelfMale; break;
        case RACE_DRAENEI:    baseIndex = NameRaceAndGender::DraeneiMale; break;
        case RACE_HUMAN:
        case RACE_UNDEAD_PLAYER:
        default:
            baseIndex = NameRaceAndGender::GenericMale;
            break;
    }

    return static_cast<NameRaceAndGender>(static_cast<uint8>(baseIndex) +
                                          ((gender >= GENDER_NONE) ? static_cast<uint8>(GENDER_MALE) : gender));
}

bool RandomPlayerbotFactory::IsValidRaceClassCombination(uint8 race, uint8 cls, uint32 expansion)
{
    // skip expansion races if not playing with expansion
    if (expansion < EXPANSION_THE_BURNING_CRUSADE && (race == RACE_BLOODELF || race == RACE_DRAENEI))
        return false;

    // skip expansion classes if not playing with expansion
    if (expansion < EXPANSION_WRATH_OF_THE_LICH_KING && cls == CLASS_DEATH_KNIGHT)
        return false;

    PlayerInfo const* info = sObjectMgr->GetPlayerInfo(race, cls);
    return info != nullptr;
}

Player* RandomPlayerbotFactory::CreateRandomBot(WorldSession* session, uint8 cls, std::unordered_map<NameRaceAndGender, std::vector<std::string>>& nameCache)
{
    // TC_LOG_DEBUG("playerbots", "Creating a new random bot for class: {}", cls);

    const bool alliance = static_cast<bool>(urand(0, 1));

    std::vector<uint8> raceOptions;
    for (uint8 race = RACE_HUMAN; race < sRaceMgr->GetMaxRaces(); ++race)
    {
        // skip disabled with config races
        //By leewheel 2026-07-10: TC中RACEMASK使用GetUInt64Config，需要显式转换为WorldInt64Configs
        //By leewheel 2026-09-03 修复C4334警告：1<<(race-1)是32位int移位，与64位掩码AND时结果被隐式提升到64位；
        //race-1可达12(uint64种族)，必须用1ULL进行64位移位
        if ((1ULL << (race - 1)) & sWorld->GetUInt64Config(static_cast<WorldInt64Configs>(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK)))
        //End By leewheel
            continue;

        // Try to get 50/50 faction distribution for random bot population balance.
        // Without this check, races from the faction with more class options would dominate.
        //By leewheel 2026-07-10: 显式转换enum避免编译器错误
        if (alliance == IsAlliance(race))
        {
            if (IsValidRaceClassCombination(race, cls, sWorld->getIntConfig(CONFIG_EXPANSION)))
                raceOptions.push_back(race);
        //End By leewheel
        }
    }

    if (raceOptions.empty())
    {
        TC_LOG_ERROR("playerbots", "No races are available for class: {}", cls);
        return nullptr;
    }

    const uint8 race = raceOptions[urand(0, raceOptions.size() - 1)];
    const uint8 gender = urand(0, 1) ? GENDER_MALE : GENDER_FEMALE;
    const auto raceAndGender = CombineRaceAndGender(race, gender);

    std::string name;
    if (!nameCache.empty())
    {
        if (nameCache[raceAndGender].empty())
        {
            TC_LOG_ERROR("playerbots", "No names found for the specified race: {} and gender: {}",
                    race, gender);
            return nullptr;
        }

        //By leewheel 2026-07-12: 延迟验证 - 从缓存取名字时才做CheckPlayerName验证
        // 缓存构建时跳过了验证(TC的ValidateName做数千条正则匹配, 209K名字会卡死)
        // 这里只验证实际用到的名字, 最多尝试10次
        int nameTries = 10;
        while (nameTries-- > 0 && !nameCache[raceAndGender].empty())
        {
            uint32 i = urand(0, nameCache[raceAndGender].size() - 1);
            std::string candidate = nameCache[raceAndGender][i];
            swap(nameCache[raceAndGender][i], nameCache[raceAndGender].back());
            nameCache[raceAndGender].pop_back();

            // 验证名字是否合法(TC的CheckPlayerName包含正则匹配, 但只对单个名字很快)
            if (sObjectMgr->CheckPlayerName(candidate, sWorld->GetDefaultDbcLocale(), true) == CHAR_NAME_SUCCESS)
            {
                name = candidate;
                break;
            }
            // 验证失败, 丢弃这个名字, 继续尝试下一个
        }
        //End By leewheel

        if (name.empty() && !nameCache[raceAndGender].empty())
        {
            // 验证全部失败, 直接用最后一个未验证的名字作为兜底
            name = nameCache[raceAndGender].back();
            nameCache[raceAndGender].pop_back();
        }
    }
    else
    {
        name = CreateRandomBotName(raceAndGender);
    }

    if (name.empty())
    {
        TC_LOG_ERROR("playerbots", "Failed to get a valid random bot name");
        return nullptr;
    }

    //By leewheel 2026-07-12: TC使用Customizations系统，需要从DB2获取有效的外观选项并随机选择
    // 否则Customizations为空，GetModelForForm访问空数组会导致ACCESS_VIOLATION崩溃
    // 过滤掉bot无法满足要求的外观选项(成就/任务/物品外观)，并添加重试机制
    auto generateCustomizations = [](uint8 race, uint8 gender, uint8 cls) -> std::vector<UF::ChrCustomizationChoice>
    {
        std::vector<UF::ChrCustomizationChoice> result;

        // 检查req是否对bot不可满足(成就/任务/物品外观)
        auto isReqImpossibleForBot = [](ChrCustomizationReqEntry const* req, uint8 race, uint8 cls) -> bool
        {
            if (!req || !req->GetFlags().HasFlag(ChrCustomizationReqFlag::HasRequirements))
                return false;
            // bot没有成就、任务进度、物品外观收集
            if (req->AchievementID || req->QuestID || req->ItemModifiedAppearanceID)
                return true;
            // 检查职业
            if (req->ClassMask && !(req->ClassMask & (1 << (cls - 1))))
                return true;
            // 检查种族
            if (!req->RaceMask.IsEmpty() && req->RaceMask.RawValue != -1 && !req->RaceMask.HasRace(Races(race)))
                return true;
            return false;
        };

        if (auto const* options = sDB2Manager.GetCustomiztionOptions(race, gender))
        {
            for (ChrCustomizationOptionEntry const* option : *options)
            {
                // 过滤option级别的不可满足要求
                if (option->ChrCustomizationReqID)
                {
                    if (ChrCustomizationReqEntry const* req = sChrCustomizationReqStore.LookupEntry(option->ChrCustomizationReqID))
                    {
                        if (isReqImpossibleForBot(req, race, cls))
                            continue;
                    }
                }

                if (auto const* choices = sDB2Manager.GetCustomiztionChoices(option->ID))
                {
                    // 过滤choice级别的不可满足要求
                    std::vector<ChrCustomizationChoiceEntry const*> validChoices;
                    for (ChrCustomizationChoiceEntry const* choice : *choices)
                    {
                        if (choice->ChrCustomizationReqID)
                        {
                            if (ChrCustomizationReqEntry const* req = sChrCustomizationReqStore.LookupEntry(choice->ChrCustomizationReqID))
                            {
                                if (isReqImpossibleForBot(req, race, cls))
                                    continue;
                            }
                        }
                        validChoices.push_back(choice);
                    }

                    if (!validChoices.empty())
                    {
                        ChrCustomizationChoiceEntry const* choice = validChoices[urand(0, validChoices.size() - 1)];
                        UF::ChrCustomizationChoice playerChoice;
                        playerChoice.ChrCustomizationOptionID = option->ID;
                        playerChoice.ChrCustomizationChoiceID = choice->ID;
                        result.push_back(playerChoice);
                    }
                }
            }
        }

        return result;
    };

    // 重试机制: 如果Player::Create因ValidateAppearance失败，重新生成外观再试
    Player* player = nullptr;
    for (int attempt = 0; attempt < 5 && !player; ++attempt)
    {
        auto characterInfo = std::make_unique<WorldPackets::Character::CharacterCreateInfo>();
        characterInfo->Name = name;
        characterInfo->Race = race;
        characterInfo->Class = cls;
        characterInfo->Sex = gender;
        //By leewheel 2026-07-12: Customizations是Array<ChrCustomizationChoice, 250>类型, 不能直接赋值vector
        auto customs = generateCustomizations(race, gender, cls);
        characterInfo->Customizations.resize(customs.size());
        for (size_t i = 0; i < customs.size(); ++i)
            characterInfo->Customizations[i] = customs[i];
        //End By leewheel

        player = new Player(session);
        player->GetMotionMaster()->Initialize();
        if (!player->Create(sObjectMgr->GetGenerator<HighGuid::Player>().Generate(), characterInfo.get()))
        {
            player->CleanupsBeforeDelete();
            delete player;
            player = nullptr;

            if (attempt < 4)
            {
                // TC_LOG_DEBUG("playerbots", "CreateRandomBot attempt {} failed for name: \"{}\", race: {}, class: {}, retrying...",
                //     attempt + 1, name.c_str(), race, cls);
            }
            else
            {
                TC_LOG_ERROR("playerbots", "Unable to create random bot after 5 attempts - name: \"{}\", race: {}, class: {}",
                    name.c_str(), race, cls);
                return nullptr;
            }
        }
    }
    //End By leewheel

    player->setCinematic(2);
    player->SetAtLoginFlag(AT_LOGIN_NONE);

    if (cls == CLASS_DEATH_KNIGHT)
    {
        player->learnSpell(50977, false);
    }

    // TC_LOG_DEBUG("playerbots", "Random bot created - name: \"{}\", race: {}, class: {}",
    //By leewheel 2026-08-01: 修复历史日志清理残留——第二行参数未被注释导致C2059
    //         name.c_str(), race, cls);
    //End By leewheel

    return player;
}

std::string const RandomPlayerbotFactory::CreateRandomBotName(NameRaceAndGender raceAndGender)
{
    std::string botName = "";
    int tries = 3;
    while (--tries)
    {
        //By leewheel 2026-07-10: TC的Query不接受fmt参数，使用PQuery
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT n.name "
            "FROM playerbots_names n "
            "LEFT OUTER JOIN characters c ON c.name = n.name "
            "WHERE c.guid IS NULL and n.gender = '{}' "
            "ORDER BY RAND() LIMIT 1",
            static_cast<uint8>(raceAndGender));
        //End By leewheel
        if (!result)
        {
            break;
        }

        Field* fields = result->Fetch();
        botName = fields[0].Get<std::string>();
        //By leewheel 2026-07-10: TC的CheckPlayerName需要locale和create参数
        if (ObjectMgr::CheckPlayerName(botName, sWorld->GetDefaultDbcLocale(), true) == CHAR_NAME_SUCCESS)
        //End By leewheel
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
            stmt->SetData(0, botName);

            if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
                continue;

            return botName;
        }
    }

    // CONLANG NAME GENERATION
    TC_LOG_ERROR("playerbots", "No more names left for random bots. Attempting conlang name generation.");
    const std::string groupCategory = "SCVKRU";
    const std::string groupFormStart[2][4] = {{"SV", "SV", "VK", "RV"}, {"V", "SU", "VS", "RV"}};
    const std::string groupFormMid[2][6] = {{"CV", "CVC", "CVC", "CVK", "VC", "VK"},
                                            {"CV", "CVC", "CVK", "KVC", "VC", "KV"}};
    const std::string groupFormEnd[2][4] = {{"CV", "VC", "VK", "CV"}, {"RU", "UR", "VR", "V"}};
    const std::string groupLetter[2][6] = {
        // S           C                            V               K           R         U
        {"dtspkThfS", "bcCdfghjkmnNqqrrlsStTvwxyz", "aaeeiouA", "ppttkkbdg", "lmmnrr", "AEO"},
        {"dtskThfS", "bcCdfghjkmmnNqrrlssStTvwyz", "aaaeeiiuAAEIO", "ppttkbbdg", "lmmnrrr", "AEOy"}};
    const std::string replaceRule[2][17] = {
        {"ST", "ka", "ko", "ku", "kr", "S", "T", "C", "N", "jj", "AA", "AI", "A", "E", "O", "I", "aa"},
        {"sth", "ca", "co", "cu", "cr", "sh", "th", "ch", "ng", "dg", "A", "ayu", "ai", "ei", "ou", "iu", "ae"}};

    const auto gender = static_cast<uint8>(raceAndGender) % 2;

    tries = 10;
    while (--tries)
    {
        botName.clear();
        // Build name from groupForms
        // Pick random start group
        botName = groupFormStart[gender][rand() % 4];
        // Pick up to 2 and then up to 1 additional middle group
        for (int i = 0; i < rand() % 3 + rand() % 2; i++)
        {
            botName += groupFormMid[gender][rand() % 6];
        }
        // Pick up to 1 end group
        botName += rand() % 2 ? groupFormEnd[gender][rand() % 4] : "";
        // If name is single letter add random end group
        botName += (botName.size() < 2) ? groupFormEnd[gender][rand() % 4] : "";

        // Replace Catagory value with random Letter from that Catagory's Letter string for a given bot gender
        for (size_t i = 0; i < botName.size(); i++)
        {
            botName[i] = groupLetter[gender][groupCategory.find(botName[i])]
                                    [rand() % groupLetter[gender][groupCategory.find(botName[i])].size()];
        }

        // Itterate over replace rules
        for (int i = 0; i < 17; i++)
        {
            int j = botName.find(replaceRule[0][i]);
            while (j > -1)
            {
                botName.replace(j, replaceRule[0][i].size(), replaceRule[1][i]);
                j = botName.find(replaceRule[0][i]);
            }
        }

        // Capitalize first letter
        botName[0] -= 32;

        //By leewheel 2026-07-11: TC的CheckPlayerName需要locale和create参数
        if (ObjectMgr::CheckPlayerName(botName, sWorld->GetDefaultDbcLocale(), true) != CHAR_NAME_SUCCESS) // 检查保留名和不当用语
        //End By leewheel
        {
            botName.clear();
            continue;
        }
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
        stmt->SetData(0, botName);

        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            botName.clear();
            continue;
        }
        return botName;
    }

    // 真随机名字生成
    TC_LOG_ERROR("playerbots", "Con​lang name generation failed. True random name fallback.");
    tries = 10;
    while (--tries)
    {
        for (uint8 i = 0; i < 10; i++)
        {
            botName += (i == 0 ? 'A' : 'a') + rand() % 26;
        }
        //By leewheel 2026-07-11: TC的CheckPlayerName需要locale和create参数
        if (ObjectMgr::CheckPlayerName(botName, sWorld->GetDefaultDbcLocale(), true) != CHAR_NAME_SUCCESS)  // 检查保留名和不当用语
        //End By leewheel
        {
            botName.clear();
            continue;
        }
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
        stmt->SetData(0, botName);

        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            botName.clear();
            continue;
        }
        return botName;
    }
    TC_LOG_ERROR("playerbots", "Random name generation failed.");
    botName.clear();
    return botName;
}

// Calculates the total number of required accounts, either using the specified randomBotAccountCount
// or determining it dynamically based on MaxRandomBots, EnablePeriodicOnlineOffline and its ratio,
// and AddClassAccountPoolSize. The system also factors in the types of existing account, as assigned by
// AssignAccountTypes()
uint32 RandomPlayerbotFactory::CalculateTotalAccountCount()
{
    // Reset account types if features are disabled
    // Reset is done here to precede needed accounts calculations
    if (sPlayerbotAIConfig.maxRandomBots == 0 || sPlayerbotAIConfig.addClassAccountPoolSize == 0)
    {
        if (sPlayerbotAIConfig.maxRandomBots == 0)
        {
            PlayerbotsDatabase.Execute("UPDATE playerbots_account_type SET account_type = 0 WHERE account_type = 1");
            TC_LOG_INFO("playerbots", "MaxRandomBots set to 0, any RNDbot accounts (type 1) will be unassigned (type 0)");
        }
        if (sPlayerbotAIConfig.addClassAccountPoolSize == 0)
        {
            PlayerbotsDatabase.Execute("UPDATE playerbots_account_type SET account_type = 0 WHERE account_type = 2");
            TC_LOG_INFO("playerbots", "AddClassAccountPoolSize set to 0, any AddClass accounts (type 2) will be unassigned (type 0)");
        }

        // Wait for DB to reflect the change, up to 1 second max. This is needed to make sure other logs don't show wrong info
        for (int waited = 0; waited < 1000; waited += 50)
        {
            //By leewheel 2026-07-10: TC使用PQuery进行格式化查询
            QueryResult res = PlayerbotsDatabase.PQuery("SELECT COUNT(*) FROM playerbots_account_type WHERE account_type IN ({}, {})",
                sPlayerbotAIConfig.maxRandomBots == 0 ? 1 : -1,
                sPlayerbotAIConfig.addClassAccountPoolSize == 0 ? 2 : -1);
            //End By leewheel

            if (!res || res->Fetch()[0].Get<uint64>() == 0)
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(50));     // Extra 50ms fixed delay for safety.
        }
    }

    // Check existing account types
    uint32 existingRndBotAccounts = 0;
    uint32 existingAddClassAccounts = 0;
    uint32 existingUnassignedAccounts = 0;

    QueryResult typeCheck = PlayerbotsDatabase.Query("SELECT account_type, COUNT(*) FROM playerbots_account_type GROUP BY account_type");
    if (typeCheck)
    {
        do
        {
            Field* fields = typeCheck->Fetch();
            uint8 accountType = fields[0].Get<uint8>();
            //By leewheel 2026-08-18: 移植 brighton-chi the-lab caa28a14(COUNT(*)按uint64读取#2578),消除启动时LONGLONG字段警告
            uint32 count = static_cast<uint32>(fields[1].Get<uint64>());
            //End By leewheel

            if (accountType == 0) existingUnassignedAccounts = count;
            else if (accountType == 1) existingRndBotAccounts = count;
            else if (accountType == 2) existingAddClassAccounts = count;
        } while (typeCheck->NextRow());
    }

    // Determine divisor based on Death Knight availability and requested A&H faction ratio
    int divisor = CalculateAvailableCharsPerAccount();

    // Calculate max bots
    int maxBots = sPlayerbotAIConfig.maxRandomBots;
    // Take periodic online/offline into account
    if (sPlayerbotAIConfig.enablePeriodicOnlineOffline)
        maxBots *= sPlayerbotAIConfig.periodicOnlineOfflineRatio;

    // Calculate number of accounts needed for RNDbots
    // Result is rounded up for maxBots not cleanly divisible by the divisor
    uint32 neededRndBotAccounts = (maxBots + divisor - 1) / divisor;
    uint32 neededAddClassAccounts = sPlayerbotAIConfig.addClassAccountPoolSize;

    // Start with existing total
    uint32 existingTotal = existingRndBotAccounts + existingAddClassAccounts + existingUnassignedAccounts;

    // Calculate shortfalls after using unassigned accounts
    uint32 availableUnassigned = existingUnassignedAccounts;
    uint32 additionalAccountsNeeded = 0;

    // Check RNDbot needs
    if (neededRndBotAccounts > existingRndBotAccounts)
    {
        uint32 rndBotShortfall = neededRndBotAccounts - existingRndBotAccounts;
        if (rndBotShortfall <= availableUnassigned)
            availableUnassigned -= rndBotShortfall;
        else
        {
            additionalAccountsNeeded += (rndBotShortfall - availableUnassigned);
            availableUnassigned = 0;
        }
    }

    // Check AddClass needs
    if (neededAddClassAccounts > existingAddClassAccounts)
    {
        uint32 addClassShortfall = neededAddClassAccounts - existingAddClassAccounts;
        if (addClassShortfall <= availableUnassigned)
            availableUnassigned -= addClassShortfall;
        else
        {
            additionalAccountsNeeded += (addClassShortfall - availableUnassigned);
            availableUnassigned = 0;
        }
    }

    // Return existing total plus any additional accounts needed
    uint32 calculatedTotal = existingTotal + additionalAccountsNeeded;

    // Manually set randomBotAccountCount meets the requirements
    if (sPlayerbotAIConfig.randomBotAccountCount >= calculatedTotal)
        return sPlayerbotAIConfig.randomBotAccountCount;
    // Manually set randomBotAccountCount doesn't meet the requirements. Using calculated value
    if (sPlayerbotAIConfig.randomBotAccountCount > 0)
        LOG_WARN("playerbots", "RandomBotAccountCount ({}) is lower than the required calculated value ({}). Using the calculated value instead.",
            sPlayerbotAIConfig.randomBotAccountCount, calculatedTotal);

    return calculatedTotal;
}

uint32 RandomPlayerbotFactory::CalculateAvailableCharsPerAccount()
{
    // Death Knight availability according to their login eligibility, and if WotLK is enabled at all.
    bool noDK = sPlayerbotAIConfig.disableDeathKnightLogin || sWorld->getIntConfig(CONFIG_EXPANSION) != EXPANSION_WRATH_OF_THE_LICH_KING;

    uint32 availableChars = noDK ? 9 : 10;

    uint32 hordeRatio = sPlayerbotAIConfig.randomBotHordeRatio;
    uint32 allianceRatio = sPlayerbotAIConfig.randomBotAllianceRatio;

    // horde : alliance = 50 : 50 -> 0%
    // horde : alliance = 0 : 50 -> 50%
    // horde : alliance = 10 : 50 -> 40%
    float unavailableRatio = static_cast<float>((std::max(hordeRatio, allianceRatio) - std::min(hordeRatio, allianceRatio))) /
        (std::max(hordeRatio, allianceRatio) * 2);

    // Conservative floor to ensure enough characters (may result in more accounts than needed).
    if (unavailableRatio != 0)
        availableChars = availableChars - availableChars * unavailableRatio;

    return availableChars;
}

void RandomPlayerbotFactory::CreateRandomBots()
{
    /* multi-thread here is meaningless? since the async db operations */

    if (sPlayerbotAIConfig.deleteRandomBotAccounts)
    {
        std::vector<uint32> botAccounts;
        std::vector<uint32> botFriends;

        // Calculates the total number of required accounts.
        uint32 totalAccountCount = CalculateTotalAccountCount();

        for (uint32 accountNumber = 0; accountNumber < totalAccountCount; ++accountNumber)
        {
            std::ostringstream out;
            out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
            std::string const accountName = out.str();

            if (uint32 accountId = AccountMgr::GetId(accountName))
                botAccounts.push_back(accountId);
        }

        TC_LOG_INFO("playerbots", "Deleting all random bot characters and accounts...");

        // First execute all the cleanup SQL commands
        // Clear playerbots_random_bots and playerbots_account_type
        PlayerbotsDatabase.Execute("DELETE FROM playerbots_random_bots");
        PlayerbotsDatabase.Execute("DELETE FROM playerbots_account_type");

        // Get the database names dynamically
        std::string loginDBName = LoginDatabase.GetConnectionInfo()->database;
        std::string characterDBName = CharacterDatabase.GetConnectionInfo()->database;

        // Delete all characters from bot accounts
        //By leewheel 2026-07-10: TC的PExecute不接受运行时拼接字符串，改用Execute+字符串拼接
        CharacterDatabase.Execute(("DELETE FROM characters WHERE account IN (SELECT id FROM " + loginDBName + ".account WHERE username LIKE '" + sPlayerbotAIConfig.randomBotAccountPrefix + "%')").c_str());
        //End By leewheel

        // Wait for the characters to be deleted before proceeding to dependent deletes
        while (CharacterDatabase.QueueSize())
        {
            std::this_thread::sleep_for(1s);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Extra 100ms fixed delay for safety.

        // Clean up orphaned entries in playerbots_guild_tasks
        //By leewheel 2026-07-11: TC的Execute不接受std::string, 需要c_str()
        PlayerbotsDatabase.Execute(("DELETE FROM playerbots_guild_tasks WHERE owner NOT IN (SELECT guid FROM " + characterDBName + ".characters)").c_str());

        // Clean up orphaned entries in playerbots_db_store
        //By leewheel 2026-07-10: TC的PExecute不接受运行时拼接字符串，改用Execute+字符串拼接
        PlayerbotsDatabase.Execute(("DELETE FROM playerbots_db_store WHERE guid NOT IN (SELECT guid FROM " + characterDBName + ".characters WHERE account IN (SELECT id FROM " + loginDBName + ".account WHERE username NOT LIKE '" + sPlayerbotAIConfig.randomBotAccountPrefix + "%'))").c_str());
        //End By leewheel

        // Clean up orphaned records in character-related tables
        //By leewheel 2026-07-13: TC没有arena_team和arena_team_member表,移除相关清理
        //TC的竞技场队伍是内存中的临时对象,统计数据存储在character_arena_stats表中
        CharacterDatabase.Execute("DELETE FROM character_account_data WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_achievement WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_achievement_progress WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_action WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_arena_stats WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_aura WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_entry_point WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_glyphs WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_homebind WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_inventory WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM item_instance WHERE owner_guid NOT IN (SELECT guid FROM characters) AND owner_guid > 0");

        // Clean up pet data
        CharacterDatabase.Execute("DELETE FROM character_pet WHERE owner NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM pet_aura WHERE guid NOT IN (SELECT id FROM character_pet)");
        CharacterDatabase.Execute("DELETE FROM pet_spell WHERE guid NOT IN (SELECT id FROM character_pet)");
        CharacterDatabase.Execute("DELETE FROM pet_spell_cooldown WHERE guid NOT IN (SELECT id FROM character_pet)");

        // Clean up character data
        CharacterDatabase.Execute("DELETE FROM character_queststatus WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_queststatus_rewarded WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_reputation WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_skills WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_social WHERE friend NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_spell WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_spell_cooldown WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM character_talent WHERE guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM corpse WHERE guid NOT IN (SELECT guid FROM characters)");

        // Clean up group data
        CharacterDatabase.Execute("DELETE FROM `groups` WHERE leaderGuid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM group_member WHERE memberGuid NOT IN (SELECT guid FROM characters)");

        // Clean up mail
        CharacterDatabase.Execute("DELETE FROM mail WHERE receiver NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM mail_items WHERE receiver NOT IN (SELECT guid FROM characters)");

        // Clean up guild data
        CharacterDatabase.Execute("DELETE FROM guild WHERE leaderguid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM guild_bank_eventlog WHERE guildid NOT IN (SELECT guildid FROM guild)");
        CharacterDatabase.Execute("DELETE FROM guild_member WHERE guildid NOT IN (SELECT guildid FROM guild) OR guid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM guild_rank WHERE guildid NOT IN (SELECT guildid FROM guild)");

        // Clean up petition data
        CharacterDatabase.Execute("DELETE FROM petition WHERE ownerguid NOT IN (SELECT guid FROM characters)");
        CharacterDatabase.Execute("DELETE FROM petition_sign WHERE ownerguid NOT IN (SELECT guid FROM characters) OR playerguid NOT IN (SELECT guid FROM characters)");

        // Finally, delete the bot accounts themselves
        TC_LOG_INFO("playerbots", "Deleting random bot accounts...");
        //By leewheel 2026-07-10: TC使用PQuery进行格式化查询，fmt中%不需要转义
        QueryResult results = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '{}%'",
                                             sPlayerbotAIConfig.randomBotAccountPrefix);
        //End By leewheel
        //By leewheel 2026-09-03 修复C4189警告：deletion_count仅被注释掉的调试日志引用；按AC原版恢复实际递增计数并在删除完成后输出日志(log中文)
        int32 deletion_count = 0;
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                uint32 accId = fields[0].Get<uint32>();
                ++deletion_count;
                AccountMgr::DeleteAccount(accId);
            } while (results->NextRow());
        }
        if (deletion_count > 0)
            TC_LOG_DEBUG("playerbots", "删除随机机器人账号 {} 个", deletion_count);
        //End By leewheel

        uint32 timer = getMSTime();

        // After ALL deletions, make sure data is commited to DB
        LoginDatabase.Execute("COMMIT");
        CharacterDatabase.Execute("COMMIT");
        PlayerbotsDatabase.Execute("COMMIT");

        // Wait for all pending database operations to complete
        while (LoginDatabase.QueueSize() || CharacterDatabase.QueueSize() || PlayerbotsDatabase.QueueSize())
        {
            std::this_thread::sleep_for(1s);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Extra 100ms fixed delay for safety.

        // Flush tables to ensure all data in memory are written to disk
        LoginDatabase.Execute("FLUSH TABLES");
        CharacterDatabase.Execute("FLUSH TABLES");
        PlayerbotsDatabase.Execute("FLUSH TABLES");

        TC_LOG_INFO("playerbots", ">> Random bot accounts and data deleted in {} ms", GetMSTimeDiffToNow(timer));
        TC_LOG_INFO("playerbots", "Please reset the AiPlayerbot.DeleteRandomBotAccounts to 0 and restart the server...");
        World::StopNow(SHUTDOWN_EXIT_CODE);
        return;
    }

    TC_LOG_INFO("server.loading", "  -> 开始创建随机机器人账号...");
    std::unordered_map<NameRaceAndGender, std::vector<std::string>> nameCache;
    std::vector<std::future<void>> account_creations;
    int account_creation = 0;

    // Calculates the total number of required accounts.
    uint32 totalAccountCount = CalculateTotalAccountCount();
    TC_LOG_INFO("server.loading", "  -> 需要账号数: {}, 开始检查/创建账号...", totalAccountCount);
    uint32 timer = getMSTime();

    for (uint32 accountNumber = 0; accountNumber < totalAccountCount; ++accountNumber)
    {
        std::ostringstream out;
        out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
        std::string const accountName = out.str();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME);
        stmt->SetData(0, accountName);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (result)
        {
            continue;
        }
        account_creation++;
        std::string password = "";
        if (sPlayerbotAIConfig.randomBotRandomPassword)
        {
            for (int i = 0; i < 10; i++)
            {
                password += (char)urand('!', 'z');
            }
        }
        else
            password = accountName;

        sAccountMgr->CreateAccount(accountName, password);

        // TC_LOG_DEBUG("playerbots", "Account {} created for random bots", accountName.c_str());
    }
    if (account_creation)
    {
        TC_LOG_INFO("server.loading", "  -> 等待 {} 个新账号写入数据库...", account_creation);
        /* wait for async accounts create to make character create correctly */

        while (LoginDatabase.QueueSize())
        {
            std::this_thread::sleep_for(1s);
        }
        TC_LOG_INFO("server.loading", "  -> {} 个账号创建完成 ({} ms)", account_creation, GetMSTimeDiffToNow(timer));
    }
    else
    {
        TC_LOG_INFO("server.loading", "  -> 所有账号已存在, 跳过创建");
    }

    TC_LOG_INFO("server.loading", "  -> 开始创建角色...");
    uint32 totalRandomBotChars = 0;
    std::vector<std::pair<Player*, uint32>> playerBots;
    std::vector<WorldSession*> sessionBots;
    int bot_creation = 0;
    timer = getMSTime();
    bool nameCached = false;
    for (uint32 accountNumber = 0; accountNumber < totalAccountCount; ++accountNumber)
    {
        std::ostringstream out;
        out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
        std::string const accountName = out.str();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME);
        stmt->SetData(0, accountName);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (!result)
            continue;

        Field* fields = result->Fetch();
        uint32 accountId = fields[0].Get<uint32>();

        sPlayerbotAIConfig.randomBotAccounts.push_back(accountId);

        uint32 count = AccountMgr::GetCharactersCount(accountId);
        if (count >= 10)
        {
            continue;
        }

        if (!nameCached)
        {
            nameCached = true;
            TC_LOG_INFO("server.loading", "  -> 构建名字缓存...");
            //By leewheel 2026-07-12: 优化名字缓存 - 原代码对每个名字做同步DB查询检查是否已存在(209K次查询导致卡死)
            // 改为: 单次查询加载所有已用角色名字到内存set, 然后在内存中检查
            uint32 nameCacheTimer = getMSTime();

            // 1. 加载所有已存在的角色名字到内存set
            std::unordered_set<std::string> usedNames;
            QueryResult usedNamesResult = CharacterDatabase.Query("SELECT name FROM characters");
            if (usedNamesResult)
            {
                do
                {
                    usedNames.insert(usedNamesResult->Fetch()[0].Get<std::string>());
                } while (usedNamesResult->NextRow());
            }
            TC_LOG_INFO("playerbots", "Loaded {} existing character names in {} ms", usedNames.size(), GetMSTimeDiffToNow(nameCacheTimer));

            // 2. 加载playerbots_names表, 在内存中检查名字是否已用
            QueryResult result = CharacterDatabase.Query("SELECT name, gender FROM playerbots_names");
            if (!result)
            {
                TC_LOG_ERROR("playerbots", "No more unused names left");
                return;
            }

            uint32 validNameCount = 0;
            uint32 skippedUsedNameCount = 0;
            do
            {
                Field* fields = result->Fetch();
                std::string name = fields[0].Get<std::string>();
                NameRaceAndGender raceAndGender = static_cast<NameRaceAndGender>(fields[1].Get<uint8>());

                // 检查名字是否已被使用(内存中检查, 不做DB查询)
                if (usedNames.find(name) != usedNames.end())
                {
                    skippedUsedNameCount++;
                    continue;
                }

                //By leewheel 2026-07-12: 缓存构建时不做CheckPlayerName验证
                // TC的CheckPlayerName内部调用sDB2Manager.ValidateName, 对每个名字做数千条正则匹配
                // 209K名字 * 数千正则 = 上亿次正则操作, 导致服务器卡死数分钟
                // AC的CheckPlayerName使用简单hash查找所以很快, TC的实现不同
                // 解决方案: 缓存构建时只检查是否已被使用, 名字验证延迟到CreateRandomBot中实际使用时再做
                // 这样只需验证实际用到的几千个名字而非全部209K个
                nameCache[raceAndGender].push_back(name);
                validNameCount++;
                //End By leewheel
            } while (result->NextRow());

            TC_LOG_INFO("server.loading", "  -> 名字缓存完成: {} 有效, {} 已用 ({} ms)",
                validNameCount, skippedUsedNameCount, GetMSTimeDiffToNow(nameCacheTimer));
            //End By leewheel
        }

        //By leewheel 2026-07-12: 每个账号输出进度到server.loading
        if ((accountNumber + 1) % 10 == 0 || accountNumber == 0)
            TC_LOG_INFO("server.loading", "  -> 创建角色进度: {}/{} (已创建: {})", accountNumber + 1, totalAccountCount, bot_creation);
        //End By leewheel
        RandomPlayerbotFactory factory;

//By leewheel 2026-07-12: 必须调用SetBot(true)，否则SaveToDB时不会跳过collection保存导致外键约束失败
WorldSession* session = new WorldSession(accountId, "", 0, nullptr, SEC_PLAYER, EXPANSION_WRATH_OF_THE_LICH_KING,
time_t(0), "", Minutes(0), 0, ClientBuild::VariantId{}, LOCALE_enUS, 0, false);
session->SetBot(true);
//End By leewheel
sessionBots.push_back(session);

        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES - count; ++cls)
        {
            // skip nonexistent classes
            if (!((1 << (cls - 1)) & CLASSMASK_ALL_PLAYABLE) || !sChrClassesStore.LookupEntry(cls))
                continue;

            // skip disabled with config classes
            if ((1 << (cls - 1)) & sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK))
                continue;

            Player* playerBot = factory.CreateRandomBot(session, cls, nameCache);
            if (!playerBot)
            {
                TC_LOG_ERROR("playerbots", "Fail to create character for account {}", accountId);
                continue;
            }

            //By leewheel 2026-07-11: TC的SaveToDB只接受1个bool参数
            playerBot->SaveToDB(true);
            //End By leewheel
            //By leewheel 2026-07-11: TC的AddCharacterCacheEntry需要8个参数(多了isDeleted)
            sCharacterCache->AddCharacterCacheEntry(playerBot->GetGUID(), accountId, playerBot->GetName(),
                                                    playerBot->getGender(), playerBot->getRace(),
                                                    playerBot->getClass(), playerBot->GetLevel(), false);
            //End By leewheel
            playerBot->CleanupsBeforeDelete();
            delete playerBot;
            bot_creation++;
        }
    }

    if (bot_creation)
    {
        TC_LOG_INFO("server.loading", "  -> 等待 {} 个角色写入数据库...", bot_creation);
        /* wait for characters load into database, or characters will fail to loggin */
        while (CharacterDatabase.QueueSize())
        {
            std::this_thread::sleep_for(1s);
        }
        TC_LOG_INFO("server.loading", "  -> {} 个角色创建完成 ({} ms)", bot_creation, GetMSTimeDiffToNow(timer));
    }
    else
    {
        TC_LOG_INFO("server.loading", "  -> 所有账号已满, 无需创建新角色");
    }

    for (WorldSession* session : sessionBots)
        delete session;

    for (uint32 accountId : sPlayerbotAIConfig.randomBotAccounts)
    {
        totalRandomBotChars += AccountMgr::GetCharactersCount(accountId);
    }

    TC_LOG_INFO("server.loading", ">> {} random bot accounts with {} characters available",
            sPlayerbotAIConfig.randomBotAccounts.size(), totalRandomBotChars);
}

std::string const RandomPlayerbotFactory::CreateRandomGuildName()
{
    std::string guildName = "";

    QueryResult result = CharacterDatabase.Query("SELECT MAX(name_id) FROM playerbots_guild_names");
    if (!result)
    {
        TC_LOG_ERROR("playerbots", "No more names left for random guilds");
        return guildName;
    }

    Field* fields = result->Fetch();
    uint32 maxId = fields[0].Get<uint32>();

    uint32 id = urand(0, maxId);
    //By leewheel 2026-07-11: TC的Query不接受格式化参数, 用PQuery
    result = CharacterDatabase.PQuery(
        "SELECT n.name FROM playerbots_guild_names n "
        "LEFT OUTER JOIN guild e ON e.name = n.name WHERE e.guildid IS NULL AND n.name_id >= {} LIMIT 1",
        id);
    if (!result)
    {
        TC_LOG_ERROR("playerbots", "No more names left for random guilds");
        return guildName;
    }

    fields = result->Fetch();
    guildName = fields[0].Get<std::string>();

    return guildName;
}

// ========== 竞技场重构(合并 the-lab #2637, TC 适配) ==========
//By leewheel 2026-08-23: 原 CreateRandomArenaTeams/InitArenaTeam 两函数被 the-lab 重构为
// IsBotArenaTeam/LoadArenaTeamData/AssignBotToArenaTeam 等按需分配机制; randomBotArenaTeams 集合字段废弃。
bool RandomPlayerbotFactory::IsBotArenaTeam(ArenaTeam const* team)
{
    if (!team)
        return false;

    ObjectGuid captainGuid = team->GetCaptain();
    if (!captainGuid || !captainGuid.IsPlayer())
        return false;

    uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(captainGuid);
    return accountId && sPlayerbotAIConfig.IsInRandomAccountList(accountId);
}

void RandomPlayerbotFactory::LoadArenaTeamData()
{
    _configTargets = {
        {ARENA_TYPE_2v2, sPlayerbotAIConfig.randomBotArenaTeam2v2Count},
        {ARENA_TYPE_3v3, sPlayerbotAIConfig.randomBotArenaTeam3v3Count},
        {ARENA_TYPE_5v5, sPlayerbotAIConfig.randomBotArenaTeam5v5Count},
    };

    _botArenaTeamRegistry.clear();

    for (auto const& [id, team] : sArenaTeamMgr->GetArenaTeams())
    {
        if (!IsBotArenaTeam(team))
            continue;

        CharacterCacheEntry const* entry = sCharacterCache->GetCharacterCacheByGuid(team->GetCaptain());
        if (!entry)
            continue;

        ArenaType type = static_cast<ArenaType>(team->GetType());
        _botArenaTeamRegistry[type].push_back(id);
    }

    _availableArenaTeamNames.clear();

    //By leewheel 2026-08-23: TC 没有 arena_team 表, the-lab 的 LEFT JOIN arena_team 查询无法使用;
    // 改为从 playerbots_arena_team_names 全量加载后洗牌(重名概率极低, Create 失败会把名字放回列表)。
    QueryResult result = CharacterDatabase.PQuery(
        "SELECT name FROM playerbots_arena_team_names");

    if (!result)
    {
        LOG_WARN("playerbots", "No arena team names left in playerbots_arena_team_names");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _availableArenaTeamNames.push_back(fields[0].Get<std::string>());
    } while (result->NextRow());

    for (size_t i = _availableArenaTeamNames.size() - 1; i > 0; --i)
    {
        size_t j = urand(0, i);
        std::swap(_availableArenaTeamNames[i], _availableArenaTeamNames[j]);
    }

    LOG_INFO("playerbots", "Loaded {} available arena team names", _availableArenaTeamNames.size());
}

void RandomPlayerbotFactory::AssignBotToArenaTeam(Player* bot)
{
    if (!sPlayerbotAIConfig.IsInRandomAccountList(bot->GetSession()->GetAccountId()))
        return;

    if (sPlayerbotAIConfig.deleteRandomBotArenaTeams)
        return;

    if (bot->GetLevel() < 70)
        return;

    for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
    {
        if (bot->GetArenaTeamId(arena_slot))
            return;
    }

    PlayerbotWorldThreadProcessor::instance().QueueOperation(
        std::make_unique<ArenaTeamAssignOperation>(bot->GetGUID()));
}

void RandomPlayerbotFactory::AssignBotToArenaTeamInternal(Player* bot)
{
    // Check if bot has team, only one per bot to avoid queue conflicts
    for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
    {
        if (bot->GetArenaTeamId(arena_slot))
            return;
    }
    //By leewheel 2026-08-23: the-lab(#2637) 使用 sCharacterCache->GetCharacterArenaTeamIdByGuid,
    // TC 的 CharacterCache 无竞技场队伍字段; 改为遍历 ArenaTeamMgr 检查成员身份(等价逻辑)。
    for (auto const& [teamId, team] : sArenaTeamMgr->GetArenaTeams())
    {
        if (team && team->IsMember(bot->GetGUID()))
            return;
    }
    //End By leewheel

    TeamId const botTeam = bot->GetTeamId();

    // Randomize type order so no single type starves the others
    std::array<ArenaType, 3> order{ARENA_TYPE_2v2, ARENA_TYPE_3v3, ARENA_TYPE_5v5};
    for (size_t i = order.size() - 1; i > 0; --i)
        std::swap(order[i], order[urand(0, i)]);

    std::vector<ArenaTeam*> candidates;
    for (ArenaType type : order)
        CollectJoinableBotArenaTeams(type, botTeam, candidates);

    for (size_t i = candidates.size(); i > 0; --i)
    {
        size_t const index = urand(0, i - 1);
        ArenaTeam* team = candidates[index];
        candidates[index] = candidates[i - 1];

        if (!team->AddMember(bot->GetGUID()))
        {
            LOG_DEBUG("playerbots", "Failed to add bot {} to arena team '{}', trying next candidate",
                      bot->GetName(), team->GetName());
            continue;
        }

        if (team->GetMembersSize() >= static_cast<uint32>(team->GetType()))
        {
            uint32 teamRating = team->GetRating();
            team->SetRatingForAll(teamRating);

            // Keep MMR synchronized with team rating so matchmaking reflects artificial bot strength
            // (1000-2000 range) instead of the global CONFIG_ARENA_START_MATCHMAKER_RATING default.
            for (auto& member : team->GetMembers())
            {
                member.MatchMakerRating = member.PersonalRating;
                member.MaxMMR = std::max(member.MaxMMR, member.PersonalRating);
            }
            team->SaveToDB(true);
        }
        return;
    }

    // No joinable team available, create one if under target count
    for (ArenaType type : order)
    {
        if (GetBotArenaTeamCount(type) < _configTargets[type])
        {
            CreateBotArenaTeam(bot, type);
            return;
        }
    }
}

void RandomPlayerbotFactory::CreateBotArenaTeam(Player* bot, ArenaType type)
{
    std::string teamName = CreateRandomArenaTeamName();
    if (teamName.empty())
        return;

    ArenaTeam* arenateam = new ArenaTeam();
    if (!arenateam->Create(bot->GetGUID(), type, teamName, 0, 0, 0, 0, 0))
    {
        LOG_ERROR("playerbots", "Error creating arena team {}", teamName);
        delete arenateam;
        _availableArenaTeamNames.push_back(std::move(teamName));
        return;
    }

    arenateam->SetRatingForAll(
        urand(sPlayerbotAIConfig.randomBotArenaTeamMinRating, sPlayerbotAIConfig.randomBotArenaTeamMaxRating));

    uint32 backgroundColor = urand(0xFF000000, 0xFFFFFFFF);
    uint32 emblemStyle = urand(0, 101);
    uint32 emblemColor = urand(0xFF000000, 0xFFFFFFFF);
    uint32 borderStyle = urand(0, 5);
    uint32 borderColor = urand(0xFF000000, 0xFFFFFFFF);
    arenateam->SetEmblem(backgroundColor, emblemStyle, emblemColor, borderStyle, borderColor);

    arenateam->SaveToDB();
    sArenaTeamMgr->AddArenaTeam(arenateam);
    _botArenaTeamRegistry[type].push_back(arenateam->GetId());

    LOG_DEBUG("playerbots", "Created {}v{} arena team '{}' with captain {}",
              type, type, teamName, bot->GetName());
}

uint32 RandomPlayerbotFactory::GetBotArenaTeamCount(ArenaType type)
{
    auto it = _botArenaTeamRegistry.find(type);
    return it != _botArenaTeamRegistry.end() ? static_cast<uint32>(it->second.size()) : 0;
}

void RandomPlayerbotFactory::CollectJoinableBotArenaTeams(ArenaType type, TeamId faction, std::vector<ArenaTeam*>& out)
{
    auto it = _botArenaTeamRegistry.find(type);
    if (it == _botArenaTeamRegistry.end())
        return;

    uint32 const capacity = static_cast<uint32>(type);
    for (uint32 teamId : it->second)
    {
        ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(teamId);
        if (!team || team->GetMembersSize() >= capacity)
            continue;

        CharacterCacheEntry const* entry = sCharacterCache->GetCharacterCacheByGuid(team->GetCaptain());
        if (entry && Player::TeamIdForRace(entry->Race) == faction)
            out.push_back(team);
    }
}

void RandomPlayerbotFactory::DeleteBotArenaTeams()
{
    LOG_INFO("playerbots", "Deleting random bot arena teams...");

    std::vector<uint32> teamsToDisband;
    for (auto const& [id, arenateam] : sArenaTeamMgr->GetArenaTeams())
    {
        if (IsBotArenaTeam(arenateam))
            teamsToDisband.push_back(id);
    }

    for (uint32 teamId : teamsToDisband)
    {
        ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(teamId);
        if (team)
            team->Disband(nullptr);
    }

    _botArenaTeamRegistry.clear();
    LOG_INFO("playerbots", "Deleted {} random bot arena teams", teamsToDisband.size());
}

std::string RandomPlayerbotFactory::CreateRandomArenaTeamName()
{
    if (_availableArenaTeamNames.empty())
    {
        LOG_ERROR("playerbots", "No more names left for random arena teams");
        return "";
    }

    std::string name = std::move(_availableArenaTeamNames.back());
    _availableArenaTeamNames.pop_back();
    return name;
}
//End By leewheel
