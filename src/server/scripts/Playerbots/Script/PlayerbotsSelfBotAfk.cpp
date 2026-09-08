/* 机器人AI模块 - 自我机器人(selfbot)AFK防登出 */
//By leewheel 2026-09-05: 移植来源 AC mod-playerbots PlayerbotsSelfBotAfk.cpp 移植适配 TC 框架(上游 6704d553 feat(selfbot))
//业务对标: AC azerothcore-wotlk modules/mod-playerbots src/Script/PlayerbotsSelfBotAfk.cpp
//TC 差异说明: AC 的 ServerScript::CanPacketReceive 拦截数据包; 本地 TC343 原仅有观察型 OnPacketReceive,
//已在核心(ScriptMgr.h/ScriptMgr.cpp/WorldSession.cpp)按 AC 语义扩展出可拦截的 CanPacketReceive 再落地本文件。
//End By leewheel

#include "CharacterPackets.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// selfbot(挂机玩家角色)被AI接管后由服务器端驱动。若其主人AFK期间收到登出请求,
// 原逻辑会直接让该角色离开世界。此脚本拦截: selfbot且带AFK标志时拒绝登出,
// 向客户端回复与核心拒绝登出一致的提示(LogoutResult=2), 保持角色在线继续游戏。
class PlayerbotsSelfBotAfkServerScript : public ServerScript
{
public:
    PlayerbotsSelfBotAfkServerScript()
        : ServerScript("PlayerbotsSelfBotAfkServerScript") {}

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (packet.GetOpcode() != CMSG_LOGOUT_REQUEST)
            return true;

        Player* player = session ? session->GetPlayer() : nullptr;
        if (!player || !IsSelfBot(player) || !player->isAFK())
            return true;

        LOG_DEBUG("playerbots", "selfbot {} 处于AFK状态,拒绝其登出请求(保持在线)", player->GetName());

        WorldPackets::Character::LogoutResponse logoutResponse;
        logoutResponse.LogoutResult = 2;
        logoutResponse.Instant = false;
        session->SendPacket(logoutResponse.Write());

        return false;
    }
};

void AddPlayerbotsSelfBotAfkScripts()
{
    new PlayerbotsSelfBotAfkServerScript();
}
