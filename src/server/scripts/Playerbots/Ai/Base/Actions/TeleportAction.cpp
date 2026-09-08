/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

//By leewheel 2026-07-10: 添加TC需要的AreaTriggerPackets头文件
#include "TeleportAction.h"

#include "AreaTriggerPackets.h"
#include "Event.h"
#include "LastMovementValue.h"
#include "AiObjectContext.h"
#include "PlayerbotAI.h"
#include "SpellMgr.h"
#include "Spell.h"
//End By leewheel

bool TeleportAction::Execute(Event /*event*/)
{
    /*
    // List of allowed portal entries (you can populate this dynamically)
    std::vector<uint32> allowedPortals = {
        187055, 195142, 195141, 201797, 202079, 194481, 195682, 191164, 176498, 182351,
        178404, 176497, 181146, 184605, 176499, 195140, 193948, 193427, 193052, 193206,
        192786, 184594, 183384, 182352, 184604, 189994, 193053, 193207, 193956, 195139,
        176296, 194011, 194012, 189993, 176500, 176501, 193955, 193425, 193772, 193604,
        191006, 191007, 191008, 191009, 191013, 191014, 191010, 190960, 191011, 191012,
        183317, 183321, 183322, 187056, 183323, 183324, 183325, 183326, 183327, 190203,
        190204, 190205, 190206, 193908, 181575, 181576, 181577, 181578, 202277, 202278
    };

    // Try teleporting using allowed portals
    GuidVector closeObjects = *context->GetValue<GuidVector>("nearest game objects no los");
    GameObject* closestPortal = nullptr;
    float closestDistance = 100.0f;

    for (ObjectGuid const& guid : closeObjects)
    {
        GameObject* go = botAI->GetGameObject(guid);
        if (!go)
            continue;

        // Check if the game object entry is in the allowed portals list
        if (std::find(allowedPortals.begin(), allowedPortals.end(), go->GetEntry()) != allowedPortals.end())
        {
            float tempDist = bot->GetDistance(go);

            if (tempDist < closestDistance)
            {
                closestDistance = tempDist;
                closestPortal = go;
            }
        }
    }

    // If a nearby portal is found, use it
    if (closestPortal && bot->IsWithinDistInMap(closestPortal, INTERACTION_DISTANCE))
    {
        std::ostringstream out;
        out << "Using portal: " << closestPortal->GetName();
        botAI->TellMasterNoFacing(out.str());

        WorldPacket data(CMSG_GAMEOBJ_USE);
        data << closestPortal->GetGUID();
        bot->GetSession()->HandleGameObjectUseOpcode(data);
        return true;
    }
    */

    // If no portal was found, fallback to spellcaster-type game objects
    GuidVector gos = *context->GetValue<GuidVector>("nearest game objects");
    for (ObjectGuid const guid : gos)
    {
        GameObject* go = botAI->GetGameObject(guid);
        if (!go)
            continue;

        GameObjectTemplate const* goInfo = go->GetGOInfo();
        if (goInfo->type != GAMEOBJECT_TYPE_SPELLCASTER && goInfo->type != GAMEOBJECT_TYPE_GOOBER)
            continue;

        //By leewheel 2026-07-10: TC中union成员名为spellCaster（大写C），字段名为spell而非spellId
        uint32 spellId = goInfo->spellCaster.spell;
        //End By leewheel
        SpellInfo const* spellInfo = SpellMgr::instance()->GetSpellInfo(spellId);
        if (!spellInfo || !spellInfo->HasEffect(SPELL_EFFECT_TELEPORT_UNITS))
            continue;

        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在使用 " << goInfo->name << " 进行传送";
        //End By leewheel
        botAI->TellMasterNoFacing(out.str());

        botAI->ChangeStrategy("-follow,+stay", BOT_STATE_NON_COMBAT);

        Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
        SpellCastTargets targets;
        targets.SetUnitTarget(bot);
                //By leewheel 2026-07-10: TC的prepare使用引用而非指针
        spell->prepare(targets, nullptr);
        //End By leewheel
        spell->cast(true);
        return true;
    }

    //By leewheel 2026-07-10: TC的AreaTrigger是ClientPacket，需要WorldPacket构造
    // 如果没有找到传送门，尝试使用上一个区域触发器
    LastMovement& movement = context->GetValue<LastMovement&>("last area trigger")->Get();
    if (movement.lastAreaTrigger)
    {
        WorldPacket data(CMSG_AREA_TRIGGER, 4 + 1 + 1);
        data << int32(movement.lastAreaTrigger);
        data.WriteBit(true);  // Entered
        data.WriteBit(false); // FromClient
        data.FlushBits();

        WorldPackets::AreaTrigger::AreaTrigger packet(std::move(data));
        packet.Read();
        bot->GetSession()->HandleAreaTriggerOpcode(packet);
        movement.lastAreaTrigger = 0;
        return true;
    }
    //End By leewheel

    // If no teleport option is found
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellError("找不到可以传送的传送门");
    //End By leewheel
    return false;
}
