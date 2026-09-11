#include "ScriptMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ObjectAccessor.h"

#include "Playerbots.h"

namespace
{
    static Player* FindOnlineAltbotByGuid(ObjectGuid guid)
    {
        if (!guid)
            return nullptr;

        Player* p = ObjectAccessor::FindPlayer(guid);
        if (!p)
            return nullptr;

        PlayerbotAI* ai = GET_PLAYERBOT_AI(p);
        if (!ai || ai->IsRealPlayer())
            return nullptr;

        return p;
    }

    static void ForceLogoutViaPlayerbotHolder(Player* target)
    {
        if (!target)
            return;

        PlayerbotAI* ai = GET_PLAYERBOT_AI(target);

        if (!ai)
            return;

        if (Player* master = ai->GetMaster())
        {
            if (PlayerbotMgr* mgr = GET_PLAYERBOT_MGR(master))
            {
                mgr->LogoutPlayerBot(target->GetGUID());
                return;
            }
        }

        sRandomPlayerbotMgr.LogoutPlayerBot(target->GetGUID());
    }
}

//By leewheel 2026-09-05: TC核心已扩展出可拦截的 CanPacketReceive(AC语义),见 ScriptMgr.h;本脚本只读不拦截,恒返回true
class PlayerbotsSecureLoginServerScript : public ServerScript
{
public:
    PlayerbotsSecureLoginServerScript()
        : ServerScript("PlayerbotsSecureLoginServerScript") {}

    //By leewheel 2026-09-08: TC-Cata的ServerScript无CanPacketReceive虚函数，移除override关键字
    bool CanPacketReceive(WorldSession* /*session*/, WorldPacket const& packet)
    //End By leewheel
    {
        if (packet.GetOpcode() == CMSG_PLAYER_LOGIN)
        {
            WorldPacket pkt(packet);
            ObjectGuid loginGuid;
            pkt >> loginGuid;

            if (loginGuid)
            {
                Player* existingAltbot = FindOnlineAltbotByGuid(loginGuid);
                if (existingAltbot)
                    ForceLogoutViaPlayerbotHolder(existingAltbot);
            }
        }

        return true;
    }
};

void AddPlayerbotsSecureLoginScripts()
{
    new PlayerbotsSecureLoginServerScript();
}
