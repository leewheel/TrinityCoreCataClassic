/*
 * 机器人AI配置
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_PLAYERBOTAICONFIG_H
#define PLAYERBOTS_PLAYERBOTAICONFIG_H

#include <mutex>
#include <unordered_map>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <string>

#include "Define.h"
#include "SharedDefines.h"
#include "Common.h"

// ========== AC到TC API兼容宏 ==========
// TC的Unit/Player使用大写开头的GetClass/GetRace/GetGender
// AC使用小写开头的getClass/getRace/getGender
// 使用宏定义实现兼容，避免修改大量代码
#define getClass GetClass
#define getRace GetRace
#define getGender GetGender

// TC的SpellMgr::GetSpellInfo需要Difficulty参数
// AC的sSpellMgr->GetSpellInfo(id) 只需1个参数
// 使用兼容宏自动添加DIFFICULTY_NONE参数
#define GET_SPELL_INFO(spellId) sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE)

enum BotCheatMask : uint32
{
    none = 0,
    taxi = 1,
    gold = 2,
    health = 4,
    mana = 8,
    power = 16,
    raid = 32,
    food = 64,
    maxMask = 128
};

enum HealingManaEfficiency : uint8
{
    VERY_LOW = 1,
    LOW = 2,
    MEDIUM = 4,
    HIGH = 8,
    VERY_HIGH = 16,
    SUPERIOR = 32
};

enum AutoPartyBuffMode : uint8
{
    DISABLED = 0,
    RAID_ONLY = 1,
    GROUP_OR_RAID = 2
};

enum NewRpgStatus : int
{
    RPG_IDLE = 0,
    RPG_GO_GRIND = 1,
    RPG_GO_CAMP = 2,
    RPG_WANDER_RANDOM = 3,
    RPG_WANDER_NPC = 4,
    RPG_DO_QUEST = 5,
    RPG_TRAVEL_FLIGHT = 6,
    RPG_REST = 7,
    RPG_OUTDOOR_PVP = 8,
    RPG_STATUS_END = 9
};

#define MAX_SPECNO 20

//By leewheel 2026-08-01: 移植头盔/披风显示三级配置(7ebe7f06)——0=始终隐藏,1=始终显示,2=随机
enum class ShowHideCosmetic : uint8
{
    ALWAYS_HIDE = 0,
    ALWAYS_SHOW = 1,
    RANDOMIZE = 2
};
//End By leewheel

//By leewheel 2026-08-15: 移植the-lab RandomBotLevelMgr依赖——随机机器人等级分档的一个档位
//(等级区间+期望占比)。百分比为配置原值，运行时工作副本见RandomBotLevelMgr
struct LevelBracketConfig
{
    uint8 lower = 1;
    uint8 upper = 80;
    uint8 pct = 0;
};
//End By leewheel

class PlayerbotAIConfig
{
public:
    static PlayerbotAIConfig& instance()
    {
        static PlayerbotAIConfig instance;
        return instance;
    }

    bool Initialize();
    //By leewheel 2026-08-15: 加载AiPlayerbot.LevelBrackets.*与AiPlayerbot.ResetBotLevel.*(见RandomBotLevelMgr)
    void LoadRandomBotLevelConfig();
    //End By leewheel
    bool IsInRandomAccountList(uint32 id);
    bool IsInRandomQuestItemList(uint32 id);
    bool IsPvpProhibited(uint32 zoneId, uint32 areaId);
    bool IsInPvpProhibitedZone(uint32 id);
    bool IsInPvpProhibitedArea(uint32 id);

    bool enabled = false;
    bool disabledWithoutRealPlayer = false;
    bool EnableICCBuffs = false;
    //By leewheel 2026-07-26: 机器人技能名称解析诊断日志开关
    //控制 SpellIdValue::Calculate 里的 SpellIdValue:FAIL/OK 及 SpellIdValueDetail 刷屏日志
    //默认关闭，仅在排查"机器人找不到某技能"时临时开启
    bool spellIdDiagnosticLog = false;
    //End By leewheel
    bool allowAccountBots = false, allowGuildBots = false, allowTrustedAccountBots = false;
    bool randomBotGuildNearby = false, randomBotInvitePlayer = false, inviteChat = false;
    uint32 globalCoolDown = 0, reactDelay = 0, maxWaitForMove = 0, disableMoveSplinePath = 0, maxMovementSearchTime = 0, expireActionTime = 0,
        dispelAuraDuration = 0, passiveDelay = 0, repeatDelay = 0, errorDelay = 0, rpgDelay = 0, sitDelay = 0, returnDelay = 0, lootDelay = 0, lootPickupDelay = 0;
    bool dynamicReactDelay = false;
    float sightDistance = 0, spellDistance = 0, reactDistance = 0, grindDistance = 0, lootDistance = 0, shootDistance = 0, fleeDistance = 0,
        tooCloseDistance = 0, meleeDistance = 0, followDistance = 0, whisperDistance = 0, contactDistance = 0, aoeRadius = 0, rpgDistance = 0,
        targetPosRecalcDistance = 0, farDistance = 0, healDistance = 0, aggroDistance = 0;
    uint32 criticalHealth = 0, lowHealth = 0, mediumHealth = 0, almostFullHealth = 0;
    uint32 lowMana = 0, mediumMana = 0, highMana = 0;
    bool autoSaveMana = false;
    uint32 saveManaThreshold = 0;
    AutoPartyBuffMode autoGreaterBlessings = AutoPartyBuffMode::DISABLED;
    AutoPartyBuffMode autoPartyBuffs = AutoPartyBuffMode::DISABLED;
    bool tellWhenMissingBuffReagents = false;
    uint32 missingBuffReagentMessageCooldown = 0;
    bool autoAvoidAoe = false;
    float maxAoeAvoidRadius = 0;
    std::set<uint32> aoeAvoidSpellWhitelist;
    bool tellWhenAvoidAoe = false;
    std::set<uint32> disallowedGameObjects;
    std::set<uint32> attunementQuests;
    std::set<uint32> unobtainableItems;

    uint32 openGoSpell = 0;
    bool randomBotAutologin = false;
    bool botAutologin = false;
    std::string randomBotMapsAsString;
    float probTeleToBankers = 0;
    bool enableWeightTeleToCityBankers = false;
    int weightTeleToStormwind = 0;
    int weightTeleToIronforge = 0;
    int weightTeleToDarnassus = 0;
    int weightTeleToExodar = 0;
    int weightTeleToOrgrimmar = 0;
    int weightTeleToUndercity = 0;
    int weightTeleToThunderBluff = 0;
    int weightTeleToSilvermoonCity = 0;
    int weightTeleToShattrathCity = 0;
    int weightTeleToDalaran = 0;
    std::vector<uint32> randomBotMaps;
    std::vector<uint32> randomBotQuestItems;
    std::vector<uint32> randomBotAccounts;
    std::vector<uint32> randomBotSpellIds;
    std::vector<uint32> randomBotQuestIds;
    uint32 randomBotTeleportDistance = 0;
    //By leewheel 2026-07-11: 添加缺失的randomBotTeleLowerLevel/HigherLevel配置项
    uint32 randomBotTeleLowerLevel = 5;
    uint32 randomBotTeleHigherLevel = 5;
    //End By leewheel 2026-07-11
    float randomGearLoweringChance = 0;
    int32 randomGearQualityLimit = 0;
    int32 randomGearScoreLimit = 0;
    bool preferClassArmorType = false;
    bool preferredSpecWeapons = false;
    float randomBotMinLevelChance = 0, randomBotMaxLevelChance = 0;
    float randomBotRpgChance = 0;
    uint32 minRandomBots = 0, maxRandomBots = 0;
    uint32 randomBotUpdateInterval = 0, randomBotCountChangeMinInterval = 0, randomBotCountChangeMaxInterval = 0;
    uint32 minRandomBotInWorldTime = 0, maxRandomBotInWorldTime = 0;
    uint32 minRandomBotRandomizeTime = 0, maxRandomBotRandomizeTime = 0;
    uint32 minRandomBotChangeStrategyTime = 0, maxRandomBotChangeStrategyTime = 0;
    uint32 minRandomBotReviveTime = 0, maxRandomBotReviveTime = 0;
    uint32 minRandomBotTeleportInterval = 0, maxRandomBotTeleportInterval = 0;
    uint32 permanentlyInWorldTime = 0;
    uint32 minRandomBotPvpTime = 0, maxRandomBotPvpTime = 0;
    uint32 randomBotsPerInterval = 0;
    uint32 minRandomBotsPriceChangeInterval = 0, maxRandomBotsPriceChangeInterval = 0;
    uint32 disabledWithoutRealPlayerLoginDelay = 0, disabledWithoutRealPlayerLogoutDelay = 0;
    bool randomBotJoinLfg = false;

    bool randomBotTalk = false;
    bool randomBotEmote = false;
    bool randomBotSuggestDungeons = false;
    bool enableBroadcasts = false;
    bool enableGreet = false;
    bool randomBotSayWithoutMaster = false;

    // 广播概率配置
    uint32 broadcastChanceMaxValue = 10000;
    uint32 broadcastChanceLootingItemPoor = 100;
    uint32 broadcastChanceLootingItemNormal = 200;
    uint32 broadcastChanceLootingItemUncommon = 4000;
    uint32 broadcastChanceLootingItemRare = 6000;
    uint32 broadcastChanceLootingItemEpic = 8000;
    uint32 broadcastChanceLootingItemLegendary = 9500;
    uint32 broadcastChanceLootingItemArtifact = 10000;
    uint32 broadcastChanceQuestAccepted = 3000;
    uint32 broadcastChanceQuestUpdateObjectiveProgress = 4000;
    uint32 broadcastChanceQuestUpdateObjectiveCompleted = 5000;
    uint32 broadcastChanceQuestUpdateFailedTimer = 1000;
    uint32 broadcastChanceQuestUpdateComplete = 6000;
    uint32 broadcastChanceQuestTurnedIn = 8000;
    uint32 broadcastChanceKillPet = 1000;
    uint32 broadcastChanceKillPlayer = 3000;
    uint32 broadcastChanceKillNormal = 500;
    uint32 broadcastChanceKillElite = 2000;
    uint32 broadcastChanceKillRareelite = 4000;
    uint32 broadcastChanceKillWorldboss = 8000;
    uint32 broadcastChanceKillRare = 6000;
    uint32 broadcastChanceKillUnknown = 1000;
    uint32 broadcastChanceLevelupMaxLevel = 9000;
    uint32 broadcastChanceLevelupTenX = 7000;
    uint32 broadcastChanceLevelupGeneric = 3000;
    uint32 broadcastChanceGuildManagement = 2000;
    uint32 broadcastChanceSuggestInstance = 1000;
    uint32 broadcastChanceSuggestQuest = 1000;
    uint32 broadcastChanceSuggestGrindMaterials = 1000;
    uint32 broadcastChanceSuggestGrindReputation = 1000;
    uint32 broadcastChanceSuggestSell = 1000;
    uint32 broadcastChanceSuggestSomething = 2000;
    uint32 broadcastChanceSuggestSomethingToxic = 500;
    uint32 broadcastChanceSuggestToxicLinks = 100;
    uint32 broadcastChanceSuggestThunderfury = 100;

    // 广播频道全局概率配置
    uint32 broadcastToGuildGlobalChance = 30000;
    uint32 broadcastToWorldGlobalChance = 30000;
    uint32 broadcastToGeneralGlobalChance = 30000;
    uint32 broadcastToTradeGlobalChance = 30000;
    uint32 broadcastToLFGGlobalChance = 30000;
    uint32 broadcastToLocalDefenseGlobalChance = 30000;
    uint32 broadcastToWorldDefenseGlobalChance = 30000;
    uint32 broadcastToGuildRecruitmentGlobalChance = 30000;

    // 钓鱼距离配置
    float fishingDistanceFromMaster = 10.0f;
    float fishingDistance = 40.0f;
    float endFishingWithMaster = 30.0f;
    //By leewheel 2026-07-10: 添加enableFishingWithMaster配置项
    bool enableFishingWithMaster = true;
    //End By leewheel
    //By leewheel 2026-07-26: 恢复参考项目的职业匹配概率配置项(移植时丢失、被硬编码50)。
    //默认100表示完全按职业匹配商业技能,杜绝战士学裁缝/法师学锻造等错乱组合。
    uint32 classMatchingProfessionChance = 100;
    //End By leewheel

    // 有毒链接前缀
    std::string toxicLinksPrefix;
    //By leewheel 2026-07-11: 添加toxicLinks和thunderfury回复概率配置
    uint32 toxicLinksRepliesChance = 0;
    uint32 thunderfuryRepliesChance = 0;
    //End By leewheel

    bool randomBotJoinBG = false;
    bool randomBotAutoJoinBG = false;

    // 竞技场和战场自动加入配置
    uint32 randomBotAutoJoinBGEYCount = 0;
    uint32 randomBotAutoJoinBGAVCount = 0;
    uint32 randomBotAutoJoinBGABCount = 0;
    uint32 randomBotAutoJoinBGWSCount = 0;
    uint32 randomBotAutoJoinBGICCount = 0;
    uint32 randomBotAutoJoinBGRatedArena2v2Count = 0;
    uint32 randomBotAutoJoinBGRatedArena3v3Count = 0;
    uint32 randomBotAutoJoinBGRatedArena5v5Count = 0;
    //By leewheel 2026-07-11: 添加缺失的randomBotAutoJoinArenaBracket配置项
    uint32 randomBotAutoJoinArenaBracket = 0;
    //End By leewheel 2026-07-11
    std::string randomBotAutoJoinEYBrackets;
    std::string randomBotAutoJoinAVBrackets;
    std::string randomBotAutoJoinABBrackets;
    std::string randomBotAutoJoinWSBrackets;
    std::string randomBotAutoJoinICBrackets;

    bool logInGroupOnly = false, logValuesPerTick = false;
    bool fleeingEnabled = false;
    bool summonAtInnkeepersEnabled = false;
    std::string combatStrategies, nonCombatStrategies;
    std::string randomBotCombatStrategies, randomBotNonCombatStrategies;
    bool applyInstanceStrategies = false;
    uint32 randomBotMinLevel = 0, randomBotMaxLevel = 0;
    float randomChangeMultiplier = 0;
    //By leewheel 2026-08-15: 对齐the-lab——随机机器人统计输出间隔(可设0关闭)、公会广播回复频率
    int32 randomBotPrintStatsInterval = 300;
    int32 guildRepliesRate = 100;
    //End By leewheel

    //By leewheel 2026-08-15: 移植the-lab RandomBotLevelMgr 29个配置项——
    //等级分档(LevelBrackets) + 满级重置(ResetBotLevel)两个子功能
    // Level brackets（百分比为as-configured值，运行时工作副本在RandomBotLevelMgr）
    bool levelBracketsEnabled = false;
    uint32 levelBracketsCheckFrequency = 300;
    uint32 levelBracketsFlaggedCheckFrequency = 15;
    uint32 levelBracketsFlaggedProcessLimit = 5;
    bool levelBracketsIgnoreGuildWithRealPlayers = true;
    bool levelBracketsIgnoreArenaTeamBots = true;
    bool levelBracketsIgnoreFriendListed = true;
    std::vector<std::string> levelBracketsExcludeNames;
    uint8 levelBracketsNumRanges = 9;
    std::vector<LevelBracketConfig> levelBracketsAlliance;
    std::vector<LevelBracketConfig> levelBracketsHorde;
    bool levelBracketsDynamicDistribution = false;
    float levelBracketsRealPlayerWeight = 1.0f;
    bool levelBracketsSyncFactions = false;

    // Level reset（随机机器人达到满级后重置）
    bool resetBotLevelEnabled = false;
    uint8 resetBotLevelMaxLevel = 80;
    uint8 resetBotLevelResetTo = 1;
    uint8 resetBotLevelSkipFrom = 0;
    uint8 resetBotLevelSkipTo = 1;
    uint8 resetBotLevelChance = 100;
    bool resetBotLevelScaledChance = false;
    bool resetBotLevelRestrictTimePlayed = false;
    uint32 resetBotLevelMinTimePlayed = 86400;
    uint32 resetBotLevelPlayedTimeCheckFrequency = 864;
    bool resetBotLevelIgnoreGuildWithRealPlayers = false;
    std::vector<std::string> resetBotLevelExcludeNames;
    //End By leewheel

    std::string commandPrefix, commandSeparator;
    std::string randomBotAccountPrefix;
    uint32 randomBotAccountCount = 0;
    bool randomBotRandomPassword = false;
    bool deleteRandomBotAccounts = false;
    uint32 randomBotGuildCount = 0, randomBotGuildSizeMax = 0;
    bool deleteRandomBotGuilds = false;
    std::vector<uint32> pvpProhibitedZoneIds;
    std::vector<uint32> pvpProhibitedAreaIds;
    bool fastReactInBG = false;

    bool randombotsWalkingRPG = false;
    bool randombotsWalkingRPGInDoors = false;
    uint32 minEnchantingBotLevel = 0;
    uint32 limitEnchantExpansion = 0;
    uint32 limitGearExpansion = 0;
    uint32 randombotStartingLevel = 0;
    bool enablePeriodicOnlineOffline = false;
    float periodicOnlineOfflineRatio = 0;
    bool gearscorecheck = false;
    bool randomBotPreQuests = false;
    bool botSendMailEnabled = false;

    bool guildTaskEnabled = false;
    uint32 minGuildTaskChangeTime = 0, maxGuildTaskChangeTime = 0;
    uint32 minGuildTaskAdvertisementTime = 0, maxGuildTaskAdvertisementTime = 0;
    uint32 minGuildTaskRewardTime = 0, maxGuildTaskRewardTime = 0;
    uint32 guildTaskAdvertCleanupTime = 0;
    uint32 guildTaskKillTaskDistance = 0;

    uint32 iterationsPerTick = 0;

    std::mutex m_logMtx;
    bool enableAutoTradeOnItemMention = false;
    std::vector<std::string> tradeActionExcludedPrefixes;
    std::vector<std::string> allowedLogFiles;
    std::unordered_map<std::string, std::pair<FILE*, bool>> logFiles;

    std::vector<std::string> botCheats;
    uint32 botCheatMask = 0;

    uint32 commandServerPort = 0;
    bool perfMonEnabled = false;
    bool summonWhenGroup = false;
    //By leewheel 2026-08-01: 按上游(7ebe7f06)从bool改为三态枚举
    ShowHideCosmetic randomBotShowHelmet = ShowHideCosmetic::ALWAYS_SHOW;
    ShowHideCosmetic randomBotShowCloak = ShowHideCosmetic::ALWAYS_SHOW;
    //End By leewheel
    bool randomBotFixedLevel = false;
    bool disableRandomLevels = false;
    float randomBotXPRate = 1.0f;
    uint32 randomBotAllianceRatio = 0;
    uint32 randomBotHordeRatio = 0;
    bool disableDeathKnightLogin = false;
    bool limitTalentsExpansion = false;
    uint32 botActiveAlone = 0;
    uint32 BotActiveAloneDurationSeconds = 0;
    uint32 BotActiveAloneForceWhenInRadius = 0;
    bool BotActiveAloneForceWhenInZone = false;
    bool BotActiveAloneForceWhenInMap = false;
    bool BotActiveAloneForceWhenIsFriend = false;
    bool BotActiveAloneForceWhenInGuild = false;
    bool botActiveAloneSmartScale = false;
    uint32 botActiveAloneSmartScaleDiffLimitfloor = 0;
    uint32 botActiveAloneSmartScaleDiffLimitCeiling = 0;
    uint32 botActiveAloneSmartScaleWhenMinLevel = 0;
    uint32 botActiveAloneSmartScaleWhenMaxLevel = 0;

    bool freeMethodLoot = false;
    int32 lootNeedRollLevel = 0;
    bool lootGreedRollLevel = false;
    bool lootRollRecipe = false;
    bool lootRollDisenchant = false;
    std::string autoPickReward;
    bool autoEquipUpgradeLoot = false;
    float equipUpgradeThreshold = 0;
    bool twoRoundsGearInit = false;
    bool syncQuestWithPlayer = false;
    bool syncQuestForPlayer = false;
    bool dropObsoleteQuests = false;
    bool allowLearnTrainerSpells = false;
    bool autoPickTalents = false;
    bool autoUpgradeEquip = false;
    int32 hunterWolfPet = 0;
    int32 defaultPetStance = 0;
    int32 petChatCommandDebug = 0;
    bool autoLearnTrainerSpells = false;
    bool autoDoQuests = false;
    bool enableNewRpgStrategy = false;
    std::unordered_map<NewRpgStatus, uint32> RpgStatusProbWeight;
    bool syncLevelWithPlayers = false;
    //By leewheel 2026-09-05: 上游164335fe——随机bot传送集中到有真实玩家在线的等级合适区域(可选)
    bool randomBotConcentrateInPlayerZone = false;
    //End By leewheel
    bool autoLearnQuestSpells = false;
    bool autoTeleportForLevel = false;
    bool randomBotGroupNearby = false;
    int32 enableRandomBotTrading = 0;
    uint32 tweakValue = 0;

    //By leewheel 2026-08-23: 合并 the-lab(#2571) —— ReadyCheck 前强制补增益
    bool forceRebuffOnReadyCheck = false;
    uint32 forceRebuffMarginSecs = 60;
    //End By leewheel

    //By leewheel 2026-08-23: the-lab(#2637) 废弃 randomBotArenaTeamCount/randomBotArenaTeams(Arena 重构后不再使用)
    uint32 randomBotArenaTeamMaxRating = 0;
    uint32 randomBotArenaTeamMinRating = 0;
    uint32 randomBotArenaTeam2v2Count = 0;
    uint32 randomBotArenaTeam3v3Count = 0;
    uint32 randomBotArenaTeam5v5Count = 0;
    bool deleteRandomBotArenaTeams = false;

    uint32 selfBotLevel = 0;
    bool downgradeMaxLevelBot = false;
    bool equipAndSpecPersistence = false;
    int32 equipAndSpecPersistenceLevel = 0;
    int32 groupInvitationPermission = 0;
    bool keepAltsInGroup = false;
    bool KeepAltsInGroup() const { return keepAltsInGroup; }
    bool allowSummonInCombat = false;
    bool allowSummonWhenMasterIsDead = false;
    bool allowSummonWhenBotIsDead = false;
    int reviveBotWhenSummoned = 0;
    bool botRepairWhenSummon = false;
    bool autoInitOnly = false;
    bool resetInstanceIdForAltBots = false;
    float autoInitEquipLevelLimitRatio = 0;
    int32 maxAddedBots = 0;
    int32 addClassCommand = 0;
    int32 addClassAccountPoolSize = 0;
    int32 maintenanceCommand = 0;
    int32 autoGearCommand = 0, autoGearCommandAltBots = 0, autoGearQualityLimit = 0, autoGearScoreLimit = 0;
    int32 autoGearBisCommand = 0;

    //By leewheel 20260710: AC兼容 - 替代维护配置成员
    bool altMaintenanceAttunementQs = false;
    bool altMaintenanceBags = false;
    bool altMaintenanceAmmo = false;
    bool altMaintenanceFood = false;
    bool altMaintenanceReagents = false;
    bool altMaintenanceConsumables = false;
    bool altMaintenancePotions = false;
    bool altMaintenanceTalentTree = false;
    bool altMaintenancePet = false;
    bool altMaintenancePetTalents = false;
    bool altMaintenanceClassSpells = false;
    bool altMaintenanceAvailableSpells = false;
    bool altMaintenanceSkills = false;
    bool altMaintenanceReputation = false;
    bool altMaintenanceSpecialSpells = false;
    bool altMaintenanceMounts = false;
    bool altMaintenanceGlyphs = false;
    bool altMaintenanceKeyring = false;
    bool altMaintenanceGemsEnchants = false;
    //End By leewheel

    uint32 useGroundMountAtMinLevel = 0;
    uint32 useFastGroundMountAtMinLevel = 0;
    uint32 useFlyMountAtMinLevel = 0;
    uint32 useFastFlyMountAtMinLevel = 0;

    uint32 botTaxiDelayMin = 0;
    uint32 botTaxiDelayMax = 0;
    uint32 botTaxiGapMs = 0;
    uint32 botTaxiGapJitterMs = 0;

    bool restrictHealerDPS = false;
    std::vector<uint32> restrictedHealerDPSMaps;
    bool IsRestrictedHealerDPSMap(uint32 mapId) const;

    std::vector<uint32> excludedHunterPetFamilies;

    //By leewheel 2026-09-04 对齐上游: 区域等级区间配置覆盖(ZoneBracket.<zoneId>="min,max"),
    //TravelMgr zone2LevelBracket 表的配置层覆盖消费点使用
    std::map<uint32, std::pair<uint32, uint32>> zoneBrackets;
    //End By leewheel 2026-09-04

    // 预设天赋配置
    std::string premadeSpecName[MAX_CLASSES][MAX_SPECNO];
    std::string premadeSpecGlyph[MAX_CLASSES][MAX_SPECNO];
    std::vector<uint32> parsedSpecGlyph[MAX_CLASSES][MAX_SPECNO];
    std::string premadeSpecLink[MAX_CLASSES][MAX_SPECNO][MAX_LEVEL];
    std::string premadeHunterPetLink[3][21];
    std::vector<std::vector<uint32>> parsedSpecLinkOrder[MAX_CLASSES][MAX_SPECNO][MAX_LEVEL];
    std::vector<std::vector<uint32>> parsedHunterPetLinkOrder[3][21];
    uint32 randomClassSpecProb[MAX_CLASSES][MAX_SPECNO];
    uint32 randomClassSpecIndex[MAX_CLASSES][MAX_SPECNO];

    std::string const GetTimestampStr();
    bool hasLog(std::string const fileName)
    {
        return std::find(allowedLogFiles.begin(), allowedLogFiles.end(), fileName) != allowedLogFiles.end();
    };
    bool openLog(std::string const fileName, char const* mode = "a");
    bool isLogOpen(std::string const fileName)
    {
        auto it = logFiles.find(fileName);
        return it != logFiles.end() && it->second.second;
    }
    void log(std::string const fileName, const char* str, ...);

    void loadWorldBuff();

    //By leewheel 2026-07-09: AC兼容 - 世界Buff数据结构
    struct WorldBuffData
    {
        uint32 spellId = 0;
        uint32 factionId = 0;
        uint32 classId = 0;
        uint32 specId = 0;
        uint32 minLevel = 0;
        uint32 maxLevel = 0;
    };
    std::vector<WorldBuffData> worldBuffs;
    //End By leewheel

    static std::vector<std::vector<uint32>> ParseTempTalentsOrder(uint32 cls, std::string temp_talents_order);
    static std::vector<std::vector<uint32>> ParseTempPetTalentsOrder(uint32 spec, std::string temp_talents_order);

private:
    PlayerbotAIConfig() = default;
    ~PlayerbotAIConfig() = default;

    PlayerbotAIConfig(const PlayerbotAIConfig&) = delete;
    PlayerbotAIConfig& operator=(const PlayerbotAIConfig&) = delete;

    PlayerbotAIConfig(PlayerbotAIConfig&&) = delete;
    PlayerbotAIConfig& operator=(PlayerbotAIConfig&&) = delete;
};

#define sPlayerbotAIConfig PlayerbotAIConfig::instance()

#endif
