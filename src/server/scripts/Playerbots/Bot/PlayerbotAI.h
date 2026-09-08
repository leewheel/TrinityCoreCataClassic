/*
 * 机器人AI核心
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_PLAYERBOTAI_H
#define PLAYERBOTS_PLAYERBOTAI_H

//By leewheel 2026-08-23: 合并 the-lab(#2571) —— 引入 ForceRebuff(补增益)状态类
#include "ForceRebuff.h"
//End By leewheel
#include <stack>
#include <functional>

#include "PlayerbotAIBase.h"
#include "ObjectGuid.h"
#include "Chat.h"
#include "ChatHelper.h"
#include "ChatFilter.h"
#include "Event.h"
#include "PlayerbotAIConfig.h"
#include "../Mgr/Security/PlayerbotSecurity.h"
#include "../Ai/World/Rpg/NewRpgInfo.h"
#include <string>
#include <sstream>
#include <map>

class Player;
class Unit;
class Creature;
class GameObject;
class WorldObject;
class WorldPacket;
//By leewheel 2026-08-01: GetMainTankGuid参数需要Group前置声明
class Group;
//End By leewheel
class AiObjectContext;
class Engine;
class ExternalEventHelper;
class Item;
class Spell;
class SpellInfo;
class Aura;
class WorldPosition;
struct AreaTableEntry;
struct CreatureData;
struct GameObjectData;
struct ItemTemplate;

enum SkillType;
enum StrategyType : uint32;

// ========== 活动类型枚举 ==========
enum ActivityType
{
    GRIND_ACTIVITY = 1,
    RPG_ACTIVITY = 2,
    TRAVEL_ACTIVITY = 3,
    OUT_OF_PARTY_ACTIVITY = 4,
    PACKET_ACTIVITY = 5,
    DETAILED_MOVE_ACTIVITY = 6,
    PARTY_ACTIVITY = 7,
    ALL_ACTIVITY = 8,
    MAX_ACTIVITY_TYPE
};

enum BotRoles : uint8
{
    BOT_ROLE_NONE = 0x00,
    BOT_ROLE_TANK = 0x01,
    BOT_ROLE_HEALER = 0x02,
    BOT_ROLE_DPS = 0x04
};

enum BotState
{
    BOT_STATE_COMBAT = 0,
    BOT_STATE_NON_COMBAT = 1,
    BOT_STATE_DEAD = 2,
    BOT_STATE_MAX
};

// ========== 聊天频道来源 ==========
enum ChatChannelSource
{
    SRC_GUILD,
    SRC_WORLD,
    SRC_GENERAL,
    SRC_TRADE,
    SRC_LOOKING_FOR_GROUP,
    SRC_LOCAL_DEFENSE,
    SRC_WORLD_DEFENSE,
    SRC_GUILD_RECRUITMENT,
    SRC_SAY,
    SRC_WHISPER,
    SRC_EMOTE,
    SRC_TEXT_EMOTE,
    SRC_YELL,
    SRC_PARTY,
    SRC_RAID,
    SRC_UNDEFINED
};

enum ChatChannelId
{
    GENERAL = 1,
    TRADE = 2,
    LOCAL_DEFENSE = 22,
    WORLD_DEFENSE = 23,
    LOOKING_FOR_GROUP = 26,
    GUILD_RECRUITMENT = 25,
};

// ========== 职业天赋树枚举 ==========
enum HUNTER_TABS
{
    HUNTER_TAB_BEAST_MASTERY,
    HUNTER_TAB_MARKSMANSHIP,
    HUNTER_TAB_SURVIVAL,
};

enum ROGUE_TABS
{
    ROGUE_TAB_ASSASSINATION,
    ROGUE_TAB_COMBAT,
    ROGUE_TAB_SUBTLETY,
};

enum PRIEST_TABS
{
    PRIEST_TAB_DISCIPLINE,
    PRIEST_TAB_HOLY,
    PRIEST_TAB_SHADOW,
};

enum DEATH_KNIGHT_TABS
{
    DEATH_KNIGHT_TAB_BLOOD,
    DEATH_KNIGHT_TAB_FROST,
    DEATH_KNIGHT_TAB_UNHOLY,
};

enum DRUID_TABS
{
    DRUID_TAB_BALANCE,
    DRUID_TAB_FERAL,
    DRUID_TAB_RESTORATION,
};

enum MAGE_TABS
{
    MAGE_TAB_ARCANE,
    MAGE_TAB_FIRE,
    MAGE_TAB_FROST,
};

enum SHAMAN_TABS
{
    SHAMAN_TAB_ELEMENTAL,
    SHAMAN_TAB_ENHANCEMENT,
    SHAMAN_TAB_RESTORATION,
};

enum PALADIN_TABS
{
    PALADIN_TAB_HOLY,
    PALADIN_TAB_PROTECTION,
    PALADIN_TAB_RETRIBUTION,
};

enum WARLOCK_TABS
{
    WARLOCK_TAB_AFFLICTION,
    WARLOCK_TAB_DEMONOLOGY,
    WARLOCK_TAB_DESTRUCTION,
};

enum WARRIOR_TABS
{
    WARRIOR_TAB_ARMS,
    WARRIOR_TAB_FURY,
    WARRIOR_TAB_PROTECTION,
};

// ========== 分组类型 ==========
enum BotTypeNumber : uint8
{
    ACTIVITY_TYPE_NUMBER = 1,
    GROUPER_TYPE_NUMBER = 2,
    GUILDER_TYPE_NUMBER = 3,
};

enum class GrouperType : uint8
{
    SOLO = 0,
    MEMBER = 1,
    LEADER_2 = 2,
    LEADER_3 = 3,
    LEADER_4 = 4,
    LEADER_5 = 5
};

enum class GuilderType : uint8
{
    SOLO = 0,
    TINY = 30,
    SMALL = 50,
    MEDIUM = 70,
    LARGE = 120,
    VERY_LARGE = 250
};

// ========== 治疗物品ID ==========
enum HealingItemId
{
    HEALTHSTONE = 5512,
    MAJOR_HEALING_POTION = 13446,
    WHIPPER_ROOT_TUBER = 11951,
    NIGHT_DRAGON_BREATH = 11952,
    LIMITED_INVULNERABILITY_POTION = 3387,
    GREATER_DREAMLESS_SLEEP_POTION = 22886,
    SUPERIOR_HEALING_POTION = 3928,
    CRYSTAL_RESTORE = 11564,
    DREAMLESS_SLEEP_POTION = 12190,
    GREATER_HEALING_POTION = 1710,
    HEALING_POTION = 929,
    LESSER_HEALING_POTION = 858,
    DISCOLORED_HEALING_POTION = 3391,
    MINOR_HEALING_POTION = 118,
    VOLATILE_HEALING_POTION = 28100,
    SUPER_HEALING_POTION = 22829,
    CRYSTAL_HEALING_POTION = 13462,
    FEL_REGENERATION_POTION = 28101,
    MAJOR_DREAMLESS_SLEEP_POTION = 20002
};

// ========== 聊天队列回复 ==========
//By leewheel 2026-07-11: 更新ChatQueuedReply结构体匹配AC代码中的成员名
struct ChatQueuedReply
{
    uint32 m_type = 0;
    uint32 m_guid1 = 0;
    uint32 m_guid2 = 0;
    std::string m_msg;
    std::string m_chanName;
    std::string m_name;
    time_t m_time = 0;
};
//End By leewheel

// ========== 数据包处理辅助类 ==========
class PacketHandlingHelper
{
public:
    void AddHandler(uint16 opcode, std::string const handler);
    void Handle(ExternalEventHelper& helper);
    void AddPacket(WorldPacket const& packet);

private:
    std::map<uint16, std::string> handlers;
    std::stack<WorldPacket> queue;
};

// ========== 聊天命令持有者 ==========
class ChatCommandHolder
{
public:
    ChatCommandHolder(std::string const command, Player* owner = nullptr, uint32 type = CHAT_MSG_WHISPER,
                      time_t time = 0)
        : command(command), owner(owner), type(type), time(time)
    {
    }
    ChatCommandHolder(ChatCommandHolder const& other)
        : command(other.command), owner(other.owner), type(other.type), time(other.time)
    {
    }

    const std::string& GetCommand() { return command; }
    Player* GetOwner() { return owner; }
    uint32& GetType() { return type; }
    time_t& GetTime() { return time; }

private:
    std::string const command;
    Player* owner;
    uint32 type;
    time_t time;
};

// ========== 复合聊天过滤器前向声明 ==========
class CompositeChatFilter;

// 聊天命令处理器 - 从 AC 移植，提供法术/任务链接解析
class PlayerbotChatHandler : protected ChatHandler
{
public:
    explicit PlayerbotChatHandler(Player* pMasterPlayer) : ChatHandler(pMasterPlayer->GetSession()) {}

    // 从聊天链接中提取法术ID
    uint32 extractSpellId(std::string const str)
    {
        std::string text = str;
        uint32 spellId = 0;

        size_t pos = text.find("Hspell:");
        if (pos != std::string::npos)
        {
            pos += 7;
            spellId = atoi(text.substr(pos).c_str());
        }

        if (spellId)
            return spellId;

        pos = text.find("Htalent:");
        if (pos != std::string::npos)
        {
            pos += 8;
            uint32 talentId = atoi(text.substr(pos).c_str());
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if (talentInfo)
                return talentInfo->SpellID;
        }

        pos = text.find("Henchant:");
        if (pos != std::string::npos)
        {
            pos += 9;
            spellId = atoi(text.substr(pos).c_str());
        }

        return spellId;
    }

    // 从聊天链接中提取任务ID
    uint32 extractQuestId(std::string const str)
    {
        std::string text = str;
        size_t pos = text.find("Hquest:");
        if (pos == std::string::npos)
            return 0;

        pos += 7;
        return atoi(text.substr(pos).c_str());
    }
};

class MinValueCalculator
{
public:
    MinValueCalculator(float def = 0.0f) : param(nullptr), minValue(def) {}

    void probe(float value, void* p)
    {
        if (!param || minValue >= value)
        {
            minValue = value;
            param = p;
        }
    }

    void* param;
    float minValue;
};

class PlayerbotAI : public PlayerbotAIBase
{
public:
    PlayerbotAI(Player* bot);
    virtual ~PlayerbotAI();

    //By leewheel 2026-07-11: 添加UpdateAI声明，基类PlayerbotAIBase有virtual UpdateAI
    void UpdateAI(uint32 elapsed, bool minimal = false) override;
    //End By leewheel
    void UpdateAIInternal(uint32 elapsed, bool minimal = false) override;

    // 命令处理
    void HandleCommand(uint32 type, std::string const text, Player* fromPlayer);
    void QueueChatResponse(const ChatQueuedReply reply);
    std::string const HandleRemoteCommand(std::string const command);

    // 数据包处理
    void HandleBotOutgoingPacket(WorldPacket const& packet);
    void HandleMasterIncomingPacket(WorldPacket const& packet);
    void HandleMasterOutgoingPacket(WorldPacket const& packet);
    void HandleTeleportAck();

    //By leewheel 2026-07-12: 改为inline const匹配AC原始设计，避免链接器找不到实现
    // 状态查询
    // 检查bot是否真的是玩家。玩家总是以自己为master。
    bool IsRealPlayer() const { return master ? (master == bot) : false; }
    //End By leewheel
    bool HasRealPlayerMaster();
    //By leewheel 2026-08-01: 按上游(4fb82ed0)统一bot判定命名约定，IsAlt改为IsAltBot
    bool IsAltBot();
    //End By leewheel
    Player* GetBot() const { return bot; }
    Player* GetMaster() const { return master; }
    void SetMaster(Player* master) { this->master = master; }

    //By leewheel 2026-07-12: 改为inline const匹配AC原始设计
    // AI 上下文
    AiObjectContext* GetAiObjectContext() const { return aiObjectContext; }
    //End By leewheel
    void SetAiObjectContext(AiObjectContext* context) { aiObjectContext = context; }

    //By leewheel 2026-07-11: GetEngine改为inline，匹配AC设计
    // 引擎
    Engine* GetEngine() const { return engine; }
    //End By leewheel
    Engine* GetCurrentUserdata() const;
    void SetEngine(Engine* engine) { this->engine = engine; }
    void SetCurrentEngine(Engine* engine) { currentEngine = engine; }
    Engine* GetEngine(BotState type) const { return engines[type]; }
    void SetEngine(BotState type, Engine* engine);

    // 引擎控制
    void ChangeEngine(BotState type);
    void ChangeEngineOnCombat();
    void ChangeEngineOnNonCombat();
    void DoNextAction(bool minimal = false);
    //By leewheel 2026-08-23: 合并 the-lab(#2571) —— 非团本地下城判断(ForceRebuff/buff 相关)
    bool IsInNonRaidDungeon() const;
    //End By leewheel
    void ReInitCurrentEngine();
    void Reset(bool full = false);
    void LeaveOrDisbandGroup();

    // 策略
    bool HasStrategy(std::string const name, BotState type);
    bool ContainsStrategy(StrategyType type);
    void ChangeStrategy(std::string const names, BotState type);
    void ClearStrategies(BotState type);
    void SelectiveResetStrategies(BotState type);
    void ResetStrategies(bool load = false);
    std::vector<std::string> GetStrategies(BotState type);
    //By leewheel 2026-07-11: 添加缺失的成员函数声明
    Strategy* GetStrategy(std::string const name, BotState type);
    void ApplyInstanceStrategies(uint32 mapId, bool tellMaster = false);
    //By leewheel 2026-07-26: 移植目标排除查询，转发至战斗引擎。
    bool HasTargetExclusions() const;
    //End By leewheel
    void EvaluateHealerDpsStrategy();
    bool AllowActive(ActivityType activityType);
    //End By leewheel
    virtual bool DoSpecificAction(std::string const name, Event event = Event(), bool silent = false,
                                  std::string const qualifier = "");

    // 通信
    //By leewheel 2026-07-11: 添加securityLevel参数匹配.cpp定义
    bool TellMasterNoFacing(std::ostringstream& stream, PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    bool TellMasterNoFacing(std::string const text, PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    //End By leewheel
    bool TellMaster(std::ostringstream& stream, PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    bool TellMaster(std::string const text, PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    bool TellError(std::string const text, PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    bool SayToGuild(const std::string& msg);
    bool SayToWorld(const std::string& msg);
    bool SayToChannel(const std::string& msg, const ChatChannelId& chanId);
    bool SayToParty(const std::string& msg);
    bool SayToRaid(const std::string& msg);
    bool Yell(const std::string& msg);
    bool Say(const std::string& msg);
    bool Whisper(const std::string& msg, const std::string& receiverName);

    // 状态
    BotState GetState() const { return currentState; }

    // 工具
    ChatHelper* GetChatHelper() const { return chatHelper; }

    // 单位获取
    Unit* GetUnit(ObjectGuid guid);
    //By leewheel 2026-07-11: 添加CreatureData重载
    Unit* GetUnit(const CreatureData* data);
    //End By leewheel
    Creature* GetCreature(ObjectGuid guid);
    Player* GetPlayer(ObjectGuid guid);
    GameObject* GetGameObject(ObjectGuid guid);
    WorldObject* GetWorldObject(ObjectGuid guid);
    std::vector<Player*> GetAllPlayersInGroup();
    std::vector<Player*> GetRealPlayersInGroup();

    // 职业判断
    static bool IsTank(Player* player, bool bySpec = false);
    static bool IsHeal(Player* player, bool bySpec = false);
    static bool IsDps(Player* player, bool bySpec = false);
    static bool IsRanged(Player* player, bool bySpec = false);
    static bool IsMelee(Player* player, bool bySpec = false);
    static bool IsCaster(Player* player, bool bySpec = false);
    static bool IsRangedDps(Player* player, bool bySpec = false);
    static bool IsCombo(Player* player);
    static bool IsBotMainTank(Player* player);
    //By leewheel 2026-08-01: 按上游(5e4d617a)重构——引入GetMainTankGuid消除嵌套循环，
    //IsMainTank改为单参(移除ignoreMemberFlag，因为所有调用点都使用默认值)
    static ObjectGuid GetMainTankGuid(Group* group);
    static bool IsMainTank(Player* player);
    //End By leewheel
    static bool IsExplicitMainTank(Player* player);
    static bool IsAssistTank(Player* player);
    static bool IsAssistTankOfIndex(Player* player, uint8 index, bool indexLivingOnly = false);
    static bool IsAssistHealOfIndex(Player* player, uint8 index, bool indexLivingOnly = false);
    static bool IsAssistRangedDpsOfIndex(Player* player, uint8 index, bool indexLivingOnly = false);
    static uint32 GetGroupTankNum(Player* player);
    static int32 GetAssistTankIndex(Player* player);
    int32 GetGroupSlotIndex(Player* player);
    int32 GetRangedIndex(Player* player);
    int32 GetClassIndex(Player* player, uint8 cls);
    int32 GetRangedDpsIndex(Player* player);
    int32 GetMeleeIndex(Player* player);

    // 战斗状态
    bool HasAggro(Unit* unit);
    bool IsMovementImpaired(Unit* unit);
    bool IsInVehicle(bool canControl = false, bool canCast = false, bool canAttack = false, bool canTurn = false,
                     bool fixed = false);
    bool CanMove();
    bool IsOpposing(Player* player);
    static bool IsOpposing(uint8 race1, uint8 race2);

    //By leewheel 2026-07-12: 无参数版本inline调用带参数版本
    // 活动控制
    bool AllowActivity() { return AllowActivity(ALL_ACTIVITY); }
    //End By leewheel
    bool AllowActivity(ActivityType activityType, bool checkNow = false);
    //By leewheel 2026-08-09: 移植上游dcda14a1(#2559)——直接读缓存的活动状态，避免PrintStats等统计路径重复计算
    bool IsActivityAllowedCached() const { return allowActive[ALL_ACTIVITY]; }
    //End By leewheel
    bool HasActivePlayerMaster();
    Player* FindNewMaster();
    Player* GetGroupLeader();
    uint32 GetFixedBotNumber(uint32 maxNum = 100);
    GrouperType GetGrouperType();
    GuilderType GetGuilderType();
    //By leewheel 2026-08-15: 默认范围改用reactDistance配置(对齐the-lab)——原硬编码200.0f
    bool HasPlayerNearby(WorldPosition* pos, float range = sPlayerbotAIConfig.reactDistance);
    bool HasPlayerNearby(float range = sPlayerbotAIConfig.reactDistance);
    //End By leewheel
    uint32 AutoScaleActivity(uint32 mod);

    // 法术相关
    virtual bool HasSpell(std::string const spellName) const;
    virtual bool HasAura(std::string const spellName, Unit* player, bool maxStack = false, bool checkIsOwner = false,
                         int maxAmount = -1, bool checkDuration = false);
    virtual bool HasAnyAuraOf(Unit* player, ...);
    virtual bool IsInterruptableSpellCasting(Unit* player, std::string const spell);
    virtual bool HasAuraToDispel(Unit* player, uint32 dispelType);
    bool CanCastSpell(uint32 spellid, Unit* target, bool checkHasSpell = true, Item* itemTarget = nullptr,
                      Item* castItem = nullptr);
    bool CanCastSpell(uint32 spellid, GameObject* goTarget, bool checkHasSpell = true);
    bool CanCastSpell(uint32 spellid, float x, float y, float z, bool checkHasSpell = true,
                      Item* itemTarget = nullptr);
    bool CastSpell(uint32 spellId, Unit* target, Item* itemTarget = nullptr);
    bool CastSpell(uint32 spellId, float x, float y, float z, Item* itemTarget = nullptr);
    virtual bool CanCastSpell(std::string const name, Unit* target, Item* itemTarget = nullptr);
    virtual bool CastSpell(std::string const name, Unit* target, Item* itemTarget = nullptr);
    Aura* GetAura(std::string const spellName, Unit* unit, bool checkIsOwner = false, bool checkDuration = false,
                  int checkStack = -1);
    bool canDispel(SpellInfo const* spellInfo, uint32 dispelType);
    bool CanCastVehicleSpell(uint32 spellid, Unit* target);
    bool CastVehicleSpell(uint32 spellId, Unit* target);
    bool CastVehicleSpell(uint32 spellId, float x, float y, float z);
    void SpellInterrupted(uint32 spellid);
    int32 CalculateGlobalCooldown(uint32 spellid);
    void RequestSpellInterrupt();
    void RemoveAura(std::string const name);
    void RemoveShapeshift();
    void WaitForSpellCast(Spell* spell);
    float GetRange(std::string const type);
    bool HasSkill(SkillType skill);
    bool IsAllowedCommand(std::string const text);

    // 表情和声音
    bool PlaySound(uint32 emote);
    bool PlayEmote(uint32 emote);
    void Ping(float x, float y);

    // 物品查找
    Item* FindPoison() const;
    Item* FindAmmo() const;
    Item* FindBandage() const;
    Item* FindOpenableItem() const;
    Item* FindLockedItem() const;
    Item* FindConsumable(uint32 itemId) const;
    Item* FindStoneFor(Item* weapon) const;
    Item* FindOilFor(Item* weapon) const;
    void ImbueItem(Item* item, uint32 targetFlag, ObjectGuid targetGUID);
    void ImbueItem(Item* item, uint8 targetInventorySlot);
    void ImbueItem(Item* item, Unit* target);
    void ImbueItem(Item* item);
    void EnchantItemT(uint32 spellid, uint8 slot);
    //By leewheel 2026-09-04 对齐上游: 默认半径改用配置视野距离 sightDistance——
    //原硬编码 200.0f 使 HealthTriggers/DpsTargetValue 等无参调用按 200 码统计, 远超实际视野
    int32 GetNearGroupMemberCount(float dis = sPlayerbotAIConfig.sightDistance);

    // 装备
    uint32 GetEquipGearScore(Player* player);
    static uint32 GetMixedGearScore(Player* player, bool withBags, bool withBank, uint32 topN = 0);
    bool IsInRealGuild();
    bool EqualLowercaseName(std::string s1, std::string s2);
    InventoryResult CanEquipItem(uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading = true) const;
    uint8 FindEquipSlot(ItemTemplate const* proto, uint32 slot, bool swap) const;
    std::vector<Item*> GetInventoryAndEquippedItems();
    std::vector<Item*> GetInventoryItems();
    uint32 GetInventoryItemsCountWithId(uint32 itemId);
    bool HasItemInInventory(uint32 itemId);
    std::vector<std::pair<const Quest*, uint32>> GetCurrentQuestsRequiringItemId(uint32 itemId);
    uint32 GetReactDelay();
    static float GetItemScoreMultiplier(ItemQualities quality);
    //By leewheel 2026-07-11: TC使用flag128代替AC的flag96
    static bool IsHealingSpell(uint32 spellFamilyName, flag128 spellFalimyFlags);
    //End By leewheel
    static SpellFamilyNames Class2SpellFamilyName(uint8 cls);

    // 任务相关
    std::vector<const Quest*> GetAllCurrentQuests();
    std::vector<const Quest*> GetCurrentIncompleteQuests();
    std::set<uint32> GetAllCurrentQuestIds();
    std::set<uint32> GetCurrentIncompleteQuestIds();

    // 宠物
    void PetFollow();

    // 作弊掩码
    bool HasCheat(BotCheatMask mask)
    {
        //By leewheel 2026-08-18: 全局配置掩码(sPlayerbotAIConfig.botCheatMask, 来自 Playerbot.BotCheats)
        //一并生效，否则配置里的 "food" 等作弊永远不会作用于任何机器人
        return ((uint32)mask & ((uint32)cheatMask | sPlayerbotAIConfig.botCheatMask)) != 0;
        //End By leewheel
    }
    BotCheatMask GetCheat() { return cheatMask; }
    void SetCheat(BotCheatMask mask) { cheatMask = mask; }

    // 安全
    bool IsSafe(Player* player);
    bool IsSafe(WorldObject* obj);
    PlayerbotSecurity* GetSecurity() { return &security; }
    ChatChannelSource GetChatChannelSource(Player* bot, uint32 type, std::string channelName);
    bool StarterLevelDistanceCheck(Player* player, const WorldLocation &loc, bool fromStartUp = false);

    // 跳跃目标
    Position GetJumpDestination() { return jumpDestination; }
    void SetJumpDestination(Position pos) { jumpDestination = pos; }
    void ResetJumpDestination() { jumpDestination = Position(); }

    // 定时事件
    void AddTimedEvent(std::function<void()> callback, uint32 delayMs);

    // 区域
    const AreaTableEntry* GetCurrentArea();
    const AreaTableEntry* GetCurrentZone();
    static std::string GetLocalizedAreaName(const AreaTableEntry* entry);
    static std::string GetLocalizedCreatureName(uint32 entry);
    static std::string GetLocalizedGameObjectName(uint32 entry);

    // 公共成员变量
    NewRpgInfo rpgInfo;
    NewRpgStatistic rpgStatistic;
    std::unordered_set<uint32> lowPriorityQuest;
    time_t bgReleaseAttemptTime = 0;
    static std::vector<std::string> dispel_whitelist;
    //By leewheel 2026-08-23: 合并 the-lab(#2571) —— ForceRebuff(补增益)状态
    ForceRebuffState forceRebuff;
    //End By leewheel

protected:
    ChatHelper* chatHelper;
    Player* bot;
    Player* master;
    uint32 accountId;
    AiObjectContext* aiObjectContext;
    Engine* currentEngine;
    Engine* engine;
    Engine* engines[BOT_STATE_MAX];
    BotState currentState;
    std::list<ChatCommandHolder> chatCommands;
    std::list<ChatQueuedReply> chatReplies;
    PacketHandlingHelper botOutgoingPacketHandlers;
    PacketHandlingHelper masterIncomingPacketHandlers;
    PacketHandlingHelper masterOutgoingPacketHandlers;
    PlayerbotSecurity security;
    CompositeChatFilter chatFilter;
    std::map<std::string, time_t> whispers;
    std::pair<ChatMsg, time_t> currentChat;
    bool allowActive[MAX_ACTIVITY_TYPE];
    time_t allowActiveCheckTimer[MAX_ACTIVITY_TYPE];
    bool inCombat = false;
    BotCheatMask cheatMask = BotCheatMask::none;
    Position jumpDestination = Position();
    uint32 nextTransportCheck = 0;
    bool spellInterruptRequested = false;

private:
    static std::set<std::string> unsecuredCommands;
    //By leewheel 2026-07-11: 移除const以匹配.cpp定义
    bool IsTellAllowed(PlayerbotSecurityLevel securityLevel = PLAYERBOT_SECURITY_ALLOW_ALL);
    //End By leewheel
    void UpdateAIGroupMaster();
    Item* FindItemInInventory(std::function<bool(ItemTemplate const*)> checkItem) const;
    void HandleCommands();
    static void _fillGearScoreData(Player* player, Item* item, std::vector<uint32>* gearScore, uint32& twoHandScore, bool mixed = false);
    inline bool IsValidUnit(const Unit* unit) const
    {
        return unit && unit->IsInWorld() && !unit->IsDuringRemoveFromWorld();
    }
    inline bool IsValidPlayer(const Player* player) const
    {
        return player && player->GetSession() && player->IsInWorld() && !player->IsDuringRemoveFromWorld() &&
               !player->IsBeingTeleported();
    }
};

// 工具函数
bool IsAlliance(uint8 race);
//By leewheel 2026-08-14: 移植brighton-chi the-lab——IsSelfBot判定(自身操控的bot)
bool IsSelfBot(Player* player);
//End By leewheel
//By leewheel 2026-08-24: 对齐master(17214b325)——IsRealPlayer(Player*)全局判定(无botAI=真人)
bool IsRealPlayer(Player* player);
//End By leewheel

#endif
