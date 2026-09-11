/*
 * Playerbots 主头文件
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 包含全局宏定义和前向声明
 */

#ifndef PLAYERBOTS_PLAYERBOTS_H
#define PLAYERBOTS_PLAYERBOTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Log.h"
//By leewheel 2026-08-15: CharStartOutfit缓存解析需要(DatabaseEnv提供HotfixDatabase/QueryResult/Field)
#include "DatabaseEnv.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
//End By leewheel

// AzerothCore 到 TrinityCore 日志宏兼容定义
// TrinityCore 使用 TC_LOG_* 前缀，AzerothCore 使用 LOG_* 前缀
// 此兼容层允许移植代码中使用原始 LOG_* 宏而无需逐文件修改
#ifndef LOG_INFO
#define LOG_INFO TC_LOG_INFO
#endif
#ifndef LOG_ERROR
#define LOG_ERROR TC_LOG_ERROR
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG TC_LOG_DEBUG
#endif
#ifndef LOG_WARN
#define LOG_WARN TC_LOG_WARN
#endif
#ifndef LOG_FATAL
#define LOG_FATAL TC_LOG_FATAL
#endif
#ifndef LOG_TRACE
#define LOG_TRACE TC_LOG_TRACE
#endif
#ifndef LOG_FILTER_MAX_UNITS
#define LOG_FILTER_MAX_UNITS 0
#endif

// 完整头文件 include（解决C2027未定义类型错误）
// 大量.cpp文件通过Playerbots.h获取PlayerbotAI和AiObjectContext的完整定义
#include "PlayerbotAI.h"
#include "AiObjectContext.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"
#include "World.h"
#include "SharedDefines.h"
#include "SpellDefines.h"
#include "Battleground.h"
#include "LootMgr.h"

// 添加缺失的完整类型定义头文件（修复C2027未定义类型错误）
#include "Entities/Item/Item.h"
#include "Entities/Item/Container/Bag.h"
#include "Entities/Pet/Pet.h"
#include "Entities/Unit/CharmInfo.h"
#include "Entities/Player/TradeData.h"
#include "Guilds/Guild.h"
#include "Groups/Group.h"
#include "Entities/Creature/TemporarySummon.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "Containers.h"
#include "SharedValueContext.h"
#include "ItemUsageValue.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "OutdoorPvP.h"
#include "Map.h"
#include "MotionMaster.h"
//By leewheel 2026-07-11: 添加TC缺失的头文件
#include "GridNotifiers.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Chat/Channels/ChannelMgr.h"
#include "Battlegrounds/BattlegroundMgr.h"
//By leewheel 2026-09-08: ArenaTeam_GetMembers需要ArenaTeam完整定义
#include "Battlegrounds/ArenaTeam.h"
//By leewheel 2026-09-08: BattlegroundAB/EY stub classes for zone compat
#include "Compat/BattlegroundZonesCompat.h"
//End By leewheel
#include "Entities/Item/ItemEnchantmentMgr.h"
#include "DataStores/DB2Stores.h"
//End By leewheel

// 前向声明
class Player;
class Unit;
class Group;
class Guild;
class Channel;
class WorldPacket;
class PlayerbotMgr;
class PlayerbotAIBase;
class Spell;
class SpellInfo;

// AI 值访问宏
#define AI_VALUE(type, name) context->GetValue<type>(name)->Get()
#define AI_VALUE2(type, name, param) context->GetValue<type>(name, param)->Get()

#define AI_VALUE_LAZY(type, name) context->GetValue<type>(name)->LazyGet()
#define AI_VALUE2_LAZY(type, name, param) context->GetValue<type>(name, param)->LazyGet()

#define AI_VALUE_REF(type, name) context->GetValue<type>(name)->RefGet()

#define SET_AI_VALUE(type, name, value) context->GetValue<type>(name)->Set(value)
#define SET_AI_VALUE2(type, name, param, value) context->GetValue<type>(name, param)->Set(value)
#define RESET_AI_VALUE(type, name) context->GetValue<type>(name)->Reset()
#define RESET_AI_VALUE2(type, name, param) context->GetValue<type>(name, param)->Reset()

// 机器人 AI 和管理器获取宏
#define GET_PLAYERBOT_AI(object) sPlayerbotsMgr.GetPlayerbotAI(object)
#define GET_PLAYERBOT_MGR(object) sPlayerbotsMgr.GetPlayerbotMgr(object)

// AI 值快捷访问宏（通过 player 变量）
#define PAI_VALUE(type, name) sPlayerbotsMgr.GetPlayerbotAI(player)->GetAiObjectContext()->GetValue<type>(name)->Get()
#define PAI_VALUE2(type, name, param) \
    sPlayerbotsMgr.GetPlayerbotAI(player)->GetAiObjectContext()->GetValue<type>(name, param)->Get()
#define GAI_VALUE(type, name) sSharedValueContext.getGlobalValue<type>(name)->Get()
#define GAI_VALUE2(type, name, param) sSharedValueContext.getGlobalValue<type>(name, param)->Get()

// 角度常量
#define CAST_ANGLE_IN_FRONT (2.f * static_cast<float>(M_PI) / 3.f)
#define EMOTE_ANGLE_IN_FRONT (2.f * static_cast<float>(M_PI) / 6.f)

//By leewheel 2026-07-12: split已在Util/Helpers.h中定义，需要包含该头文件
// 工具函数声明 - split在Util/Helpers.h中定义
#include "Helpers.h"
//End By leewheel

// 脚本注册函数声明
void AddPlayerbotsScripts();

// ========== AC到TC兼容性常量定义 ==========

// PlayerbotSecurityLevel 已在 Mgr/Security/PlayerbotSecurity.h 中定义
// 这里仅 include 该头文件确保定义可用
#include "PlayerbotSecurity.h"

//By leewheel 2026-08-15: 删除重复宏——TC 3.4.3 的 PLAYER_SELF_RES_SPELL 是 UpdateFields 偏移9
//(见Player.cpp case 9)，下方"Player相关"段已正确#define PLAYER_SELF_RES_SPELL 9。
//原占位宏(=0)是地雷：先include到它的TU会使自我复活判定永久失效
//End By leewheel

// ========== TC缺失的AC常量定义 ==========

// TC不使用PHASEMASK常量系统，使用PhaseShift代替
// 为了兼容AC代码中引用PHASEMASK_NORMAL的地方
#ifndef PHASEMASK_NORMAL
#define PHASEMASK_NORMAL 0x00000001
#endif

// TC使用NPCFlags2枚举类型，AC使用UNIT_NPC_FLAG2_NONE常量
// TC的GetNPCIfCanInteractWith需要NPCFlags2类型参数
#ifndef UNIT_NPC_FLAG2_NONE
#define UNIT_NPC_FLAG2_NONE NPCFlags2(0)
#endif

// TC不使用QUEST_PARTY_MSG系列
#ifndef QUEST_PARTY_MSG_ACCEPT_QUEST
#define QUEST_PARTY_MSG_ACCEPT_QUEST 0
#endif

// QUEST_OBJECTIVES_COUNT 和 QUEST_ITEM_OBJECTIVES_COUNT 已在 QuestDef.h 中定义

// ========== AC到TC方法名兼容宏 ==========
// TC的Unit/Player使用大写开头的GetClass/GetRace/GetGender
// AC使用小写开头的getClass/getRace/getGender
// 通过宏定义实现兼容，无需逐文件修改
#define getClass GetClass
#define getRace GetRace
#define getGender GetGender

//By leewheel 2026-07-10: TC的Log系统使用字符串作为filter，而非AC的枚举
//TC使用TC_LOG_*宏替代直接调用outMessage
//兼容定义：LOG_FILTER_PLAYERBOTS 在TC中使用字符串"playerbots"
#define LOG_FILTER_PLAYERBOTS "playerbots"
//End By leewheel

//By leewheel 2026-07-10: 移除旧的outMessage宏
//原因：sLog->outMessage(...)会被宏展开为sLog->TC_LOG_INFO(...)，这是无效语法
//源文件中应直接使用TC_LOG_DEBUG/TC_LOG_INFO等宏替代sLog->outMessage调用
//End By leewheel

// TC的SpellMgr::GetSpellInfo需要Difficulty参数
// AC的sSpellMgr->GetSpellInfo(id) 只需1个参数
// 使用兼容宏自动添加DIFFICULTY_NONE参数
#define GET_SPELL_INFO(spellId) sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE)

// AC成员访问到TC方法调用的宏映射
// 注意：InventoryType 不能用宏映射，因为它同时被用作类型名
// 需要在源文件中手动将 proto->InventoryType 改为 proto->GetInventoryType()
//By leewheel 2026-07-11: 移除 #define ItemId GetId() 宏
// 原因：与Class/Quality宏同样的问题，会破坏结构体中的ItemId成员
// 所有代码应直接使用GetId()方法
//End By leewheel
//By leewheel 2026-07-11: 移除 #define Class GetClass() 宏
// 原因：此宏会全局替换所有Class为GetClass()，包括TC核心头文件中的Class字段
// 导致BattlegroundPackets.h/PartyPackets.h/CharacterPackets.h/SocialMgr.h等中的
// int32 Class = 0; 被替换为 int32 GetClass() = 0; (被当作纯虚函数说明符)
// 所有代码应直接使用GetClass()方法
//End By leewheel
#define SubClass GetSubClass()
//By leewheel 2026-07-11: TC聊天系统兼容定义
// AC使用SMSG_MESSAGECHAT/SMSG_GM_MESSAGECHAT/CHAT_TAG_GM/GetChatTag
// TC使用SMSG_CHAT/CHAT_FLAG_GM/GetChatFlags
#define SMSG_MESSAGECHAT SMSG_CHAT
#define SMSG_GM_MESSAGECHAT SMSG_CHAT
#define CHAT_TAG_GM CHAT_FLAG_GM
#define CHAT_TAG_NONE CHAT_FLAG_NONE
#define GetChatTag() GetChatFlags()
//By leewheel 2026-07-11: AC→TC Opcode兼容映射
// AC使用旧opcode名，TC使用不同名称或已移除
// 有TC等价的直接映射，无等价的用0xFF00段定义使代码可编译
#define CMSG_QUESTGIVER_HELLO CMSG_QUEST_GIVER_HELLO
//By leewheel 2026-07-26: 修复任务opcode映射缺失(组队机器人不跟随接任务的根因)
// 自定义客户端把接/交/推送任务opcode改名为 CMSG_QUEST_GIVER_* / CMSG_PUSH_QUEST_TO_PARTY
// 而下方0xFF00兜底段误将这些旧名(CMSG_QUESTGIVER_ACCEPT_QUEST等)定义为0，
// 导致 masterIncomingPacketHandlers 把 "accept quest"/"complete quest"/"quest share" 注册到opcode 0，
// 玩家(主人)接任务时真实opcode(0x3498等)无法匹配到处理器，组队机器人永远收不到接任务事件。
// 在此显式映射到真实opcode，使下方#ifndef兜底失效(已定义则跳过)。
#define CMSG_QUESTGIVER_ACCEPT_QUEST CMSG_QUEST_GIVER_ACCEPT_QUEST
#define CMSG_QUESTGIVER_COMPLETE_QUEST CMSG_QUEST_GIVER_COMPLETE_QUEST
#define CMSG_PUSHQUESTTOPARTY CMSG_PUSH_QUEST_TO_PARTY
//End By leewheel
//By leewheel 2026-07-26: 修复6处主人/机器人镜像opcode映射(原被下方0xFF00兑底段定义为0导致静默失效)
// 语义对应(左AC旧名 → 右TC真实opcode):
//   CMSG_GAMEOBJ_USE     → CMSG_GAME_OBJ_USE(0x34EE)        主人使用物件→机器人镜像跟用
//   CMSG_AREATRIGGER     → CMSG_AREA_TRIGGER(0x31D6)        主人触发区域触发器→机器人镜像
//   CMSG_GROUP_UNINVITE  → CMSG_PARTY_UNINVITE(0x364A)      踢人(本fork仅按GUID一个opcode)
//   MSG_RAID_READY_CHECK → SMSG_READY_CHECK_STARTED(0x25F6) 团队准备确认→机器人自动响应
//   SMSG_GROUP_LIST      → SMSG_PARTY_UPDATE(0x25F4)        队伍名单更新→机器人端处理
// 注: CMSG_GROUP_UNINVITE_GUID 在本fork无独立opcode(客户端只按GUID踢人),保持0兑底避免覆盖"uninvite"注册
#define CMSG_GAMEOBJ_USE CMSG_GAME_OBJ_USE
#define CMSG_GAMEOBJ_REPORT_USE CMSG_GAME_OBJ_REPORT_USE
#define CMSG_AREATRIGGER CMSG_AREA_TRIGGER
#define CMSG_GROUP_UNINVITE CMSG_PARTY_UNINVITE
#define MSG_RAID_READY_CHECK SMSG_READY_CHECK_STARTED
#define SMSG_GROUP_LIST SMSG_PARTY_UPDATE
//End By leewheel
#define SMSG_LEVELUP_INFO SMSG_LEVEL_UP_INFO
#define SMSG_LOG_XPGAIN SMSG_LOG_XP_GAIN
#define SMSG_GROUP_SET_LEADER SMSG_GROUP_NEW_LEADER
#define MSG_RAID_READY_CHECK_FINISHED SMSG_READY_CHECK_COMPLETED
#define SMSG_LOOT_START_ROLL SMSG_START_LOOT_ROLL
#define MSG_MOVE_TELEPORT_ACK CMSG_MOVE_TELEPORT_ACK
#define MSG_MINIMAP_PING SMSG_MINIMAP_PING
// 以下opcode在TC中无等价，用0xFF00段定义避免编译错误
//By leewheel 2026-07-14: 修复opcode映射 - TC使用不同的opcode名称
// taxi相关 - TC使用CMSG_ENABLE_TAXI_NODE和CMSG_TAXI_NODE_STATUS_QUERY
#define CMSG_TAXICLEARALLNODES CMSG_ENABLE_TAXI_NODE
#define CMSG_TAXICLEARNODE CMSG_TAXI_NODE_STATUS_QUERY
// 组队相关 - TC使用PARTY前缀
#define CMSG_GROUP_DISBAND CMSG_PARTY_UNINVITE
#define SMSG_GROUP_INVITE SMSG_PARTY_INVITE
// 移动相关 - TC使用MOVE前缀
#define SMSG_FORCE_RUN_SPEED_CHANGE SMSG_MOVE_SET_RUN_SPEED
//By leewheel 2026-07-26: 修正映射。AC的SMSG_TRADE_STATUS_EXTENDED承载"交易窗口物品内容"，
//在TC中语义对应SMSG_TRADE_UPDATED(0x2581，SendUpdateTrade发送)，而非状态包SMSG_TRADE_STATUS(0x2582)。
//旧映射到SMSG_TRADE_STATUS会与"trade status"处理器撞同一opcode(handlers为map单值，后注册者覆盖)，
//导致真正的"accept trade"被顶掉失效；且按AC扁平字节流解析TC状态包会越界读取→崩溃。
//改映射到SMSG_TRADE_UPDATED后两个处理器各归其位，扩展处理器在物品变更时触发，时机与AC一致。
#define SMSG_TRADE_STATUS_EXTENDED SMSG_TRADE_UPDATED
//End By leewheel
// 任务相关 - TC使用QUEST_GIVER和QUEST_UPDATE前缀
#define SMSG_QUESTGIVER_OFFER_REWARD SMSG_QUEST_GIVER_OFFER_REWARD_MESSAGE
#define SMSG_QUESTUPDATE_COMPLETE SMSG_QUEST_UPDATE_COMPLETE
#define SMSG_QUESTUPDATE_ADD_KILL SMSG_QUEST_UPDATE_ADD_CREDIT
//By leewheel 2026-07-24: 任务失败包映射 - TC用QUEST_GIVER_QUEST_FAILED和QUEST_UPDATE_FAILED_TIMER
#define SMSG_QUESTUPDATE_FAILED SMSG_QUEST_GIVER_QUEST_FAILED
#define SMSG_QUESTUPDATE_FAILED_TIMER SMSG_QUEST_UPDATE_FAILED_TIMER
//End By leewheel
// 移动根状态 - TC使用MOVE前缀
#define SMSG_FORCE_MOVE_ROOT SMSG_MOVE_ROOT
#define SMSG_FORCE_MOVE_UNROOT SMSG_MOVE_UNROOT
// BG状态映射
#define SMSG_BATTLEFIELD_STATUS SMSG_BATTLEFIELD_STATUS_QUEUED
//End By leewheel 2026-07-14
//By leewheel 2026-07-11: AC→TC常量兼容映射
// SLOT_MAIN_HAND等AC装备槽名 → TC EQUIPMENT_SLOT_*
#define SLOT_MAIN_HAND EQUIPMENT_SLOT_MAINHAND
#define SLOT_OFF_HAND EQUIPMENT_SLOT_OFFHAND
#define SLOT_RANGED EQUIPMENT_SLOT_RANGED
// CHAT_TAG_NONE → TC使用值0
//By leewheel 2026-09-03 修复C4005宏重定义：第213行已定义为CHAT_FLAG_NONE，此处包裹#ifndef防止重定义
#ifndef CHAT_TAG_NONE
#define CHAT_TAG_NONE 0
#endif
// SPELLMOD_DURATION → TC SpellModOp::Duration
#define SPELLMOD_DURATION SpellModOp::Duration
// SPELL_INTERRUPT_FLAG_INTERRUPT → TC没有直接等价，使用Movement flag
#define SPELL_INTERRUPT_FLAG_INTERRUPT static_cast<uint32>(SpellInterruptFlags::Movement)
// MOTION_SLOT_CONTROLLED → TC枚举名
#define MOTION_SLOT_CONTROLLED MovementSlot::MOTION_SLOT_CONTROLLED
// AC ELEMENTAL_SHARPENING_STONE 物品ID
#define ELEMENTAL_SHARPENING_STONE 18262
//End By leewheel
//End By leewheel
//By leewheel 2025-01-16
// 移除 #define Quality GetQuality() 宏
// 原因：此宏会全局替换所有Quality为GetQuality()，包括TC核心头文件中的Quality字段
// 导致AuctionHousePackets.h中的 int32 Quality = 0; 被替换为 int32 GetQuality() = 0;
// 所有代码应直接使用GetQuality()方法
//End By leewheel

//By leewheel 2026-07-09: 移除 #define Spells Effects 全局宏
// 原因：此宏会全局替换所有 Spells 为 Effects，包括 WorldPackets::Spells:: 命名空间
// 导致2579个C2664错误。需要在源代码中手动修改 proto->Spells 为 proto->Effects。
//End By leewheel

// ========== TC Guild API兼容层 ==========
// Guild.h已添加public包装方法：HasRankRight, GetRankRights, GetMemberSize
// AC代码直接调用guild->HasRankRight()等，TC的Guild.h已提供public包装
// 无需额外兼容层

// Guild.h已提供public方法，AC代码可直接调用guild->HasRankRight()等

// ========== TC缺失的AC常量和类型定义 ==========

// AC使用PvPDifficultyEntry，TC使用PVPDifficultyEntry（大写）
#define PvPDifficultyEntry PVPDifficultyEntry

// AC使用FORM_TRAVEL等，TC使用FORM_TRAVEL_FORM等
#ifndef FORM_TRAVEL
#define FORM_TRAVEL FORM_TRAVEL_FORM
#endif
#ifndef FORM_FLIGHT
#define FORM_FLIGHT FORM_FLIGHT_FORM
#endif
#ifndef FORM_FLIGHT_EPIC
#define FORM_FLIGHT_EPIC FORM_FLIGHT_FORM_EPIC
#endif

// AC使用DUNGEON_DIFFICULTY_NORMAL/HEROIC等常量，TC使用Difficulty枚举值
// 定义兼容宏，将AC风格映射到TC的Difficulty枚举
// By leewheel 2026-07-09
#ifndef DUNGEON_DIFFICULTY_NORMAL
#define DUNGEON_DIFFICULTY_NORMAL DIFFICULTY_NORMAL
#endif
#ifndef DUNGEON_DIFFICULTY_HEROIC
#define DUNGEON_DIFFICULTY_HEROIC DIFFICULTY_HEROIC
#endif
// End By leewheel 2026-07-09
#ifndef RAID_DIFFICULTY_10MAN_NORMAL
#define RAID_DIFFICULTY_10MAN_NORMAL Difficulty::DIFFICULTY_RAID_10_N
#endif
#ifndef RAID_DIFFICULTY_10MAN_HEROIC
#define RAID_DIFFICULTY_10MAN_HEROIC Difficulty::DIFFICULTY_RAID_10_HC
#endif
#ifndef RAID_DIFFICULTY_25MAN_NORMAL
#define RAID_DIFFICULTY_25MAN_NORMAL Difficulty::DIFFICULTY_RAID_25_N
#endif
#ifndef RAID_DIFFICULTY_25MAN_HEROIC
#define RAID_DIFFICULTY_25MAN_HEROIC Difficulty::DIFFICULTY_RAID_25_HC
#endif
// End By leewheel 2026-07-09

// TC没有ARENA_TYPE_NONE
#ifndef ARENA_TYPE_NONE
#define ARENA_TYPE_NONE 0
#endif

//By leewheel 2026-07-11: AC使用AURA_INTERRUPT_FLAG_*，TC使用enum class SpellAuraInterruptFlags
// 使用inline constexpr变量代替宏，确保模板推导为SpellAuraInterruptFlags类型
// AC: AURA_INTERRUPT_FLAG_TELEPORTED(0x00080000) → TC: SpellAuraInterruptFlags::LeaveWorld
// AC: AURA_INTERRUPT_FLAG_CHANGE_MAP(0x00400000) → TC: SpellAuraInterruptFlags::EnterWorld
inline constexpr SpellAuraInterruptFlags AURA_INTERRUPT_FLAG_TELEPORTED = SpellAuraInterruptFlags::LeaveWorld;
inline constexpr SpellAuraInterruptFlags AURA_INTERRUPT_FLAG_CHANGE_MAP = SpellAuraInterruptFlags::EnterWorld;
//End By leewheel

// AC使用CMSG_AREATRIGGER等 opcode，TC使用WorldPackets系统
// 定义占位符常量，实际使用需要适配
#ifndef CMSG_AREATRIGGER
#define CMSG_AREATRIGGER 0
#endif

// AC的Battleground::GetBgTypeID() → TC: GetTypeID()
#define GetBgTypeID GetTypeID

// AC的BattlegroundMgr::BGTemplateId → TC使用不同API
// AC的BattlegroundMgr::BGArenaType → TC使用不同API

// TC缺失的配置常量（与AC保持一致）
#ifndef CONFIG_MIN_DUALSPEC_LEVEL
#define CONFIG_MIN_DUALSPEC_LEVEL CONFIG_MIN_DUAL_SPEC_LEVEL
#endif

// TC使用CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL，AC使用CONFIG_START_HEROIC_PLAYER_LEVEL
#ifndef CONFIG_START_HEROIC_PLAYER_LEVEL
#define CONFIG_START_HEROIC_PLAYER_LEVEL CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL
#endif

// TC没有VMAP_INVALID_HEIGHT_VALUE定义，使用IVMapManager头文件中的定义
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，WotLK的IVMapManager.h在Cata更名为VMapManager.h
#include "VMapManager.h"
//End By leewheel

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata核心移除了SkillType枚举，Compat层补齐SKILL_*常量
#include "SkillDefinesCompat.h"
//End By leewheel

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，补齐各战场游戏对象entry常量(AB/WS/EY/IC)
#include "BattlegroundEntriesCompat.h"
//End By leewheel

// ========== AC到TC类型兼容宏 ==========

// AC的PLAYER_FLAGS枚举类型 → TC的PlayerFlags枚举类型
#define PLAYER_FLAGS PlayerFlags

// AC使用UNIT_FLAG_NOT_SELECTABLE，TC使用UNIT_FLAG_UNINTERACTIBLE
#ifndef UNIT_FLAG_NOT_SELECTABLE
#define UNIT_FLAG_NOT_SELECTABLE UNIT_FLAG_UNINTERACTIBLE
#endif

// TC的Opcode命名略有不同
#define CMSG_AUTOEQUIP_ITEM_SLOT CMSG_AUTO_EQUIP_ITEM_SLOT

// TC的TradeStatus枚举值略有不同
#define TRADE_STATUS_TRADE_ACCEPT TRADE_STATUS_ACCEPTED

// ========== AC到TC方法名兼容宏 ==========

// AC的Cell::VisitObjects → TC的Cell::VisitGridObjects
// AC的VisitObjects只访问GridTypeMapContainer，等价于TC的VisitGridObjects
#define VisitObjects VisitGridObjects

// AC的Group::GetLeader() → TC的Group::GetLeaderGUID()
#define GetLeader GetLeaderGUID

// AC的Player::IsRooted() 方法不存在
// 需要通过HasAura检查ROOT_AURA状态
#define IsRooted() HasAuraType(SPELL_AURA_MOD_ROOT)

// ========== TC缺失的AC副本难度常量 ==========
// AC使用DUNGEON_DIFFICULTY_*，TC使用DIFFICULTY_*
#ifndef DUNGEON_DIFFICULTY_NORMAL
#define DUNGEON_DIFFICULTY_NORMAL DIFFICULTY_NORMAL
#endif
#ifndef DUNGEON_DIFFICULTY_HEROIC
#define DUNGEON_DIFFICULTY_HEROIC DIFFICULTY_HEROIC
#endif

// ========== TC缺失的AC生物精英类型常量 ==========
// AC使用CreatureEliteType枚举，TC使用CreatureClassifications枚举类
// 注意：已由下方第二批兼容宏(第2981行附近)提供正确的CreatureClassifications映射
// 此处不再重复定义旧的整数常量，避免与enum class类型不匹配导致switch/case编译错误
// CREATURE_UNKNOWN单独保留供特殊场景使用
#ifndef CREATURE_UNKNOWN
#define CREATURE_UNKNOWN 5
#endif

// ========== TC缺失的AC区域标志常量 ==========
#ifndef AREA_FLAG_NO_FLY_ZONE
#define AREA_FLAG_NO_FLY_ZONE 0x20000000
#endif

// ========== TC缺失的AC战场常量 ==========
// 战场自动移除时间（毫秒）
#ifndef TIME_TO_AUTOREMOVE
#define TIME_TO_AUTOREMOVE 120000
#endif

// 战场队列常量 (TC-Cata: 普通BG用BattlemasterListId作为队列ID, Type=Battleground=0)
//By leewheel 2026-09-09: 修复自引用宏定义，使用实际BattlemasterListId
#ifndef BATTLEGROUND_QUEUE_AV
#define BATTLEGROUND_QUEUE_AV BATTLEGROUND_AV
#endif
#ifndef BATTLEGROUND_QUEUE_WS
#define BATTLEGROUND_QUEUE_WS BATTLEGROUND_WS
#endif
#ifndef BATTLEGROUND_QUEUE_AB
#define BATTLEGROUND_QUEUE_AB BATTLEGROUND_AB
#endif
#ifndef BATTLEGROUND_QUEUE_EY
#define BATTLEGROUND_QUEUE_EY BATTLEGROUND_EY
#endif
#ifndef BATTLEGROUND_QUEUE_IC
#define BATTLEGROUND_QUEUE_IC BATTLEGROUND_IC
#endif
// 竞技场队列 (用于BattlegroundData map键，需完整BattlegroundQueueTypeId结构体)
//By leewheel 2026-09-09: TC-Cata竞技场队列是结构体，在代码中用BattlegroundMgr::BGQueueTypeId构造
//End By leewheel

// AV战场BOSS位置常量
#ifndef AV_CPLACE_A_BOSS
#define AV_CPLACE_A_BOSS 381
#endif
#ifndef AV_CPLACE_H_BOSS
#define AV_CPLACE_H_BOSS 443
#endif

// EY战场物件入口常量
#ifndef BG_OBJECT_FLAG3_EY_ENTRY
#define BG_OBJECT_FLAG3_EY_ENTRY 184142
#endif

// EY战场据点常量
#ifndef POINT_FEL_REAVER
#define POINT_FEL_REAVER 0
#endif
#ifndef POINT_BLOOD_ELF
#define POINT_BLOOD_ELF 1
#endif
#ifndef POINT_DRAENEI_RUINS
#define POINT_DRAENEI_RUINS 2
#endif
#ifndef POINT_MAGE_TOWER
#define POINT_MAGE_TOWER 3
#endif

// EY战场旗帜物件常量
#ifndef BG_EY_OBJECT_FLAG_FEL_REAVER
#define BG_EY_OBJECT_FLAG_FEL_REAVER 43
#endif
#ifndef BG_EY_OBJECT_FLAG_BLOOD_ELF
#define BG_EY_OBJECT_FLAG_BLOOD_ELF 44
#endif
#ifndef BG_EY_OBJECT_FLAG_DRAENEI_RUINS
#define BG_EY_OBJECT_FLAG_DRAENEI_RUINS 45
#endif
#ifndef BG_EY_OBJECT_FLAG_MAGE_TOWER
#define BG_EY_OBJECT_FLAG_MAGE_TOWER 46
#endif

// EY战场区域触发器常量
#ifndef AT_FEL_REAVER_POINT
#define AT_FEL_REAVER_POINT 4514
#endif
#ifndef AT_BLOOD_ELF_POINT
#define AT_BLOOD_ELF_POINT 4476
#endif
#ifndef AT_MAGE_TOWER_POINT
#define AT_MAGE_TOWER_POINT 4516
#endif
#ifndef AT_DRAENEI_RUINS_POINT
#define AT_DRAENEI_RUINS_POINT 4518
#endif

// AB战场节点状态常量 - 与TC的BG_AB_NODE_STATUS_*值保持一致
#ifndef BG_AB_NODE_STATE_NEUTRAL
#define BG_AB_NODE_STATE_NEUTRAL 0
#endif

#ifndef BG_AB_NODE_STATE_ALLY_CONTESTED
#define BG_AB_NODE_STATE_ALLY_CONTESTED 1
#endif

#ifndef BG_AB_NODE_STATE_HORDE_CONTESTED
#define BG_AB_NODE_STATE_HORDE_CONTESTED 2
#endif

#ifndef BG_AB_NODE_STATE_ALLY_OCCUPIED
#define BG_AB_NODE_STATE_ALLY_OCCUPIED 3
#endif

#ifndef BG_AB_NODE_STATE_HORDE_OCCUPIED
#define BG_AB_NODE_STATE_HORDE_OCCUPIED 4
#endif

//By leewheel 2026-09-08: AB节点ID(WotLK与Cata一致)
#ifndef BG_AB_NODE_STABLES
#define BG_AB_NODE_STABLES 0
#endif
#ifndef BG_AB_NODE_BLACKSMITH
#define BG_AB_NODE_BLACKSMITH 1
#endif
#ifndef BG_AB_NODE_FARM
#define BG_AB_NODE_FARM 2
#endif
#ifndef BG_AB_NODE_LUMBER_MILL
#define BG_AB_NODE_LUMBER_MILL 3
#endif
#ifndef BG_AB_NODE_GOLD_MINE
#define BG_AB_NODE_GOLD_MINE 4
#endif

// ========== TC缺失的AC Opcode常量 ==========
// TC的Opcode命名略有不同，添加兼容宏
#ifndef CMSG_GUILD_INVITE
#define CMSG_GUILD_INVITE CMSG_GUILD_INVITE_BY_NAME
#endif
#ifndef CMSG_GUILD_PROMOTE
#define CMSG_GUILD_PROMOTE CMSG_GUILD_PROMOTE_MEMBER
#endif
#ifndef CMSG_GUILD_DEMOTE
#define CMSG_GUILD_DEMOTE CMSG_GUILD_DEMOTE_MEMBER
#endif
#ifndef CMSG_GUILD_REMOVE
#define CMSG_GUILD_REMOVE CMSG_GUILD_OFFICER_REMOVE_MEMBER
#endif

// TC缺失的AC Opcode常量

// ========== TC缺失的AC移动标志常量 ==========
// AC: MOVEMENTFLAG_ONTRANSPORT = 0x00000200
// TC: 0x00000200 是 MOVEMENTFLAG_DISABLE_GRAVITY
// Playerbots代码中使用此标志检查是否在载具上
#ifndef MOVEMENTFLAG_ONTRANSPORT
#define MOVEMENTFLAG_ONTRANSPORT 0x00000200
#endif

// ========== TC缺失的AC物品常量 ==========
// AC的MAX_ITEM_PROTO_SPELLS常量
#ifndef MAX_ITEM_PROTO_SPELLS
#define MAX_ITEM_PROTO_SPELLS 5
#endif

// TC缺失的AC配置常量
// TC没有CONFIG_START_HEROIC_PLAYER_LEVEL，使用70作为默认值
#ifndef CONFIG_START_HEROIC_PLAYER_LEVEL
#define CONFIG_START_HEROIC_PLAYER_LEVEL 70
#endif

// BattlegroundMgr兼容方法已在BattlegroundMgr.h中添加为类成员

// ========== AreaTableEntry兼容字段 ==========
// TC的AreaTableEntry已有FactionGroupMask字段, AreaName(LocalizedString), Flags(array)
// 在DB2Structure.h中添加了flags和AreaName_lang兼容访问器

// ========== TC的Quest API兼容辅助宏 ==========
// AC: quest->GetTitle() → TC: quest->GetLogTitle()
// AC: quest->GetRequiredClasses() → TC: quest->GetAllowableClasses()
// 这些在个别文件中直接修改

// ========== 缺失的SMSG Opcode常量 ==========
#ifndef SMSG_PLAY_SPELL_IMPACT
#define SMSG_PLAY_SPELL_IMPACT SMSG_PLAY_SPELL_VISUAL
#endif

// ========== 缺失的PLAYER_FLAGS常量 ==========
#ifndef PLAYER_FLAGS_HIDE_CLOAK
#define PLAYER_FLAGS_HIDE_CLOAK 0x00004000
#endif
#ifndef PLAYER_FLAGS_HIDE_HELM
#define PLAYER_FLAGS_HIDE_HELM 0x00001000
#endif

// ========== 缺失的EQUIP_ERR常量 ==========
#ifndef EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT
#define EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT EQUIP_ERR_WRONG_SLOT
#endif
#ifndef EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG
#define EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS
#endif
#ifndef EQUIP_ERR_CANT_TRADE_EQUIP_BAGS
#define EQUIP_ERR_CANT_TRADE_EQUIP_BAGS EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS
#endif
#ifndef EQUIP_ERR_ONLY_AMMO_CAN_GO_HERE
#define EQUIP_ERR_ONLY_AMMO_CAN_GO_HERE EQUIP_ERR_WRONG_SLOT
#endif
#ifndef EQUIP_ERR_NO_REQUIRED_PROFICIENCY
#define EQUIP_ERR_NO_REQUIRED_PROFICIENCY EQUIP_ERR_CANT_EQUIP_LEVEL_I
#endif

// ========== 缺失的Gossip常量 ==========
#ifndef GossipText
#define GossipText uint32
#endif
#ifndef MAX_GOSSIP_TEXT_OPTIONS
#define MAX_GOSSIP_TEXT_OPTIONS 32
#endif

// ========== 缺失的法术ID常量（盗贼毒药）==========
#define INSTANT_POISON_IX 2823
#define INSTANT_POISON_VIII 2824
#define INSTANT_POISON_VII 2825
#define INSTANT_POISON_VI 2826
#define INSTANT_POISON_V 2827
#define INSTANT_POISON_IV 2828
#define INSTANT_POISON_III 2829
#define INSTANT_POISON_II 2830
#define INSTANT_POISON 2831
#define DEADLY_POISON_IX 2819
#define DEADLY_POISON_VIII 2820
#define DEADLY_POISON_VII 2821
#define DEADLY_POISON_VI 2822
#define DEADLY_POISON_V 2818
#define DEADLY_POISON_IV 2817
#define DEADLY_POISON_III 2816
#define DEADLY_POISON_II 2815
#define DEADLY_POISON 2814

// ========== 缺失的物品类常量 ==========
#ifndef ITEM_CLASS_MISC
#define ITEM_CLASS_MISC 15
#endif

// ========== 缺失的Map液态常量 ==========
//By leewheel 2026-08-15: 对齐Compat/GridTerrainData.h的0x0F——原0xFFFF与垫片0x0F冲突
//(双值地雷)。实际代码已改用TC原生map_liquidHeaderTypeFlags::AllLiquids，此处宏仅为兼容垫片
#ifndef MAP_ALL_LIQUIDS
#define MAP_ALL_LIQUIDS 0x0F
#endif

// ========== 缺失的副本难度常量 ==========
#ifndef DUNGEON_DIFFICULTY_NORMAL
#define DUNGEON_DIFFICULTY_NORMAL DIFFICULTY_NORMAL
#endif
#ifndef DUNGEON_DIFFICULTY_HEROIC
#define DUNGEON_DIFFICULTY_HEROIC DIFFICULTY_HEROIC
#endif
#ifndef DUNGEON_DIFFICULTY_EPIC
#define DUNGEON_DIFFICULTY_EPIC DIFFICULTY_EPIC
#endif
#ifndef DUNGEON_DIFFICULTY_RAID_10MAN_NORMAL
#define DUNGEON_DIFFICULTY_RAID_10MAN_NORMAL DIFFICULTY_10_N
#endif
#ifndef DUNGEON_DIFFICULTY_RAID_25MAN_NORMAL
#define DUNGEON_DIFFICULTY_RAID_25MAN_NORMAL DIFFICULTY_25_N
#endif

// ========== 缺失的NPC ID常量 ==========
#ifndef NPC_RISEN_GHOUL
#define NPC_RISEN_GHOUL 31746  // DK复活食尸鬼
#endif

// ========== 缺失的Opcode常量 ==========
#ifndef SMSG_PLAY_SPELL_IMPACT
#define SMSG_PLAY_SPELL_IMPACT SMSG_SPELL_GO_PLAY_VISUAL_KIT
#endif
//By leewheel 2026-08-27: 修正——原 CHOOSE_REWARD 指向未定义的 REQUEST_REWARD 造成宏链断裂，
//直接映射到 TC 真实 opcode(CHOOSE_REWARD 0x349A / REQUEST_REWARD 0x349B)
#ifndef CMSG_QUESTGIVER_CHOOSE_REWARD
#define CMSG_QUESTGIVER_CHOOSE_REWARD CMSG_QUEST_GIVER_CHOOSE_REWARD
#endif
#ifndef CMSG_QUESTGIVER_REQUEST_REWARD
#define CMSG_QUESTGIVER_REQUEST_REWARD CMSG_QUEST_GIVER_REQUEST_REWARD
#endif
//End By leewheel

// ========== 缺失的类型别名 ==========
//By leewheel 2026-07-10: TC使用LFGDungeonsEntry(带s)而非LFGDungeonData
#define LFGDungeonEntry LFGDungeonsEntry
#define sLFGDungeonStore sLFGDungeonsStore
//End By leewheel
#ifndef MAP_LIQUID_TYPE_NO_WATER
//By leewheel 2026-08-15: 对齐Compat/GridTerrainData.h的0x00(无水=0位)——原0x01与垫片冲突
#define MAP_LIQUID_TYPE_NO_WATER 0x00
#endif

// ========== AC的GetUInt32Value兼容层 ==========
// TC不再使用基于索引的UpdateField访问
// 在Playerbots代码中直接使用这些宏来访问对应的值
// Item相关
#define ITEM_FIELD_DURABILITY 0
#define ITEM_FIELD_MAXDURABILITY 1
// Player相关
#define PLAYER_XP 2
#define PLAYER_NEXT_LEVEL_XP 3
#define PLAYER_REST_STATE_EXPERIENCE 4
#define PLAYER_AMMO_ID 5
#define PLAYER_EXPERTISE 6
#define PLAYER_FIELD_COMBAT_RATING_1 7
//By leewheel 2026-07-11: TC已定义CR_ARMOR_PENETRATION为enum(值24), 移除此宏避免冲突
// #define CR_ARMOR_PENETRATION 8
//End By leewheel
#define PLAYER_SELF_RES_SPELL 9
#define PLAYER_EXPLORED_ZONES_1 10
// Unit相关
#define UNIT_CREATED_BY_SPELL 11

// DBC结构兼容宏 - 将方法调用转换为直接成员访问
// 这些在Playerbots代码中被当作成员变量使用
// 由于我们在DBC结构中添加了方法而非成员，需要用宏来适配
// 但是宏会全局替换，所以不如直接修改Playerbots代码中的调用

//By leewheel 2026-07-09: TalentEntry AC兼容字段映射
// AC: TalentID/RankID/Row/Col/TalentTab/DependsOn/DependsOnRank
// TC: SpellRank[]/TierID/ColumnIndex/TabID/PrereqTalent[]/PrereqRank[]
// 由于Playerbots代码中直接访问entry->TalentID等成员，需要添加兼容访问器
// 使用内联函数而非宏以避免全局替换问题
// 注意：这些在DB2Structure.h的TalentEntry中已有定义，这里仅做映射说明
// 实际兼容通过修改Playerbots代码中的访问方式实现
//End By leewheel

// GetUInt32Value兼容 - 通过内联函数实现
// Item::GetUInt32Value
inline uint32 Item_GetUInt32Value(Item const* item, uint16 index)
{
    //By leewheel 2026-09-06: 移植到TrinityCore-Cata
    //TC-Cata的Item改用m_itemData结构化字段(有Durability/MaxDurability字段)，
    //不再有GetUInt32Value索引访问；UpdateField包装器需解引用取值
    switch (index)
    {
        case ITEM_FIELD_DURABILITY: return *item->m_itemData->Durability;
        case ITEM_FIELD_MAXDURABILITY: return *item->m_itemData->MaxDurability;
        default: return 0;
    }
    //End By leewheel
}

// ========== 缺失的InventoryResult常量映射 ==========

// ========== CreatureData兼容字段 ==========
// AC使用mapid/posX/posY/posZ, TC使用不同的字段名
// 需要在CreatureData中添加兼容成员

// ========== AC EQUIP_ERR 常量映射到 TC InventoryResult ==========
#ifndef EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS
#define EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS EQUIP_ERR_DESTROY_NONEMPTY_BAG
#endif
#ifndef EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE
#define EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE EQUIP_ERR_NO_SLOT_AVAILABLE
#endif
#ifndef EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE2
#define EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE2 EQUIP_ERR_NO_SLOT_AVAILABLE_2
#endif
#ifndef EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE3
#define EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE3 EQUIP_ERR_NO_SLOT_AVAILABLE_3
#endif
#ifndef EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM
#define EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM EQUIP_ERR_CANT_EQUIP_EVER
#endif
#ifndef EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM2
#define EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM2 EQUIP_ERR_CANT_EQUIP_EVER_2
#endif
#ifndef EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED
#define EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED EQUIP_ERR_2HANDED_EQUIPPED
#endif
#ifndef EQUIP_ERR_CANT_DUAL_WIELD
#define EQUIP_ERR_CANT_DUAL_WIELD EQUIP_ERR_2HSKILLNOTFOUND
#endif
#ifndef EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG
#define EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG EQUIP_ERR_WRONG_BAG_TYPE
#endif
#ifndef EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG2
#define EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG2 EQUIP_ERR_WRONG_BAG_TYPE_2
#endif
#ifndef EQUIP_ERR_CANT_CARRY_MORE_OF_THIS
#define EQUIP_ERR_CANT_CARRY_MORE_OF_THIS EQUIP_ERR_ITEM_MAX_COUNT
#endif
#ifndef EQUIP_ERR_ITEM_CANT_STACK
#define EQUIP_ERR_ITEM_CANT_STACK EQUIP_ERR_CANT_STACK
#endif
#ifndef EQUIP_ERR_ITEM_CANT_STACK2
#define EQUIP_ERR_ITEM_CANT_STACK2 EQUIP_ERR_CANT_STACK_2
#endif
#ifndef EQUIP_ERR_ITEM_CANT_BE_EQUIPPED
#define EQUIP_ERR_ITEM_CANT_BE_EQUIPPED EQUIP_ERR_NOT_EQUIPPABLE
#endif
#ifndef EQUIP_ERR_ITEMS_CANT_BE_SWAPPED
#define EQUIP_ERR_ITEMS_CANT_BE_SWAPPED EQUIP_ERR_CANT_SWAP
#endif
#ifndef EQUIP_ERR_SLOT_IS_EMPTY
#define EQUIP_ERR_SLOT_IS_EMPTY EQUIP_ERR_SLOT_EMPTY
#endif
#ifndef EQUIP_ERR_CANT_DROP_SOULBOUND
#define EQUIP_ERR_CANT_DROP_SOULBOUND EQUIP_ERR_DROP_BOUND_ITEM
#endif
#ifndef EQUIP_ERR_TRIED_TO_SPLIT_MORE_THAN_COUNT
#define EQUIP_ERR_TRIED_TO_SPLIT_MORE_THAN_COUNT EQUIP_ERR_TOO_FEW_TO_SPLIT
#endif
#ifndef EQUIP_ERR_COULDNT_SPLIT_ITEMS
#define EQUIP_ERR_COULDNT_SPLIT_ITEMS EQUIP_ERR_SPLIT_FAILED
#endif
#ifndef EQUIP_ERR_MISSING_REAGENT
#define EQUIP_ERR_MISSING_REAGENT EQUIP_ERR_SPELL_FAILED_REAGENTS_GENERIC
#endif
#ifndef EQUIP_ERR_DONT_OWN_THAT_ITEM
#define EQUIP_ERR_DONT_OWN_THAT_ITEM EQUIP_ERR_NOT_OWNER
#endif
#ifndef EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER
#define EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER EQUIP_ERR_ONLY_ONE_QUIVER
#endif
#ifndef EQUIP_ERR_CAN_EQUIP_ONLY1_BOLT
#define EQUIP_ERR_CAN_EQUIP_ONLY1_BOLT EQUIP_ERR_ONLY_ONE_BOLT
#endif
#ifndef EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH
#define EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH EQUIP_ERR_ONLY_ONE_AMMO
#endif
#ifndef EQUIP_ERR_MUST_PURCHASE_THAT_BAG_SLOT
#define EQUIP_ERR_MUST_PURCHASE_THAT_BAG_SLOT EQUIP_ERR_NO_BANK_SLOT
#endif
#ifndef EQUIP_ERR_TOO_FAR_AWAY_FROM_BANK
#define EQUIP_ERR_TOO_FAR_AWAY_FROM_BANK EQUIP_ERR_NO_BANK_HERE
#endif
#ifndef EQUIP_ERR_YOU_ARE_STUNNED
#define EQUIP_ERR_YOU_ARE_STUNNED EQUIP_ERR_GENERIC_STUNNED
#endif
#ifndef EQUIP_ERR_YOU_ARE_DEAD
#define EQUIP_ERR_YOU_ARE_DEAD EQUIP_ERR_PLAYER_DEAD
#endif
#ifndef EQUIP_ERR_CANT_DO_RIGHT_NOW
#define EQUIP_ERR_CANT_DO_RIGHT_NOW EQUIP_ERR_CLIENT_LOCKED_OUT
#endif
#ifndef EQUIP_ERR_INT_BAG_ERROR
#define EQUIP_ERR_INT_BAG_ERROR EQUIP_ERR_INTERNAL_BAG_ERROR
#endif
#ifndef EQUIP_ERR_STACKABLE_CANT_BE_WRAPPED
#define EQUIP_ERR_STACKABLE_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_STACKABLE
#endif
#ifndef EQUIP_ERR_EQUIPPED_CANT_BE_WRAPPED
#define EQUIP_ERR_EQUIPPED_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_EQUIPPED
#endif
#ifndef EQUIP_ERR_WRAPPED_CANT_BE_WRAPPED
#define EQUIP_ERR_WRAPPED_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_WRAPPED
#endif
#ifndef EQUIP_ERR_BOUND_CANT_BE_WRAPPED
#define EQUIP_ERR_BOUND_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_BOUND
#endif
#ifndef EQUIP_ERR_UNIQUE_CANT_BE_WRAPPED
#define EQUIP_ERR_UNIQUE_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_UNIQUE
#endif
#ifndef EQUIP_ERR_BAGS_CANT_BE_WRAPPED
#define EQUIP_ERR_BAGS_CANT_BE_WRAPPED EQUIP_ERR_CANT_WRAP_BAGS
#endif
#ifndef EQUIP_ERR_ALREADY_LOOTED
#define EQUIP_ERR_ALREADY_LOOTED EQUIP_ERR_LOOT_GONE
#endif
#ifndef EQUIP_ERR_INVENTORY_FULL
#define EQUIP_ERR_INVENTORY_FULL EQUIP_ERR_INV_FULL
#endif
#ifndef EQUIP_ERR_ITEM_IS_CURRENTLY_SOLD_OUT
#define EQUIP_ERR_ITEM_IS_CURRENTLY_SOLD_OUT EQUIP_ERR_VENDOR_SOLD_OUT
#endif
#ifndef EQUIP_ERR_ITEM_SOLD_OUT
#define EQUIP_ERR_ITEM_SOLD_OUT EQUIP_ERR_VENDOR_SOLD_OUT
#endif
#ifndef EQUIP_ERR_ITEM_NOT_FOUND2
#define EQUIP_ERR_ITEM_NOT_FOUND2 EQUIP_ERR_ITEM_NOT_FOUND_2
#endif
#ifndef EQUIP_ERR_BAG_FULL3
#define EQUIP_ERR_BAG_FULL3 EQUIP_ERR_BAG_FULL_3
#endif
#ifndef EQUIP_ERR_BAG_FULL4
#define EQUIP_ERR_BAG_FULL4 EQUIP_ERR_BAG_FULL_4
#endif
#ifndef EQUIP_ERR_BAG_FULL6
#define EQUIP_ERR_BAG_FULL6 EQUIP_ERR_BAG_FULL_6
#endif

// ========== AC 物品绑定类型映射到 TC ItemBondingType ==========
#ifndef BIND_WHEN_PICKED_UP
#define BIND_WHEN_PICKED_UP BIND_ON_ACQUIRE
#endif
#ifndef BIND_WHEN_EQUIPPED
#define BIND_WHEN_EQUIPPED BIND_ON_EQUIP
#endif
#ifndef BIND_WHEN_USED
#define BIND_WHEN_USED BIND_ON_USE
#endif
#ifndef BIND_WHEN_USE
#define BIND_WHEN_USE BIND_ON_USE
#endif
#ifndef BIND_QUEST_ITEM
#define BIND_QUEST_ITEM BIND_QUEST
#endif

// ========== AC 物品子类映射到 TC ==========
#ifndef ITEM_SUBCLASS_WEAPON_MISC
#define ITEM_SUBCLASS_WEAPON_MISC ITEM_SUBCLASS_WEAPON_MISCELLANEOUS
#endif
#ifndef ITEM_SUBCLASS_FOOD
#define ITEM_SUBCLASS_FOOD ITEM_SUBCLASS_FOOD_DRINK
#endif
#ifndef ITEM_SUBCLASS_JUNK
#define ITEM_SUBCLASS_JUNK ITEM_SUBCLASS_MISCELLANEOUS_JUNK
#endif

// ========== AC 更新字段常量（TC不使用索引式UpdateField）==========
// 这些值作为标记使用，实际功能通过兼容方法实现
// Unit标志字段
#ifndef UNIT_FIELD_FLAGS
#define UNIT_FIELD_FLAGS 0
#endif
#ifndef UNIT_DYNAMIC_FLAGS
#define UNIT_DYNAMIC_FLAGS 1
#endif
#ifndef UNIT_FIELD_FLAGS_2
#define UNIT_FIELD_FLAGS_2 2
#endif
#ifndef GAMEOBJECT_FLAGS
#define GAMEOBJECT_FLAGS 3
#endif
#ifndef PLAYER_FLAGS
#define PLAYER_FLAGS 4
#endif
#ifndef ITEM_FIELD_RANDOM_PROPERTIES_ID
#define ITEM_FIELD_RANDOM_PROPERTIES_ID 5
#endif

// ========== AC CMSG Opcode 占位符 ==========
// TC使用WorldPackets系统替代旧的CMSG opcode
// 这些占位符仅用于让代码编译，实际调用需要适配为WorldPackets
#ifndef CMSG_GAMEOBJ_USE
#define CMSG_GAMEOBJ_USE 0
#endif
#ifndef CMSG_GAMEOBJ_REPORT_USE
#define CMSG_GAMEOBJ_REPORT_USE 0
#endif
//By leewheel 2026-08-27: 修正——TC有真实opcode CMSG_BATTLEFIELD_LEAVE(0x3175)/CMSG_REQUEST_BATTLEFIELD_STATUS(0x35DD)，不再用0占位
#ifndef CMSG_LEAVE_BATTLEFIELD
#define CMSG_LEAVE_BATTLEFIELD CMSG_BATTLEFIELD_LEAVE
#endif
#ifndef CMSG_BATTLEFIELD_STATUS
#define CMSG_BATTLEFIELD_STATUS CMSG_REQUEST_BATTLEFIELD_STATUS
#endif
//End By leewheel
//By leewheel 2026-08-27: 修正——TC有真实opcode CMSG_QUEST_GIVER_ACCEPT_QUEST(0x3498)/CMSG_TALK_TO_GOSSIP(0x3492)
#ifndef CMSG_QUESTGIVER_ACCEPT_QUEST
#define CMSG_QUESTGIVER_ACCEPT_QUEST CMSG_QUEST_GIVER_ACCEPT_QUEST
#endif
#ifndef CMSG_GOSSIP_HELLO
#define CMSG_GOSSIP_HELLO CMSG_TALK_TO_GOSSIP
#endif
//End By leewheel
//By leewheel 2026-07-14: 删除CMSG_LOOT和CMSG_AUTOSTORE_LOOT_ITEM的opcode 0定义，导致Unhandled opcode 0错误
// TC中正确的opcode是CMSG_LOOT_UNIT和CMSG_LOOT_ITEM
//End By leewheel
//By leewheel 2026-08-27: 修正——TC用CMSG_DF_*系列(DF_JOIN 0x360B/DF_SET_ROLES 0x3617/DF_PROPOSAL_RESPONSE 0x3609/DF_LEAVE 0x3614/DF_TELEPORT 0x3619)
#ifndef CMSG_LFG_JOIN
#define CMSG_LFG_JOIN CMSG_DF_JOIN
#endif
#ifndef CMSG_LFG_SET_ROLES
#define CMSG_LFG_SET_ROLES CMSG_DF_SET_ROLES
#endif
#ifndef CMSG_LFG_PROPOSAL_RESULT
#define CMSG_LFG_PROPOSAL_RESULT CMSG_DF_PROPOSAL_RESPONSE
#endif
#ifndef CMSG_LFG_LEAVE
#define CMSG_LFG_LEAVE CMSG_DF_LEAVE
#endif
#ifndef CMSG_LFG_TELEPORT
#define CMSG_LFG_TELEPORT CMSG_DF_TELEPORT
#endif
//End By leewheel
#ifndef CMSG_GROUP_UNINVITE
#define CMSG_GROUP_UNINVITE CMSG_PARTY_UNINVITE
#endif
#ifndef CMSG_GROUP_UNINVITE_GUID
#define CMSG_GROUP_UNINVITE_GUID CMSG_PARTY_UNINVITE
#endif

// ========== AC 区域标志常量 ==========
#ifndef AREA_FLAG_CAPITAL
#define AREA_FLAG_CAPITAL 0x00040000
#endif

// ========== AC 宠物常量 ==========
#ifndef PET_FOLLOW_DIST
#define PET_FOLLOW_DIST 1.0f
#endif
#ifndef FORCED_MOVEMENT_NONE
#define FORCED_MOVEMENT_NONE 0
#endif

// ========== AC 战场常量 ==========
#ifndef BG_AB_OBJECTS_PER_NODE
#define BG_AB_OBJECTS_PER_NODE 8
#endif
#ifndef BG_EY_OBJECT_FLAG_NETHERSTORM
#define BG_EY_OBJECT_FLAG_NETHERSTORM 25
#endif
#ifndef BG_WS_TRIGGER_HORDE_FLAG_SPAWN
#define BG_WS_TRIGGER_HORDE_FLAG_SPAWN 3646
#endif
#ifndef BG_WS_TRIGGER_ALLIANCE_FLAG_SPAWN
#define BG_WS_TRIGGER_ALLIANCE_FLAG_SPAWN 3647
#endif

// ========== AC 召唤槽常量 ==========
#ifndef SUMMON_SLOT_TOTEM_EARTH
#define SUMMON_SLOT_TOTEM_EARTH 0
#endif

// ========== AC 运动类型常量 ==========
#ifndef NULL_MOTION_TYPE
#define NULL_MOTION_TYPE 0
#endif

// ========== AC GameObjectTemplate::trap 兼容字段 ==========
// TC中trap结构体使用不同的字段名
// 这些定义仅用于让代码编译，实际访问需要在源文件中修改

// ========== AC ControlSet 兼容 ==========
// AC使用Unit::ControlSet，TC使用Unit::ControlList
#define ControlSet ControlList

// ========== 缺失的CMSG/SMSG/MSG Opcode常量 ==========
// TC使用WorldPackets系统，这些旧opcode仅作为switch-case标签使用
// 实际的数据包处理需要适配为WorldPackets结构体
//By leewheel 2026-08-27: 修正——TC有真实opcode(CHOOSE_REWARD 0x349A/LOG_REMOVE 0x352E/AUTO_STORE_BAG_ITEM 0x3999/COMPLETE_QUEST 0x3499/PUSH_QUEST_TO_PARTY 0x349F)
#ifndef CMSG_QUESTGIVER_CHOOSE_REWARD
#define CMSG_QUESTGIVER_CHOOSE_REWARD CMSG_QUEST_GIVER_CHOOSE_REWARD
#endif
#ifndef CMSG_QUESTLOG_REMOVE_QUEST
#define CMSG_QUESTLOG_REMOVE_QUEST CMSG_QUEST_LOG_REMOVE_QUEST
#endif
#ifndef CMSG_AUTOSTORE_BAG_ITEM
#define CMSG_AUTOSTORE_BAG_ITEM CMSG_AUTO_STORE_BAG_ITEM
#endif
#ifndef CMSG_QUESTGIVER_COMPLETE_QUEST
#define CMSG_QUESTGIVER_COMPLETE_QUEST CMSG_QUEST_GIVER_COMPLETE_QUEST
#endif
#ifndef CMSG_PUSHQUESTTOPARTY
#define CMSG_PUSHQUESTTOPARTY CMSG_PUSH_QUEST_TO_PARTY
#endif
//End By leewheel
//By leewheel 2026-07-09: TC使用CMSG_ACTIVATE_TAXI(0x34AB)，AC使用CMSG_ACTIVATETAXI
//TC没有CMSG_ACTIVATETAXIEXPRESS的等价物，使用0xFFFF避免与真实opcode冲突
#ifndef CMSG_ACTIVATETAXI
#define CMSG_ACTIVATETAXI 0x34AB
#endif
#ifndef CMSG_ACTIVATETAXIEXPRESS
#define CMSG_ACTIVATETAXIEXPRESS 0xFFFF
#endif
//End By leewheel
#ifndef MSG_PETITION_DECLINE
#define MSG_PETITION_DECLINE 0
#endif
//By leewheel 2026-08-27: 修正——TC有真实opcode(MSG_RAID_READY_CHECK 已在上方映射为SMSG_READY_CHECK_STARTED；SMSG_GROUP_LIST 已在上方映射为SMSG_PARTY_UPDATE)
#ifndef MSG_RAID_READY_CHECK
#define MSG_RAID_READY_CHECK SMSG_READY_CHECK_STARTED
#endif
#ifndef SMSG_GROUP_LIST
#define SMSG_GROUP_LIST SMSG_PARTY_UPDATE
#endif
//End By leewheel

// ========== 缺失的Form常量（德鲁伊形态） ==========
#ifndef FORM_AQUA
#define FORM_AQUA FORM_AQUATIC_FORM
#endif
#ifndef FORM_BEAR
#define FORM_BEAR FORM_BEAR_FORM
#endif
#ifndef FORM_DIREBEAR
#define FORM_DIREBEAR FORM_DIRE_BEAR_FORM
#endif
#ifndef FORM_CAT
#define FORM_CAT FORM_CAT_FORM
#endif
#ifndef FORM_SPIRITOFREDEMPTION
#define FORM_SPIRITOFREDEMPTION FORM_SPIRIT_OF_REDEMPTION
#endif

// ========== 缺失的物品绑定常量 ==========
#ifndef NO_BIND
#define NO_BIND BIND_NONE
#endif

// ========== 缺失的区域常量 ==========
#ifndef AREA_NAGRAND
#define AREA_NAGRAND 3518
#endif

//By leewheel 20260709: AC的Arena Slot常量，TC中使用纯数字
#ifndef ARENA_SLOT_2v2
#define ARENA_SLOT_2v2 0
#endif
#ifndef ARENA_SLOT_3v3
#define ARENA_SLOT_3v3 1
#endif
#ifndef ARENA_SLOT_5v5
#define ARENA_SLOT_5v5 2
#endif
//End By leewheel

#ifndef AREA_FLAG_ALLOW_DUELS
#define AREA_FLAG_ALLOW_DUELS 0x00000001
#endif

// ========== 缺失的组类型常量 ==========
#ifndef GROUPTYPE_LFG
#define GROUPTYPE_LFG 0x00000040
#endif

// ========== 缺失的NPC标志常量 ==========
#ifndef UNIT_NPC_FLAG_SPIRITHEALER
#define UNIT_NPC_FLAG_SPIRITHEALER NPCFlags(1)
#endif

// ========== 缺失的NPC ID常量 ==========
#ifndef NPC_RISEN_GHOUL
#define NPC_RISEN_GHOUL 280000
#endif
// NPC_THE_LICH_KING 在ICCTriggers.h中已定义为枚举，不在此处定义以避免冲突

// ========== 缺失的SECTION_TYPE常量（角色外观） ==========
#ifndef SECTION_TYPE_SKIN
#define SECTION_TYPE_SKIN 0
#endif
#ifndef SECTION_TYPE_FACE
#define SECTION_TYPE_FACE 1
#endif
#ifndef SECTION_TYPE_FACIAL_HAIR
#define SECTION_TYPE_FACIAL_HAIR 2
#endif
#ifndef SECTION_TYPE_HAIR
#define SECTION_TYPE_HAIR 3
#endif

// ========== 缺失的规格掩码常量 ==========
#ifndef SPEC_MASK_ALL
#define SPEC_MASK_ALL 0x7F
#endif

// ========== 缺失的召唤槽常量 ==========
#ifndef SUMMON_SLOT_TOTEM_FIRE
#define SUMMON_SLOT_TOTEM_FIRE SummonSlot(1)
#endif
#ifndef SUMMON_SLOT_TOTEM_WATER
#define SUMMON_SLOT_TOTEM_WATER SummonSlot(2)
#endif
#ifndef SUMMON_SLOT_TOTEM_AIR
#define SUMMON_SLOT_TOTEM_AIR SummonSlot(3)
#endif

// ========== 缺失的法术附魔常量 ==========
#ifndef MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS
#define MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS 3
#endif

// ========== 缺失的过程标志常量 ==========
//By leewheel 2025-07-10
// TC的PROC_FLAG系统已经重构，将AC的PROC_FLAG映射到TC的对应值
// AC的PROC_FLAG_DONE_PERIODIC对应TC的PROC_FLAG_DEAL_HARMFUL_PERIODIC
#ifndef PROC_FLAG_DONE_PERIODIC
#define PROC_FLAG_DONE_PERIODIC PROC_FLAG_DEAL_HARMFUL_PERIODIC
#endif
// PERIODIC_PROC_FLAG_MASK 在TC中对应 PROC_FLAG_TAKE_HARMFUL_PERIODIC
#ifndef PERIODIC_PROC_FLAG_MASK
#define PERIODIC_PROC_FLAG_MASK PROC_FLAG_TAKE_HARMFUL_PERIODIC
#endif
// 这些值在TC的新系统中不存在，暂时定义为0以避免编译错误
#ifndef PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS
#define PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS 0
#endif
#ifndef PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS
#define PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS 0
#endif
#ifndef PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG
#define PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG 0
#endif
#ifndef PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG
#define PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG 0
#endif
//End By leewheel 2025-07-10

// ========== 缺失的GameObject类型常量 ==========
#ifndef GAMEOBJECT_TYPE_MO_TRANSPORT
#define GAMEOBJECT_TYPE_MO_TRANSPORT 15
#endif

// ========== 缺失的物品子类常量 ==========
#ifndef ITEM_SUBCLASS_WEAPON_FIST
#define ITEM_SUBCLASS_WEAPON_FIST ITEM_SUBCLASS_WEAPON_FIST_WEAPON
#endif
#ifndef ITEM_SUBCLASS_ARMOR_MISC
#define ITEM_SUBCLASS_ARMOR_MISC ITEM_SUBCLASS_ARMOR_MISCELLANEOUS
#endif

// ========== 缺失的关闭代码常量 ==========
#ifndef SHUTDOWN_EXIT_CODE
#define SHUTDOWN_EXIT_CODE 0
#endif

// ========== 缺失的配置常量 ==========
//By leewheel 2026-07-11: TC已在WorldIntConfigs/WorldInt64Configs枚举中定义这些值, 删除宏避免覆盖枚举值
//End By leewheel

// ========== 缺失的ServerHook常量 ==========
#ifndef SERVERHOOK_CAN_PACKET_RECEIVE
#define SERVERHOOK_CAN_PACKET_RECEIVE 0
#endif

// ========== 缺失的DIALOG_STATUS常量 ==========
#ifndef DIALOG_STATUS_NONE
#define DIALOG_STATUS_NONE 0
#endif
#ifndef DIALOG_STATUS_REWARD_REP
#define DIALOG_STATUS_REWARD_REP 4
#endif
#ifndef DIALOG_STATUS_REWARD2
#define DIALOG_STATUS_REWARD2 5
#endif
#ifndef DIALOG_STATUS_INCOMPLETE
#define DIALOG_STATUS_INCOMPLETE 6
#endif
#ifndef DIALOG_STATUS_AVAILABLE
#define DIALOG_STATUS_AVAILABLE 1
#endif
#ifndef DIALOG_STATUS_LOW_LEVEL_AVAILABLE
#define DIALOG_STATUS_LOW_LEVEL_AVAILABLE 2
#endif
#ifndef DIALOG_STATUS_UNAVAILABLE
#define DIALOG_STATUS_UNAVAILABLE 3
#endif
#ifndef DIALOG_STATUS_REWARD
#define DIALOG_STATUS_REWARD 4
#endif

// ========== 缺失的技能常量 ==========
#ifndef SKILL_2H_AXES
#define SKILL_2H_AXES SKILL_TWO_HANDED_AXES
#endif
#ifndef SKILL_2H_MACES
#define SKILL_2H_MACES SKILL_TWO_HANDED_MACES
#endif
#ifndef SKILL_2H_SWORDS
#define SKILL_2H_SWORDS SKILL_TWO_HANDED_SWORDS
#endif

// ========== 缺失的物品ID常量（巫师油/法力油/磨刀石/配重石） ==========
#define BRILLIANT_WIZARD_OIL 22522
#define SUPERIOR_WIZARD_OIL 22521
#define WIZARD_OIL 9252
#define LESSER_WIZARD_OIL 6229
#define MINOR_WIZARD_OIL 6228
#define BRILLIANT_MANA_OIL 22517
#define SUPERIOR_MANA_OIL 22516
#define LESSER_MANA_OIL 20745
#define MINOR_MANA_OIL 20744
#define ADAMANTITE_SHARPENING_STONE 28420
#define FEL_SHARPENING_STONE 23528
#define DENSE_SHARPENING_STONE 12404
#define SOLID_SHARPENING_STONE 7965
#define HEAVY_SHARPENING_STONE 2863
#define COARSE_SHARPENING_STONE 2862
#define ROUGH_SHARPENING_STONE 2861
#define ADAMANTITE_WEIGHTSTONE 28421
#define FEL_WEIGHTSTONE 23529
#define DENSE_WEIGHTSTONE 12643
#define SOLID_WEIGHTSTONE 7966
#define HEAVY_WEIGHTSTONE 3375
#define COARSE_WEIGHTSTONE 3374
#define ROUGH_WEIGHTSTONE 3239

// ========== 缺失的类型前向声明 ==========
struct CharSectionsEntry;
struct CharacterCacheEntry;
struct LFGDungeonEntry;
// CharacterCreateInfo 在 TC 中已被移除，使用替代方案
struct CharacterCreateInfo { std::string Name; uint8 Race = 0; uint8 Class = 0; uint8 Gender = 0; uint8 Skin = 0; uint8 Face = 0; uint8 HairStyle = 0; uint8 HairColor = 0; uint8 FacialHair = 0; uint8 OutfitId = 0; };

// ========== LocalizedString 转换辅助函数 ==========
// AC的LocalizedString可隐式转换为const char*，TC需要显式转换
inline std::string LocalizedStringToStd(const LocalizedString& ls)
{
    return ls[DEFAULT_LOCALE] ? ls[DEFAULT_LOCALE] : "";
}
inline const char* LocalizedStringToCStr(const LocalizedString& ls)
{
    return ls[DEFAULT_LOCALE] ? ls[DEFAULT_LOCALE] : "";
}

//By leewheel 2026-07-13: 多locale技能名称查找辅助函数
// 获取spell的最佳可用名称：先尝试enUS，再尝试zhCN，再尝试其他locale
inline const char* GetSpellNameBestLocale(LocalizedString const* spellName)
{
    if (!spellName)
        return "";

    // 优先enUS（策略中使用英文名）
    const char* name = (*spellName)[LOCALE_enUS];
    if (name && *name)
        return name;

    // 其次zhCN（中文客户端）
    name = (*spellName)[LOCALE_zhCN];
    if (name && *name)
        return name;

    // 尝试所有其他locale
    for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
    {
        if (i == LOCALE_enUS || i == LOCALE_zhCN || i == LOCALE_none)
            continue;
        name = (*spellName)[LocaleConstant(i)];
        if (name && *name)
            return name;
    }

    return "";
}

// 检查spell名称是否匹配qualifier，尝试所有可用locale
// qualifier是策略中定义的英文名（如"fireball"），但也支持中文名匹配
//By leewheel 2026-08-09: 性能优化——qualifier(策略法术名,有限集合)的宽字符小写转换结果缓存,
//原实现每次调用都 Utf8toWStr+wstrToLower(CPU采样热点2%+), 且频繁分配wstring(内存碎片)。
//thread_local 避免多线程(MapUpdater/World)锁竞争, 缓存封顶(qualifier集合有限)非泄漏
inline std::wstring const* GetQualifierWLower(std::string const& qualifier)
{
    static thread_local std::unordered_map<std::string, std::wstring> qualifierCache;
    auto it = qualifierCache.find(qualifier);
    if (it != qualifierCache.end())
        return &it->second;

    std::wstring w;
    if (!Utf8toWStr(qualifier, w))
        return nullptr;
    wstrToLower(w);
    auto [i2, ok] = qualifierCache.emplace(qualifier, std::move(w));
    return ok ? &i2->second : nullptr;
}
//End By leewheel

//By leewheel 2026-08-09: spell locale名称的宽字符小写转换缓存——SpellNameMatches 的 zhCN/其他locale 分支
//每次调用都 Utf8toWStr+wstrToLower+分配wstring, 缓存后减少重复转换与分配(CPU+内存碎片双收益)
//spellName 指针指向 DB2 静态数据, 运行期稳定可作 key; thread_local 避免多线程锁竞争; 缓存封顶(spell数固定)非泄漏
inline std::wstring const* GetSpellLocaleWLower(LocalizedString const* spellName, LocaleConstant loc)
{
    static thread_local std::unordered_map<LocalizedString const*, std::array<std::wstring, TOTAL_LOCALES>> spellCache;
    auto it = spellCache.find(spellName);
    if (it != spellCache.end())
        return &it->second[loc];

    std::array<std::wstring, TOTAL_LOCALES> arr;
    for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
    {
        char const* n = (*spellName)[LocaleConstant(i)];
        if (n && *n)
        {
            std::wstring w;
            if (Utf8toWStr(n, w))
            {
                wstrToLower(w);
                arr[i] = std::move(w);
            }
        }
    }
    auto [i2, ok] = spellCache.emplace(spellName, std::move(arr));
    return ok ? &i2->second[loc] : nullptr;
}
//End By leewheel

inline bool SpellNameMatches(LocalizedString const* spellName, const std::string& qualifier)
{
    if (!spellName)
        return false;

    //By leewheel 2026-08-09: 用缓存替代每次转换
    std::wstring const* pWnamepart = GetQualifierWLower(qualifier);
    if (!pWnamepart)
        return false;
    std::wstring const& wnamepart = *pWnamepart;
    //End By leewheel

    char firstSymbol = tolower(qualifier.empty() ? '\0' : qualifier[0]);
    size_t spellLength = wnamepart.length();

    // 尝试enUS
    const char* name = (*spellName)[LOCALE_enUS];
    if (name && *name &&
        tolower(name[0]) == firstSymbol &&
        strlen(name) == spellLength &&
        Utf8FitTo(name, wnamepart))
        return true;

    // 尝试zhCN - By leewheel 2026-08-02: 收紧匹配——先精确(wstring长度+首字符+全等)再子串，
    // 原实现仅Utf8FitTo(子串匹配)导致"诱惑"等名可匹配到多个含相同子串的法术(含控制类)，解析可能选中错误法术
    name = (*spellName)[LOCALE_zhCN];
    if (name && *name)
    {
        //By leewheel 2026-08-09: 用缓存替代每次Utf8toWStr+wstrToLower
        std::wstring const* pwname = GetSpellLocaleWLower(spellName, LOCALE_zhCN);
        if (pwname)
        {
            std::wstring const& wname = *pwname;
            if (wname.length() == wnamepart.length() && tolower(name[0]) == firstSymbol && wname == wnamepart)
                return true;  // 精确匹配
            if (wname.find(wnamepart) != std::wstring::npos)
                return true;  // 子串fallback(处理带等级后缀等场景)
        }
        //End By leewheel
    }

    // 尝试其他locale - By leewheel 2026-08-02: 同样收紧(精确优先)
    for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
    {
        if (i == LOCALE_enUS || i == LOCALE_zhCN || i == LOCALE_none)
            continue;
        name = (*spellName)[LocaleConstant(i)];
        if (name && *name)
        {
            //By leewheel 2026-08-09: 用缓存替代每次Utf8toWStr+wstrToLower
            std::wstring const* pwname = GetSpellLocaleWLower(spellName, LocaleConstant(i));
            if (pwname)
            {
                std::wstring const& wname = *pwname;
                if (wname.length() == wnamepart.length() && tolower(name[0]) == firstSymbol && wname == wnamepart)
                    return true;  // 精确匹配
                if (wname.find(wnamepart) != std::wstring::npos)
                    return true;  // 子串fallback
            }
            //End By leewheel
        }
    }

    return false;
}
//End By leewheel

//By leewheel 2026-07-14: spellnameeng表缓存 - 用于DB2 enUS locale数据缺失时的fallback
// 用户在hotfix数据库创建了spellnameeng表(ID int, Name_lang text)存储英文技能名
// 当(*spellName)[LOCALE_enUS]为空时，用spellId从此缓存查找英文名
extern std::unordered_map<uint32, std::string> sSpellNameEngCache;

// 从spellnameeng缓存获取英文技能名
inline const char* GetSpellNameEngFromCache(uint32 spellId)
{
    auto it = sSpellNameEngCache.find(spellId);
    if (it != sSpellNameEngCache.end() && !it->second.empty())
        return it->second.c_str();
    return nullptr;
}

// 带spellnameeng缓存的SpellNameMatches - 先试DB2多locale匹配，失败后查缓存
inline bool SpellNameMatchesWithCache(uint32 spellId, LocalizedString const* spellName, const std::string& qualifier)
{
    // 先试DB2多locale匹配
    if (SpellNameMatches(spellName, qualifier))
        return true;

    // Fallback: 查spellnameeng缓存
    const char* engName = GetSpellNameEngFromCache(spellId);
    if (engName && *engName)
    {
        std::wstring wnamepart;
        if (Utf8toWStr(qualifier, wnamepart))
        {
            wstrToLower(wnamepart);
            char firstSymbol = tolower(qualifier.empty() ? '\0' : qualifier[0]);
            size_t spellLength = wnamepart.length();

            if (tolower(engName[0]) == firstSymbol &&
                strlen(engName) == spellLength &&
                Utf8FitTo(engName, wnamepart))
                return true;
        }
    }
    return false;
}

// 带spellnameeng缓存的GetSpellNameBestLocale
//By leewheel 2026-07-22: 英文名优先！bot策略全部使用英文名，必须确保返回英文
// 优先级: DB2 enUS > cache英文 > DB2 zhCN > 其他locale
// 原bug: GetSpellNameBestLocale返回zhCN后就不查cache了，HasAura用中文匹配英文永远失败
inline const char* GetSpellNameBestLocaleWithCache(uint32 spellId, LocalizedString const* spellName)
{
    if (!spellName)
        return "";

    // 1. 优先DB2 enUS
    const char* name = (*spellName)[LOCALE_enUS];
    if (name && *name)
        return name;

    // 2. 查spellnameeng缓存（classic_db2.spellname完整英文名）
    const char* engName = GetSpellNameEngFromCache(spellId);
    if (engName && *engName)
        return engName;

    // 3. 退而求其次用zhCN（至少不为空）
    name = (*spellName)[LOCALE_zhCN];
    if (name && *name)
        return name;

    // 4. 尝试其他locale
    for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
    {
        if (i == LOCALE_enUS || i == LOCALE_zhCN || i == LOCALE_none)
            continue;
        name = (*spellName)[LocaleConstant(i)];
        if (name && *name)
            return name;
    }

    return "";
}
//End By leewheel

// ========== ItemTemplateContainer 指针兼容 ==========
// AC: sObjectMgr->GetItemTemplateStore() 返回指针
// TC: sObjectMgr->GetItemTemplateStore() 返回引用
// 在使用处需要加 & 取地址，或者使用此辅助函数
inline const ItemTemplateContainer* GetItemTemplateStorePtr()
{
    return &sObjectMgr->GetItemTemplateStore();
}

// ========== CreatureTemplateContainer 指针兼容 ==========
inline const CreatureTemplateContainer* GetCreatureTemplateStorePtr()
{
    // By leewheel 2026-07-09: TC使用GetCreatureTemplates()而不是GetCreatureTemplateStore()
    return &sObjectMgr->GetCreatureTemplates();
    // End By leewheel
}

// ========== GameObjectTemplateContainer 指针兼容 ==========
inline const GameObjectTemplateContainer* GetGameObjectTemplateStorePtr()
{
    // By leewheel 2026-07-09: TC使用GetGameObjectTemplates()而不是GetGameObjectTemplateStore()
    return &sObjectMgr->GetGameObjectTemplates();
    // End By leewheel
}

// ========== Trinity::Containers::SelectRandomContainerElement 兼容 ==========
// AC有此函数，TC也有但在不同命名空间
#ifndef SelectRandomContainerElement
#define SelectRandomContainerElement Trinity::Containers::SelectRandomContainerElement
#endif

// ========== EMOTE_ONESHOT_NOD 常量 ==========
//By leewheel 20260710: TC的Emote是普通enum，值名为EMOTE_ONESHOT_BOW不是Emote::OneShotBow
#ifndef EMOTE_ONESHOT_NOD
#define EMOTE_ONESHOT_NOD EMOTE_ONESHOT_BOW
#endif
//End By leewheel

// ========== sCharacterCache 兼容 ==========
// AC有sCharacterCache全局单例，TC使用sCharacterCache
// 确保头文件已包含
#include "CharacterCache.h"

// ========== AC HasFlag/SetFlag/RemoveFlag 兼容层 ==========
// AC使用HasFlag(UNIT_FIELD_FLAGS, flag)方式，TC使用HasUnitFlag(flag)等方式
// 通过宏将AC风格映射到TC风格
// 注意：这些宏需要与UNIT_FIELD_FLAGS等占位常量配合使用
// 由于无法通过宏自动分发，直接在源文件中修改调用点

// ========== PlayerbotTextMgr 头文件包含 ==========
#include "Mgr/Text/PlayerbotTextMgr.h"

// ========== AllSpellScript/AllCreatureScript 兼容类 ==========
#include "Compat/AllScriptCompat.h"

// ========== WorldPackets 兼容层 ==========
// 提供内联辅助函数，自动构造WorldPackets结构体并调用WorldSession handler
// 用法: WPPCompat::DoGossipHello(session, guid) 替代手动构造WorldPacket
#include "Compat/PlayerbotsWPPCompat.h"

// ========== Player/Unit 方法兼容宏 ==========
// AC使用小写方法名，TC使用大写开头
#define isSwimming() IsInWater()
#define isResurrectRequested() IsResurrectRequested()
#define GetCharm() GetCharmed()
#define GetSpellCooldownDelay(spellId) GetSpellCooldown(spellId)
//By leewheel 2026-07-22: 改为可变参数宏，原0参数宏在MSVC下丢弃resetTalents(true)的参数
#define resetTalents(...) ResetTalents(__VA_ARGS__)
#define RemoveAllSpellCooldown() RemoveAllSpellCooldowns()
#define BotCanUseItem(proto) CanUseItem(proto, true)
#define HasTankSpec() (GetTalentBranchSpec(GetActiveSpec()) == 0)
//By leewheel 2026-07-10: AC的Player::GetActiveSpec()和HasTalent()在TC中不存在，添加兼容宏
#define GetActiveSpec() GetActiveTalentGroup()
#define HasTalent(spellId, spec) HasSpell(spellId)
//End By leewheel

// ========== Unit 方法兼容宏 ==========
#define HasFeatherFallAura() HasAuraType(SPELL_AURA_FEATHER_FALL)
#define HasWaterWalkAura() HasAuraType(SPELL_AURA_WATER_WALK)
#define HasFlyAura() HasAuraType(SPELL_AURA_FLY)
#define HasIncreaseMountedFlightSpeedAura() HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
#define HasConfuseAura() HasAuraType(SPELL_AURA_MOD_CONFUSE)
#define HasStunAura() HasAuraType(SPELL_AURA_MOD_STUN)
#define HasFearAura() HasAuraType(SPELL_AURA_MOD_FEAR)
#define HasSpiritOfRedemptionAura() HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION)

// ========== PathGenerator 方法兼容宏 ==========
//By leewheel 2026-07-11: 移除getPathLength()宏, 它会污染WorldPosition::getPathLength()方法
// #define getPathLength() GetPathLength()
//End By leewheel

// ========== MotionMaster 兼容辅助 ==========
// AC有MoveForwards/MovePointBackwards，TC没有
// MoveForwards 等价于 MovePoint 前方位置
// MovePointBackwards 等价于 MovePoint 但反向移动
// 在使用处直接替换为MovePoint调用

// ========== QuestRelationBounds 类型兼容 ==========
// AC使用QuestRelationBounds，TC使用QuestRelationResult
using QuestRelationBounds = QuestRelationResult;
using QuestRelationBoundsReverse = Trinity::IteratorPair<QuestRelationsReverse::const_iterator>;

// AC的ObjectMgr方法名兼容宏
#define GetCreatureQuestRelationMap() GetCreatureQuestRelationMapHACK()
//By leewheel 20260709: 修复Involved关系map的宏，使用新增的HACK方法
#define GetCreatureQuestInvolvedRelationMap() GetCreatureQuestInvolvedRelationMapHACK()
#define GetGOQuestRelationMap() GetGOQuestRelationMapHACK()
#define GetGOQuestInvolvedRelationMap() GetGOQuestInvolvedRelationMapHACK()
//End By leewheel
#define GetCreatureQuestRelationBounds(entry) sObjectMgr->GetCreatureQuestRelations(entry)
#define GetCreatureQuestInvolvedRelationBounds(entry) sObjectMgr->GetCreatureQuestInvolvedRelations(entry)
#define GetGOQuestRelationBounds(entry) sObjectMgr->GetGOQuestRelations(entry)
#define GetGOQuestInvolvedRelationBounds(entry) sObjectMgr->GetGOQuestInvolvedRelations(entry)

// ========== DBC结构成员兼容宏 ==========
// TC将许多DBC成员改为方法，需要添加()调用
// 这些宏不能全局替换（会与类型名冲突），需要在源文件中手动修改

// ========== ItemTemplate 方法兼容宏 ==========
// AC使用直接成员访问，TC使用getter方法
// 注意：不能用宏全局替换因为会影响类型定义
// 在源文件中将 proto->Member 改为 proto->GetMember()

// ========== Item 方法兼容 ==========
// AC的Item::GetUInt32Value已通过Item_GetUInt32Value内联函数兼容
// 但源文件中直接调用 item->GetUInt32Value(index) 需要修改为兼容调用
// 在源文件中修改为直接使用GetDurability()/GetMaxDurability()等

// ========== WorldSafeLocsEntry 兼容 ==========
// AC使用GetPositionX/Y/Z，TC使用Loc.X/Y/Z
// 需要在源文件中修改

// ========== G3D::Vector2 头文件包含 ==========
#include "G3D/Vector2.h"

// ========== Corpse 头文件包含 ==========
#include "Entities/Corpse/Corpse.h"

// ========== FlightPathMovementGenerator 前向声明 ==========
class FlightPathMovementGenerator;

// ========== Spell 前向声明和类型 ==========
struct TargetInfo;

// ========== OutdoorPvP 兼容 ==========
// AC的OutdoorPvP::GetCapturePoints返回map引用
// TC使用不同API，需要在源文件中适配

// ========== POIInfo 兼容 ==========
// AC的POIInfo使用G3D::Vector2，已通过包含G3D/Vector2.h解决

// ========== getMSTime 兼容 ==========
// AC有getMSTime()返回uint32，TC也有但getMSTime_diff_to_now签名不同
inline uint32 getMSTimeDiffToNow(uint32 oldMSTime)
{
    return GetMSTimeDiffToNow(oldMSTime);
}

// ========== Map2ZoneCoordinates / Zone2MapCoordinates 兼容 ==========
// AC有这两个坐标转换函数，TC没有
// AC签名: void Map2ZoneCoordinates(float& x, float& y, uint32 zoneId)
// TC没有对应的坐标转换函数，这里做空操作保持兼容
// By leewheel 2026-07-08
//By leewheel 2026-09-03 修复C4100警告：TC为空操作，x/y参数不被引用，显式省略参数名
inline void Map2ZoneCoordinates(float& /*x*/, float& /*y*/, uint32 /*zoneId*/)
{
    // TC不需要坐标转换，直接保持原值
}
inline void Zone2MapCoordinates(float& /*x*/, float& /*y*/, uint32 /*zoneId*/)
{
    // TC不需要坐标转换，直接保持原值
}
// End By leewheel

// ========== GetVirtualMapForMapAndZone 兼容 ==========
// AC有此函数，TC通过Map系统处理
//By leewheel 2026-09-03 修复C4100警告：zoneId参数不被引用，显式省略参数名
inline uint32 GetVirtualMapForMapAndZone(uint32 mapid, uint32 /*zoneId*/)
{
    return mapid;
}

// ========== Quest 兼容方法 ==========
// AC: quest->GetRepObjectiveFaction() → TC: quest->RewardFactionID1
// AC: quest->GetRepObjectiveValue() → TC: quest->RewardFactionValue1
// AC: quest->GetRewOrReqMoney() → TC: quest->GetRewMoney() or quest->GetReqMoney()
// AC: quest->IsAutoComplete() → TC: quest->IsAutoComplete() (check if exists)
// 这些需要在源文件中修改

// ========== Item::CreateItem 兼容 ==========
// AC: Item::CreateItem(entry, count, player) 
// TC: Item::CreateItem(entry, count, player->GetMap(), ...)

// ========== Player::StoreNewItemInBestSlots 兼容 ==========
// AC: StoreNewItemInBestSlots(item, count) - 2 params
// TC: StoreNewItemInBestSlots(item, count, ...) - different params

// ========== Player::CanRewardQuest 兼容 ==========
// AC: CanRewardQuest(quest, slot) - 2 params (or 3)
// TC: CanRewardQuest(quest) - 1 param

// ========== Player::SaveToDB 兼容 ==========
// AC: SaveToDB(...) - different params
// TC: SaveToDB() - no params (or different)

// ========== Player::FindEquipSlot 兼容 ==========
// AC: FindEquipSlot(ItemTemplate*, uint32, bool)
// TC: FindEquipSlot(Item const*, uint32, bool, bool)

// ========== BattlegroundWS 兼容 ==========
// AC: GetOtherTeamId() → TC: need different approach
// AC: GetFlagState(TeamId) → TC: GetFlagState(Team)

// ========== BG_AV_NodeInfo 兼容 ==========
// AC: OwnerId → TC: different member name

// ========== Roll 结构兼容 ==========
// AC: Roll::itemGUID → TC: different member
// AC: Group::GetRolls() → TC: different API
// AC: Group::CountRollVote() → TC: different signature

// ========== LootTemplate::Process 兼容 ==========
// AC: Process(LootStore, ...) → TC: Process(Loot, bool, uint16, uint8)

// ========== ObjectGuid::Create 兼容 ==========
// AC有ObjectGuid::Create静态方法，TC使用构造函数

// ========== sObjectMgr->GetGossipText 兼容 ==========
// AC有此方法，TC使用不同API

// ========== sObjectMgr->GetQuestPOIVector 兼容 ==========
// AC有此方法，TC使用QuestPOIManager

// ========== CharacterCacheEntry 兼容 ==========
// AC有CharacterCacheEntry结构，TC使用CharacterCacheEntryPointer
// 需要在源文件中适配

// ========== Database Query/Execute 兼容 ==========
// AC使用PQuery/PExecute，TC使用PreparedStatement
// 需要在源文件中修改

// ========== ServerScript 兼容 ==========
// AC的ServerScript构造接受(name, hooks)，TC只接受name

// ========== EmblemInfo 兼容 ==========
// AC: EmblemInfo(style, color, borderStyle, borderColor, background) - 5 params
// TC: EmblemInfo() - default constructor, use SetStyle etc.

// ========== ChatCommand 兼容 ==========
// AC和TC的ChatCommand系统不同

// ========== Guild 兼容 ==========
// Guild的GetMember/Member/_GetRankRights是private的
// 需要在Guild.h中添加public包装方法

// ========== Vehicle 兼容 ==========
// AC: VehicleSeatEntry::CanControl → TC: different
// AC: VehicleEntry::m_flags → TC: different

// ========== DurabilityCostsEntry/DurabilityQualityEntry 兼容 ==========
// AC: multiplier/quality_mod → TC: different

// ========== Spell::GetUniqueTargetInfo 兼容 ==========
// AC有此方法，TC使用m_uniqueTargets

// ========== Spell::prepare 兼容 ==========
// AC: prepare(SpellCastTargets*) → TC: prepare(const SpellCastTargets&)

// ========== SpellCastTargets::Write 兼容 ==========
// AC: Write(WorldPacket&) → TC: Write(WorldPackets::Spells::SpellTargetData&)

// ========== WorldPacket::appendPackGUID 兼容 ==========
// TC不支持，需要用ObjectGuid的Write方法
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，WotLK的ObjectGuid::WriteAsPacked被移除，
//Cata通过ByteBuffer的operator<<直接写入压缩GUID
inline void WorldPacketAppendPackGUID(WorldPacket& data, ObjectGuid guid)
{
    data << guid;
}
#define appendPackGUID(guid) WorldPacketAppendPackGUID(*this, guid)

// ========== WorldSession Handler 方法名映射 ==========
// AC和TC的handler方法名不同，通过宏映射
// 这些宏仅在Playerbots模块内生效
#define HandleGuildAcceptOpcode HandleGuildAcceptInvite
#define HandleGuildDeclineOpcode HandleDeclineGuildInvites
#define HandleGuildLeaveOpcode HandleGuildLeave
#define HandlePetitionBuyOpcode HandlePetitionBuy
#define HandleOfferPetitionOpcode HandleOfferPetition
#define HandleTurnInPetitionOpcode HandleTurnInPetition
#define HandlePetitionDeclineOpcode HandleDeclinePetition
#define HandleGroupInviteOpcode HandlePartyInviteOpcode

// ========== Player 方法兼容宏 ==========
#ifndef learnSpell
#define learnSpell LearnSpell
#endif
#ifndef getPowerType
#define getPowerType GetPowerType
#endif

// ========== Unit 方法兼容宏 ==========
#ifndef GetPackGUID
#define GetPackGUID() (GetGUID())
#endif

//By leewheel 2026-07-09: AC兼容 - Creature::GetSummonerGUID
// TC使用GetCharmerOrOwnerGUID或GetCharmerOrOwnerUnit
#define GetSummonerGUID() GetCharmerOrOwnerGUID()
//End By leewheel

// ========== CharmInfo 方法兼容 ==========
#ifndef SetPlayerReactState
// By leewheel 2026-07-11: TC的CharmInfo没有SetPlayerReactState，用SetReactState替代
// 注意: SetReactState是Unit的方法不是CharmInfo的，需要通过Unit调用
// 在使用处直接用pet->SetReactState()替代ci->SetPlayerReactState()
#endif
// By leewheel 2026-07-11: TC的CharmInfo没有SetStayPosition方法，只有SaveStayPosition()
// RemoveStayPosition应该用SetIsAtStay(false)来清除停留状态
#ifndef RemoveStayPosition
#define RemoveStayPosition() SetIsAtStay(false)
#endif

// ========== Creature 方法兼容 ==========
// AC: Creature::IsClass → TC: 使用ToPlayer()->GetClass()等
// IsClass在TC中不存在于Creature，需要通过GetClass()比较

// ========== NPC Flag 兼容 ==========
// By leewheel 2026-07-08
// AC使用UNIT_NPC_FLAG_SPIRITGUIDE，TC中对应UNIT_NPC_FLAG_SPIRIT_HEALER
#ifndef UNIT_NPC_FLAG_SPIRITGUIDE
#define UNIT_NPC_FLAG_SPIRITGUIDE UNIT_NPC_FLAG_SPIRIT_HEALER
#endif
// End By leewheel

// ========== DBC结构成员兼容辅助函数 ==========
// TC将许多DBC成员从直接访问改为getter方法
// 这些内联函数提供统一的访问接口

// AreaTableEntry: 已在下方1763行统一定义AreaTableEntry_IsFlyable，此处移除重复定义
// By leewheel 2026-07-08
// End By leewheel

// CreatureTemplate
// By leewheel 2026-07-09: TC中LootID和GossipMenuId是方法不是成员变量，type_flags需要通过CreatureDifficulty获取
//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//LootID在Cata的CreatureDifficulty结构中；GossipMenuId变为GossipMenuIds向量
inline uint32 CreatureTemplate_GetLootID(const CreatureTemplate* entry)
{
    auto diff = entry->GetDifficulty(DIFFICULTY_NORMAL);
    return diff ? diff->LootID : 0;
}
inline uint32 CreatureTemplate_GetTypeFlags(const CreatureTemplate* entry)
{
    auto diff = entry->GetDifficulty(DIFFICULTY_NORMAL);
    return diff ? diff->TypeFlags : 0;
}
inline uint32 CreatureTemplate_GetGossipMenuId(const CreatureTemplate* entry)
{
    //Cata的GossipMenuIds是向量，返回首个菜单ID
    return (entry && !entry->GossipMenuIds.empty()) ? entry->GossipMenuIds[0] : 0;
}
// End By leewheel

// EmotesTextEntry
// By leewheel 2026-07-09: TC中EmotesTextEntry使用textid()方法而非TextID成员
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata的EmotesTextEntry只有{ID,Name,EmoteID}，
//原textid()语义(文本表情对应的动作ID)由EmoteID字段承担
inline uint16 EmotesTextEntry_GetTextId(const EmotesTextEntry* entry)
{
    return entry->EmoteID;
}
// End By leewheel
inline uint32 EmotesTextEntry_GetId(const EmotesTextEntry* entry)
{
    return entry->ID;
}

// GlyphPropertiesEntry
inline uint32 GlyphPropertiesEntry_GetSpellId(const GlyphPropertiesEntry* entry)
{
    return entry->SpellID;
}
inline uint32 GlyphPropertiesEntry_GetId(const GlyphPropertiesEntry* entry)
{
    return entry->ID;
}

// GlyphSlotEntry
// By leewheel 2026-07-09: TC中GlyphSlotEntry使用Type而非TypeFlags
inline uint32 GlyphSlotEntry_GetTypeFlags(const GlyphSlotEntry* entry)
{
    return entry->Type;
}
// End By leewheel

// DurabilityCostsEntry - TC使用不同的字段名
// AC: multiplier → TC: 不存在此字段，需要通过ItemSubClass计算
// DurabilityQualityEntry - AC: quality_mod → TC: 不存在此字段

// PVPDifficultyEntry
inline uint8 PVPDifficultyEntry_GetMinLevel(const PVPDifficultyEntry* entry)
{
    return entry->MinLevel;
}
inline uint8 PVPDifficultyEntry_GetMaxLevel(const PVPDifficultyEntry* entry)
{
    return entry->MaxLevel;
}

// WorldSafeLocsEntry
// By leewheel 2026-07-09: TC中WorldLocation使用GetPositionX/Y/Z()方法而非X/Y/Z成员
inline float WorldSafeLocsEntry_GetPositionX(const WorldSafeLocsEntry* entry)
{
    return entry->Loc.GetPositionX();
}
inline float WorldSafeLocsEntry_GetPositionY(const WorldSafeLocsEntry* entry)
{
    return entry->Loc.GetPositionY();
}
inline float WorldSafeLocsEntry_GetPositionZ(const WorldSafeLocsEntry* entry)
{
    return entry->Loc.GetPositionZ();
}
// End By leewheel

// ========== LiquidData 兼容 ==========
// TC的LiquidData结构与AC完全不同
// AC: LiquidData { Entry, Level, DepthLevel }
// TC: LiquidData { ... different fields ... }

// ========== PlayerInfo 兼容 ==========
// AC: PlayerInfo { positionX, positionY, positionZ, orientation, mapid }
// TC: PlayerInfo { Pos { X, Y, Z, O }, MapId }

// ========== Trainer 兼容 ==========
// AC: Trainer::Trainer 有 GetSpells(), CanTeachSpell() 等public方法
// TC: Trainer 使用不同的API

// ========== Spell 兼容 ==========
// AC: Spell::GetUniqueTargetInfo() → TC: m_uniqueTargets
// AC: Spell::prepare(SpellCastTargets*) → TC: prepare(const SpellCastTargets&)
// AC: SpellCastTargets::Write(WorldPacket&) → TC: Write(SpellTargetData&)

// ========== Missing types and constants ==========
#ifndef MAX_GAMEOBJECT_QUEST_ITEMS
#define MAX_GAMEOBJECT_QUEST_ITEMS 6
#endif

// GameObjectQuestItemList - AC类型，TC不使用
using GameObjectQuestItemList = std::vector<uint32>;

// GossipMenuItemsMapBounds - AC类型，TC使用不同类型
// TC: sObjectMgr->GetGossipMenuItemsMapBounds(entry) 返回 IteratorPair
#include "GossipDef.h"

// ========== LFG DBC Store 兼容 ==========
// AC: sLFGDungeonStore, LFGDungeonEntry
// TC: sLFGDungeonsStore, LFGDungeonsEntry (已通过上方宏映射)
// TC的LFGDungeonsEntry已有ID/Name/MinLevel/MaxLevel/TypeID等字段，直接使用即可

// ========== ReputationMgr 兼容 ==========
// AC: ReputationMgr::PointsInRank → TC: 不存在此方法

// ========== Vehicle 兼容 ==========
// AC: VehicleSeatEntry::CanControl → TC: 不同
// AC: VehicleEntry::m_flags → TC: 不同

// ========== SpellMgr 兼容 ==========
// AC: SpellMgr::GetSpellInfoStoreSize → TC: 不存在

// ========== Player 缺失方法兼容 ==========
// AC: Player::GetActiveSpecMask → TC: 不存在
// AC: Player::GetSpellCooldownMap → TC: 不存在
// AC: Player::GetSpellCooldownDelay → TC: GetSpellCooldown (已映射)
// AC: PlayerSpellState → TC: 使用不同枚举

// ========== PlayerSpell::specMask 兼容 ==========
// AC: PlayerSpell { specMask } → TC: 不同

// ========== TempSummon → Creature 兼容 ==========
// AC返回Creature*，TC返回TempSummon*
// TempSummon继承自Creature，可以直接使用
// 但某些地方需要显式转换
inline Creature* TempSummonToCreature(TempSummon* summon)
{
    return summon;
}

// ========== Item 缺失方法兼容 ==========
// AC: Item::GetUInt32Value → TC: 不存在 (使用GetDurability等)
// AC: Item::GetInt32Value → TC: 不存在
// AC: Item::RemoveFromUpdateQueueOf → TC: 不存在
// AC: Item::CreateItem(entry, count, player) → TC: CreateItem(entry, count, itemContext)

// ========== ItemTemplate 缺失成员兼容 ==========
// AC: ContainerSlots → TC: GetContainerSlots()
// 已在源文件中手动修改

// ========== Battleground 兼容 ==========
// AC: BattlegroundWS::GetOtherTeamId → TC: 不同API
// AC: BG_AV_NodeInfo::OwnerId → TC: 不同成员名

// ========== Group::GetRolls / CountRollVote 兼容 ==========
// AC API与TC不同

// ========== OutdoorPvP 兼容 ==========
// AC: OutdoorPvP::GetCapturePoints → TC: 不同API

// ========== sObjectMgr 缺失方法 ==========
// AC: GetGossipText → TC: 不存在

// ========== ItemSubClassToDurabilityMultiplierId 兼容 ==========
// AC有此函数，TC不存在
inline uint32 ItemSubClassToDurabilityMultiplierId(uint32 /*itemClass*/, uint32 /*itemSubClass*/)
{
    return 0;
}

// ========== GetSkillLineAbilitiesBySkillLine 兼容 ==========
// AC有此函数，TC不存在
inline std::vector<uint32> GetSkillLineAbilitiesBySkillLine(uint32 /*skillLine*/)
{
    return {};
}

// ========== GetSkillRaceClassInfo 兼容 ==========
// AC有此函数，TC不存在
inline bool GetSkillRaceClassInfo(uint32 /*skillId*/, uint32 /*race*/, uint32 /*class_*/)
{
    return false;
}

// ========== GetTalentSpellPos 兼容 ==========
// AC有此全局函数，TC使用sDB2Manager.GetTalentSpellPos方法，返回TalentSpellPos*
// 调用处已改为使用sDB2Manager.GetTalentSpellPos

// ========== GetTalentTabPages 兼容 ==========
// AC有此全局函数，TC使用sDB2Manager.GetTalentTabPages方法，返回uint32*
// 调用处已改为使用sDB2Manager.GetTalentTabPages

// ========== confirmAccept 兼容 ==========
// AC: quest->confirmAccept → TC: Quest::GetConfirmAccept() 返回string

// ========== PlayerSpellState 枚举兼容 ==========
// TC已在Player.h中定义 PlayerSpellState : uint8 枚举，包含:
// PLAYERSPELL_UNCHANGED=0, PLAYERSPELL_CHANGED=1, PLAYERSPELL_NEW=2, PLAYERSPELL_REMOVED=3, PLAYERSPELL_TEMPORARY=4
// 无需重复定义

// PlayerSpell::specMask 兼容 - TC的PlayerSpell没有specMask字段
// AC代码中使用specMask的地方需要适配为始终返回SPEC_MASK_ALL

// ========== Roll 兼容 ==========
// AC: Roll::itemGUID → TC: Roll没有itemGUID字段，使用lootObjectGuid或itemid
// AC代码中使用roll->itemGUID的地方需要修改为获取Roll关联的ObjectGuid
// TC的Roll通过Group::GetRoll(lootObjectGuid, lootListId)获取

// ========== Group::GetRolls 兼容 ==========
// AC: group->GetRolls() 返回Roll向量引用
// TC: Group没有public GetRolls方法，Roll存储在Group内部
// 需要在Group.h中添加public包装方法

// ========== CreatureTemplate loot字段兼容 ==========
// AC: creatureTemplate->pickpocketLootId / creatureTemplate->SkinLootId
// TC: 这些字段在CreatureDifficulty中，通过creature->GetCreatureDifficulty()访问
// 对于CreatureTemplate，TC没有这些字段
// 提供兼容内联函数
inline uint32 CreatureTemplate_GetPickpocketLootId(const Creature* creature)
{
    return creature->GetCreatureDifficulty()->PickPocketLootID;
}
inline uint32 CreatureTemplate_GetSkinLootId(const Creature* creature)
{
    return creature->GetCreatureDifficulty()->SkinLootID;
}

// ========== Map::GetLiquidData 兼容 ==========
// AC: map->GetLiquidData(x, y, z, liquidType) 返回LiquidData引用
// TC: map->GetLiquidStatus(phaseShift, x, y, z, reqLiquidType, &liquidData) 返回ZLiquidStatus
// 提供兼容内联函数
inline LiquidData Map_GetLiquidData(Map* map, Unit* unit, float x, float y, float z)
{
    LiquidData data;
    map->GetLiquidStatus(unit->GetPhaseShift(), x, y, z, map_liquidHeaderTypeFlags::AllLiquids, &data);
    return data;
}

// ========== Map::isInLineOfSight 兼容 ==========
// AC: map->isInLineOfSight(x1, y1, z1, x2, y2, z2, phasemask)
// TC: map->isInLineOfSight(phaseShift, x1, y1, z1, x2, y2, z2, checks, flags)
inline bool Map_isInLineOfSight(Map* map, Unit* unit, float x1, float y1, float z1, float x2, float y2, float z2)
{
    return map->isInLineOfSight(unit->GetPhaseShift(), x1, y1, z1, x2, y2, z2, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}

// ========== Map::GetHeight 兼容 ==========
// AC: map->GetHeight(x, y, z, checkWater) 
// TC: map->GetHeight(phaseShift, x, y, z, ...)
inline float Map_GetHeight(Map* map, Unit* unit, float x, float y, float z, bool checkWater = false)
{
    return map->GetHeight(unit->GetPhaseShift(), x, y, z, checkWater);
}

// ========== Unit::SetSpeed 兼容 ==========
// AC: unit->SetSpeed(type, rate, forced) - 3 params
// TC: unit->SetSpeed(type, rate) - 2 params
// 使用宏丢弃第三个参数（仅在Playerbots模块内生效）
#define SetSpeed3(type, rate, forced) SetSpeed(type, rate)

// ========== Player::GetNPCIfCanInteractWith 兼容 ==========
// AC: player->GetNPCIfCanInteractWith(guid, flags) - 2 params
// TC: player->GetNPCIfCanInteractWith(guid, flags1, flags2) - 3 params
// 使用宏自动添加第三个参数
#define GetNPCIfCanInteractWith2(guid, flags) GetNPCIfCanInteractWith(guid, flags, NPCFlags2(0))

// ========== PlayerSpell field compat ==========
// AC uses .State and .Active (without parentheses)
// TC has .state and .active (fields) with .State() and .Active() (methods)
// 用宏映射大写无括号访问到小写字段
// 注意: 这只在使用PlayerSpell成员时生效
#define specMask SPEC_MASK_ALL

// ========== Player::GetActiveSpecMask 兼容 ==========
//By leewheel 2026-09-03 修复C4100警告：本兼容函数恒返回SPEC_MASK_ALL，player参数不被引用，显式省略参数名
inline uint32 Player_GetActiveSpecMask(Player* /*player*/)
{
    return SPEC_MASK_ALL;
}

// ========== Player::GetSpellCooldownMap 兼容 ==========
// TC不使用GetSpellCooldownMap，返回空
// 代码中调用此方法的地方需要适配

// ========== Player::isSwimming 兼容 ==========
// 已通过宏定义: #define isSwimming() IsInWater()

// ========== AreaTableEntry::IsFlyable 兼容 ==========
// AC: area->IsFlyable()
// TC: 没有此方法，通过flags判断
inline bool AreaTableEntry_IsFlyable(const AreaTableEntry* area)
{
    return (area->Flags[0] & AREA_FLAG_NO_FLY_ZONE) == 0;
}
#define IsFlyable() AreaTableEntry_IsFlyable(this)

// ========== getMSTimeDiff 兼容 ==========
// AC: getMSTimeDiff(oldTime) - 1 param
// TC: GetMSTimeDiffToNow(oldTime)
inline uint32 getMSTimeDiff(uint32 oldTime)
{
    return GetMSTimeDiffToNow(oldTime);
}
//By leewheel 2026-07-09: TC的Timer.h已经定义了2参数版本的getMSTimeDiff(uint32, uint32)，不再重复定义
//End By leewheel

// ========== ObjectGuid::Create 兼容 ==========
// AC: ObjectGuid::Create(HIGHGUID_PLAYER, realm, dbId)
// TC: 使用ObjectGuid构造函数
// By leewheel 2026-07-09: TC的ObjectGuid::Create签名与AC不同
// Player: Create<HighGuid::Player>(dbId) - 1个参数
// Creature/GameObject: Create<HighGuid::Creature>(mapId, entry, counter) - 3个参数
inline ObjectGuid ObjectGuid_CreatePlayer(uint32 realm, uint32 dbId)
{
    (void)realm; // TC不需要realm参数
    return ObjectGuid::Create<HighGuid::Player>(dbId);
}
// TC没有HighGuid::Unit，使用HighGuid::Creature替代
inline ObjectGuid ObjectGuid_CreateUnit(uint32 mapId, uint32 dbId)
{
    return ObjectGuid::Create<HighGuid::Creature>((uint16)mapId, 0, dbId);
}
inline ObjectGuid ObjectGuid_CreateCreature(uint32 mapId, uint32 dbId)
{
    return ObjectGuid::Create<HighGuid::Creature>((uint16)mapId, 0, dbId);
}
inline ObjectGuid ObjectGuid_CreateGameObject(uint32 mapId, uint32 dbId)
{
    return ObjectGuid::Create<HighGuid::GameObject>((uint16)mapId, 0, dbId);
}
// End By leewheel
inline ObjectGuid ObjectGuid_CreateItem(uint32 dbId)
{
    return ObjectGuid::Create<HighGuid::Item>(dbId);
}

// ========== Player::StoreNewItemInBestSlots 兼容 ==========
// AC: StoreNewItemInBestSlots(item, count) - 2 params
// TC: StoreNewItemInBestSlots(item, count, ...) - more params
// 在Player.h中已添加2参数重载

// ========== Item::CreateItem 兼容 ==========
// AC: Item::CreateItem(entry, count, player)
// TC: Item::CreateItem(entry, count, player->GetMap(), ...)
// By leewheel 2026-07-09: TC的ItemContext使用NONE（全大写），且CreateItem第4参数是Player*而非Map*
inline Item* Item_CreateItem(uint32 entry, uint32 count, Player* player)
{
    return Item::CreateItem(entry, count, ItemContext::NONE, player);
}
// End By leewheel

// ========== Player::FindEquipSlot 兼容 ==========
// AC: FindEquipSlot(ItemTemplate*, uint32, bool)
// TC: FindEquipSlot(Item const*, uint32, bool, bool)
// 需要在源文件中修改为传递Item*或适配

// ========== EmblemInfo 兼容 ==========
// AC: EmblemInfo(style, color, borderStyle, borderColor, background) - 5 params
// TC: EmblemInfo() default constructor, use SetStyle/SetColor etc.
// By leewheel 2026-07-09: TC的EmblemInfo没有SetStyle等方法，使用5参数构造函数
inline EmblemInfo EmblemInfo_Create(uint32 style, uint32 color, uint32 borderStyle, uint32 borderColor, uint32 background)
{
    return EmblemInfo(style, color, borderStyle, borderColor, background);
}
// End By leewheel

// ========== Transport::AddPassenger 兼容 ==========
// AC: transport->AddPassenger(unit, seat) - 2 params
// TC: transport->AddPassenger(unit) - 1 param
#define AddPassenger2(unit, seat) AddPassenger(unit)

// ========== MotionMaster::MovePoint 兼容 ==========
// AC: MovePoint(id, x, y, z, genPath, forceDestination, slopeAngle, offset)
// TC: MovePoint(id, x, y, z, genPath, forceDestination, slopeAngle, offset, VehicleExitSeat) 
// 参数数量可能有差异，需要在源文件中检查

// ========== Unit::IsClass 兼容 ==========
// By leewheel 2026-07-10: TC没有IsClass方法，使用getClass() == X替代
// 移除宏定义，在各调用处直接使用getClass()比较
// End By leewheel

// ========== Player::IsWithinRange 兼容 ==========
// AC: player->IsWithinRange(target, dist)
// TC: player->IsWithinDist(target, dist)
#define IsWithinRange(target, dist) IsWithinDist(target, dist)

//By leewheel 2026-07-08 移除有缺陷的HasPlayerFlag宏
// 该宏在obj->HasPlayerFlag(flag)调用时会展开为obj->(IsPlayer() && ToPlayer()->HasPlayerFlag(flag))
// 导致语法错误(E0133: 应输入成员名)
// 所有调用方都使用Player*对象，直接调用Player::HasPlayerFlag方法即可
//End By leewheel

// ========== Creature::GetFollowAngle 兼容 ==========
// AC: creature->GetFollowAngle()
// TC: 使用PET_FOLLOW_DIST或默认角度
inline float Creature_GetFollowAngle(Unit* /*unit*/)
{
    return float(M_PI / 2);
}

// ========== Transport::IsStaticTransport 兼容 ==========
// AC: transport->IsStaticTransport()
// TC: 使用IsStaticTransport()方法或type检查
#define IsStaticTransport() (GetTypeId() == TYPEID_GAMEOBJECT && ToGameObject()->GetGoType() == GAMEOBJECT_TYPE_TRANSPORT)

// ========== Spell::GetUniqueTargetInfo 兼容 ==========
// AC: spell->GetUniqueTargetInfo()
// TC: 使用m_uniqueTargets（private），需要friend或在Spell.h中添加public方法

// ========== Spell::prepare 兼容 ==========
// By leewheel 2026-07-10: TC使用引用参数，AC使用指针参数
// 移除宏定义，在各调用处手动修改参数类型
// End By leewheel

// ========== SpellCastTargets::Write 兼容 ==========
// AC: targets.Write(WorldPacket&)
// TC: targets.Write(WorldPackets::Spells::SpellTargetData&)
// 需要在源文件中修改

// ========== Guild::GetMember/Member/_GetRankRights 兼容 ==========
// 需要在Guild.h中添加public包装方法

// ========== LootTemplate::Process 兼容 ==========
// AC: lootTemplate->Process(loot, store, groupId, reference)
// TC: lootTemplate->Process(loot, bool, uint16, uint8)
// 需要在源文件中修改

// ========== Database Query 兼容 ==========
// AC使用PQuery (format string + args)
// TC使用Query (PreparedStatement)
// 需要在源文件中修改

// ========== sLFGDungeonStore 兼容 ==========
// TC使用LFGMgr，不使用sLFGDungeonStore
// LFGDungeonEntry已在上方定义

// ========== CreatureTemplate::LootID 兼容 ==========
// TC: CreatureDifficulty::LootID
// AC: CreatureTemplate::LootID
// CreatureTemplate已有LootID成员（通过GetCreatureDifficulty访问）

// ========== Player::GetSpellCooldownDelay 兼容 ==========
// AC: player->GetSpellCooldownDelay(spellId) 返回uint32
// TC: player->GetSpellCooldown(spellId) 返回uint32
// 已通过宏映射: #define GetSpellCooldownDelay(spellId) GetSpellCooldown(spellId)

// ========== sCharacterCache 兼容 ==========
// TC已有sCharacterCache全局单例（通过CharacterCache.h包含）

// ========== QUEST_TYPE 兼容 ==========
// AC: QUEST_TYPE_ELITE = 1, QUEST_TYPE_DUNGEON = 81
// TC: QUEST_INFO_GROUP = 1, QUEST_INFO_DUNGEON = 81
// By leewheel 2026-07-10
#ifndef QUEST_TYPE_ELITE
#define QUEST_TYPE_ELITE QUEST_INFO_GROUP
#endif
#ifndef QUEST_TYPE_DUNGEON
#define QUEST_TYPE_DUNGEON QUEST_INFO_DUNGEON
#endif
//End By leewheel 2026-07-10

// ========== Player::GetSpellCooldownMap 兼容 ==========
// AC: player->GetSpellCooldownMap() 返回SpellCooldowns结构
// TC: 使用player->GetSpellCooldowns()或直接遍历
// By leewheel 2026-07-10
//End By leewheel 2026-07-10

// ========== MapManager::GetAreaId 兼容 ==========
// AC: sMapMgr->GetAreaId(PHASEMASK_NORMAL, mapId, x, y, z)
// TC: 使用map->GetAreaId(phaseShift, x, y, z)
// By leewheel 2026-07-10
inline uint32 MapMgr_GetAreaId(Map* map, Unit* unit, float x, float y, float z)
{
    return map->GetAreaId(unit->GetPhaseShift(), x, y, z);
}
//End By leewheel 2026-07-10

// ========== Map::GetWaterLevel 兼容 ==========
// AC: map->GetWaterLevel(x, y)
// TC: map->GetWaterLevel(phaseShift, x, y)
// By leewheel 2026-07-10
inline float Map_GetWaterLevel(Map* map, Unit* unit, float x, float y)
{
    return map->GetWaterLevel(unit->GetPhaseShift(), x, y);
}
//End By leewheel 2026-07-10

// ========== Player::GetLiquidData 兼容 ==========
// AC: player->GetLiquidData().Status
// TC: 使用Map::GetLiquidStatus获取ZLiquidStatus
// By leewheel 2026-07-10
inline ZLiquidStatus Player_GetLiquidStatus(Player* player, float x, float y, float z, LiquidData* data = nullptr)
{
    return player->GetMap()->GetLiquidStatus(player->GetPhaseShift(), x, y, z, map_liquidHeaderTypeFlags::AllLiquids, data);
}
//End By leewheel 2026-07-10

// ========== Map::CanReachPositionAndGetValidCoords 兼容 ==========
// AC: map->CanReachPositionAndGetValidCoords(unit, x, y, z)
//     语义(AC Map.cpp:3377): 1)碰撞校验(CheckCollisionAndGetValidCoords) 2)可走性校验(水/飞行/坡度)
// TC 343: 有CheckCollisionAndGetValidCoords(碰撞)但无AC的IsWalkableClimb(坡度)与HasEnoughWater判断
// By leewheel 2026-07-10
// By leewheel 2026-09-03 按AC原版语义完整移植：
//   1) 碰撞校验走TC原生CheckCollisionAndGetValidCoords(failOnCollision=true)
//   2) 水体可走性：Creature按CanEnterWater判断；玩家/飞行单位放行(与AC source->ToPlayer()/CanFly()逻辑一致)
//   3) 坡度校验：AC的IsWalkableClimb基于PathGenerator::getSlopeAngleAbs坡度角与碰撞高度计算可攀爬高度，
//      TC无此API，按AC Geometry.h/PathGenerator.cpp原式移植内联实现(移植来源: AC PathGenerator::IsWalkableClimb)
//   修复C4100警告：map参数在完整语义下被实际使用
inline bool PathGenerator_IsWalkableClimb(float x, float y, float z, float destX, float destY, float destZ, float sourceHeight)
{
    // AC Geometry.h::getSlopeAngle / getSlopeAngleAbs
    float const floorDist = std::sqrt(std::pow(destY - y, 2.0f) + std::pow(destX - x, 2.0f));
    float const slopeAngle = std::atan(std::abs(destZ - z) / std::abs(floorDist));
    float const slopeAngleDegree = (slopeAngle * 180.0f / float(M_PI));
    // AC PathGenerator::GetRequiredHeightToClimb
    float const climbableHeight = sourceHeight - (sourceHeight * (slopeAngleDegree / 100));
    float const diffHeight = std::abs(destZ - z);
    return diffHeight <= climbableHeight;
}

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//TC-Cata没有Map::CheckCollisionAndGetValidCoords，按WotLK算法用getObjectHitPos+GetHeight实现
//算法来源: TC-WotLK Map::CheckCollisionAndGetValidCoords(碰撞射线+地面高度钳制)
inline bool Map_CheckCollisionAndGetValidCoords(Map* map, WorldObject const* source,
    float startX, float startY, float startZ, float& destX, float& destY, float& destZ, bool failOnCollision = true)
{
    // 验证坐标有效性
    if (!Trinity::IsValidMapCoord(destX, destY, destZ) || !Trinity::IsValidMapCoord(startX, startY, startZ))
        return false;

    PhaseShift const& phaseShift = source->GetPhaseShift();

    // 检查动态物件碰撞（游戏物件等），命中时把终点钳制到接触点
    float hitX = destX, hitY = destY, hitZ = destZ;
    bool collided = map->getObjectHitPos(phaseShift,
        startX, startY, startZ + source->GetCollisionHeight() * 0.5f,
        destX, destY, destZ + source->GetCollisionHeight() * 0.5f,
        hitX, hitY, hitZ, -1.0f);

    if (collided)
    {
        destX = hitX;
        destY = hitY;
        destZ = hitZ - source->GetCollisionHeight() * 0.5f;
    }

    // 获取地面高度并调整Z坐标
    float groundZ = map->GetHeight(phaseShift, destX, destY, destZ + 2.0f, true, 50.0f);
    if (groundZ <= INVALID_HEIGHT)
        return false;

    // 如果地面高度与目标高度差距过大，使用地面高度
    if (std::abs(destZ - groundZ) > 5.0f)
        destZ = groundZ;

    destZ = std::max(destZ, groundZ);

    return !failOnCollision || !collided;
}
//End By leewheel

inline bool Map_CanReachPositionAndGetValidCoords(Map* map, Unit* unit, float& x, float& y, float& z)
{
    if (!map || !unit)
        return false;

    //By leewheel 2026-09-06: 移植到TrinityCore-Cata
    //TC-Cata没有Map::CheckCollisionAndGetValidCoords，用getObjectHitPos+GetHeight按原算法实现
    float const startX = x, startY = y, startZ = z;
    if (!Map_CheckCollisionAndGetValidCoords(map, unit, startX, startY, startZ, x, y, z, true))
        return false;
    //End By leewheel

    // AC: walkable checks（Map.cpp:3392-3403）
    //By leewheel 2026-09-06: TC-Cata的IsInWater只有5参数(无collisionHeight)
    bool const isWaterNext = map->IsInWater(unit->GetPhaseShift(), x, y, z, nullptr);
    Creature const* creature = unit->ToCreature();
    //By leewheel 2026-09-06: TC-Cata无Creature::CanWalk()(Cata移除InhabitType，生物默认可地面行走)，移除该项
    bool const cannotEnterWater = isWaterNext && (creature && !creature->CanEnterWater());
    bool const cannotWalkOrFly = !isWaterNext && !unit->ToPlayer() && !unit->CanFly();
    //End By leewheel
    if (cannotEnterWater || cannotWalkOrFly)
        return false;

    // AC: failOnSlopes=true时校验非可走坡度
    if (!PathGenerator_IsWalkableClimb(startX, startY, startZ, x, y, z, unit->GetCollisionHeight()))
        return false;

    return true;
}

//By leewheel 2026-09-04: 兼容封装两点版——碰撞射线从(startX,startY,startZ)到(x,y,z)整段校验,
//对应 AC Map::CanReachPositionAndGetValidCoords(bot, botX, botY, botZ, candX, candY, candZ, true, failOnSlopes)
//的原生语义; 原4参数版把 x,y,z 同时当起点和终点, 只适用于"自身附近落点"场景(TravelMgr/NewRpg 用法)
inline bool Map_CanReachPositionAndGetValidCoordsTwoPoint(
    Map* map, Unit* unit, float startX, float startY, float startZ, float& x, float& y, float& z)
{
    if (!map || !unit)
        return false;

    // TC: 整段碰撞校验(起点=机器人坐标), 目标坐标被clamp到接触点时通过引用回传
    //By leewheel 2026-09-06: 移植到TrinityCore-Cata，改用Compat实现的碰撞校验
    if (!Map_CheckCollisionAndGetValidCoords(map, unit, startX, startY, startZ, x, y, z, true))
        return false;
    //End By leewheel

    // AC: walkable checks（Map.cpp:3392-3403）
    //By leewheel 2026-09-06: TC-Cata的IsInWater只有5参数；无CanWalk()(生物默认可地面行走)
    bool const isWaterNext = map->IsInWater(unit->GetPhaseShift(), x, y, z, nullptr);
    Creature const* creature = unit->ToCreature();
    bool const cannotEnterWater = isWaterNext && (creature && !creature->CanEnterWater());
    bool const cannotWalkOrFly = !isWaterNext && !unit->ToPlayer() && !unit->CanFly();
    //End By leewheel
    if (cannotEnterWater || cannotWalkOrFly)
        return false;

    // AC: failOnSlopes=true时校验非可走坡度(AC getSlopeAngle 本就接收起点/终点6坐标)
    if (!PathGenerator_IsWalkableClimb(startX, startY, startZ, x, y, z, unit->GetCollisionHeight()))
        return false;

    return true;
}
//End By leewheel 2026-07-10

// ========== ObjectAccessor::GetSpawnedGameObjectByDBGUID / GetSpawnedCreatureByDBGUID 兼容 ==========
// AC: ObjectAccessor::GetSpawnedGameObjectByDBGUID(mapId, spawnId)
// TC: 使用map->GetGameObjectBySpawnId / map->GetCreatureBySpawnId
// By leewheel 2026-07-10
inline GameObject* ObjectAccessor_GetSpawnedGameObjectByDBGUID(Map* map, uint32 spawnId)
{
    return map->GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(map->GetId(), 0, spawnId));
}
inline Creature* ObjectAccessor_GetSpawnedCreatureByDBGUID(Map* map, uint32 spawnId)
{
    return map->GetCreature(ObjectGuid::Create<HighGuid::Creature>(map->GetId(), 0, spawnId));
}
//End By leewheel 2026-07-10

// ========== MotionMaster::top 兼容 ==========
// AC: motionMaster->top()
// TC: 不存在，使用GetCurrentMovementGenerator()
// By leewheel 2026-07-10
//End By leewheel 2026-07-10

// ========== MotionMaster::MovePointBackwards 兼容 ==========
// AC: motionMaster->MovePointBackwards(id, x, y, z, ...)
// TC: 已在MotionMaster.h中添加MOD_PLAYERBOTS兼容
// By leewheel 2026-07-10
//End By leewheel 2026-07-10

// ========== WorldObject::GetNearPoint 兼容 ==========
// AC: obj->GetNearPoint(obj, x, y, z, dist, angle, targetX, targetY, targetZ)
// TC: obj->GetNearPoint(x, y, z, dist, angle) - 参数更少
// By leewheel 2026-07-10
//End By leewheel 2026-07-10

// ========== Player::StoreNewItemInBestSlots 兼容 ==========
// AC: player->StoreNewItemInBestSlots(itemId, count)
// TC: player->StoreNewItemInBestSlots(itemId, count, count)
// By leewheel 2026-07-10
//End By leewheel 2026-07-10

// ========== CMSG 常量兼容 ==========
// TC不再使用CMSG_XXX常量，改用WorldPackets系统
// By leewheel 2026-07-10
// CMSG_PETITION_SIGN -> 不需要，通过WorldSession方法调用
// CMSG_LFG_JOIN -> 不需要，通过WorldSession方法调用
// CMSG_LFG_PROPOSAL_RESULT -> 不需要，通过WorldSession方法调用
// CMSG_QUESTGIVER_REQUEST_REWARD -> 不需要，通过WorldSession方法调用
//End By leewheel 2026-07-10

// ========== 缺失的CMSG常量 ==========
#ifndef CMSG_QUESTGIVER_REQUEST_REWARD
#define CMSG_QUESTGIVER_REQUEST_REWARD 0
#endif

// ========== Player::GetSpellCooldown 兼容 ==========
// AC: player->GetSpellCooldown(spellId) 返回uint32(剩余冷却时间)
// TC: 使用GetSpellHistory()->GetRemainingCooldown(spellInfo) 返回Duration
// By leewheel 2026-07-10
inline uint32 Player_GetSpellCooldown(Player* player, uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;
    auto cooldown = player->GetSpellHistory()->GetRemainingCooldown(spellInfo);
    return uint32(std::chrono::duration_cast<std::chrono::milliseconds>(cooldown).count());
}
#define GetSpellCooldown(spellId) Player_GetSpellCooldown(this, spellId)
//End By leewheel 2026-07-10

// ========== Player::GetSpellCooldownMap 兼容 ==========
// AC: player->GetSpellCooldownMap() 返回SpellCooldowns map
// TC: 不存在，返回空map
// By leewheel 2026-07-10
inline std::unordered_map<uint32, uint32> Player_GetSpellCooldownMap(Player* /*player*/)
{
    return {};
}
#define GetSpellCooldownMap() Player_GetSpellCooldownMap(this)
//End By leewheel 2026-07-10

// ========== Player::StoreNewItemInBestSlots 2参数兼容 ==========
// AC: StoreNewItemInBestSlots(itemId, count)
// TC: StoreNewItemInBestSlots(itemId, count, ItemContext)
// By leewheel 2026-07-10
inline bool Player_StoreNewItemInBestSlots(Player* player, uint32 itemId, uint32 count)
{
    return player->StoreNewItemInBestSlots(itemId, count, ItemContext::NONE);
}
#define StoreNewItemInBestSlots2(itemId, count) Player_StoreNewItemInBestSlots(this, itemId, count)
//End By leewheel 2026-07-10

// ========== Player::LearnPetTalent 2参数兼容 ==========
// AC: LearnPetTalent(talentId, talentRank)
// TC-WotLK: LearnPetTalent(petGuid, talentId, talentRank)
//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata移除了WotLK的宠物天赋树系统(改为野兽控制/狡诈/坚韧三系专精自动学习天赋)，
//Player/Pet类均无LearnPetTalent接口，故此处按Cata语义实现为无操作
inline void Player_LearnPetTalent(Player* /*player*/, uint32 /*talentId*/, uint32 /*talentRank*/)
{
    //Cata宠物天赋由专精自动分配，无需手动学习
}
#define LearnPetTalent2(talentId, talentRank) Player_LearnPetTalent(this, talentId, talentRank)
//End By leewheel 2026-07-10

// ========== Player::CanRewardQuest 3参数兼容 ==========
// AC: CanRewardQuest(quest, slot) 
// TC: CanRewardQuest(quest, msg) - 2 params
// By leewheel 2026-07-10
inline bool Player_CanRewardQuest(Player* player, Quest const* quest, uint32 /*slot*/)
{
    return player->CanRewardQuest(quest, false);
}
//End By leewheel 2026-07-10

// ========== WorldObject::GetNearPoint 7参数兼容 ==========
// AC: GetNearPoint(obj, x, y, z, dist, angle, targetX, targetY, targetZ)
// TC: GetNearPoint(searcher, x, y, z, distance2d, absAngle) - 6 params
// By leewheel 2026-07-10
inline void WorldObject_GetNearPoint7(WorldObject* obj, WorldObject const* searcher, float& x, float& y, float& z, float dist, float absAngle, float /*targetX*/, float /*targetY*/, float /*targetZ*/)
{
    obj->GetNearPoint(searcher, x, y, z, dist, absAngle);
}
//End By leewheel 2026-07-10

// ========== PLAYER_SKILL_INDEX 兼容 ==========
// AC使用PLAYER_SKILL_INDEX常量，TC不使用
#ifndef PLAYER_SKILL_INDEX
#define PLAYER_SKILL_INDEX(skill) (skill * 16)
#endif

// ========== CharStartOutfitEntry 兼容 ==========
//By leewheel 2026-08-15: 修复——原GetCharStartOutfitEntry返回static dummy(ItemId全0)，低等级
//(level<5)bot初始装备全部落空。TC 3.4.3核心未注册CharStartOutfit.db2 store，但客户端DB2文件
//在(ClientData/dbc/*/CharStartOutfit.db2)，老大已导入classic_db2.charstartoutfit表。
//实现：LoadCharStartOutfitCache()启动时经HotfixDatabase加载全部行到inline静态缓存
//(键=race|class<<8|gender<<16)，GetCharStartOutfitEntry查询直接返回。
//ItemID列是逗号分隔的24个int(0=空槽)，与AC的CharStartOutfitEntry.ItemId[24]一一对应
#define MAX_OUTFIT_ITEMS 24
struct CharStartOutfitEntry
{
uint32 ID;
uint8 Race;
uint8 Class;
uint8 Gender;
int32 ItemId[MAX_OUTFIT_ITEMS];
int32 GetOutfitItemId(int32 index) const { return ItemId[index]; }
};
inline std::unordered_map<uint32, CharStartOutfitEntry>& GetCharStartOutfitCache()
{
    static std::unordered_map<uint32, CharStartOutfitEntry> cache;
    return cache;
}
// 启动时从classic_db2.charstartoutfit加载缓存(由Playerbots.cpp OnConfigLoad调用)
inline void LoadCharStartOutfitCache()
{
    uint32 oldMSTime = getMSTime();
    std::unordered_map<uint32, CharStartOutfitEntry>& cache = GetCharStartOutfitCache();
    cache.clear();

    if (QueryResult result = HotfixDatabase.Query("SELECT ID, ClassID, SexID, RaceID, ItemID FROM classic_db2.charstartoutfit"))
    {
        do
        {
            Field* fields = result->Fetch();
            CharStartOutfitEntry entry;
            entry.ID = fields[0].Get<uint32>();
            entry.Class = static_cast<uint8>(fields[1].Get<uint8>());
            entry.Gender = static_cast<uint8>(fields[2].Get<uint8>());
            entry.Race = static_cast<uint8>(fields[3].Get<uint8>());
            std::memset(entry.ItemId, 0, sizeof(entry.ItemId));

            // ItemID列是逗号分隔的24个int(0=空槽)
            std::string const itemStr = fields[4].Get<std::string>();
            size_t start = 0;
            uint32 slot = 0;
            while (slot < MAX_OUTFIT_ITEMS)
            {
                size_t comma = itemStr.find(',', start);
                std::string token = (comma == std::string::npos)
                    ? itemStr.substr(start) : itemStr.substr(start, comma - start);
                // 去空格
                token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) { return std::isspace(c); }),
                           token.end());
                if (!token.empty())
                    entry.ItemId[slot] = std::atoi(token.c_str());

                ++slot;
                if (comma == std::string::npos)
                    break;
                start = comma + 1;
            }

            cache[entry.Race | (entry.Class << 8) | (entry.Gender << 16)] = entry;
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", "CharStartOutfit: 从classic_db2.charstartoutfit加载了 {} 条初始装备，耗时 {} 毫秒",
        cache.size(), GetMSTimeDiffToNow(oldMSTime));
}
// 按 race/class/gender 查询初始装备，未命中返回nullptr
inline CharStartOutfitEntry const* GetCharStartOutfitEntry(uint8 race, uint8 class_, uint8 gender)
{
    std::unordered_map<uint32, CharStartOutfitEntry> const& cache = GetCharStartOutfitCache();
    auto itr = cache.find(race | (class_ << 8) | (gender << 16));
    if (itr == cache.end())
        return nullptr;

    return &itr->second;
}
//End By leewheel

// ========== AreaTriggerTeleport 兼容 ==========
//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//原WotLK兼容宏(#define AreaTriggerTeleport AreaTriggerStruct)已删除：
//TC-Cata的传送区域触发器类型就是AreaTriggerTeleport(=WorldSafeLocsEntry别名)，
//模块代码直接使用AreaTriggerTeleport即可
//End By leewheel 2026-07-10

// ========== PlayerbotAIConfig 缺失成员兼容 ==========
// AC的PlayerbotAIConfig有botTextBased和randomBotAutoJoinArenaBracket成员
// TC没有，定义默认值
// By leewheel 2026-07-10
#ifndef BOT_TEXT_BASED_DEFAULT
#define BOT_TEXT_BASED_DEFAULT false
#endif
#ifndef RANDOM_BOT_AUTO_JOIN_ARENA_BRACKET_DEFAULT
#define RANDOM_BOT_AUTO_JOIN_ARENA_BRACKET_DEFAULT 0
#endif
//End By leewheel 2026-07-10

// ========== ItemTemplate::Socket 兼容 ==========
// AC的ItemTemplate有Socket[3]数组，TC使用GetSocketColor(slot)
// By leewheel 2026-07-10
inline uint32 ItemTemplate_GetSocket(const ItemTemplate* proto, uint8 slot)
{
    if (slot >= 3)
        return 0;
    return proto->GetSocketColor(slot);
}
//End By leewheel 2026-07-10

// ========== ItemTemplate::BuyCount 兼容 ==========
// AC的ItemTemplate有BuyCount成员，TC使用GetBuyCount()
// By leewheel 2026-07-10
inline uint32 ItemTemplate_GetBuyCount(const ItemTemplate* proto)
{
    return proto->GetBuyCount();
}
//End By leewheel 2026-07-10

// ========== ItemTemplate::GetArmor 兼容 ==========
// AC的ItemTemplate有GetArmor()方法，TC的armor在BasicData->Resistances[SPELL_SCHOOL_NORMAL]
// By leewheel 2026-09-09
inline int32 ItemTemplate_GetArmor(const ItemTemplate* proto)
{
    return proto->BasicData->Resistances[SPELL_SCHOOL_NORMAL];
}
//End By leewheel

// ========== ItemTemplate::GetShieldBlockValue 兼容 ==========
// AC有此方法，TC Cata中移除了盾牌格挡值属性
// By leewheel 2026-09-09
inline int32 ItemTemplate_GetShieldBlockValue(const ItemTemplate* /*proto*/, uint32 /*itemLevel*/)
{
    return 0;
}
//End By leewheel

// ========== MAX_ITEM_PROTO_DAMAGES 兼容 ==========
// AC有此常量，TC Cata中伤害数组大小为5
// By leewheel 2026-09-09
#ifndef MAX_ITEM_PROTO_DAMAGES
#define MAX_ITEM_PROTO_DAMAGES 5
#endif
//End By leewheel

// ========== SKILL_THROWN 兼容 ==========
// AC有投掷武器技能，TC Cata中移除了投掷武器
// By leewheel 2026-09-09
#ifndef SKILL_THROWN
#define SKILL_THROWN SKILL_NONE
#endif
//End By leewheel

// ========== SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT 兼容 ==========
// AC有此光环类型，TC Cata中已重命名为SPELL_AURA_199且未使用
// By leewheel 2026-09-09
#ifndef SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT
#define SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT SPELL_AURA_199
#endif
//End By leewheel

// ========== Player::GetArenaPoints 兼容 ==========
// AC有竞技场点数，TC Cata中移除了竞技场点数
// By leewheel 2026-09-09
inline uint32 Player_GetArenaPoints(const Player* /*player*/)
{
    return 0;
}
//End By leewheel

// ========== ItemTemplate::DisenchantID 兼容 ==========
// AC有DisenchantID成员，TC Cata中通过ItemDisenchantLoot DB2表查询
// By leewheel 2026-09-09
inline uint32 ItemTemplate_GetDisenchantID(const ItemTemplate* proto)
{
    ItemDisenchantLootEntry const* disenchant = Item::GetDisenchantLoot(proto, proto->GetQuality(), proto->GetItemLevel());
    return disenchant ? disenchant->ID : 0;
}
//End By leewheel

// ========== ItemSetEffect 结构定义兼容 ==========
// TC的ItemSetEffect定义在Item.cpp中，Player.h只有前向声明
// StatsWeightCalculator.cpp需要完整定义才能访问成员
// By leewheel 2026-09-09
struct ItemSetEffect
{
    uint32 ItemSetID;
    std::unordered_set<Item const*> EquippedItems;
    std::unordered_set<ItemSetSpellEntry const*> SetBonuses;
};
//End By leewheel

// ========== CreatureTemplate 拾取字段兼容 (绕过GetDifficulty宏) ==========
// AC使用creatureTemplate->pickpocketLootId/SkinLootId
// TC的字段在CreatureDifficulty中，但GetDifficulty()被宏拦截
// 使用difficultyStore直接访问以避免宏冲突
// By leewheel 2026-09-09
inline CreatureDifficulty const* CreatureTemplate_GetDifficultyEntry(const CreatureTemplate* entry, Difficulty diff)
{
    auto it = entry->difficultyStore.find(diff);
    if (it != entry->difficultyStore.end())
        return &it->second;
    return nullptr;
}

inline uint32 CreatureTemplate_GetPickPocketLootID(const CreatureTemplate* entry)
{
    auto diff = CreatureTemplate_GetDifficultyEntry(entry, DIFFICULTY_NORMAL);
    return diff ? diff->PickPocketLootID : 0;
}

inline uint32 CreatureTemplate_GetSkinLootID(const CreatureTemplate* entry)
{
    auto diff = CreatureTemplate_GetDifficultyEntry(entry, DIFFICULTY_NORMAL);
    return diff ? diff->SkinLootID : 0;
}

// CreatureTemplate兼容: AC mingold/maxgold → TC CreatureDifficulty::GoldMin/GoldMax
//By leewheel 2026-09-09: TC-Cata的金币字段在CreatureDifficulty中
inline uint32 CreatureTemplate_GetGoldMin(const CreatureTemplate* entry)
{
    auto diff = CreatureTemplate_GetDifficultyEntry(entry, DIFFICULTY_NORMAL);
    return diff ? diff->GoldMin : 0;
}

inline uint32 CreatureTemplate_GetGoldMax(const CreatureTemplate* entry)
{
    auto diff = CreatureTemplate_GetDifficultyEntry(entry, DIFFICULTY_NORMAL);
    return diff ? diff->GoldMax : 0;
}
//End By leewheel

// ========== SpellItemEnchantmentEntry::slot 兼容 ==========
// AC的SpellItemEnchantmentEntry有slot成员，TC没有
// By leewheel 2026-07-10
inline uint32 SpellItemEnchantmentEntry_GetSlot(const SpellItemEnchantmentEntry* /*entry*/)
{
    return 0;
}
//End By leewheel 2026-07-10

// ========== Guild Rank 兼容 ==========
// AC有GR_OFFICER和GR_INITIATE等公会等级常量，TC只有GuildRankId::GuildMaster
// By leewheel 2026-07-11
#ifndef GR_GUILD_MASTER
#define GR_GUILD_MASTER 0
#endif
#ifndef GR_OFFICER
#define GR_OFFICER 1
#endif
#ifndef GR_VETERAN
#define GR_VETERAN 2
#endif
#ifndef GR_MEMBER
#define GR_MEMBER 3
#endif
#ifndef GR_INITIATE
#define GR_INITIATE 4
#endif
//End By leewheel

// ========== ReputationMgr::ReputationRankToStanding 兼容 ==========
// AC有ReputationMgr::ReputationRankToStanding静态方法，TC没有
// By leewheel 2026-07-11
inline int32 ReputationMgr_ReputationRankToStanding(ReputationRank rank)
{
    static constexpr int32 PointsInRank[] = {36000, 3000, 3000, 3000, 3000, 3000, 3000, 3000};
    int32 standing = -42000;
    for (uint8 i = 0; i <= static_cast<uint8>(rank) && i < 8; ++i)
        standing += PointsInRank[i];
    return std::max(standing - 1, -42000);
}
//End By leewheel

// ========== Player::removeSpell 兼容 ==========
// AC使用removeSpell(小写)，TC使用RemoveSpell
#define removeSpell RemoveSpell

// ========== CreatureTemplate::type_flags 兼容 ==========
// AC使用type_flags直接成员，TC使用CreatureDifficulty
#define type_flags TypeFlags

// ========== GameObjectTemplate::moTransport::taxiPathId 兼容 ==========
// AC使用taxiPathId，TC使用不同字段名
// 需要在源文件中检查具体字段名

// ========== Position::m_orientation 兼容 ==========
// AC使用m_orientation(protected)，TC使用GetOrientation()
// 需要在源文件中使用GetOrientation()替代

// ========== Trinity::Containers 兼容 ==========
// AC使用Trinity::Containers::SelectRandomContainerElement
// TC也使用Trinity命名空间但可能需要不同包含
// 已通过宏定义SelectRandomContainerElement解决

// ========== ChannelMgr::forTeam / GetChannel 兼容 ==========
// AC: ChannelMgr::forTeam(team) 返回ChannelMgr*
// TC-WotLK: ChannelMgr::ForTeam(uint32 team) 返回ChannelMgr*
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，ForTeam参数类型改为Team枚举，显式转换
inline ChannelMgr* ChannelMgr_forTeam(uint32 team)
{
    return ChannelMgr::ForTeam(static_cast<Team>(team));
}
// AC代码中使用 ChannelMgr::forTeam(team)，改为 TC的 ChannelMgr::ForTeam(team)
#define forTeam(team) ForTeam(static_cast<Team>(team))
//End By leewheel 2026-07-11

// ========== BattlegroundTemplate::GetMapId 兼容 ==========
// AC使用BattlegroundTemplate::BattlemapEntry->MapID
// TC-WotLK使用BattlegroundTemplate::BattlemasterEntry->MapID[0]
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata的BattlemasterListEntry没有MapID字段，
//战场地图列表在BattlegroundTemplate::MapIDs(vector<int32>)中
inline uint32 BattlegroundTemplate_GetMapId(BattlegroundTemplate const* bgTemplate)
{
    return (bgTemplate && !bgTemplate->MapIDs.empty()) ? uint32(bgTemplate->MapIDs[0]) : 0;
}
//End By leewheel 2026-07-11

// ========== GetBattlegroundBracketByLevel 兼容 ==========
// AC使用全局函数GetBattlegroundBracketByLevel(mapId, level)返回PvPDifficultyEntry const*
// TC使用DB2Manager::GetBattlegroundBracketByLevel(mapId, level)返回PVPDifficultyEntry const*
// By leewheel 2026-07-11: 修复TC API调用
inline PVPDifficultyEntry const* GetBattlegroundBracketByLevel(uint32 mapId, uint32 level)
{
    return DB2Manager::GetBattlegroundBracketByLevel(mapId, level);
}
//End By leewheel 2026-07-11

// ========== ObjectMgr::CheckPlayerName 兼容 ==========
// AC: ObjectMgr::CheckPlayerName(name) - 1 param, returns ResponseCode
// TC: ObjectMgr::CheckPlayerName(name, LocaleConstant, bool create) - 3 params
// By leewheel 2026-07-11: 删除宏定义, TC已有正确的3参数版本, 宏中的false会破坏LocaleConstant参数
//End By leewheel

// ========== CharacterCache::AddCharacterCacheEntry 兼容 ==========
// AC: AddCharacterCacheEntry(guid, name, ...) - 7 params
// TC: AddCharacterCacheEntry(guid, ...) - different params
// 需要在源文件中适配

// ========== Database Query 兼容宏 ==========
// AC使用PQuery, TC使用Query
// 已在源文件中逐个修改

// ========== Item::GenerateItemRandomPropertyId 兼容 ==========
// AC: Item::GenerateItemRandomPropertyId(itemEntry) 返回int32
// TC-WotLK: ItemEnchantmentMgr::instance()->GenerateRandomProperties(itemId) 返回ItemRandomProperties
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，改为自由函数GenerateItemRandomPropertiesId(itemId)直接返回int32
inline int32 Item_GenerateItemRandomPropertyId(uint32 itemEntry)
{
    return GenerateItemRandomPropertiesId(itemEntry);
}
//End By leewheel 2026-07-11

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，WotLK→Cata剩余API差异批量映射
// ========== GetAngle → GetAbsoluteAngle ==========
// WotLK的Position::GetAngle(x,y)/(Position*)在Cata更名为GetAbsoluteAngle，
// 模块全部GetAngle调用(对象指针/坐标对)与GetAbsoluteAngle重载完全对应
#define GetAngle GetAbsoluteAngle

// ========== WotLK的RAID难度枚举 → Cata难度枚举 ==========
// Cata统一了10/25人普通/英雄团队难度：普通=NORMAL_RAID(14)，英雄=HEROIC_RAID(15)
#ifndef DIFFICULTY_RAID_25_N
#define DIFFICULTY_RAID_25_N DIFFICULTY_NORMAL_RAID
#endif
#ifndef DIFFICULTY_RAID_10_N
#define DIFFICULTY_RAID_10_N DIFFICULTY_NORMAL_RAID
#endif
#ifndef DIFFICULTY_RAID_25MAN_NORMAL
#define DIFFICULTY_RAID_25MAN_NORMAL DIFFICULTY_NORMAL_RAID
#endif
#ifndef DIFFICULTY_RAID_10MAN_NORMAL
#define DIFFICULTY_RAID_10MAN_NORMAL DIFFICULTY_NORMAL_RAID
#endif
#ifndef DIFFICULTY_RAID_25_HC
#define DIFFICULTY_RAID_25_HC DIFFICULTY_HEROIC_RAID
#endif
#ifndef DIFFICULTY_RAID_10_HC
#define DIFFICULTY_RAID_10_HC DIFFICULTY_HEROIC_RAID
#endif

// ========== WotLK常量补齐 ==========
// 雕文槽位数(Cata的ActivePlayerData::GlyphSlots仍是9槽布局)
#ifndef MAX_GLYPH_SLOT_INDEX
#define MAX_GLYPH_SLOT_INDEX 9
#endif
// 天赋最大等级(宠物天赋在Cata已自动化，此常量仅供编译，配合Pet::GetFreeTalentPoints()=0为无操作)
#ifndef MAX_TALENT_RANK
#define MAX_TALENT_RANK 5
#endif
// 施法触发标志：Cata拆分为两个独立标志
#ifndef TRIGGERED_IGNORE_POWER_AND_REAGENT_COST
#define TRIGGERED_IGNORE_POWER_AND_REAGENT_COST (TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_REAGENT_COST)
#endif
// 生物精英等级：AC的CREATURE_ELITE_*枚举 → Cata的CreatureClassifications作用域枚举
// 注意：WotLK的WORLDBOSS(3)在Cata分类中为Obsolete(生物自身由标志位标记世界首领)
#ifndef CREATURE_ELITE_NORMAL
#define CREATURE_ELITE_NORMAL CreatureClassifications::Normal
#endif
#ifndef CREATURE_ELITE_ELITE
#define CREATURE_ELITE_ELITE CreatureClassifications::Elite
#endif
#ifndef CREATURE_ELITE_RAREELITE
#define CREATURE_ELITE_RAREELITE CreatureClassifications::RareElite
#endif
#ifndef CREATURE_ELITE_WORLDBOSS
#define CREATURE_ELITE_WORLDBOSS CreatureClassifications::Obsolete
#endif
#ifndef CREATURE_ELITE_RARE
#define CREATURE_ELITE_RARE CreatureClassifications::Rare
#endif
// 区域标志：Cata的AreaTableEntry.Flags[0]位定义与WotLK不同，按语义映射
// ALLOW_DUELS(0x40)两代一致；CAPITAL在Cata无此位，改用LinkedChat(0x100,"Set in cities")判断城市；
// NO_FLY_ZONE在Cata已移除(飞行由地图规则决定)，映射为0使禁飞检查恒不触发
#ifndef AREA_FLAG_ALLOW_DUELS
#define AREA_FLAG_ALLOW_DUELS 0x00000040
#endif
#ifndef AREA_FLAG_CAPITAL
#define AREA_FLAG_CAPITAL 0x00000100
#endif
#ifndef AREA_FLAG_NO_FLY_ZONE
#define AREA_FLAG_NO_FLY_ZONE 0
#endif
//End By leewheel

// ========== 第二批API兼容宏（2026-09-08 编译错误修复） ==========
//By leewheel 2026-09-08: 移植mod-playerbots到TC-Cata，补充缺失的API兼容宏和辅助函数

// AC使用小写方法名，TC使用大写开头
#ifndef isFrozen
#define isFrozen() IsFrozen()
#endif
#ifndef IsLevitating
#define IsLevitating() HasFeatherFallAura()
#endif
#ifndef getStandState
#define getStandState() GetStandState()
#endif

// Map方法兼容: AC GetDifficulty() → TC GetDifficultyID()
#ifndef GetDifficulty
#define GetDifficulty() GetDifficultyID()
#endif

// Quest方法兼容: AC GetTitle() → TC GetLogTitle()
#ifndef GetTitle
#define GetTitle() GetLogTitle()
#endif

// TalentEntry方法兼容: AC GetTalentID() → TC直接访问ID成员
#ifndef GetTalentID
#define GetTalentID() ID
#endif

// AreaTableEntry成员兼容: AC area_name → TC AreaName (LocalizedString)
// area_name是AC专用字段名，不会与C++通用标识符冲突
#define area_name AreaName

// TalentTabEntry成员兼容: AC PetTalentMask → TC没有宠物天赋树
#ifndef PetTalentMask
#define PetTalentMask 0
#endif

// CreatureTemplate辅助函数: maxlevel → CreatureDifficulty::MaxLevel
//By leewheel 2026-09-08: TC-Cata的CreatureTemplate没有MinLevel字段,GetDifficulty被宏拦截,直接用difficultyStore
inline uint8 CreatureTemplate_GetMaxLevel(const CreatureTemplate* entry)
{
    auto it = entry->difficultyStore.find(DIFFICULTY_NORMAL);
    if (it != entry->difficultyStore.end())
        return it->second.MaxLevel;
    if (!entry->difficultyStore.empty())
        return entry->difficultyStore.begin()->second.MaxLevel;
    return 0;
}
// MinLevel版本
inline uint8 CreatureTemplate_GetMinLevel(const CreatureTemplate* entry)
{
    auto it = entry->difficultyStore.find(DIFFICULTY_NORMAL);
    if (it != entry->difficultyStore.end())
        return it->second.MinLevel;
    if (!entry->difficultyStore.empty())
        return entry->difficultyStore.begin()->second.MinLevel;
    return 0;
}
//End By leewheel

// CreatureTemplate辅助函数: rank → CreatureDifficulty中的分类
//By leewheel 2026-09-08: TC-Cata的Classification是enum class CreatureClassifications,需static_cast
inline uint32 CreatureTemplate_GetRank(const CreatureTemplate* entry)
{
    return static_cast<uint32>(entry->Classification);
}
//End By leewheel

// CreatureTemplate辅助函数: IsExotic/IsTameable → 需要CreatureDifficulty参数
//By leewheel 2026-09-09: TC-Cata的IsExotic/IsTameable需要CreatureDifficulty指针参数
inline bool CreatureTemplate_IsExotic(CreatureTemplate const* creature)
{
    auto it = creature->difficultyStore.find(DIFFICULTY_NORMAL);
    if (it != creature->difficultyStore.end())
        return (it->second.TypeFlags & CREATURE_TYPE_FLAG_TAMEABLE_EXOTIC) != 0;
    return false;
}
inline bool CreatureTemplate_IsExotic(CreatureTemplate const& creature)
{
    return CreatureTemplate_IsExotic(&creature);
}
inline bool CreatureTemplate_IsTameable(CreatureTemplate const* creature, bool canTameExotic)
{
    auto it = creature->difficultyStore.find(DIFFICULTY_NORMAL);
    if (it == creature->difficultyStore.end())
        return false;
    CreatureDifficulty const* diff = &it->second;
    if (creature->type != CREATURE_TYPE_BEAST || creature->family == CREATURE_FAMILY_NONE
        || (diff->TypeFlags & CREATURE_TYPE_FLAG_TAMEABLE) == 0)
        return false;
    return canTameExotic || !(diff->TypeFlags & CREATURE_TYPE_FLAG_TAMEABLE_EXOTIC);
}
inline bool CreatureTemplate_IsTameable(CreatureTemplate const& creature, bool canTameExotic)
{
    return CreatureTemplate_IsTameable(&creature, canTameExotic);
}
//End By leewheel

// Creature辅助函数: loot → m_loot(unique_ptr<Loot>)
inline Loot* Creature_GetLoot(Creature* creature)
{
    return creature->m_loot.get();
}
inline Loot const* Creature_GetLoot(Creature const* creature)
{
    return creature->m_loot.get();
}

// Creature辅助函数: GetCreatedBySpell → TC的Unit有SetCreatedBySpell但没有getter
inline uint32 Creature_GetCreatedBySpell(Creature const* creature)
{
    return *creature->m_unitData->CreatedBySpell;
}

// TaxiNodesEntry辅助函数: AC map_id/x/y/z → TC ContinentID/Pos
inline uint32 TaxiNodesEntry_GetMapId(const TaxiNodesEntry* entry)
{
    return entry->ContinentID;
}
inline float TaxiNodesEntry_GetX(const TaxiNodesEntry* entry)
{
    return entry->Pos.X;
}
inline float TaxiNodesEntry_GetY(const TaxiNodesEntry* entry)
{
    return entry->Pos.Y;
}
inline float TaxiNodesEntry_GetZ(const TaxiNodesEntry* entry)
{
    return entry->Pos.Z;
}

// TaxiPathNodeEntry辅助函数: AC mapid/x/y/z → TC ContinentID/Loc
inline uint32 TaxiPathNodeEntry_GetMapId(const TaxiPathNodeEntry* entry)
{
    return entry->ContinentID;
}
inline float TaxiPathNodeEntry_GetX(const TaxiPathNodeEntry* entry)
{
    return entry->Loc.X;
}
inline float TaxiPathNodeEntry_GetY(const TaxiPathNodeEntry* entry)
{
    return entry->Loc.Y;
}
inline float TaxiPathNodeEntry_GetZ(const TaxiPathNodeEntry* entry)
{
    return entry->Loc.Z;
}

// ItemTemplate辅助函数: AC Class → TC GetClass()
inline uint32 ItemTemplate_GetClass(const ItemTemplate* proto)
{
    return proto->GetClass();
}

// Quest方法兼容: AC IsAutoComplete() → TC检查标志位
inline bool Quest_IsAutoComplete(Quest const* quest)
{
    return quest->GetFlags() & QUEST_FLAGS_AUTO_COMPLETE;
}

// Quest方法兼容: AC GetRequiredClasses() → TC GetAllowableClasses()
//By leewheel 2026-09-09: TC-Cata的Quest用GetAllowableClasses而非GetRequiredClasses
inline uint32 Quest_GetRequiredClasses(Quest const* quest)
{
    return quest->GetAllowableClasses();
}
//End By leewheel

// Player兼容: AC InitTalentForLevel() → TC UpdateAvailableTalentPoints()
//By leewheel 2026-09-09: TC-Cata用UpdateAvailableTalentPoints替代InitTalentForLevel
inline void Player_InitTalentForLevel(Player* player)
{
    player->UpdateAvailableTalentPoints();
}
//End By leewheel

// SpellMgr兼容: AC GetSpellInfoStoreSize() → TC没有此方法
inline uint32 SpellMgr_GetSpellInfoStoreSize()
{
    return 0;
}
#define GetSpellInfoStoreSize() SpellMgr_GetSpellInfoStoreSize()

// BattlegroundMgr兼容: AC BGArenaType → TC通过BattlegroundQueueTypeId.TeamSize获取
//By leewheel 2026-09-08: TC-Cata的BattlegroundQueueTypeId无GetArenaType方法,用TeamSize字段替代
inline uint8 BattlegroundMgr_BGArenaType(BattlegroundQueueTypeId queueId)
{
    return queueId.TeamSize;
}
//End By leewheel

// ArenaTeam兼容: AC GetMembers() → TC使用m_membersBegin/m_membersEnd
// 返回一个可迭代的范围
//By leewheel 2026-09-08: TC-Cata的m_membersBegin/m_membersEnd是非const方法,需非const指针
inline auto ArenaTeam_GetMembers(ArenaTeam* team)
{
    return Trinity::IteratorPair(team->m_membersBegin(), team->m_membersEnd());
}
//End By leewheel

// ArenaTeamMgr兼容: AC GetPersonalArenaTeams → TC没有此概念
inline ArenaTeam const* ArenaTeamMgr_GetPersonalArenaTeam(ObjectGuid /*guid*/)
{
    return nullptr;
}

// Player兼容: AC GetComboPoints() → TC通过Power系统获取
//By leewheel 2026-09-08: TC-Cata无GetComboPoints方法,用GetPower(POWER_COMBO_POINTS)替代
inline uint8 Player_GetComboPoints(Player const* player)
{
    return uint8(player->GetPower(POWER_COMBO_POINTS));
}
//End By leewheel

// Player兼容: AC GetDivider/SetDivider → TC没有此成员，返回空ObjectGuid
//By leewheel 2026-09-08: 移除宏定义(宏无法处理obj->方法()语法),改为在源文件中直接调用自由函数
inline ObjectGuid Player_GetDivider(Player const* /*player*/)
{
    return ObjectGuid::Empty;
}
inline void Player_SetDivider(Player* /*player*/, ObjectGuid /*guid*/)
{
}
//End By leewheel

// Player兼容: AC InitTalentForLevel() → TC使用UpdateAvailableTalentPoints
// 注意：不能用宏因为调用对象可能不是this，需要在源文件中逐个修改

// QuestStatusData兼容: AC CreatureOrGOCount → TC使用QuestObjective系统
inline uint32 QuestStatusData_GetCreatureOrGOCount(QuestStatusData const* /*data*/, uint32 /*questId*/, uint32 /*creatureOrGOEntry*/)
{
    // TODO: 按Cata的QuestObjective系统重新实现
    return 0;
}

// NotNormalLootItem兼容: AC index → TC字段名不同
//By leewheel 2026-09-08: TC-Cata的NotNormalLootItem用LootListId而非Index
inline uint8 NotNormalLootItem_GetIndex(NotNormalLootItem const* item)
{
    return item->LootListId;
}
//End By leewheel

// CreatureData兼容: AC mapid → TC MapID
inline uint32 CreatureData_GetMapId(CreatureData const* data)
{
    return data->mapId;
}

// Trainer兼容: AC GetSpells/GetTrainerType → TC使用不同API
// 需要在源文件中适配
//By leewheel 2026-09-09: TC-Cata的Trainer::Trainer API不同，添加兼容辅助函数
inline bool Trainer_IsTrainerValidForPlayer(Trainer::Trainer const* /*trainer*/, Player const* /*player*/)
{
    // TC中没有直接对应的IsTrainerValidForPlayer方法
    // 简化处理：总是返回true，由后续的CanTeachSpell逐个法术检查过滤
    return true;
}
//End By leewheel

// ItemTemplate兼容: AC GetName()无参 → TC GetName(locale)需locale参数
//By leewheel 2026-09-09: TC-Cata的ItemTemplate::GetName需要LocaleConstant参数
inline char const* ItemTemplate_GetName(ItemTemplate const* proto)
{
    return proto->GetName(sWorld->GetDefaultDbcLocale());
}
//End By leewheel

// CreatureTemplate兼容: AC Class → TC unit_class
//By leewheel 2026-09-09: TC-Cata的CreatureTemplate用unit_class而非Class
inline uint32 CreatureTemplate_GetClass(CreatureTemplate const* proto)
{
    return proto->unit_class;
}
//End By leewheel

// Creature兼容: AC lootid → TC GetLootId()
//By leewheel 2026-09-09: TC-Cata用GetLootId()方法而非lootid成员
inline uint32 Creature_GetLootId(Creature const* creature)
{
    return creature->GetLootId();
}
//End By leewheel

// ObjectGuid兼容: AC WriteAsPacked(buf) → TC buf << guid
//By leewheel 2026-09-09: TC-Cata通过operator<<写入packed guid
inline void ObjectGuid_WriteAsPacked(ObjectGuid const& guid, ByteBuffer& buf)
{
    buf << guid;
}
//End By leewheel

// Guild兼容: AC GetMemberRankRights公开 → TC私有,改用HasAnyRankRight
//By leewheel 2026-09-09: TC-Cata的GetMemberRankRights/GetRankInfo都是private,只能用HasAnyRankRight
inline bool Guild_HasRankRight(Guild const* guild, ObjectGuid const& guid, uint32 right)
{
    Guild::Member const* member = guild->GetMember(guid);
    if (!member)
        return false;
    return guild->HasAnyRankRight(member->GetRankId(), GuildRankRights(right));
}
// Guild银行标签权限兼容(TODO:需通过其他方式实现,当前用stub)
inline bool Guild_MemberHasTabRights(Guild const* /*guild*/, ObjectGuid const& /*guid*/, uint8 /*tabId*/, int32 /*rights*/)
{
    // TODO: TC-Cata无公开API可查rank bank tab rights,暂时返回true避免功能完全失效
    return true;
}
//End By leewheel

// CombatRating兼容: AC CR_* 整数宏 → TC CombatRating枚举
//By leewheel 2026-09-09: TC-Cata的CombatRating枚举中已移除CR_HIT_TAKEN_*和CR_CRIT_TAKEN_*，
// 这些等级在Cata中不存在，使用MAX_COMBAT_RATING作为安全占位值（位掩码中无实际效果）
#ifndef CR_HIT_TAKEN_MELEE
#define CR_HIT_TAKEN_MELEE    MAX_COMBAT_RATING
#endif
#ifndef CR_HIT_TAKEN_RANGED
#define CR_HIT_TAKEN_RANGED   MAX_COMBAT_RATING
#endif
#ifndef CR_HIT_TAKEN_SPELL
#define CR_HIT_TAKEN_SPELL    MAX_COMBAT_RATING
#endif
#ifndef CR_CRIT_TAKEN_MELEE
#define CR_CRIT_TAKEN_MELEE   MAX_COMBAT_RATING
#endif
#ifndef CR_CRIT_TAKEN_RANGED
#define CR_CRIT_TAKEN_RANGED  MAX_COMBAT_RATING
#endif
#ifndef CR_CRIT_TAKEN_SPELL
#define CR_CRIT_TAKEN_SPELL   MAX_COMBAT_RATING
#endif
//By leewheel 2026-09-09: TC-Cata已移除CR_WEAPON_SKILL(原WotLK中=0)，
//StatsCollector中循环遍历战斗评级从0开始，使用CR_UNUSED_0(=0)兼容
#ifndef CR_WEAPON_SKILL
#define CR_WEAPON_SKILL       0
#endif
//End By leewheel

// BattlegroundWS stub (TC-Cata无此类)
//By leewheel 2026-09-09: 添加BattlegroundWS存根类
class BattlegroundWS : public Battleground
{
public:
    ObjectGuid GetFlagPickerGUID(TeamId /*teamId*/) const { return ObjectGuid::Empty; }
};
//End By leewheel

// ========== TC-Cata 新增兼容层 (2026-09-09) ==========

// DEFAULT_FOLLOW_ANGLE → TC使用PET_FOLLOW_ANGLE
#ifndef DEFAULT_FOLLOW_ANGLE
#define DEFAULT_FOLLOW_ANGLE PET_FOLLOW_ANGLE
#endif

// GetPhaseMask → TC使用GetPhaseShift (PhaseShift而非uint32 phasemask)
// Map::GetAreaId/GetZoneId 需要PhaseShift const&参数
#define GetPhaseMask() GetPhaseShift()

// StopMovingOnCurrentPos → TC使用StopMoving
#define StopMovingOnCurrentPos() StopMoving()

// ScalingStatDistributionEntry: AC用MaxLevel，TC用Maxlevel(小写l)
// 由于是结构体成员，不能用宏全局替换，在使用处手动修改

// BuildChatPacket兼容: AC的ChatHandler::BuildChatPacket → TC的WorldPackets::Chat::Chat
// 需要包含ChatPackets.h
#include "Server/Packets/ChatPackets.h"

// 兼容函数1: BuildChatPacket(data, chatType, language, sender, receiver, message)
inline void BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, WorldObject const* sender,
    WorldObject const* receiver, std::string_view message)
{
    WorldPackets::Chat::Chat packet;
    packet.Initialize(chatType, language, sender, receiver, message);
    data = *packet.Write();
}

// 兼容函数2: BuildChatPacket(data, chatType, message, language, chatFlags, senderGuid, senderName)
// AC版本带有chat tag/guid/name参数，TC通过Initialize自动设置
inline void BuildChatPacket(WorldPacket& data, ChatMsg chatType, const char* message, Language language,
    uint16 /*chatFlags*/, ObjectGuid senderGuid, const char* senderName)
{
    WorldPackets::Chat::Chat packet;
    packet.SlashCmd = chatType;
    packet._Language = language;
    packet.SenderGUID = senderGuid;
    packet.SenderName = senderName ? senderName : "";
    packet.ChatText = message ? message : "";
    data = *packet.Write();
}

// CharmInfo兼容: SetForcedSpell/SetForcedTargetGUID 在TC-Cata中不存在
// 提供空实现的存根函数（通过宏替换为no-op，避免影响运行时行为）
// 注意：这些是CharmInfo的成员函数，无法直接扩展，使用宏在调用处替换为空
// 由于CharmInfo类定义在核心中不可修改，这里提供全局存根
// 实际调用处需要注释掉或改为空操作

// Creature loot兼容: AC的creature->loot.isLooted() → TC的creature->m_loot.get() && creature->m_loot.get()->isLooted()
// TC的Creature没有GetLoot()方法，m_loot是unique_ptr<Loot>，使用已有的Creature_GetLoot兼容函数
inline bool Creature_IsLooted(Creature const* creature)
{
    Loot const* loot = Creature_GetLoot(creature);
    return loot && loot->isLooted();
}

// IsImmunedToSpell兼容: AC用(spellInfo, caster) → TC用(spellInfo, effectMask, caster)
inline bool Unit_IsImmunedToSpell(Unit const* unit, SpellInfo const* spellInfo, WorldObject const* caster)
{
    return unit->IsImmunedToSpell(spellInfo, MAX_EFFECT_MASK, caster);
}

//End By leewheel 2026-09-09

//End By leewheel 2026-09-08

#endif // PLAYERBOTS_PLAYERBOTS_H
