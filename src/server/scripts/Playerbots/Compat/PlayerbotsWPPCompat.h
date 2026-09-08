/*
 * WorldPackets 兼容层
 * 从 AzerothCore (AC) 移植到 TrinityCore (TC)
 * AC的WorldSession handler接受WorldPacket参数，TC使用WorldPackets结构体
 * 此兼容层提供内联辅助函数，自动构造正确的WorldPackets结构体并调用handler
 */

#ifndef PLAYERBOTS_WPP_COMPAT_H
#define PLAYERBOTS_WPP_COMPAT_H

#include "NPCPackets.h"
#include "GameObjectPackets.h"
#include "SpellPackets.h"
#include "ChatPackets.h"
#include "VehiclePackets.h"
#include "TradePackets.h"
#include "LootPackets.h"
#include "AreaTriggerPackets.h"
#include "QuestPackets.h"
#include "MailPackets.h"
#include "CombatPackets.h"
#include "LFGPackets.h"
#include "GuildPackets.h"
#include "PetitionPackets.h"
#include "AuctionHousePackets.h"
#include "BattlegroundPackets.h"
#include "MiscPackets.h"
#include "ItemPackets.h"
#include "InstancePackets.h"
#include "PartyPackets.h"
#include "DuelPackets.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "ObjectGuid.h"
#include "ObjectAccessor.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Battleground.h"
#include "Player.h"
#include "Item.h"
#include "Define.h"

/*
 * 兼容层命名空间: WPPCompat
 * 用法: 将代码中的
 *   WorldPacket data(CMSG_GOSSIP_HELLO);
 *   data << guid;
 *   bot->GetSession()->HandleGossipHelloOpcode(data);
 * 替换为
 *   WPPCompat::DoGossipHello(bot->GetSession(), guid);
 */
namespace WPPCompat
{
    // ========== NPC 处理器 ==========

    // DoGossipHello - 对应 HandleGossipHelloOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoGossipHello(WorldSession* session, ObjectGuid guid)
    {
        WorldPacket data(CMSG_TALK_TO_GOSSIP);
        WorldPackets::NPC::Hello packet(std::move(data));
        packet.Unit = guid;
        session->HandleGossipHelloOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // DoGossipSelectOption - 对应 HandleGossipSelectOptionOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoGossipSelectOption(WorldSession* session, ObjectGuid gossipUnit, int32 gossipOptionID, int32 gossipID = 0, const std::string& promotionCode = "")
    {
        WorldPacket data(CMSG_GOSSIP_SELECT_OPTION);
        WorldPackets::NPC::GossipSelectOption packet(std::move(data));
        packet.GossipUnit = gossipUnit;
        packet.GossipOptionID = gossipOptionID;
        packet.GossipID = gossipID;
        packet.PromotionCode = promotionCode;
        session->HandleGossipSelectOptionOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // DoSpiritHealerActivate - 对应 HandleSpiritHealerActivate
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoSpiritHealerActivate(WorldSession* session, ObjectGuid healer)
    {
        WorldPacket data(CMSG_SPIRIT_HEALER_ACTIVATE);
        WorldPackets::NPC::SpiritHealerActivate packet(std::move(data));
        packet.Healer = healer;
        session->HandleSpiritHealerActivate(packet);
    }
    // End By leewheel 2026-07-09

    // ========== GameObject 处理器 ==========

    // GameObjectUse - 对应 HandleGameObjectUseOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void GameObjectUse(WorldSession* session, ObjectGuid guid)
    {
        WorldPacket data(CMSG_GAME_OBJ_USE);
        WorldPackets::GameObject::GameObjUse packet(std::move(data));
        packet.Guid = guid;
        session->HandleGameObjectUseOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // GameObjectReportUse - 对应 HandleGameobjectReportUse
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void GameObjectReportUse(WorldSession* session, ObjectGuid guid)
    {
        WorldPacket data(CMSG_GAME_OBJ_REPORT_USE);
        WorldPackets::GameObject::GameObjReportUse packet(std::move(data));
        packet.Guid = guid;
        session->HandleGameobjectReportUse(packet);
    }
    // End By leewheel 2026-07-09

    // ========== Spell 处理器 ==========

    // DoCancelMountAura - 对应 HandleCancelMountAuraOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoCancelMountAura(WorldSession* session)
    {
        WorldPacket data(CMSG_CANCEL_MOUNT_AURA);
        WorldPackets::Spells::CancelMountAura packet(std::move(data));
        session->HandleCancelMountAuraOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // DoCancelAura - 对应 HandleCancelAuraOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoCancelAura(WorldSession* session, ObjectGuid casterGUID, int32 spellID)
    {
        WorldPacket data(CMSG_CANCEL_AURA);
        WorldPackets::Spells::CancelAura packet(std::move(data));
        packet.CasterGUID = casterGUID;
        packet.SpellID = spellID;
        session->HandleCancelAuraOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // DoOpenItem - 对应 HandleOpenItemOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoOpenItem(WorldSession* session, uint8 slot, uint8 packSlot)
    {
        WorldPacket data(CMSG_OPEN_ITEM);
        WorldPackets::Spells::OpenItem packet(std::move(data));
        packet.Slot = slot;
        packet.PackSlot = packSlot;
        session->HandleOpenItemOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // DoUseItem - 对应 HandleUseItemOpcode
    // 注意: TC的UseItem结构包含SpellCastRequest，比较复杂
    //By leewheel 2026-08-15: 修复——原wrapper不填Cast.SpellID，TC HandleUseItemOpcode的
    //GetSpellInfo(0)失败直接return导致用物品静默无效。现要求调用方传spellId并填Cast.SpellID
    //(物品使用法术ID)。两步构造避免MSVC的Most Vexing Parse问题
    inline void DoUseItem(WorldSession* session, uint8 packSlot, uint8 slot, ObjectGuid castItem, uint32 spellId)
    {
        WorldPacket data(CMSG_USE_ITEM);
        WorldPackets::Spells::UseItem packet(std::move(data));
        packet.PackSlot = packSlot;
        packet.Slot = slot;
        packet.CastItem = castItem;
        packet.Cast.SpellID = static_cast<int32>(spellId);
        session->HandleUseItemOpcode(packet);
    }
    //End By leewheel

    // ========== Chat 处理器 ==========

    // DoTextEmote - 对应 HandleTextEmoteOpcode
    // By leewheel 2026-07-09: 使用两步构造避免MSVC的Most Vexing Parse问题
    inline void DoTextEmote(WorldSession* session, ObjectGuid target, int32 emoteID, int32 soundIndex = -1)
    {
        WorldPacket data(CMSG_SEND_TEXT_EMOTE);
        WorldPackets::Chat::CTextEmote packet(std::move(data));
        packet.Target = target;
        packet.EmoteID = emoteID;
        packet.SoundIndex = soundIndex;
        session->HandleTextEmoteOpcode(packet);
    }
    // End By leewheel 2026-07-09

    // ========== Vehicle 处理器 ==========

    // DoRequestVehicleExit - 对应 HandleRequestVehicleExit
    inline void DoRequestVehicleExit(WorldSession* session)
    {
        WorldPacket data(CMSG_REQUEST_VEHICLE_EXIT);
        WorldPackets::Vehicle::RequestVehicleExit packet(std::move(data));
        session->HandleRequestVehicleExit(packet);
    }

    // ========== Trade 处理器 ==========

    // DoInitiateTrade - 对应 HandleInitiateTradeOpcode
    inline void DoInitiateTrade(WorldSession* session, ObjectGuid guid)
    {
        WorldPacket data(CMSG_INITIATE_TRADE);
        WorldPackets::Trade::InitiateTrade packet(std::move(data));
        packet.Guid = guid;
        session->HandleInitiateTradeOpcode(packet);
    }

    // DoSetTradeGold - 对应 HandleSetTradeGoldOpcode
    inline void DoSetTradeGold(WorldSession* session, uint64 coinage)
    {
        WorldPacket data(CMSG_SET_TRADE_GOLD);
        WorldPackets::Trade::SetTradeGold packet(std::move(data));
        packet.Coinage = coinage;
        session->HandleSetTradeGoldOpcode(packet);
    }

    // DoSetTradeItem - 对应 HandleSetTradeItemOpcode
    inline void DoSetTradeItem(WorldSession* session, uint8 tradeSlot, uint8 packSlot, uint8 itemSlotInPack)
    {
        WorldPacket data(CMSG_SET_TRADE_ITEM);
        WorldPackets::Trade::SetTradeItem packet(std::move(data));
        packet.TradeSlot = tradeSlot;
        packet.PackSlot = packSlot;
        packet.ItemSlotInPack = itemSlotInPack;
        session->HandleSetTradeItemOpcode(packet);
    }

    // DoClearTradeItem - 对应 HandleClearTradeItemOpcode
    inline void DoClearTradeItem(WorldSession* session, uint8 tradeSlot)
    {
        WorldPacket data(CMSG_CLEAR_TRADE_ITEM);
        WorldPackets::Trade::ClearTradeItem packet(std::move(data));
        packet.TradeSlot = tradeSlot;
        session->HandleClearTradeItemOpcode(packet);
    }

    // DoAcceptTrade - 对应 HandleAcceptTradeOpcode
    inline void DoAcceptTrade(WorldSession* session, uint32 stateIndex = 0)
    {
        WorldPacket data(CMSG_ACCEPT_TRADE);
        WorldPackets::Trade::AcceptTrade packet(std::move(data));
        packet.StateIndex = stateIndex;
        session->HandleAcceptTradeOpcode(packet);
    }

    inline void BeginTrade(WorldSession* session)
    {
        WorldPacket data(CMSG_BEGIN_TRADE);
        WorldPackets::Trade::BeginTrade packet(std::move(data));
        session->HandleBeginTradeOpcode(packet);
    }

    inline void CancelTrade(WorldSession* session)
    {
        WorldPacket data(CMSG_CANCEL_TRADE);
        WorldPackets::Trade::CancelTrade packet(std::move(data));
        session->HandleCancelTradeOpcode(packet);
    }

    inline void UnacceptTrade(WorldSession* session)
    {
        WorldPacket data(CMSG_UNACCEPT_TRADE);
        WorldPackets::Trade::UnacceptTrade packet(std::move(data));
        session->HandleUnacceptTradeOpcode(packet);
    }

    // ========== Loot 处理器 ==========

    inline void LootUnit(WorldSession* session, ObjectGuid unit)
    {
        WorldPacket data(CMSG_LOOT_UNIT);
        WorldPackets::Loot::LootUnit packet(std::move(data));
        packet.Unit = unit;
        session->HandleLootOpcode(packet);
    }

    inline void LootRelease(WorldSession* session, ObjectGuid unit)
    {
        WorldPacket data(CMSG_LOOT_RELEASE);
        WorldPackets::Loot::LootRelease packet(std::move(data));
        packet.Unit = unit;
        session->HandleLootReleaseOpcode(packet);
    }

    inline void LootMoney(WorldSession* session)
    {
        WorldPacket data(CMSG_LOOT_MONEY);
        WorldPackets::Loot::LootMoney packet(std::move(data));
        session->HandleLootMoneyOpcode(packet);
    }

    // ========== AreaTrigger 处理器 ==========

    inline void AreaTrigger(WorldSession* session, int32 areaTriggerID, bool entered = true, bool fromClient = true)
    {
        WorldPacket data(CMSG_AREA_TRIGGER);
        WorldPackets::AreaTrigger::AreaTrigger packet(std::move(data));
        packet.AreaTriggerID = areaTriggerID;
        packet.Entered = entered;
        packet.FromClient = fromClient;
        session->HandleAreaTriggerOpcode(packet);
    }

    // ========== Quest 处理器 ==========

    inline void QuestGiverHello(WorldSession* session, ObjectGuid questGiverGUID)
    {
        WorldPacket data(CMSG_QUEST_GIVER_HELLO);
        WorldPackets::Quest::QuestGiverHello packet(std::move(data));
        packet.QuestGiverGUID = questGiverGUID;
        session->HandleQuestgiverHelloOpcode(packet);
    }

    inline void QuestGiverAcceptQuest(WorldSession* session, ObjectGuid questGiverGUID, int32 questID, bool startCheat = false)
    {
        WorldPacket data(CMSG_QUEST_GIVER_ACCEPT_QUEST);
        WorldPackets::Quest::QuestGiverAcceptQuest packet(std::move(data));
        packet.QuestGiverGUID = questGiverGUID;
        packet.QuestID = questID;
        packet.StartCheat = startCheat;
        session->HandleQuestgiverAcceptQuestOpcode(packet);
    }

    inline void QuestGiverChooseReward(WorldSession* session, ObjectGuid questGiverGUID, int32 questID, int32 choiceID)
    {
        WorldPacket data(CMSG_QUEST_GIVER_CHOOSE_REWARD);
        WorldPackets::Quest::QuestGiverChooseReward packet(std::move(data));
        packet.QuestGiverGUID = questGiverGUID;
        packet.QuestID = questID;
        //By leewheel 2026-07-09: TC的QuestChoiceItem使用Item.ItemID而非直接ItemID
        packet.Choice.Item.ItemID = choiceID;
        //End By leewheel
        session->HandleQuestgiverChooseRewardOpcode(packet);
    }

    inline void QuestGiverCompleteQuest(WorldSession* session, ObjectGuid questGiverGUID, int32 questID, bool fromScript = false)
    {
        WorldPacket data(CMSG_QUEST_GIVER_COMPLETE_QUEST);
        WorldPackets::Quest::QuestGiverCompleteQuest packet(std::move(data));
        packet.QuestGiverGUID = questGiverGUID;
        packet.QuestID = questID;
        packet.FromScript = fromScript;
        session->HandleQuestgiverCompleteQuest(packet);
    }

    inline void QuestGiverQueryQuest(WorldSession* session, ObjectGuid questGiverGUID, int32 questID, bool respondToGiver = false)
    {
        WorldPacket data(CMSG_QUEST_GIVER_QUERY_QUEST);
        WorldPackets::Quest::QuestGiverQueryQuest packet(std::move(data));
        packet.QuestGiverGUID = questGiverGUID;
        packet.QuestID = questID;
        packet.RespondToGiver = respondToGiver;
        session->HandleQuestgiverQueryQuestOpcode(packet);
    }

    inline void QuestLogRemoveQuest(WorldSession* session, uint8 entry)
    {
        WorldPacket data(CMSG_QUEST_LOG_REMOVE_QUEST);
        WorldPackets::Quest::QuestLogRemoveQuest packet(std::move(data));
        packet.Entry = entry;
        session->HandleQuestLogRemoveQuest(packet);
    }

    // ========== Mail 处理器 ==========

    inline void MailTakeMoney(WorldSession* session, ObjectGuid mailbox, uint64 mailID, uint64 money)
    {
        WorldPacket data(CMSG_MAIL_TAKE_MONEY);
        WorldPackets::Mail::MailTakeMoney packet(std::move(data));
        packet.Mailbox = mailbox;
        packet.MailID = mailID;
        packet.Money = money;
        session->HandleMailTakeMoney(packet);
    }

    inline void MailTakeItem(WorldSession* session, ObjectGuid mailbox, uint64 mailID, uint64 attachID)
    {
        WorldPacket data(CMSG_MAIL_TAKE_ITEM);
        WorldPackets::Mail::MailTakeItem packet(std::move(data));
        packet.Mailbox = mailbox;
        packet.MailID = mailID;
        packet.AttachID = attachID;
        session->HandleMailTakeItem(packet);
    }

    inline void MailDelete(WorldSession* session, uint64 mailID, int32 deleteReason = 0)
    {
        WorldPacket data(CMSG_MAIL_DELETE);
        WorldPackets::Mail::MailDelete packet(std::move(data));
        packet.MailID = mailID;
        packet.DeleteReason = deleteReason;
        session->HandleMailDelete(packet);
    }

    // ========== Combat 处理器 ==========

    inline void AttackSwing(WorldSession* session, ObjectGuid victim)
    {
        WorldPacket data(CMSG_ATTACK_SWING);
        WorldPackets::Combat::AttackSwing packet(std::move(data));
        packet.Victim = victim;
        session->HandleAttackSwingOpcode(packet);
    }

    inline void AttackStop(WorldSession* session)
    {
        WorldPacket data(CMSG_ATTACK_STOP);
        WorldPackets::Combat::AttackStop packet(std::move(data));
        session->HandleAttackStopOpcode(packet);
    }

    // ========== LFG 处理器 ==========

    inline void LfgLeave(WorldSession* session)
    {
        WorldPacket data(CMSG_DF_LEAVE);
        WorldPackets::LFG::DFLeave packet(std::move(data));
        session->HandleLfgLeaveOpcode(packet);
    }

    inline void LfgSetRoles(WorldSession* session, uint8 rolesDesired)
    {
        WorldPacket data(CMSG_DF_SET_ROLES);
        WorldPackets::LFG::DFSetRoles packet(std::move(data));
        packet.RolesDesired = rolesDesired;
        session->HandleLfgSetRolesOpcode(packet);
    }

    inline void LfgTeleport(WorldSession* session, bool teleportOut = false)
    {
        WorldPacket data(CMSG_DF_TELEPORT);
        WorldPackets::LFG::DFTeleport packet(std::move(data));
        packet.TeleportOut = teleportOut;
        session->HandleLfgTeleportOpcode(packet);
    }

    // ========== Guild 处理器 ==========

    // GuildLeave - 对应 HandleGuildLeave
    inline void GuildLeave(WorldSession* session)
    {
        WorldPacket data(CMSG_GUILD_LEAVE);
        WorldPackets::Guild::GuildLeave packet(std::move(data));
        session->HandleGuildLeave(packet);
    }

    // ========== Petition 处理器 ==========

    inline void PetitionShowList(WorldSession* session, ObjectGuid petitionUnit)
    {
        WorldPacket data(CMSG_PETITION_SHOW_LIST);
        WorldPackets::Petition::PetitionShowList packet(std::move(data));
        packet.PetitionUnit = petitionUnit;
        session->HandlePetitionShowList(packet);
    }

    // DoPetitionBuy - 对应 HandlePetitionBuy
    inline void DoPetitionBuy(WorldSession* session, ObjectGuid unit, const std::string& title)
    {
        WorldPacket data(CMSG_PETITION_BUY);
        WorldPackets::Petition::PetitionBuy packet(std::move(data));
        packet.Unit = unit;
        packet.Title = title;
        session->HandlePetitionBuy(packet);
    }

    inline void PetitionShowSignatures(WorldSession* session, ObjectGuid item)
    {
        WorldPacket data(CMSG_PETITION_SHOW_SIGNATURES);
        WorldPackets::Petition::PetitionShowSignatures packet(std::move(data));
        packet.Item = item;
        session->HandlePetitionShowSignatures(packet);
    }

    inline void PetitionRenameGuild(WorldSession* session, ObjectGuid petitionGuid, const std::string& newGuildName)
    {
        WorldPacket data(CMSG_PETITION_RENAME_GUILD);
        WorldPackets::Petition::PetitionRenameGuild packet(std::move(data));
        packet.PetitionGuid = petitionGuid;
        packet.NewGuildName = newGuildName;
        session->HandlePetitionRenameGuild(packet);
    }

    // ========== TC中不存在的AC处理器实现 ==========
    // 这些处理器在AC中存在但在TC中没有对应的opcode handler
    // 需要直接实现功能逻辑

    // AC: HandleGuildAcceptOpcode - 接受公会邀请
    // TC中没有此handler，需要通过Guild::HandleAcceptMember实现
    inline void GuildAccept(WorldSession* session)
    {
        if (Player* player = session->GetPlayer())
        {
            if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildIdInvited()))
            {
                guild->HandleAcceptMember(player->GetSession());
            }
        }
    }

    // AC: HandleGuildDeclineOpcode - 拒绝公会邀请
    inline void GuildDecline(WorldSession* session)
    {
        if (Player* player = session->GetPlayer())
        {
            player->SetGuildIdInvited(0);
        }
    }

    // AC: HandleGroupInviteOpcode - 邀请玩家加入队伍
    //By leewheel 2026-07-09: TC使用Group::AddInvite而非Player::InviteToGroup
    inline void GroupInvite(WorldSession* session, const std::string& playerName)
    {
        if (Player* player = session->GetPlayer())
        {
            if (Player* target = ObjectAccessor::FindPlayerByName(playerName))
            {
                Group* group = player->GetGroup();
                if (!group)
                {
                    group = new Group;
                    group->Create(player);
                }
                group->AddInvite(target);
            }
        }
    }
    //End By leewheel

    // AC: HandleGroupUninviteOpcode - 将玩家移出队伍
    inline void GroupUninvite(WorldSession* session, ObjectGuid targetGuid)
    {
        if (Player* player = session->GetPlayer())
        {
            if (Group* group = player->GetGroup())
            {
                group->RemoveMember(targetGuid);
            }
        }
    }

    // AC: HandleGroupUninviteGuidOpcode - 通过GUID将玩家移出队伍
    inline void GroupUninviteGuid(WorldSession* session, ObjectGuid targetGuid)
    {
        GroupUninvite(session, targetGuid);
    }

    // AC: HandleOfferPetitionOpcode - 展示请愿书给其他玩家签名
    inline void OfferPetition(WorldSession* session, ObjectGuid itemGuid, ObjectGuid targetGuid)
    {
        if (Player* player = session->GetPlayer())
        {
            // 直接使用TC的Petition签名逻辑
            if (Item* item = player->GetItemByGuid(itemGuid))
            {
                if (Player* target = ObjectAccessor::FindConnectedPlayer(targetGuid))
                {
                    // 发送请愿书签名请求
                    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, 8 + 8 + 4);
                    data << itemGuid;
                    data << player->GetGUID();
                    data << uint32(0); // 签名数量
                    target->GetSession()->SendPacket(&data);
                }
            }
        }
    }

    //By leewheel 2026-08-15: 删除误导性stub——原TurnInPetition自称"简化版"实际只发
    //PetitionShowSignatures包(不创建公会)，是死代码(全仓库无调用点)。真正的公会提交走
    //typed TurnInPetitionOpcode→session->HandleTurnInPetition(见下方Petition处理器段)
    //End By leewheel

    // AC: HandleLeaveBattlefieldOpcode - 离开战场
    inline void LeaveBattlefield(WorldSession* session)
    {
        if (Player* player = session->GetPlayer())
        {
            if (Battleground* bg = player->GetBattleground())
            {
                //By leewheel 2026-07-09: TC的RemovePlayerAtLeave需要3个参数
            bg->RemovePlayerAtLeave(player->GetGUID(), false, true);
            //End By leewheel
            }
        }
    }

    // AC: HandleSocketOpcode - 镶嵌宝石
    // TC中通过Item::SocketItem实现
    inline void SocketItem(WorldSession* session, ObjectGuid itemGuid, ObjectGuid gemGuid, uint8 slot)
    {
        if (Player* player = session->GetPlayer())
        {
            if (Item* item = player->GetItemByGuid(itemGuid))
            {
                if (Item* gem = player->GetItemByGuid(gemGuid))
                {
                    //By leewheel 2026-07-09: TC使用_ApplyItemMods而非ApplyItemMod
                    player->_ApplyItemMods(item, slot, true);
                    //End By leewheel
                }
            }
        }
    }

    // ========== MountSpecialAnim ==========
    inline void MountSpecialAnim(WorldSession* session)
    {
        WorldPacket data(CMSG_MOUNT_SPECIAL_ANIM);
        WorldPackets::Misc::MountSpecial packet(std::move(data));
        session->HandleMountSpecialAnimOpcode(packet);
    }

    // ========== AutostoreLootItem ==========
    //By leewheel 2026-08-15: 修复——packet.Loot是底层std::vector(初始size=0)，
    //原operator[0]直接越界写=未定义行为。改用emplace_back后赋值；
    //且LootListID为1-based(服务端req.LootListID-1取槽)，入参统一按1-based传
    inline void AutostoreLootItem(WorldSession* session, uint8 lootSlot)
    {
        WorldPacket data(CMSG_LOOT_ITEM);
        WorldPackets::Loot::LootItem packet(std::move(data));
        packet.Loot.emplace_back();
        packet.Loot[0].Object = ObjectGuid::Empty;
        packet.Loot[0].LootListID = lootSlot;
        session->HandleAutostoreLootItemOpcode(packet);
    }
    //End By leewheel

    // ========== BattlefieldLeave ==========
    inline void BattlefieldLeave(WorldSession* session)
    {
        WorldPacket data(CMSG_BATTLEFIELD_LEAVE);
        WorldPackets::Battleground::BattlefieldLeave packet(std::move(data));
        session->HandleBattlefieldLeaveOpcode(packet);
    }

    // ========== RequestBattlefieldStatus ==========
    inline void RequestBattlefieldStatus(WorldSession* session)
    {
        WorldPacket data(CMSG_REQUEST_BATTLEFIELD_STATUS);
        WorldPackets::Battleground::RequestBattlefieldStatus packet(std::move(data));
        session->HandleRequestBattlefieldStatusOpcode(packet);
    }

    // ========== AutoStoreBagItem ==========
    // AC: WorldPacket data(CMSG_AUTOSTORE_BAG_ITEM); data << srcBag << srcSlot << dstBag;
    inline void AutoStoreBagItem(WorldSession* session, uint8 containerSlotA, uint8 slotA, uint8 containerSlotB)
    {
        WorldPacket data(CMSG_AUTO_STORE_BAG_ITEM);
        WorldPackets::Item::AutoStoreBagItem packet(std::move(data));
        packet.ContainerSlotA = containerSlotA;
        packet.SlotA = slotA;
        packet.ContainerSlotB = containerSlotB;
        session->HandleAutoStoreBagItemOpcode(packet);
    }

    // ========== AutoEquipItemSlot ==========
    // AC: WorldPacket data(CMSG_AUTOEQUIP_ITEM_SLOT); data << itemguid << dstSlot;
    //By leewheel 2026-08-02: 修复装备失败根因 —— TC的HandleAutoEquipItemSlotOpcode要求
    //   packet.Inv.Items.size()==1（源物品位置），否则直接return静默拒绝装备包。
    //   之前未填充Inv.Items导致所有AutoEquipItemSlot装备包被服务器拒绝，
    //   装备失败→物品留在背包→AI反复选中→反复输出"正在装备"→占满AI决策。
    //   现从itemGuid反查物品位置填充Inv.Items，装备包才能被服务器接受。
    inline void AutoEquipItemSlot(WorldSession* session, ObjectGuid itemGuid, uint8 dstSlot)
    {
        WorldPacket data(CMSG_AUTO_EQUIP_ITEM_SLOT);
        WorldPackets::Item::AutoEquipItemSlot packet(std::move(data));
        packet.Item = itemGuid;
        packet.ItemDstSlot = dstSlot;
        if (Player* player = session->GetPlayer())
        {
            if (Item* item = player->GetItemByGuid(itemGuid))
            {
                packet.Inv.Items.resize(1);
                packet.Inv.Items[0].ContainerSlot = item->GetBagSlot();
                packet.Inv.Items[0].Slot = item->GetSlot();
            }
        }
        session->HandleAutoEquipItemSlotOpcode(packet);
    }
    //End By leewheel

    // ========== SignPetition ==========
    inline void SignPetition(WorldSession* session, ObjectGuid itemGuid)
    {
        //By leewheel 2026-07-11: TC使用CMSG_SIGN_PETITION而非CMSG_PETITION_SIGN
        WorldPacket data(CMSG_SIGN_PETITION);
        //End By leewheel
        WorldPackets::Petition::SignPetition packet(std::move(data));
        //By leewheel 2026-07-09: TC的SignPetition使用PetitionGUID而非Item
        packet.PetitionGUID = itemGuid;
        //End By leewheel
        session->HandleSignPetition(packet);
    }

    // ========== LfgJoin ==========
    //By leewheel 2026-08-15: 修复——TC HandleLfgJoinOpcode的if(Slots.empty())return直接返回，
    //原wrapper只填Roles导致入队静默无效。现支持传入dungeonId列表(为空时取服务器当前可排随机本
    //由调用方决定)，Slots填满后入队才有效
    inline void LfgJoin(WorldSession* session, uint8 roles = 0, std::vector<uint32> const& slots = {})
    {
        //By leewheel 2026-07-11: TC使用CMSG_DF_JOIN而非CMSG_LFG_JOIN
        WorldPacket data(CMSG_DF_JOIN);
        //End By leewheel
        WorldPackets::LFG::DFJoin packet(std::move(data));
        packet.Roles = roles;
        //By leewheel 2026-08-16: 修复——packet.Slots是Array<uint32,50>(vector语义,初始size=0)，
        //原 i<packet.Slots.size() 恒false且operator[]不扩容，Slots永不被填充(入队仍静默无效)。
        //改为push_back填充(Array自带max_capacity=50上限检查)
        for (uint32 slot : slots)
            packet.Slots.push_back(slot);
        //End By leewheel
        session->HandleLfgJoinOpcode(packet);
    }
    //End By leewheel

    // ========== LfgProposalResult ==========
    inline void LfgProposalResult(WorldSession* session, uint32 proposalID, bool accept)
    {
        //By leewheel 2026-07-11: TC使用CMSG_DF_PROPOSAL_RESPONSE而非CMSG_LFG_PROPOSAL_RESULT
        WorldPacket data(CMSG_DF_PROPOSAL_RESPONSE);
        //End By leewheel
        WorldPackets::LFG::DFProposalResponse packet(std::move(data));
        packet.ProposalID = proposalID;
        packet.Accepted = accept;
        session->HandleLfgProposalResultOpcode(packet);
    }

    // ========== AttackSwing (for use with WorldPacket-based calls) ==========
    // These are already defined above but provide WorldPacket-accepting overloads
    inline void HandleTextEmoteOpcode(WorldSession* session, WorldPacket& data)
    {
        uint32 emoteID;
        ObjectGuid target;
        data >> emoteID;
        data >> target;
        //By leewheel 2026-07-09: 修复函数名，TextEmote应为DoTextEmote
        DoTextEmote(session, target, emoteID);
    }

    inline void HandleGossipHelloOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        DoGossipHello(session, guid);
    }

    inline void HandleCancelMountAuraOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        //By leewheel 2026-07-09: 修复函数名，CancelMountAura应为DoCancelMountAura
        DoCancelMountAura(session);
    }

    inline void HandleAcceptTradeOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        //By leewheel 2026-07-09: 修复函数名，AcceptTrade应为DoAcceptTrade
        DoAcceptTrade(session);
        //End By leewheel
    }

    inline void HandleMountSpecialAnimOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        MountSpecialAnim(session);
    }

    inline void HandleGameObjectUseOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        GameObjectUse(session, guid);
    }

    inline void HandleGameobjectReportUse(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        GameObjectReportUse(session, guid);
    }

    inline void HandleCancelAuraOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid casterGUID;
        int32 spellID;
        data >> casterGUID >> spellID;
        //By leewheel 2026-07-09: 修复函数名，CancelAura应为DoCancelAura
        DoCancelAura(session, casterGUID, spellID);
    }

    inline void HandleOpenItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 slot, packSlot;
        data >> packSlot >> slot;
        //By leewheel 2026-07-09: 修复函数名，OpenItem应为DoOpenItem
        DoOpenItem(session, slot, packSlot);
    }

    inline void HandleUseItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 packSlot, slot;
        data >> packSlot >> slot;
        //By leewheel 2026-07-09: 修复函数名，UseItem应为DoUseItem
        //By leewheel 2026-08-15: DoUseItem新增spellId参数(填Cast.SpellID否则TC handler直接return)，
        //此扁平包路径无法得知法术ID——传0由handler内部解析物品默认法术
        DoUseItem(session, packSlot, slot, ObjectGuid::Empty, 0);
        //End By leewheel
    }

    inline void HandleLootOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        LootUnit(session, guid);
    }

    inline void HandleLootReleaseOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        LootRelease(session, guid);
    }

    inline void HandleLootMoneyOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        LootMoney(session);
    }

    inline void HandleAutostoreLootItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 lootSlot;
        data >> lootSlot;
        AutostoreLootItem(session, lootSlot);
    }

    inline void HandleAttackSwingOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid victim;
        data >> victim;
        AttackSwing(session, victim);
    }

    inline void HandleAttackStopOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        AttackStop(session);
    }

    inline void HandleInitiateTradeOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        //By leewheel 2026-07-09: 修复函数名，InitiateTrade应为DoInitiateTrade
        DoInitiateTrade(session, guid);
    }

    inline void HandleSetTradeGoldOpcode(WorldSession* session, WorldPacket& data)
    {
        uint64 coinage;
        data >> coinage;
        //By leewheel 2026-07-09: 修复函数名，SetTradeGold应为DoSetTradeGold
        DoSetTradeGold(session, coinage);
    }

    inline void HandleSetTradeItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 tradeSlot, packSlot, itemSlot;
        data >> tradeSlot >> packSlot >> itemSlot;
        //By leewheel 2026-07-09: 修复函数名，SetTradeItem应为DoSetTradeItem
        DoSetTradeItem(session, tradeSlot, packSlot, itemSlot);
    }

    inline void HandleClearTradeItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 tradeSlot;
        data >> tradeSlot;
        //By leewheel 2026-07-09: 修复函数名，ClearTradeItem应为DoClearTradeItem
        DoClearTradeItem(session, tradeSlot);
    }

    inline void HandleBeginTradeOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        BeginTrade(session);
    }

    inline void HandleCancelTradeOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        CancelTrade(session);
    }

    inline void HandleUnacceptTradeOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        UnacceptTrade(session);
    }

    inline void HandleAreaTriggerOpcode(WorldSession* session, WorldPacket& data)
    {
        int32 areaTriggerID;
        data >> areaTriggerID;
        AreaTrigger(session, areaTriggerID);
    }

    inline void HandleQuestgiverHelloOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        QuestGiverHello(session, guid);
    }

    inline void HandleQuestgiverAcceptQuestOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        int32 questID;
        data >> guid >> questID;
        QuestGiverAcceptQuest(session, guid, questID);
    }

    inline void HandleQuestgiverChooseRewardOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        int32 questID, choice;
        data >> guid >> questID >> choice;
        QuestGiverChooseReward(session, guid, questID, choice);
    }

    inline void HandleQuestgiverCompleteQuest(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        int32 questID;
        data >> guid >> questID;
        QuestGiverCompleteQuest(session, guid, questID);
    }

    inline void HandleQuestgiverQueryQuestOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        int32 questID;
        data >> guid >> questID;
        QuestGiverQueryQuest(session, guid, questID);
    }

    inline void HandleQuestLogRemoveQuest(WorldSession* session, WorldPacket& data)
    {
        uint8 entry;
        data >> entry;
        QuestLogRemoveQuest(session, entry);
    }

    //By leewheel 20260709: 修复RequestVehicleExit未声明标识符，调用本文件已定义的DoRequestVehicleExit
    inline void HandleRequestVehicleExit(WorldSession* session, WorldPacket& /*data*/)
    {
        DoRequestVehicleExit(session);
    }
    //End By leewheel

    inline void HandleBattlefieldLeaveOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        BattlefieldLeave(session);
    }

    inline void HandleRequestBattlefieldStatusOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        RequestBattlefieldStatus(session);
    }

    inline void HandleAutoStoreBagItemOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 srcBag, srcSlot, dstBag;
        data >> srcBag >> srcSlot >> dstBag;
        AutoStoreBagItem(session, srcBag, srcSlot, dstBag);
    }

    inline void HandleAutoEquipItemSlotOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid itemGuid;
        uint8 dstSlot;
        data >> itemGuid >> dstSlot;
        AutoEquipItemSlot(session, itemGuid, dstSlot);
    }

    inline void HandleLfgLeaveOpcode(WorldSession* session, WorldPacket& /*data*/)
    {
        LfgLeave(session);
    }

    inline void HandleLfgSetRolesOpcode(WorldSession* session, WorldPacket& data)
    {
        uint8 roles;
        data >> roles;
        LfgSetRoles(session, roles);
    }

    inline void HandleLfgTeleportOpcode(WorldSession* session, WorldPacket& data)
    {
        //By leewheel 2026-09-06: 移植到TrinityCore-Cata，ByteBuffer移除了operator>>(bool&)，
        //按uint8读取后再转换
        uint8 teleportOutFlag = 0;
        data >> teleportOutFlag;
        bool teleportOut = teleportOutFlag != 0;
        LfgTeleport(session, teleportOut);
    }

    //By leewheel 20260709: 修复SpiritHealerActivate未声明标识符，调用本文件已定义的DoSpiritHealerActivate
    inline void HandleSpiritHealerActivate(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid healer;
        data >> healer;
        DoSpiritHealerActivate(session, healer);
    }
    //End By leewheel

    inline void HandleMailTakeMoney(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid mailbox;
        uint64 mailID;
        data >> mailbox >> mailID;
        MailTakeMoney(session, mailbox, mailID, 0);
    }

    inline void HandleMailTakeItem(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid mailbox;
        uint64 mailID;
        data >> mailbox >> mailID;
        MailTakeItem(session, mailbox, mailID, 0);
    }

    inline void HandleMailDelete(WorldSession* session, WorldPacket& data)
    {
        uint64 mailID;
        data >> mailID;
        MailDelete(session, mailID);
    }

    inline void HandlePetitionShowList(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid guid;
        data >> guid;
        PetitionShowList(session, guid);
    }

    inline void HandlePetitionShowSignatures(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid item;
        data >> item;
        PetitionShowSignatures(session, item);
    }

    inline void HandleGossipSelectOptionOpcode(WorldSession* session, WorldPacket& data)
    {
        ObjectGuid gossipUnit;
        int32 gossipOptionID, gossipID;
        data >> gossipUnit >> gossipOptionID >> gossipID;
        DoGossipSelectOption(session, gossipUnit, gossipOptionID, gossipID);
    }

    // ========== 额外AC兼容处理器 ==========
    // By leewheel 2026-07-09: 添加TC中存在但AC命名不同的处理器兼容函数

    // RepopRequest - 对应 HandleRepopRequestOpcode
    inline void RepopRequest(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Misc::RepopRequest packet(std::move(data));
        packet.Read();
        //By leewheel 2026-07-09: TC方法名为HandleRepopRequest而非HandleRepopRequestOpcode
        session->HandleRepopRequest(packet);
        //End By leewheel
    }

    // SelfRes - 对应 HandleSelfResOpcode
    inline void SelfRes(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Spells::SelfRes packet(std::move(data));
        packet.Read();
        session->HandleSelfResOpcode(packet);
    }

    // ReclaimCorpse - 对应 HandleReclaimCorpseOpcode
    inline void ReclaimCorpse(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Misc::ReclaimCorpse packet(std::move(data));
        packet.Read();
        //By leewheel 2026-07-09: TC方法名为HandleReclaimCorpse而非HandleReclaimCorpseOpcode
        session->HandleReclaimCorpse(packet);
        //End By leewheel
    }

    // ResetInstances - 对应 HandleResetInstancesOpcode
    inline void ResetInstances(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Instance::ResetInstances packet(std::move(data));
        packet.Read();
        session->HandleResetInstancesOpcode(packet);
    }

    // PartyInviteResponse - 对应 HandlePartyInviteResponseOpcode
    inline void PartyInviteResponse(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Party::PartyInviteResponse packet(std::move(data));
        packet.Read();
        session->HandlePartyInviteResponseOpcode(packet);
    }

    // QueryNextMailTime - 对应 HandleQueryNextMailTime
    inline void QueryNextMailTime(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Mail::MailQueryNextMailTime packet(std::move(data));
        packet.Read();
        session->HandleQueryNextMailTime(packet);
    }

    // ReadyCheckResponse - 对应 HandleReadyCheckResponseOpcode
    inline void ReadyCheckResponse(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Party::ReadyCheckResponseClient packet(std::move(data));
        packet.Read();
        session->HandleReadyCheckResponseOpcode(packet);
    }

    // SellItem - 对应 HandleSellItemOpcode
    inline void SellItem(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Item::SellItem packet(std::move(data));
        packet.Read();
        session->HandleSellItemOpcode(packet);
    }

    // PushQuestToParty - 对应 HandlePushQuestToParty
    inline void PushQuestToParty(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Quest::PushQuestToParty packet(std::move(data));
        packet.Read();
        session->HandlePushQuestToParty(packet);
    }

    // QuestConfirmAccept - 对应 HandleQuestConfirmAccept
    inline void QuestConfirmAccept(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Quest::QuestConfirmAccept packet(std::move(data));
        packet.Read();
        session->HandleQuestConfirmAccept(packet);
    }

    // BattleFieldPort - 对应 HandleBattleFieldPortOpcode
    inline void BattleFieldPort(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Battleground::BattlefieldPort packet(std::move(data));
        packet.Read();
        session->HandleBattleFieldPortOpcode(packet);
    }

    // DeclinePetition - 对应 HandleDeclinePetition
    inline void DeclinePetition(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Petition::DeclinePetition packet(std::move(data));
        packet.Read();
        session->HandleDeclinePetition(packet);
    }

    // GuildAcceptInvite - 对应 HandleGuildAcceptInvite (AC: HandleGuildAcceptOpcode)
    inline void GuildAcceptInvite(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Guild::AcceptGuildInvite packet(std::move(data));
        packet.Read();
        session->HandleGuildAcceptInvite(packet);
    }

    // GuildDeclineInvitation - 对应 HandleGuildDeclineInvitation (AC: HandleGuildDeclineOpcode)
    inline void GuildDeclineInvitation(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Guild::GuildDeclineInvitation packet(std::move(data));
        packet.Read();
        session->HandleGuildDeclineInvitation(packet);
    }

    // GuildLeaveOpcode - 对应 HandleGuildLeave (AC: HandleGuildLeaveOpcode)
    inline void GuildLeaveOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Guild::GuildLeave packet(std::move(data));
        packet.Read();
        session->HandleGuildLeave(packet);
    }

    // PetitionBuyOpcode - 对应 HandlePetitionBuy (AC: HandlePetitionBuyOpcode)
    inline void PetitionBuyOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Petition::PetitionBuy packet(std::move(data));
        packet.Read();
        session->HandlePetitionBuy(packet);
    }

    // OfferPetitionOpcode - 对应 HandleOfferPetition (AC: HandleOfferPetitionOpcode)
    inline void OfferPetitionOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Petition::OfferPetition packet(std::move(data));
        packet.Read();
        session->HandleOfferPetition(packet);
    }

    // TurnInPetitionOpcode - 对应 HandleTurnInPetition (AC: HandleTurnInPetitionOpcode)
    inline void TurnInPetitionOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Petition::TurnInPetition packet(std::move(data));
        packet.Read();
        session->HandleTurnInPetition(packet);
    }

    // GroupInviteOpcode - 对应 HandlePartyInviteOpcode (AC: HandleGroupInviteOpcode)
    inline void GroupInviteOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Party::PartyInviteClient packet(std::move(data));
        packet.Read();
        session->HandlePartyInviteOpcode(packet);
    }

    // PetitionSignOpcode - 对应 HandleSignPetition (AC: HandlePetitionSignOpcode)
    inline void PetitionSignOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Petition::SignPetition packet(std::move(data));
        packet.Read();
        session->HandleSignPetition(packet);
    }

    // SocketGemsOpcode - 对应 HandleSocketGems (AC: HandleSocketOpcode)
    inline void SocketGemsOpcode(WorldSession* session, WorldPacket& data)
    {
        WorldPackets::Item::SocketGems packet(std::move(data));
        packet.Read();
        session->HandleSocketGems(packet);
    }

    // End By leewheel

} // namespace WPPCompat

#endif // PLAYERBOTS_WPP_COMPAT_H
