/*
 * 机器人AI配置实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 使用 TrinityCore 的 sConfigMgr API (GetBoolDefault/GetIntDefault/GetFloatDefault/GetStringDefault)
 */

//By leewheel 2026-07-12: 添加缺失的初始化模块头文件（CreateRandomBots等）
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Config.h"
#include "Log.h"
#include "Util.h"
#include "DB2Stores.h"
#include "RandomPlayerbotFactory.h"
#include "PlayerbotGuildMgr.h"
#include "RandomItemMgr.h"
#include "BisListMgr.h"
#include "TravelMgr.h"
#include "PlayerbotFactory.h"
#include "PlayerbotDungeonRepository.h"
#include "Mgr/Text/PlayerbotTextMgr.h"
//End By leewheel
#include <ctime>
#include <cstdarg>
//By leewheel 2026-08-15: LoadRandomBotLevelConfig需要
#include <cctype>
//By leewheel 2026-09-04: loadWorldBuff 解析需要
#include <sstream>
//End By leewheel 2026-09-04
#include <sstream>
//End By leewheel

bool PlayerbotAIConfig::Initialize()
{
    TC_LOG_INFO("server.loading", "正在初始化机器人AI配置...");

    enabled = sConfigMgr->GetBoolDefault("Playerbot.Enabled", true);
    disabledWithoutRealPlayer = sConfigMgr->GetBoolDefault("Playerbot.DisableWithoutRealPlayer", false);
    EnableICCBuffs = sConfigMgr->GetBoolDefault("Playerbot.EnableICCBuffs", true);
    //By leewheel 2026-07-26: 技能名称解析诊断日志开关，默认关闭避免刷屏
    spellIdDiagnosticLog = sConfigMgr->GetBoolDefault("Playerbot.SpellIdDiagnosticLog", false);
    //End By leewheel

    allowAccountBots = sConfigMgr->GetBoolDefault("Playerbot.AllowAccountBots", true);
    allowGuildBots = sConfigMgr->GetBoolDefault("Playerbot.AllowGuildBots", true);
    allowTrustedAccountBots = sConfigMgr->GetBoolDefault("Playerbot.AllowTrustedAccountBots", true);

    randomBotGuildNearby = sConfigMgr->GetBoolDefault("Playerbot.RandomBotGuildNearby", false);
    randomBotInvitePlayer = sConfigMgr->GetBoolDefault("Playerbot.RandomBotInvitePlayer", false);
    inviteChat = sConfigMgr->GetBoolDefault("Playerbot.InviteChat", false);

    globalCoolDown = sConfigMgr->GetIntDefault("Playerbot.GlobalCoolDown", 500);
    reactDelay = sConfigMgr->GetIntDefault("Playerbot.ReactDelay", 100);
    maxWaitForMove = sConfigMgr->GetIntDefault("Playerbot.MaxWaitForMove", 5000);
    disableMoveSplinePath = sConfigMgr->GetIntDefault("Playerbot.DisableMoveSplinePath", 0);
    maxMovementSearchTime = sConfigMgr->GetIntDefault("Playerbot.MaxMovementSearchTime", 3);
    expireActionTime = sConfigMgr->GetIntDefault("Playerbot.ExpireActionTime", 5000);
    dispelAuraDuration = sConfigMgr->GetIntDefault("Playerbot.DispelAuraDuration", 700);
    passiveDelay = sConfigMgr->GetIntDefault("Playerbot.PassiveDelay", 10000);
    repeatDelay = sConfigMgr->GetIntDefault("Playerbot.RepeatDelay", 2000);
    errorDelay = sConfigMgr->GetIntDefault("Playerbot.ErrorDelay", 100);
    rpgDelay = sConfigMgr->GetIntDefault("Playerbot.RpgDelay", 10000);
    sitDelay = sConfigMgr->GetIntDefault("Playerbot.SitDelay", 20000);
    returnDelay = sConfigMgr->GetIntDefault("Playerbot.ReturnDelay", 2000);
    lootDelay = sConfigMgr->GetIntDefault("Playerbot.LootDelay", 1000);
    //By leewheel 2026-08-07: 拾取动作间短延迟——拾取已改为直接调用StoreLootItem即时完成，
    //不再需要1000ms的包往返等待。用短延迟(150ms)防动画/CPU峰值，同时大幅提升捡尸速度。
    lootPickupDelay = sConfigMgr->GetIntDefault("Playerbot.LootPickupDelay", 150);
    //End By leewheel

    dynamicReactDelay = sConfigMgr->GetBoolDefault("Playerbot.DynamicReactDelay", true);

    sightDistance = sConfigMgr->GetFloatDefault("Playerbot.SightDistance", 100.0f);
    spellDistance = sConfigMgr->GetFloatDefault("Playerbot.SpellDistance", 28.5f);
    reactDistance = sConfigMgr->GetFloatDefault("Playerbot.ReactDistance", 150.0f);
    grindDistance = sConfigMgr->GetFloatDefault("Playerbot.GrindDistance", 75.0f);
    lootDistance = sConfigMgr->GetFloatDefault("Playerbot.LootDistance", 15.0f);
    shootDistance = sConfigMgr->GetFloatDefault("Playerbot.ShootDistance", 5.0f);
    fleeDistance = sConfigMgr->GetFloatDefault("Playerbot.FleeDistance", 5.0f);
    tooCloseDistance = sConfigMgr->GetFloatDefault("Playerbot.TooCloseDistance", 5.0f);
    meleeDistance = sConfigMgr->GetFloatDefault("Playerbot.MeleeDistance", 0.75f);
    followDistance = sConfigMgr->GetFloatDefault("Playerbot.FollowDistance", 1.5f);
    whisperDistance = sConfigMgr->GetFloatDefault("Playerbot.WhisperDistance", 6000.0f);
    contactDistance = sConfigMgr->GetFloatDefault("Playerbot.ContactDistance", 0.45f);
    aoeRadius = sConfigMgr->GetFloatDefault("Playerbot.AoeRadius", 10.0f);
    rpgDistance = sConfigMgr->GetFloatDefault("Playerbot.RpgDistance", 200.0f);
    targetPosRecalcDistance = sConfigMgr->GetFloatDefault("Playerbot.TargetPosRecalcDistance", 0.1f);
    farDistance = sConfigMgr->GetFloatDefault("Playerbot.FarDistance", 20.0f);
    healDistance = sConfigMgr->GetFloatDefault("Playerbot.HealDistance", 38.5f);
    aggroDistance = sConfigMgr->GetFloatDefault("Playerbot.AggroDistance", 22.0f);

    criticalHealth = sConfigMgr->GetIntDefault("Playerbot.CriticalHealth", 25);
    lowHealth = sConfigMgr->GetIntDefault("Playerbot.LowHealth", 45);
    mediumHealth = sConfigMgr->GetIntDefault("Playerbot.MediumHealth", 65);
    almostFullHealth = sConfigMgr->GetIntDefault("Playerbot.AlmostFullHealth", 85);

    lowMana = sConfigMgr->GetIntDefault("Playerbot.LowMana", 15);
    mediumMana = sConfigMgr->GetIntDefault("Playerbot.MediumMana", 40);
    highMana = sConfigMgr->GetIntDefault("Playerbot.HighMana", 65);

    //By leewheel 2026-08-18: 加载 Playerbot.BotCheats（food/taxi/gold/health/mana/power/raid，逗号分隔）
    //修复：配置项此前从未被加载(botCheatMask恒为0)，导致"food"等全局作弊永远不生效(机器人不喝水/不吃东西)
    botCheatMask = 0;
    std::string const botCheatsStr = sConfigMgr->GetStringDefault("Playerbot.BotCheats", "");
    for (std::string const& cheat : split(botCheatsStr, ','))
    {
        if (cheat == "taxi")
            botCheatMask |= BotCheatMask::taxi;
        else if (cheat == "gold")
            botCheatMask |= BotCheatMask::gold;
        else if (cheat == "health")
            botCheatMask |= BotCheatMask::health;
        else if (cheat == "mana")
            botCheatMask |= BotCheatMask::mana;
        else if (cheat == "power")
            botCheatMask |= BotCheatMask::power;
        else if (cheat == "raid")
            botCheatMask |= BotCheatMask::raid;
        else if (cheat == "food")
            botCheatMask |= BotCheatMask::food;
    }
    //End By leewheel

    autoSaveMana = sConfigMgr->GetBoolDefault("Playerbot.AutoSaveMana", true);
    saveManaThreshold = sConfigMgr->GetIntDefault("Playerbot.SaveManaThreshold", 60);

    openGoSpell = sConfigMgr->GetIntDefault("Playerbot.OpenGoSpell", 6477);

    randomBotAutologin = sConfigMgr->GetBoolDefault("Playerbot.RandomBotAutologin", true);
    botAutologin = sConfigMgr->GetBoolDefault("Playerbot.BotAutologin", false);

    randomBotTeleportDistance = sConfigMgr->GetIntDefault("Playerbot.RandomBotTeleportDistance", 100);
    //By leewheel 2026-07-11: 加载randomBotTeleLowerLevel/HigherLevel配置项
    randomBotTeleLowerLevel = sConfigMgr->GetIntDefault("Playerbot.RandomBotTeleLowerLevel", 1);
    randomBotTeleHigherLevel = sConfigMgr->GetIntDefault("Playerbot.RandomBotTeleHigherLevel", 3);
    //End By leewheel 2026-07-11
    randomGearLoweringChance = sConfigMgr->GetFloatDefault("Playerbot.RandomGearLoweringChance", 0.0f);
    randomGearQualityLimit = sConfigMgr->GetIntDefault("Playerbot.RandomGearQualityLimit", 3);
    randomGearScoreLimit = sConfigMgr->GetIntDefault("Playerbot.RandomGearScoreLimit", 0);

    randomBotMinLevelChance = sConfigMgr->GetFloatDefault("Playerbot.RandomBotMinLevelChance", 0.1f);
    randomBotMaxLevelChance = sConfigMgr->GetFloatDefault("Playerbot.RandomBotMaxLevelChance", 0.1f);
    randomBotRpgChance = sConfigMgr->GetFloatDefault("Playerbot.RandomBotRpgChance", 0.2f);

    minRandomBots = sConfigMgr->GetIntDefault("Playerbot.MinRandomBots", 500);
    maxRandomBots = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBots", 500);

    randomBotUpdateInterval = sConfigMgr->GetIntDefault("Playerbot.RandomBotUpdateInterval", 20);
    randomBotCountChangeMinInterval = sConfigMgr->GetIntDefault("Playerbot.RandomBotCountChangeMinInterval", 1800);
    randomBotCountChangeMaxInterval = sConfigMgr->GetIntDefault("Playerbot.RandomBotCountChangeMaxInterval", 7200);

    minRandomBotInWorldTime = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotInWorldTime", 600);
    maxRandomBotInWorldTime = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotInWorldTime", 28800);
    permanentlyInWorldTime = sConfigMgr->GetIntDefault("Playerbot.PermanentlyInWorldTime", 31104000);

    minRandomBotRandomizeTime = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotRandomizeTime", 7200);
    maxRandomBotRandomizeTime = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotRandomizeTime", 1209600);

    minRandomBotChangeStrategyTime = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotChangeStrategyTime", 180);
    maxRandomBotChangeStrategyTime = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotChangeStrategyTime", 720);

    minRandomBotReviveTime = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotReviveTime", 60);
    maxRandomBotReviveTime = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotReviveTime", 300);

    minRandomBotTeleportInterval = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotTeleportInterval", 3600);
    maxRandomBotTeleportInterval = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotTeleportInterval", 18000);

    minRandomBotPvpTime = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotPvpTime", 600);
    maxRandomBotPvpTime = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotPvpTime", 1200);

    randomBotsPerInterval = sConfigMgr->GetIntDefault("Playerbot.RandomBotsPerInterval", 60);

    minRandomBotsPriceChangeInterval = sConfigMgr->GetIntDefault("Playerbot.MinRandomBotsPriceChangeInterval", 7200);
    maxRandomBotsPriceChangeInterval = sConfigMgr->GetIntDefault("Playerbot.MaxRandomBotsPriceChangeInterval", 172800);

    disabledWithoutRealPlayerLoginDelay = sConfigMgr->GetIntDefault("Playerbot.DisabledWithoutRealPlayerLoginDelay", 30);
    disabledWithoutRealPlayerLogoutDelay = sConfigMgr->GetIntDefault("Playerbot.DisabledWithoutRealPlayerLogoutDelay", 300);

    randomBotJoinLfg = sConfigMgr->GetBoolDefault("Playerbot.RandomBotJoinLfg", true);

    randomBotTalk = sConfigMgr->GetBoolDefault("Playerbot.RandomBotTalk", true);
    randomBotEmote = sConfigMgr->GetBoolDefault("Playerbot.RandomBotEmote", false);
    randomBotSuggestDungeons = sConfigMgr->GetBoolDefault("Playerbot.RandomBotSuggestDungeons", true);
    enableBroadcasts = sConfigMgr->GetBoolDefault("Playerbot.EnableBroadcasts", true);
    enableGreet = sConfigMgr->GetBoolDefault("Playerbot.EnableGreet", false);
    randomBotSayWithoutMaster = sConfigMgr->GetBoolDefault("Playerbot.RandomBotSayWithoutMaster", false);

    // 广播频道全局概率
    broadcastToGuildGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToGuildGlobalChance", 30000);
    broadcastToWorldGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToWorldGlobalChance", 30000);
    broadcastToGeneralGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToGeneralGlobalChance", 30000);
    broadcastToTradeGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToTradeGlobalChance", 30000);
    broadcastToLFGGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToLFGGlobalChance", 30000);
    broadcastToLocalDefenseGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToLocalDefenseGlobalChance", 30000);
    broadcastToWorldDefenseGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToWorldDefenseGlobalChance", 30000);
    broadcastToGuildRecruitmentGlobalChance = sConfigMgr->GetIntDefault("Playerbot.BroadcastToGuildRecruitmentGlobalChance", 30000);

    // 钓鱼距离配置
    fishingDistanceFromMaster = sConfigMgr->GetFloatDefault("Playerbot.FishingDistanceFromMaster", 10.0f);
    fishingDistance = sConfigMgr->GetFloatDefault("Playerbot.FishingDistance", 40.0f);
    endFishingWithMaster = sConfigMgr->GetFloatDefault("Playerbot.EndFishingWithMaster", 30.0f);
    //By leewheel 2026-07-10: 添加enableFishingWithMaster配置加载
    enableFishingWithMaster = sConfigMgr->GetBoolDefault("Playerbot.EnableFishingWithMaster", true);
    //End By leewheel
    //By leewheel 2026-07-26: 恢复职业匹配概率配置加载，默认100(完全按职业匹配)。
    classMatchingProfessionChance =
        std::min<uint32>(100, sConfigMgr->GetIntDefault("Playerbot.ClassMatchingProfessionChance", 100));
    //End By leewheel

    // 有毒链接前缀
    toxicLinksPrefix = sConfigMgr->GetStringDefault("Playerbot.ToxicLinksPrefix", "gnomes");
    //By leewheel 2026-07-11: 加载toxicLinks和thunderfury回复概率配置
    toxicLinksRepliesChance = sConfigMgr->GetIntDefault("Playerbot.ToxicLinksRepliesChance", 30);
    thunderfuryRepliesChance = sConfigMgr->GetIntDefault("Playerbot.ThunderfuryRepliesChance", 40);
    //End By leewheel

    randomBotJoinBG = sConfigMgr->GetBoolDefault("Playerbot.RandomBotJoinBG", true);
    randomBotAutoJoinBG = sConfigMgr->GetBoolDefault("Playerbot.RandomBotAutoJoinBG", false);

    //By leewheel 2026-07-11: 加载randomBotAutoJoinArenaBracket配置
    randomBotAutoJoinArenaBracket = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinArenaBracket", 14);
    //End By leewheel 2026-07-11

    logInGroupOnly = sConfigMgr->GetBoolDefault("Playerbot.LogInGroupOnly", true);
    logValuesPerTick = sConfigMgr->GetBoolDefault("Playerbot.LogValuesPerTick", false);
    fleeingEnabled = sConfigMgr->GetBoolDefault("Playerbot.FleeingEnabled", true);
    summonAtInnkeepersEnabled = sConfigMgr->GetBoolDefault("Playerbot.SummonAtInnkeepersEnabled", true);

    combatStrategies = sConfigMgr->GetStringDefault("Playerbot.CombatStrategies", "");
    nonCombatStrategies = sConfigMgr->GetStringDefault("Playerbot.NonCombatStrategies", "");
    randomBotCombatStrategies = sConfigMgr->GetStringDefault("Playerbot.RandomBotCombatStrategies", "");
    randomBotNonCombatStrategies = sConfigMgr->GetStringDefault("Playerbot.RandomBotNonCombatStrategies", "");
    applyInstanceStrategies = sConfigMgr->GetBoolDefault("Playerbot.ApplyInstanceStrategies", true);

    randomBotMinLevel = sConfigMgr->GetIntDefault("Playerbot.RandomBotMinLevel", 1);
    randomBotMaxLevel = sConfigMgr->GetIntDefault("Playerbot.RandomBotMaxLevel", 80);
    randomChangeMultiplier = sConfigMgr->GetFloatDefault("Playerbot.RandomChangeMultiplier", 1.0f);
    //By leewheel 2026-08-15: 对齐the-lab——统计输出间隔(0=关闭)、公会广播回复频率
    randomBotPrintStatsInterval = sConfigMgr->GetIntDefault("Playerbot.RandomBotPrintStatsInterval", 300);
    guildRepliesRate = sConfigMgr->GetIntDefault("Playerbot.GuildRepliesRate", 100);
    //End By leewheel

    commandPrefix = sConfigMgr->GetStringDefault("Playerbot.CommandPrefix", "");
    //By leewheel 2026-09-04 对齐上游: 分隔符默认改回两字符 "\\\\"——
    //单个 '\' 会与聊天文本中的路径/转义意外冲突, 上游 dist 同为两字符
    commandSeparator = sConfigMgr->GetStringDefault("Playerbot.CommandSeparator", "\\\\");

    randomBotAccountPrefix = sConfigMgr->GetStringDefault("Playerbot.RandomBotAccountPrefix", "rndbot");
    randomBotAccountCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAccountCount", 0);
    randomBotRandomPassword = sConfigMgr->GetBoolDefault("Playerbot.RandomBotRandomPassword", false);
    deleteRandomBotAccounts = sConfigMgr->GetBoolDefault("Playerbot.DeleteRandomBotAccounts", false);

    randomBotGuildCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotGuildCount", 20);
    randomBotGuildSizeMax = sConfigMgr->GetIntDefault("Playerbot.RandomBotGuildSizeMax", 15);
    deleteRandomBotGuilds = sConfigMgr->GetBoolDefault("Playerbot.DeleteRandomBotGuilds", false);

    fastReactInBG = sConfigMgr->GetBoolDefault("Playerbot.FastReactInBG", true);

    randombotsWalkingRPG = sConfigMgr->GetBoolDefault("Playerbot.RandombotsWalkingRPG", false);
    randombotsWalkingRPGInDoors = sConfigMgr->GetBoolDefault("Playerbot.RandombotsWalkingRPGInDoors", false);
    minEnchantingBotLevel = sConfigMgr->GetIntDefault("Playerbot.MinEnchantingBotLevel", 60);
    limitEnchantExpansion = sConfigMgr->GetIntDefault("Playerbot.LimitEnchantExpansion", 1);
    limitGearExpansion = sConfigMgr->GetIntDefault("Playerbot.LimitGearExpansion", 1);
    randombotStartingLevel = sConfigMgr->GetIntDefault("Playerbot.RandombotStartingLevel", 1);
    enablePeriodicOnlineOffline = sConfigMgr->GetBoolDefault("Playerbot.EnablePeriodicOnlineOffline", false);
    periodicOnlineOfflineRatio = sConfigMgr->GetFloatDefault("Playerbot.PeriodicOnlineOfflineRatio", 2.0f);
    gearscorecheck = sConfigMgr->GetBoolDefault("Playerbot.Gearscorecheck", false);
    randomBotPreQuests = sConfigMgr->GetBoolDefault("Playerbot.RandomBotPreQuests", false);
    botSendMailEnabled = sConfigMgr->GetBoolDefault("Playerbot.BotSendMailEnabled", true);

    guildTaskEnabled = sConfigMgr->GetBoolDefault("Playerbot.GuildTaskEnabled", false);
    minGuildTaskChangeTime = sConfigMgr->GetIntDefault("Playerbot.MinGuildTaskChangeTime", 172800);
    maxGuildTaskChangeTime = sConfigMgr->GetIntDefault("Playerbot.MaxGuildTaskChangeTime", 432000);
    minGuildTaskAdvertisementTime = sConfigMgr->GetIntDefault("Playerbot.MinGuildTaskAdvertisementTime", 300);
    maxGuildTaskAdvertisementTime = sConfigMgr->GetIntDefault("Playerbot.MaxGuildTaskAdvertisementTime", 28800);
    minGuildTaskRewardTime = sConfigMgr->GetIntDefault("Playerbot.MinGuildTaskRewardTime", 300);
    maxGuildTaskRewardTime = sConfigMgr->GetIntDefault("Playerbot.MaxGuildTaskRewardTime", 3600);
    guildTaskAdvertCleanupTime = sConfigMgr->GetIntDefault("Playerbot.GuildTaskAdvertCleanupTime", 300);
    //By leewheel 2026-08-15: 默认值对齐the-lab 2000(原200使击杀任务怪物距离判定严10倍)
    guildTaskKillTaskDistance = sConfigMgr->GetIntDefault("Playerbot.GuildTaskKillTaskDistance", 2000);
    //End By leewheel

    iterationsPerTick = sConfigMgr->GetIntDefault("Playerbot.IterationsPerTick", 10);

    enableAutoTradeOnItemMention = sConfigMgr->GetBoolDefault("Playerbot.EnableAutoTradeOnItemMention", true);

    commandServerPort = sConfigMgr->GetIntDefault("Playerbot.CommandServerPort", 8888);
    perfMonEnabled = sConfigMgr->GetBoolDefault("Playerbot.PerfMonEnabled", false);
    summonWhenGroup = sConfigMgr->GetBoolDefault("Playerbot.SummonWhenGroup", true);
    //By leewheel 2026-08-01: 移植头盔/披风三级显示配置(7ebe7f06)——0=始终隐藏,1=始终显示(默认),2=随机
    switch (sConfigMgr->GetIntDefault("Playerbot.RandomBotShowHelmet", 1))
    {
        case 0:
            randomBotShowHelmet = ShowHideCosmetic::ALWAYS_HIDE;
            break;
        case 2:
            randomBotShowHelmet = ShowHideCosmetic::RANDOMIZE;
            break;
        case 1:
        default:
            randomBotShowHelmet = ShowHideCosmetic::ALWAYS_SHOW;
            break;
    }
    switch (sConfigMgr->GetIntDefault("Playerbot.RandomBotShowCloak", 1))
    {
        case 0:
            randomBotShowCloak = ShowHideCosmetic::ALWAYS_HIDE;
            break;
        case 2:
            randomBotShowCloak = ShowHideCosmetic::RANDOMIZE;
            break;
        case 1:
        default:
            randomBotShowCloak = ShowHideCosmetic::ALWAYS_SHOW;
            break;
    }
    //End By leewheel
    randomBotFixedLevel = sConfigMgr->GetBoolDefault("Playerbot.RandomBotFixedLevel", false);
    disableRandomLevels = sConfigMgr->GetBoolDefault("Playerbot.DisableRandomLevels", false);
    randomBotXPRate = sConfigMgr->GetFloatDefault("Playerbot.RandomBotXPRate", 1.0f);
    randomBotAllianceRatio = sConfigMgr->GetIntDefault("Playerbot.RandomBotAllianceRatio", 50);
    randomBotHordeRatio = sConfigMgr->GetIntDefault("Playerbot.RandomBotHordeRatio", 50);
    disableDeathKnightLogin = sConfigMgr->GetBoolDefault("Playerbot.DisableDeathKnightLogin", false);
    limitTalentsExpansion = sConfigMgr->GetBoolDefault("Playerbot.LimitTalentsExpansion", false);

    //By leewheel 2026-07-20: 恢复AC默认值10，100%活跃导致500bot全量AI运算严重卡顿，10%轮换+SmartScale足够
    botActiveAlone = sConfigMgr->GetIntDefault("Playerbot.BotActiveAlone", 10);
    //End By leewheel
    BotActiveAloneDurationSeconds = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneDurationSeconds", 30);
    BotActiveAloneForceWhenInRadius = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneForceWhenInRadius", 150);
    BotActiveAloneForceWhenInZone = sConfigMgr->GetBoolDefault("Playerbot.BotActiveAloneForceWhenInZone", true);
    BotActiveAloneForceWhenInMap = sConfigMgr->GetBoolDefault("Playerbot.BotActiveAloneForceWhenInMap", false);
    BotActiveAloneForceWhenIsFriend = sConfigMgr->GetBoolDefault("Playerbot.BotActiveAloneForceWhenIsFriend", false);
    BotActiveAloneForceWhenInGuild = sConfigMgr->GetBoolDefault("Playerbot.BotActiveAloneForceWhenInGuild", true);
    //By leewheel 2026-07-20: 恢复AC默认值true，SmartScale根据服务器负载自动缩放bot活跃度，防止卡顿
    botActiveAloneSmartScale = sConfigMgr->GetBoolDefault("Playerbot.BotActiveAloneSmartScale", true);
    //End By leewheel
    botActiveAloneSmartScaleDiffLimitfloor = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneSmartScaleDiffLimitFloor", 50);
    botActiveAloneSmartScaleDiffLimitCeiling = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneSmartScaleDiffLimitCeiling", 200);
    botActiveAloneSmartScaleWhenMinLevel = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneSmartScaleWhenMinLevel", 1);
    botActiveAloneSmartScaleWhenMaxLevel = sConfigMgr->GetIntDefault("Playerbot.BotActiveAloneSmartScaleWhenMaxLevel", 80);

    freeMethodLoot = sConfigMgr->GetBoolDefault("Playerbot.FreeMethodLoot", false);
    //By leewheel 2026-08-24: 让机器人Roll点贴近真实玩家——默认允许Need(级别2)和Greed(开启),
    //配合LootRollAction新策略: 绿装+比当前装备好→Need, 能穿但不如当前→Greed。
    lootNeedRollLevel = sConfigMgr->GetIntDefault("Playerbot.LootNeedRollLevel", 2);
    lootGreedRollLevel = sConfigMgr->GetBoolDefault("Playerbot.LootGreedRollLevel", true);
    //End By leewheel
    lootRollRecipe = sConfigMgr->GetBoolDefault("Playerbot.LootRollRecipe", false);
    lootRollDisenchant = sConfigMgr->GetBoolDefault("Playerbot.LootRollDisenchant", false);
    autoPickReward = sConfigMgr->GetStringDefault("Playerbot.AutoPickReward", "yes");
    autoEquipUpgradeLoot = sConfigMgr->GetBoolDefault("Playerbot.AutoEquipUpgradeLoot", true);
    equipUpgradeThreshold = sConfigMgr->GetFloatDefault("Playerbot.EquipUpgradeThreshold", 1.1f);
    twoRoundsGearInit = sConfigMgr->GetBoolDefault("Playerbot.TwoRoundsGearInit", false);
    syncQuestWithPlayer = sConfigMgr->GetBoolDefault("Playerbot.SyncQuestWithPlayer", true);
    syncQuestForPlayer = sConfigMgr->GetBoolDefault("Playerbot.SyncQuestForPlayer", false);
    dropObsoleteQuests = sConfigMgr->GetBoolDefault("Playerbot.DropObsoleteQuests", true);
    allowLearnTrainerSpells = sConfigMgr->GetBoolDefault("Playerbot.AllowLearnTrainerSpells", true);
    autoPickTalents = sConfigMgr->GetBoolDefault("Playerbot.AutoPickTalents", true);
    autoUpgradeEquip = sConfigMgr->GetBoolDefault("Playerbot.AutoUpgradeEquip", true);
    hunterWolfPet = sConfigMgr->GetIntDefault("Playerbot.HunterWolfPet", 0);
    defaultPetStance = sConfigMgr->GetIntDefault("Playerbot.DefaultPetStance", 1);
    petChatCommandDebug = sConfigMgr->GetIntDefault("Playerbot.PetChatCommandDebug", 0);
    autoLearnTrainerSpells = sConfigMgr->GetBoolDefault("Playerbot.AutoLearnTrainerSpells", true);
    autoDoQuests = sConfigMgr->GetBoolDefault("Playerbot.AutoDoQuests", true);
    enableNewRpgStrategy = sConfigMgr->GetBoolDefault("Playerbot.EnableNewRpgStrategy", true);

    //By leewheel 2026-07-14: 修复RpgStatusProbWeight未初始化的bug
    //原版AC在AiPlayerbot前缀下初始化这些权重，TC移植时遗漏了
    //所有权重默认为0导致RandomChangeStatus无法选择任何RPG状态，机器人永远停在Idle
    RpgStatusProbWeight[RPG_WANDER_RANDOM] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.WanderRandom", 15);
    RpgStatusProbWeight[RPG_WANDER_NPC] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.WanderNpc", 20);
    RpgStatusProbWeight[RPG_GO_GRIND] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.GoGrind", 15);
    RpgStatusProbWeight[RPG_GO_CAMP] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.GoCamp", 10);
    RpgStatusProbWeight[RPG_DO_QUEST] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.DoQuest", 60);
    RpgStatusProbWeight[RPG_TRAVEL_FLIGHT] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.TravelFlight", 15);
    RpgStatusProbWeight[RPG_REST] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.Rest", 5);
    RpgStatusProbWeight[RPG_OUTDOOR_PVP] = sConfigMgr->GetIntDefault("Playerbot.RpgStatusProbWeight.OutdoorPvp", 10);
    //End By leewheel

    syncLevelWithPlayers = sConfigMgr->GetBoolDefault("Playerbot.SyncLevelWithPlayers", false);
    //By leewheel 2026-09-05: 上游164335fe——随机bot传送集中到玩家所在区域
    randomBotConcentrateInPlayerZone =
        sConfigMgr->GetBoolDefault("Playerbot.RandomBotConcentrateInPlayerZone", false);
    //End By leewheel
    autoLearnQuestSpells = sConfigMgr->GetBoolDefault("Playerbot.AutoLearnQuestSpells", false);
    autoTeleportForLevel = sConfigMgr->GetBoolDefault("Playerbot.AutoTeleportForLevel", true);
    randomBotGroupNearby = sConfigMgr->GetBoolDefault("Playerbot.RandomBotGroupNearby", false);
    enableRandomBotTrading = sConfigMgr->GetIntDefault("Playerbot.EnableRandomBotTrading", 1);
    tweakValue = sConfigMgr->GetIntDefault("Playerbot.TweakValue", 0);

    randomBotArenaTeamMaxRating = sConfigMgr->GetIntDefault("Playerbot.RandomBotArenaTeamMaxRating", 2000);
    randomBotArenaTeamMinRating = sConfigMgr->GetIntDefault("Playerbot.RandomBotArenaTeamMinRating", 1000);
    randomBotArenaTeam2v2Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotArenaTeam2v2Count", 10);
    randomBotArenaTeam3v3Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotArenaTeam3v3Count", 10);
    randomBotArenaTeam5v5Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotArenaTeam5v5Count", 5);
    deleteRandomBotArenaTeams = sConfigMgr->GetBoolDefault("Playerbot.DeleteRandomBotArenaTeams", false);

    selfBotLevel = sConfigMgr->GetIntDefault("Playerbot.SelfBotLevel", 1);
    downgradeMaxLevelBot = sConfigMgr->GetBoolDefault("Playerbot.DowngradeMaxLevelBot", false);
    equipAndSpecPersistence = sConfigMgr->GetBoolDefault("Playerbot.EquipAndSpecPersistence", true);
    equipAndSpecPersistenceLevel = sConfigMgr->GetIntDefault("Playerbot.EquipAndSpecPersistenceLevel", 1);
    groupInvitationPermission = sConfigMgr->GetIntDefault("Playerbot.GroupInvitationPermission", 1);
    keepAltsInGroup = sConfigMgr->GetBoolDefault("Playerbot.KeepAltsInGroup", false);
    allowSummonInCombat = sConfigMgr->GetBoolDefault("Playerbot.AllowSummonInCombat", true);
    allowSummonWhenMasterIsDead = sConfigMgr->GetBoolDefault("Playerbot.AllowSummonWhenMasterIsDead", true);
    allowSummonWhenBotIsDead = sConfigMgr->GetBoolDefault("Playerbot.AllowSummonWhenBotIsDead", true);
    reviveBotWhenSummoned = sConfigMgr->GetIntDefault("Playerbot.ReviveBotWhenSummoned", 1);
    botRepairWhenSummon = sConfigMgr->GetBoolDefault("Playerbot.BotRepairWhenSummon", true);
    autoInitOnly = sConfigMgr->GetBoolDefault("Playerbot.AutoInitOnly", false);
    resetInstanceIdForAltBots = sConfigMgr->GetBoolDefault("Playerbot.ResetInstanceIdForAltBots", false);
    autoInitEquipLevelLimitRatio = sConfigMgr->GetFloatDefault("Playerbot.AutoInitEquipLevelLimitRatio", 1.0f);
    maxAddedBots = sConfigMgr->GetIntDefault("Playerbot.MaxAddedBots", 40);
    addClassCommand = sConfigMgr->GetIntDefault("Playerbot.AddClassCommand", 1);
    addClassAccountPoolSize = sConfigMgr->GetIntDefault("Playerbot.AddClassAccountPoolSize", 50);
    maintenanceCommand = sConfigMgr->GetIntDefault("Playerbot.MaintenanceCommand", 1);
    autoGearCommand = sConfigMgr->GetIntDefault("Playerbot.AutoGearCommand", 1);
    autoGearCommandAltBots = sConfigMgr->GetIntDefault("Playerbot.AutoGearCommandAltBots", 1);
    autoGearQualityLimit = sConfigMgr->GetIntDefault("Playerbot.AutoGearQualityLimit", 3);
    autoGearScoreLimit = sConfigMgr->GetIntDefault("Playerbot.AutoGearScoreLimit", 0);
    autoGearBisCommand = sConfigMgr->GetIntDefault("Playerbot.AutoGearBisCommand", 0);

    //By leewheel 20260710: AC兼容 - 替代维护配置加载
    altMaintenanceAttunementQs = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceAttunementQuests", true);
    altMaintenanceBags = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceBags", true);
    altMaintenanceAmmo = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceAmmo", true);
    altMaintenanceFood = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceFood", true);
    altMaintenanceReagents = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceReagents", true);
    altMaintenanceConsumables = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceConsumables", true);
    altMaintenancePotions = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenancePotions", true);
    altMaintenanceTalentTree = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceTalentTree", true);
    altMaintenancePet = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenancePet", true);
    altMaintenancePetTalents = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenancePetTalents", true);
    altMaintenanceClassSpells = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceClassSpells", true);
    altMaintenanceAvailableSpells = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceAvailableSpells", true);
    altMaintenanceSkills = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceSkills", true);
    altMaintenanceReputation = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceReputation", true);
    altMaintenanceSpecialSpells = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceSpecialSpells", true);
    altMaintenanceMounts = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceMounts", true);
    altMaintenanceGlyphs = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceGlyphs", true);
    altMaintenanceKeyring = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceKeyring", true);
    altMaintenanceGemsEnchants = sConfigMgr->GetBoolDefault("Playerbot.AltMaintenanceGemsEnchants", true);
    //End By leewheel

    useGroundMountAtMinLevel = sConfigMgr->GetIntDefault("Playerbot.UseGroundMountAtMinLevel", 20);
    useFastGroundMountAtMinLevel = sConfigMgr->GetIntDefault("Playerbot.UseFastGroundMountAtMinLevel", 40);
    useFlyMountAtMinLevel = sConfigMgr->GetIntDefault("Playerbot.UseFlyMountAtMinLevel", 60);
    useFastFlyMountAtMinLevel = sConfigMgr->GetIntDefault("Playerbot.UseFastFlyMountAtMinLevel", 70);

    botTaxiDelayMin = sConfigMgr->GetIntDefault("Playerbot.BotTaxiDelayMin", 350);
    botTaxiDelayMax = sConfigMgr->GetIntDefault("Playerbot.BotTaxiDelayMax", 5000);
    botTaxiGapMs = sConfigMgr->GetIntDefault("Playerbot.BotTaxiGapMs", 200);
    botTaxiGapJitterMs = sConfigMgr->GetIntDefault("Playerbot.BotTaxiGapJitterMs", 100);

    //By leewheel 2026-07-12: 补全缺失的TalentSpecs加载（移植时遗漏，导致randomClassSpecProb全为0，urand(1,0)崩溃）
    TC_LOG_INFO("server.loading", "Loading TalentSpecs...");

    for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
    {
        if (cls == 10)
        {
            continue;
        }
        for (uint32 spec = 0; spec < MAX_SPECNO; ++spec)
        {
            //By leewheel 2026-07-21: 配置前缀修正，worldserver.conf使用Playerbot.前缀(非AC的AiPlayerbot.)
            std::ostringstream os;
            os << "Playerbot.PremadeSpecName." << cls << "." << spec;
            premadeSpecName[cls][spec] = sConfigMgr->GetStringDefault(os.str(), "", true);
            os.str("");
            os.clear();
            os << "Playerbot.PremadeSpecGlyph." << cls << "." << spec;
            //End By leewheel
            premadeSpecGlyph[cls][spec] = sConfigMgr->GetStringDefault(os.str(), "", true);
            std::vector<std::string> splitSpecGlyph = split(premadeSpecGlyph[cls][spec], ',');
            for (std::string& split : splitSpecGlyph)
            {
                if (split.size() != 0)
                {
                    parsedSpecGlyph[cls][spec].push_back(atoi(split.c_str()));
                }
            }
            for (uint32 level = 0; level < MAX_LEVEL; ++level)
            {
                //By leewheel 2026-07-21: 配置前缀修正AiPlayerbot→Playerbot
                std::ostringstream os;
                os << "Playerbot.PremadeSpecLink." << cls << "." << spec << "." << level;
                //End By leewheel
                premadeSpecLink[cls][spec][level] = sConfigMgr->GetStringDefault(os.str(), "", true);
                parsedSpecLinkOrder[cls][spec][level] = ParseTempTalentsOrder(cls, premadeSpecLink[cls][spec][level]);
            }
        }
        for (uint32 spec = 0; spec < 3; ++spec)
        {
            for (uint32 points = 0; points < 21; ++points)
            {
                //By leewheel 2026-07-21: 配置前缀修正AiPlayerbot→Playerbot
                std::ostringstream os;
                os << "Playerbot.PremadeHunterPetLink." << spec << "." << points;
                //End By leewheel
                premadeHunterPetLink[spec][points] = sConfigMgr->GetStringDefault(os.str(), "", true);
                parsedHunterPetLinkOrder[spec][points] =
                    ParseTempPetTalentsOrder(spec, premadeHunterPetLink[spec][points]);
            }
        }
        for (uint32 spec = 0; spec < MAX_SPECNO; ++spec)
        {
            //By leewheel 2026-07-21: 配置前缀修正AiPlayerbot→Playerbot
            std::ostringstream os;
            os << "Playerbot.RandomClassSpecProb." << cls << "." << spec;
            //End By leewheel
            //By leewheel 2026-07-23: 坦克/治疗天赋概率翻倍，解决随机本缺少坦克和治疗的问题
            //默认spec 0/1/2 对应天赋页 0/1/2，根据职业判断哪些页是坦克/治疗
            uint32 def;
            if (spec >= 3)
                def = 0;
            else
            {
                bool isTankHeal = false;
                switch (cls)
                {
                    case CLASS_WARRIOR:       isTankHeal = (spec == 2); break; // 防护=坦克
                    case CLASS_PALADIN:      isTankHeal = (spec == 0 || spec == 1); break; // 神圣=治疗, 防护=坦克
                    case CLASS_DEATH_KNIGHT:  isTankHeal = (spec == 0); break; // 鲜血=坦克
                    case CLASS_DRUID:         isTankHeal = (spec == 1 || spec == 2); break; // 野性=坦克, 恢复=治疗
                    case CLASS_PRIEST:        isTankHeal = (spec == 0 || spec == 1); break; // 戒律=治疗, 神圣=治疗
                    case CLASS_SHAMAN:        isTankHeal = (spec == 2); break; // 恢复=治疗
                    default: break; // 纯DPS职业不翻倍
                }
                def = isTankHeal ? 50 : 25;
            }
            //End By leewheel
            randomClassSpecProb[cls][spec] = sConfigMgr->GetIntDefault(os.str(), def, true);
            os.str("");
            os.clear();
            //By leewheel 2026-07-21: 配置前缀修正AiPlayerbot→Playerbot
            os << "Playerbot.RandomClassSpecIndex." << cls << "." << spec;
            //End By leewheel
            randomClassSpecIndex[cls][spec] = sConfigMgr->GetIntDefault(os.str(), spec, true);
        }
    }
    //End By leewheel

    restrictHealerDPS = sConfigMgr->GetBoolDefault("Playerbot.RestrictHealerDPS", true);

    //By leewheel 2026-09-04 对齐上游: 补加载受限奶妈DPS地图列表——
    //.h 字段与 IsRestrictedHealerDPSMap 消费点早已就位, 但此列表从未加载导致恒空,
    //"开关打开但列表永远空", 奶妈DPS限制功能整体死路; 键名沿用本地 Playerbot. 约定,
    //默认值取上游 AiPlayerbot.RestrictedHealerDPSMaps 原表, 并把 restrictHealerDPS
    //前置判断并入 IsRestrictedHealerDPSMap(上游 952-968 行样式)
    std::string restrictedHealerDpsMaps = sConfigMgr->GetStringDefault("Playerbot.RestrictedHealerDPSMaps",
        "33,34,36,43,47,48,70,90,109,129,209,229,230,329,349,389,429,1001,1004,"
        "1007,269,540,542,543,545,546,547,552,553,554,555,556,557,558,560,585,574,"
        "575,576,578,595,599,600,601,602,604,608,619,632,650,658,668,409,469,509,"
        "531,532,534,544,548,550,564,565,580,249,533,603,615,616,624,631,649,724");
    if (!restrictedHealerDpsMaps.empty())
    {
        std::vector<std::string> mapTokens = split(restrictedHealerDpsMaps, ',');
        for (auto& token : mapTokens)
        {
            uint32 mapId = atoi(token.c_str());
            if (mapId > 0)
                restrictedHealerDPSMaps.push_back(mapId);
        }
    }

    //By leewheel 2026-09-04 对齐上游: 补加载交易动作排除前缀——
    //TradeAction 消费点早已就位, 但列表从未加载导致恒空, 前缀排除功能死路
    tradeActionExcludedPrefixes.clear();
    {
        std::string excludedPrefixes = sConfigMgr->GetStringDefault("Playerbot.TradeActionExcludedPrefixes", "");
        if (!excludedPrefixes.empty())
        {
            std::vector<std::string> prefixTokens = split(excludedPrefixes, ',');
            for (auto& token : prefixTokens)
                if (!token.empty())
                    tradeActionExcludedPrefixes.push_back(token);
        }
    }

    //By leewheel 2026-09-04 对齐上游: 补加载猎人排除宠物族群——
    //PlayerbotFactory 宠物选择消费点早已就位, 但列表从未加载导致恒空, 排除功能死路
    excludedHunterPetFamilies.clear();
    {
        std::string excludedFamilies = sConfigMgr->GetStringDefault("Playerbot.ExcludedHunterPetFamilies", "");
        if (!excludedFamilies.empty())
        {
            std::vector<std::string> familyTokens = split(excludedFamilies, ',');
            for (auto& token : familyTokens)
            {
                uint32 familyId = atoi(token.c_str());
                if (familyId > 0)
                    excludedHunterPetFamilies.push_back(familyId);
            }
        }
    }

    //By leewheel 2026-09-04 对齐上游: 补加载日志文件白名单——
    //.h 的 allowedLogFiles 字段与 hasLog 消费点(TravelMgr/TravelNode 调试CSV、随机机器人位置日志)
    //早已就位, 但列表从未加载导致恒空, 全部调试日志入口被静默关闭
    allowedLogFiles.clear();
    {
        std::string allowedLogs = sConfigMgr->GetStringDefault("Playerbot.AllowedLogFiles", "");
        if (!allowedLogs.empty())
        {
            std::vector<std::string> logTokens = split(allowedLogs, ',');
            for (auto& token : logTokens)
                if (!token.empty())
                    allowedLogFiles.push_back(token);
        }
    }

    //By leewheel 2026-09-04 对齐上游: broadcastChanceMaxValue 与总开关联动——
    //enableBroadcasts=false 时归 0 作为第二道保险(各入口已有前置判断, 不会执行到 urand(1,0));
    //数值沿用本地 1/10000 概率刻度(与 .h 默认一致, 本地广播概率体系为有意重调, 不照抄上游 30000 刻度)
    broadcastChanceMaxValue = enableBroadcasts ? 10000 : 0;

    //By leewheel 2026-09-04 对齐上游: 区域等级区间配置覆盖加载(ZoneBracket.<zoneId>="min,max")——
    //覆盖 TravelMgr zone2LevelBracket 硬编码表的指定区域
    {
        static constexpr uint32 zoneBracketZoneIds[] = {
            // 经典旧世 - 区域
            4, 28, 46, 139, 361, 490, 618, 1377,
            // 燃烧的远征 - 区域
            3483, 3518, 3519, 3520, 3521, 3522, 3523, 4080,
            // 巫妖王之怒 - 区域
            65, 66, 67, 210, 394, 495, 2817, 3537, 3711, 4197
        };
        for (uint32 zoneId : zoneBracketZoneIds)
        {
            std::string setting = "Playerbot.ZoneBracket." + std::to_string(zoneId);
            std::string value = sConfigMgr->GetStringDefault(setting, "");
            if (!value.empty())
            {
                size_t commaPos = value.find(',');
                if (commaPos != std::string::npos)
                {
                    uint32 minLevel = atoi(value.substr(0, commaPos).c_str());
                    uint32 maxLevel = atoi(value.substr(commaPos + 1).c_str());
                    if (minLevel > 0 && maxLevel >= minLevel)
                        zoneBrackets[zoneId] = std::make_pair(minLevel, maxLevel);
                }
            }
        }
    }

    //By leewheel 2026-09-04 对齐上游: 补世界Buff矩阵加载——
    //.h 的 WorldBuffData/worldBuffs 字段与 WorldBuffAction 消费点早已就位,
    //但 loadWorldBuff 既无定义也无调用, 世界Buff功能整体空转; 键名沿用本地 Playerbot. 约定
    loadWorldBuff();

    // 解析 PvP 禁止区域
    std::string pvpProhibitedZones = sConfigMgr->GetStringDefault("Playerbot.PvpProhibitedZoneIds", "2255,656,2361,2362,2363,976,35,2268,3425,392,541,1446,3828,3712,3738,3565,3539,3623,4152,3988,4658,4284,4418,4436,4275,4323,4395,3703,4298,3951");
    if (!pvpProhibitedZones.empty())
    {
        std::vector<std::string> zones = split(pvpProhibitedZones, ',');
        for (auto& zone : zones)
        {
            uint32 zoneId = atoi(zone.c_str());
            if (zoneId > 0)
                pvpProhibitedZoneIds.push_back(zoneId);
        }
    }

    std::string pvpProhibitedAreas = sConfigMgr->GetStringDefault("Playerbot.PvpProhibitedAreaIds", "976,35,392,2268,4161,4010,4317,4312,3649,3887,3958,3724,4080,3938,3754,3786,3973,4085,4086,4087,4088");
    if (!pvpProhibitedAreas.empty())
    {
        std::vector<std::string> areas = split(pvpProhibitedAreas, ',');
        for (auto& area : areas)
        {
            uint32 areaId = atoi(area.c_str());
            if (areaId > 0)
                pvpProhibitedAreaIds.push_back(areaId);
        }
    }

    // 解析机器人地图
    randomBotMapsAsString = sConfigMgr->GetStringDefault("Playerbot.RandomBotMaps", "0,1,530,571");
    if (!randomBotMapsAsString.empty())
    {
        std::vector<std::string> maps = split(randomBotMapsAsString, ',');
        for (auto& map : maps)
        {
            uint32 mapId = atoi(map.c_str());
            randomBotMaps.push_back(mapId);
        }
    }

    // 解析随机机器人账号
    std::string randomBotAccountsStr = sConfigMgr->GetStringDefault("Playerbot.RandomBotAccounts", "");
    if (!randomBotAccountsStr.empty())
    {
        std::vector<std::string> accounts = split(randomBotAccountsStr, ',');
        for (auto& account : accounts)
        {
            uint32 accountId = atoi(account.c_str());
            if (accountId > 0)
                randomBotAccounts.push_back(accountId);
        }
    }

    //By leewheel 2026-07-26: 补全移植时遗漏的配置读取
    //以下成员在.h声明并被功能代码消费,但config.cpp从未赋值,导致卡在.h默认值(false/0/空),功能被静默禁用
    //默认值与AC参考(AiPlayerbot.*)保持一致,key前缀统一为Playerbot.*
    // —— 自动团队Buff / 强效祝福 ——
    switch (sConfigMgr->GetIntDefault("Playerbot.AutoGreaterBlessings", 1))
    {
        case 0: autoGreaterBlessings = AutoPartyBuffMode::DISABLED; break;
        case 2: autoGreaterBlessings = AutoPartyBuffMode::GROUP_OR_RAID; break;
        default: autoGreaterBlessings = AutoPartyBuffMode::RAID_ONLY; break;
    }
    switch (sConfigMgr->GetIntDefault("Playerbot.AutoPartyBuffs", 2))
    {
        case 0: autoPartyBuffs = AutoPartyBuffMode::DISABLED; break;
        case 1: autoPartyBuffs = AutoPartyBuffMode::RAID_ONLY; break;
        default: autoPartyBuffs = AutoPartyBuffMode::GROUP_OR_RAID; break;
    }
    tellWhenMissingBuffReagents = sConfigMgr->GetBoolDefault("Playerbot.TellWhenMissingBuffReagents", true);
    missingBuffReagentMessageCooldown = sConfigMgr->GetIntDefault("Playerbot.MissingBuffReagentMessageCooldown", 300);
    //By leewheel 2026-08-23: 合并 the-lab(#2571) —— ReadyCheck 前强制补增益配置
    forceRebuffOnReadyCheck = sConfigMgr->GetBoolDefault("Playerbot.ForceRebuffOnReadyCheck", false);
    forceRebuffMarginSecs = std::min(sConfigMgr->GetIntDefault("Playerbot.ForceRebuffMarginSecs", 60), 3600);
    //End By leewheel

    // —— AoE躲避（否则机器人会站在火里不躲） ——
    autoAvoidAoe = sConfigMgr->GetBoolDefault("Playerbot.AutoAvoidAoe", true);
    maxAoeAvoidRadius = sConfigMgr->GetFloatDefault("Playerbot.MaxAoeAvoidRadius", 15.0f);
    tellWhenAvoidAoe = sConfigMgr->GetBoolDefault("Playerbot.TellWhenAvoidAoe", false);
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.AoeAvoidSpellWhitelist", "50759,57491,13810,29946");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                aoeAvoidSpellWhitelist.insert(v);
        }
    }

    // —— 装备偏好 ——
    preferClassArmorType = sConfigMgr->GetBoolDefault("Playerbot.PreferClassArmorType", false);
    preferredSpecWeapons = sConfigMgr->GetBoolDefault("Playerbot.PreferredSpecWeapons", false);

    // —— 城市传送分布权重（否则权重全为0,机器人不会分散到各主城） ——
    probTeleToBankers = sConfigMgr->GetFloatDefault("Playerbot.ProbTeleToBankers", 0.25f);
    enableWeightTeleToCityBankers = sConfigMgr->GetBoolDefault("Playerbot.EnableWeightTeleToCityBankers", false);
    weightTeleToStormwind = sConfigMgr->GetIntDefault("Playerbot.TeleToStormwindWeight", 2);
    weightTeleToIronforge = sConfigMgr->GetIntDefault("Playerbot.TeleToIronforgeWeight", 1);
    weightTeleToDarnassus = sConfigMgr->GetIntDefault("Playerbot.TeleToDarnassusWeight", 1);
    weightTeleToExodar = sConfigMgr->GetIntDefault("Playerbot.TeleToExodarWeight", 1);
    weightTeleToOrgrimmar = sConfigMgr->GetIntDefault("Playerbot.TeleToOrgrimmarWeight", 2);
    weightTeleToUndercity = sConfigMgr->GetIntDefault("Playerbot.TeleToUndercityWeight", 1);
    weightTeleToThunderBluff = sConfigMgr->GetIntDefault("Playerbot.TeleToThunderBluffWeight", 1);
    weightTeleToSilvermoonCity = sConfigMgr->GetIntDefault("Playerbot.TeleToSilvermoonCityWeight", 1);
    weightTeleToShattrathCity = sConfigMgr->GetIntDefault("Playerbot.TeleToShattrathCityWeight", 1);
    weightTeleToDalaran = sConfigMgr->GetIntDefault("Playerbot.TeleToDalaranWeight", 1);

    // —— 集合类:禁用游戏对象/开门任务/不可获得物品 ——
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.DisallowedGameObjects", "176213,17155,2656,74448,19020,3719,3658,3705,3706,105579,75293,2857,179490,141596,160836,160845,179516,176224,181085,176112,128308,128403,165739,165738,175245,175970,176325,176327,123329,2560");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                disallowedGameObjects.insert(v);
        }
    }
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.AttunementQuests", "10279,10277,10282,10283,10284,10285,10296,10297,10298,11481,11482,11488,11490,11492,10901,10888,10445,10985");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                attunementQuests.insert(v);
        }
    }
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.UnobtainableItems", "12468,44869,44870,46978");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                unobtainableItems.insert(v);
        }
    }

    // —— 随机机器人任务/技能列表 ——
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.RandomBotQuestItems", "5175,5176,5177,5178,6948,11000,12382,13704,16309");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                randomBotQuestItems.push_back(v);
        }
    }
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.RandomBotSpellIds", "54197");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                randomBotSpellIds.push_back(v);
        }
    }
    {
        std::string s = sConfigMgr->GetStringDefault("Playerbot.RandomBotQuestIds", "3802,5505,6502,7761,7848,10277,10285,11492,13188,13189,24499,24511,24710,24712");
        for (auto& p : split(s, ','))
        {
            uint32 v = atoi(p.c_str());
            if (v > 0)
                randomBotQuestIds.push_back(v);
        }
    }

    // —— 战场/竞技场自动填充数量与档位(仅当RandomBotAutoJoinBG开启时生效) ——
    randomBotAutoJoinWSBrackets = sConfigMgr->GetStringDefault("Playerbot.RandomBotAutoJoinWSBrackets", "7");
    randomBotAutoJoinABBrackets = sConfigMgr->GetStringDefault("Playerbot.RandomBotAutoJoinABBrackets", "6");
    randomBotAutoJoinAVBrackets = sConfigMgr->GetStringDefault("Playerbot.RandomBotAutoJoinAVBrackets", "3");
    randomBotAutoJoinEYBrackets = sConfigMgr->GetStringDefault("Playerbot.RandomBotAutoJoinEYBrackets", "2");
    randomBotAutoJoinICBrackets = sConfigMgr->GetStringDefault("Playerbot.RandomBotAutoJoinICBrackets", "1");
    randomBotAutoJoinBGWSCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGWSCount", 1);
    randomBotAutoJoinBGABCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGABCount", 1);
    randomBotAutoJoinBGAVCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGAVCount", 0);
    randomBotAutoJoinBGEYCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGEYCount", 1);
    randomBotAutoJoinBGICCount = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGICCount", 0);
    randomBotAutoJoinBGRatedArena2v2Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGRatedArena2v2Count", 0);
    randomBotAutoJoinBGRatedArena3v3Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGRatedArena3v3Count", 0);
    randomBotAutoJoinBGRatedArena5v5Count = sConfigMgr->GetIntDefault("Playerbot.RandomBotAutoJoinBGRatedArena5v5Count", 0);
    //End By leewheel

    //By leewheel 2026-07-12: 补全缺失的初始化调用链（移植时遗漏了CreateRandomBots等关键初始化）
    // 使用server.loading频道输出进度, playerbots频道在控制台不可见
    uint32 initTimer = getMSTime();

    TC_LOG_INFO("server.loading", "  [1/7] 创建随机机器人账号和角色...");
    RandomPlayerbotFactory::CreateRandomBots();
    if (World::IsStopped())
    {
        return true;
    }
    TC_LOG_INFO("server.loading", "  [1/7] 完成 ({} ms)", GetMSTimeDiffToNow(initTimer));

    TC_LOG_INFO("server.loading", "  [2/7] 分配账号类型...");
    uint32 stepTimer = getMSTime();
    sRandomPlayerbotMgr.AssignAccountTypes();
    TC_LOG_INFO("server.loading", "  [2/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));

    //By leewheel 2026-07-13: 交换初始化顺序
    //PlayerbotGuildMgr::Init()必须在RandomPlayerbotMgr::Init()之前运行
    //因为DeleteBotGuilds()需要从playerbots_random_bots读取event='add'的bot列表
    //而RandomPlayerbotMgr::Init()会清空这些条目
    //AC中用Execute(异步)存在竞态条件让DeleteBotGuilds碰巧能读到数据
    //TC中用DirectExecute(同步)会立即清空,导致DeleteBotGuilds永远找不到bot
    //正确做法: 先执行公会清理(读取playerbots_random_bots),再清空playerbots_random_bots
    TC_LOG_INFO("server.loading", "  [3/7] 初始化公会/物品/BIS列表...");
    stepTimer = getMSTime();
    PlayerbotGuildMgr::instance().Init();
    sRandomItemMgr.Init();
    sRandomItemMgr.InitAfterAhBot();
    sBisListMgr->LoadAll();
    //By leewheel 2026-08-23: 合并 the-lab(#2637 Arena refactor) —— 启动时加载/创建机器人竞技场队伍
    sRandomPlayerbotMgr.InitArenaTeams();
    //End By leewheel
    TC_LOG_INFO("server.loading", "  [3/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));

    TC_LOG_INFO("server.loading", "  [4/7] 初始化随机机器人管理器...");
    stepTimer = getMSTime();
    if (sPlayerbotAIConfig.enabled)
    {
        sRandomPlayerbotMgr.Init();
    }
    TC_LOG_INFO("server.loading", "  [4/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));
    //End By leewheel

    TC_LOG_INFO("server.loading", "  [5/7] 加载机器人文本...");
    stepTimer = getMSTime();
    PlayerbotTextMgr::instance().LoadBotTexts();
    PlayerbotTextMgr::instance().LoadBotTextChance();
    TC_LOG_INFO("server.loading", "  [5/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));

    TC_LOG_INFO("server.loading", "  [6/7] 初始化AI上下文/旅行管理器...");
    stepTimer = getMSTime();
    //By leewheel 2026-07-28: 诊断6/7卡死 - 拆分Init/BuildAllSharedContexts为独立计时日志
    {
        uint32 subTimer = getMSTime();
        TC_LOG_INFO("server.loading", "  [6/7-1] 进入PlayerbotFactory::Init()...");
        PlayerbotFactory::Init();
        TC_LOG_INFO("server.loading", "  [6/7-1] PlayerbotFactory::Init()完成 ({} ms)", GetMSTimeDiffToNow(subTimer));
    }
    {
        uint32 subTimer = getMSTime();
        TC_LOG_INFO("server.loading", "  [6/7-2] 进入AiObjectContext::BuildAllSharedContexts()...");
        AiObjectContext::BuildAllSharedContexts();
        TC_LOG_INFO("server.loading", "  [6/7-2] AiObjectContext::BuildAllSharedContexts()完成 ({} ms)", GetMSTimeDiffToNow(subTimer));
    }
    //End By leewheel
    TC_LOG_INFO("server.loading", "  [6/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));

    TC_LOG_INFO("server.loading", "  [7/7] 加载副本数据...");
    stepTimer = getMSTime();
    if (sPlayerbotAIConfig.randomBotSuggestDungeons)
    {
        PlayerbotDungeonRepository::instance().LoadDungeonSuggestions();
    }
    TC_LOG_INFO("server.loading", "  [7/7] 完成 ({} ms)", GetMSTimeDiffToNow(stepTimer));

    //By leewheel 2026-07-14: sTravelMgr.Init()移至OnStartup中调用
    //因为PrepareDestinationCache需要sMapMgr->FindMap返回有效Map对象
    //而PreloadContinents在OnConfigLoad之后才执行
    //End By leewheel
    TC_LOG_INFO("server.loading", "机器人初始化总耗时: {} ms (旅行数据将在OnStartup中初始化)", GetMSTimeDiffToNow(initTimer));
    //End By leewheel

    //By leewheel 2026-08-15: 加载RandomBotLevelMgr配置(等级分档+满级重置)
    LoadRandomBotLevelConfig();
    //End By leewheel

    TC_LOG_INFO("server.loading", "机器人AI配置初始化完成。启用: {}", enabled ? "是" : "否");

    return true;
}

//By leewheel 2026-08-15: 移植the-lab LoadRandomBotLevelConfig——加载AiPlayerbot.LevelBrackets.*
//与AiPlayerbot.ResetBotLevel.*(见RandomBotLevelMgr)。配置重载(.reload config)时由
//RandomBotLevelWorldScript::OnAfterConfigLoad再次调用。分档边界/百分比仅为配置原值，
//RandomBotLevelMgr::LoadConfig()会复制到其自身工作状态(动态分布与钳位再平衡会改运行期百分比)
void PlayerbotAIConfig::LoadRandomBotLevelConfig()
{
    // ---- Level brackets ----
    levelBracketsEnabled = sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.Enabled", false);
    levelBracketsIgnoreGuildWithRealPlayers =
        sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.IgnoreGuildBotsWithRealPlayers", true);
    levelBracketsIgnoreArenaTeamBots =
        sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.IgnoreArenaTeamBots", true);

    levelBracketsCheckFrequency = sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.CheckFrequency", 300);
    levelBracketsFlaggedCheckFrequency =
        sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.CheckFlaggedFrequency", 15);
    levelBracketsDynamicDistribution =
        sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.Dynamic.UseDynamicDistribution", false);
    levelBracketsRealPlayerWeight =
        sConfigMgr->GetFloatDefault("Playerbot.LevelBrackets.Dynamic.RealPlayerWeight", 1.0f);
    levelBracketsSyncFactions = sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.Dynamic.SyncFactions", false);
    levelBracketsIgnoreFriendListed = sConfigMgr->GetBoolDefault("Playerbot.LevelBrackets.IgnoreFriendListed", true);
    levelBracketsFlaggedProcessLimit =
        sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.FlaggedProcessLimit", 5);

    levelBracketsExcludeNames.clear();
    {
        std::string const csv = sConfigMgr->GetStringDefault("Playerbot.LevelBrackets.ExcludeNames", "");
        std::istringstream f(csv);
        std::string s;
        while (getline(f, s, ','))
        {
            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
            if (!s.empty())
                levelBracketsExcludeNames.push_back(s);
        }
    }

    levelBracketsNumRanges =
        static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.NumRanges", 9));
    levelBracketsAlliance.resize(levelBracketsNumRanges);
    levelBracketsHorde.resize(levelBracketsNumRanges);

    for (uint8 i = 0; i < levelBracketsNumRanges; ++i)
    {
        std::string idx = std::to_string(i + 1);
        uint32 defaultLower = (i == 0 ? 1 : i * 10);
        uint32 defaultUpper = (i < levelBracketsNumRanges - 1 ? i * 10 + 9 : randomBotMaxLevel);
        levelBracketsAlliance[i].lower = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Alliance.Range" + idx + ".Lower", defaultLower));
        levelBracketsAlliance[i].upper = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Alliance.Range" + idx + ".Upper", defaultUpper));
        levelBracketsAlliance[i].pct = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Alliance.Range" + idx + ".Pct", 11));
    }

    for (uint8 i = 0; i < levelBracketsNumRanges; ++i)
    {
        std::string idx = std::to_string(i + 1);
        uint32 defaultLower = (i == 0 ? 1 : i * 10);
        uint32 defaultUpper = (i < levelBracketsNumRanges - 1 ? i * 10 + 9 : randomBotMaxLevel);
        levelBracketsHorde[i].lower = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Horde.Range" + idx + ".Lower", defaultLower));
        levelBracketsHorde[i].upper = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Horde.Range" + idx + ".Upper", defaultUpper));
        levelBracketsHorde[i].pct = static_cast<uint8>(
            sConfigMgr->GetIntDefault("Playerbot.LevelBrackets.Horde.Range" + idx + ".Pct", 11));
    }

    // 阵营档位不匹配会强制禁用SyncFactions并记日志，绝不让服务器崩溃
    if (levelBracketsSyncFactions)
    {
        for (uint8 i = 0; i < levelBracketsNumRanges; ++i)
        {
            if (levelBracketsAlliance[i].lower != levelBracketsHorde[i].lower ||
                levelBracketsAlliance[i].upper != levelBracketsHorde[i].upper)
            {
                TC_LOG_ERROR("server.loading",
                    "[RandomBotLevelMgr] Bracket mismatch detected between factions at index {}. Alliance: {}-{}, "
                    "Horde: {}-{}. SyncFactions requires both bracket count and min/max levels to match exactly; "
                    "forcibly disabling SyncFactions for this session. Check your configuration.",
                    i, levelBracketsAlliance[i].lower, levelBracketsAlliance[i].upper,
                    levelBracketsHorde[i].lower, levelBracketsHorde[i].upper);
                levelBracketsSyncFactions = false;
                break;
            }
        }
    }

    // ---- Level reset ----
    resetBotLevelEnabled = sConfigMgr->GetBoolDefault("Playerbot.ResetBotLevel.Enabled", false);

    resetBotLevelMaxLevel =
        static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.MaxLevel", 80));
    if ((resetBotLevelMaxLevel < 2 || resetBotLevelMaxLevel > 80) && resetBotLevelMaxLevel != 0)
    {
        TC_LOG_ERROR("server.loading",
            "[RandomBotLevelMgr] Invalid AiPlayerbot.ResetBotLevel.MaxLevel value: {}. Using default value 80.",
            resetBotLevelMaxLevel);
        resetBotLevelMaxLevel = 80;
    }

    resetBotLevelResetTo =
        static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.ResetToLevel", 1));
    if (resetBotLevelResetTo < 1 || (resetBotLevelMaxLevel > 0 && resetBotLevelResetTo >= resetBotLevelMaxLevel))
    {
        TC_LOG_ERROR("server.loading",
            "[RandomBotLevelMgr] Invalid AiPlayerbot.ResetBotLevel.ResetToLevel value: {}. Using default value 1.",
            resetBotLevelResetTo);
        resetBotLevelResetTo = 1;
    }

    resetBotLevelSkipFrom =
        static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.SkipFromLevel", 0));
    if (resetBotLevelSkipFrom > 80 || (resetBotLevelMaxLevel > 0 && resetBotLevelSkipFrom >= resetBotLevelMaxLevel))
    {
        TC_LOG_ERROR("server.loading",
            "[RandomBotLevelMgr] Invalid AiPlayerbot.ResetBotLevel.SkipFromLevel value: {}. Using default value 0 "
            "(disabled).",
            resetBotLevelSkipFrom);
        resetBotLevelSkipFrom = 0;
    }

    resetBotLevelSkipTo = static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.SkipToLevel", 1));
    if (resetBotLevelSkipTo < 1 || resetBotLevelSkipTo > 80 ||
        (resetBotLevelMaxLevel > 0 && resetBotLevelSkipTo > resetBotLevelMaxLevel))
    {
        TC_LOG_ERROR("server.loading",
            "[RandomBotLevelMgr] Invalid AiPlayerbot.ResetBotLevel.SkipToLevel value: {}. Using default value 1.",
            resetBotLevelSkipTo);
        resetBotLevelSkipTo = 1;
    }

    resetBotLevelChance =
        static_cast<uint8>(sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.ResetChance", 100));
    if (resetBotLevelChance > 100)
    {
        TC_LOG_ERROR("server.loading",
            "[RandomBotLevelMgr] Invalid AiPlayerbot.ResetBotLevel.ResetChance value: {}. Using default value 100.",
            resetBotLevelChance);
        resetBotLevelChance = 100;
    }

    resetBotLevelScaledChance = sConfigMgr->GetBoolDefault("Playerbot.ResetBotLevel.ScaledChance", false);

    resetBotLevelRestrictTimePlayed =
        sConfigMgr->GetBoolDefault("Playerbot.ResetBotLevel.RestrictTimePlayed", false);
    resetBotLevelMinTimePlayed = sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.MinTimePlayed", 86400);
    resetBotLevelPlayedTimeCheckFrequency =
        sConfigMgr->GetIntDefault("Playerbot.ResetBotLevel.PlayedTimeCheckFrequency", 864);

    resetBotLevelIgnoreGuildWithRealPlayers =
        sConfigMgr->GetBoolDefault("Playerbot.ResetBotLevel.IgnoreGuildBotsWithRealPlayers", false);

    resetBotLevelExcludeNames.clear();
    {
        std::string const csv = sConfigMgr->GetStringDefault("Playerbot.ResetBotLevel.ExcludeNames", "");
        std::istringstream f(csv);
        std::string s;
        while (getline(f, s, ','))
        {
            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
            if (!s.empty())
                resetBotLevelExcludeNames.push_back(s);
        }
    }
}
//End By leewheel

bool PlayerbotAIConfig::IsInRandomAccountList(uint32 id)
{
    return std::find(randomBotAccounts.begin(), randomBotAccounts.end(), id) != randomBotAccounts.end();
}

bool PlayerbotAIConfig::IsInRandomQuestItemList(uint32 id)
{
    return std::find(randomBotQuestItems.begin(), randomBotQuestItems.end(), id) != randomBotQuestItems.end();
}

bool PlayerbotAIConfig::IsPvpProhibited(uint32 zoneId, uint32 areaId)
{
    return IsInPvpProhibitedZone(zoneId) || IsInPvpProhibitedArea(areaId);
}

bool PlayerbotAIConfig::IsInPvpProhibitedZone(uint32 id)
{
    return std::find(pvpProhibitedZoneIds.begin(), pvpProhibitedZoneIds.end(), id) != pvpProhibitedZoneIds.end();
}

bool PlayerbotAIConfig::IsInPvpProhibitedArea(uint32 id)
{
    return std::find(pvpProhibitedAreaIds.begin(), pvpProhibitedAreaIds.end(), id) != pvpProhibitedAreaIds.end();
}

bool PlayerbotAIConfig::IsRestrictedHealerDPSMap(uint32 mapId) const
{
    //By leewheel 2026-09-04 对齐上游: 并入 restrictHealerDPS 总开关前置判断
    return restrictHealerDPS &&
        std::find(restrictedHealerDPSMaps.begin(), restrictedHealerDPSMaps.end(), mapId) != restrictedHealerDPSMaps.end();
}

//By leewheel 2026-09-04 对齐上游: 世界Buff矩阵加载(移植自 AC loadWorldBuff, 键名改 Playerbot.)——
//格式: "mapId:factionId,classId,specId,minLevel,maxLevel:spellId1,spellId2,...;条目2;..."
//factionId 0=双方; classId/specId 0=不限; minLevel/maxLevel 0=不限(消费点按 WorldBuffAction 语义判 0 跳过)
void PlayerbotAIConfig::loadWorldBuff()
{
    std::string matrix = sConfigMgr->GetStringDefault("Playerbot.WorldBuffMatrix", "");
    if (matrix.empty())
        return;

    std::istringstream entryStream(matrix);
    std::string entry;

    while (std::getline(entryStream, entry, ';'))
    {
        // 去首尾空白
        entry.erase(0, entry.find_first_not_of(" \t\r\n"));
        entry.erase(entry.find_last_not_of(" \t\r\n") + 1);

        size_t firstColon = entry.find(':');
        size_t secondColon = entry.find(':', firstColon + 1);

        if (firstColon == std::string::npos || secondColon == std::string::npos)
        {
            TC_LOG_ERROR("server.loading", "机器人世界Buff配置条目格式错误: [{}]", entry);
            continue;
        }

        std::string metaPart = entry.substr(firstColon + 1, secondColon - firstColon - 1);
        std::string spellPart = entry.substr(secondColon + 1);

        std::vector<uint32> ids;
        std::istringstream metaStream(metaPart);
        std::string token;
        while (std::getline(metaStream, token, ','))
        {
            try
            {
                ids.push_back(static_cast<uint32>(std::stoi(token)));
            }
            catch (...)
            {
                TC_LOG_ERROR("server.loading", "机器人世界Buff配置元数据无效: [{}]", entry);
                break;
            }
        }

        if (ids.size() != 5)
        {
            TC_LOG_ERROR("server.loading", "机器人世界Buff配置元数据不完整(需5项): [{}]", entry);
            continue;
        }

        std::istringstream spellStream(spellPart);
        while (std::getline(spellStream, token, ','))
        {
            try
            {
                WorldBuffData wb;
                wb.spellId = static_cast<uint32>(std::stoi(token));
                wb.factionId = ids[0];
                wb.classId = ids[1];
                wb.specId = ids[2];
                wb.minLevel = ids[3];
                wb.maxLevel = ids[4];
                worldBuffs.push_back(wb);            }
            catch (...)
            {
                TC_LOG_ERROR("server.loading", "机器人世界Buff配置法术ID无效: [{}]", entry);
            }
        }
    }

    TC_LOG_INFO("server.loading", "机器人世界Buff配置已加载 {} 条", worldBuffs.size());
}
//End By leewheel 2026-09-04

std::string const PlayerbotAIConfig::GetTimestampStr()
{
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    // Sized for the widest output snprintf can produce for these int conversions, so the
    // result is never truncated.
    char buf[128];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf);
}

bool PlayerbotAIConfig::openLog(std::string const fileName, char const* mode)
{
    auto it = logFiles.find(fileName);
    if (it != logFiles.end() && it->second.second)
        return true;

    FILE* file = fopen(fileName.c_str(), mode);
    if (!file)
        return false;

    logFiles[fileName] = std::make_pair(file, true);
    return true;
}

void PlayerbotAIConfig::log(std::string const fileName, const char* str, ...)
{
    std::lock_guard<std::mutex> lock(m_logMtx);
    auto it = logFiles.find(fileName);
    if (it == logFiles.end() || !it->second.second || !it->second.first)
        return;

    va_list ap;
    va_start(ap, str);
    vfprintf(it->second.first, str, ap);
    va_end(ap);
    fflush(it->second.first);
}

//By leewheel 2026-07-12: split已在Util/Helpers.h中定义，删除重复实现避免LNK2005
//End By leewheel

std::vector<std::vector<uint32>> PlayerbotAIConfig::ParseTempTalentsOrder(uint32 cls, std::string tab_link)
{
    // 检查无效链接
    uint32 classMask = 1 << (cls - 1);
    std::vector<std::vector<uint32>> res;
    std::vector<std::string> tab_links = split(tab_link, '-');
    std::map<uint32, std::vector<TalentEntry const*>> spells;
    std::vector<std::vector<std::vector<uint32>>> orders(3);

    for (TalentEntry const* talentInfo : sTalentStore)
    {
        if (!talentInfo)
            continue;

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TabID);
        if (!talentTabInfo)
            continue;

        //By leewheel 2026-09-09: 修复变量名拼写错误 talentTabinfo -> talentTabInfo
        if ((classMask & talentTabInfo->ClassMask) == 0)
            continue;

        spells[talentTabInfo->OrderIndex].push_back(talentInfo);
    }

    for (int tab = 0; tab < 3; tab++)
    {
        if (tab_links.size() <= (size_t)tab)
            break;

        std::sort(spells[tab].begin(), spells[tab].end(),
                  [&](TalentEntry const* lhs, TalentEntry const* rhs)
                  { return lhs->TierID != rhs->TierID ? lhs->TierID < rhs->TierID : lhs->ColumnIndex < rhs->ColumnIndex; });

        for (uint32 i = 0; i < tab_links[tab].size(); i++)
        {
            if (i >= spells[tab].size())
                break;

            int lvl = tab_links[tab][i] - '0';
            if (lvl == 0)
                continue;

            orders[tab].push_back({(uint32)tab, spells[tab][i]->TierID, spells[tab][i]->ColumnIndex, (uint32)lvl});
        }
    }

    // 按天赋标签页大小排序
    std::sort(orders.begin(), orders.end(), [&](auto& lhs, auto& rhs) { return lhs.size() > rhs.size(); });
    for (auto& order : orders)
    {
        res.insert(res.end(), order.begin(), order.end());
    }
    return res;
}

std::vector<std::vector<uint32>> PlayerbotAIConfig::ParseTempPetTalentsOrder(uint32 spec, std::string tab_link)
{
    std::vector<TalentEntry const*> spells;
    std::vector<std::vector<uint32>> orders;

    for (TalentEntry const* talentInfo : sTalentStore)
    {
        if (!talentInfo)
            continue;

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TabID);
        if (!talentTabInfo)
            continue;

        //By leewheel 2026-09-09: TC的TalentTabEntry无PetTalentMask字段，宠物天赋用OrderIndex匹配spec
        if (talentTabInfo->OrderIndex != (int32)spec)
            continue;

        // 跳过一些重复法术如冲刺/俯冲
        if (talentInfo->ID == 2201 || talentInfo->ID == 2208 || talentInfo->ID == 2219 ||
            talentInfo->ID == 2202 || talentInfo->ID == 2206 || talentInfo->ID == 2215)
            continue;

        spells.push_back(talentInfo);
    }

    std::sort(spells.begin(), spells.end(),
              [&](TalentEntry const* lhs, TalentEntry const* rhs)
              { return lhs->TierID != rhs->TierID ? lhs->TierID < rhs->TierID : lhs->ColumnIndex < rhs->ColumnIndex; });

    for (uint32 i = 0; i < tab_link.size(); i++)
    {
        if (i >= spells.size())
            break;

        int lvl = tab_link[i] - '0';
        if (lvl == 0)
            continue;

        orders.push_back({spells[i]->TierID, spells[i]->ColumnIndex, (uint32)lvl});
    }

    return orders;
}
