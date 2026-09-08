/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AllCreatureScript.h"
#include "EncounterHelpers.h"
#include "HyjalHelpers.h"
#include "Player.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "Spell.h"
//By leewheel 2026-08-23: 两个法术监听器改用 TC SpellScript(见下), 需要完整 SpellScript 定义
#include "SpellScript.h"
//End By leewheel
#include "Timer.h"

using namespace HyjalHelpers;
using namespace EncounterHelpers;

namespace
{
//By leewheel 2026-08-23: 法术监听器改用 TC SpellScript(GetExplTargetUnit)后已无 Caller, 保留 Doxygen 说明供参考
// 原 the-lab 通过 spell->m_targets.GetUnitTarget() 取施法目标单位
//End By leewheel

bool ShouldInterruptForArchimondeAirBurst(Player* bot, Unit* caster, Player* target)
{
    if (!target)
        return false;

    Unit* activeTank = caster->GetVictim();
    if (!activeTank || activeTank == bot)
        return false;

    if (target != activeTank && target != bot)
        return false;

    float const distanceToActiveTank = bot->GetExactDist2d(activeTank);
    return distanceToActiveTank < AIR_BURST_SAFE_DISTANCE;
}

}

// Doomfire's mechanic is pretty interesting. A Doomfire Spirit trigger NPC teleports up to 8y
// every 1.6s, and the Doomfire trigger NPC follows it after each teleport and drops the hazards.
// The hook reads the Doomfire NPC since it accompanies the visual fire trail. Real players cannot
// see the spirit so keying off of that would be a cheat.
class ArchimondeDoomfireTrailCreatureScript : public AllCreatureScript
{
public:
    ArchimondeDoomfireTrailCreatureScript()
        : AllCreatureScript("ArchimondeDoomfireTrailCreatureScript") {}

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature->GetEntry() != Id(HyjalNpcs::NPC_DOOMFIRE))
            return;

        Map::PlayerList const& players = creature->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            Player* player = it->GetSource();
            if (!player || !player->IsAlive())
                continue;

            PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
            if (!botAI || !botAI->HasStrategy("hyjal", BOT_STATE_COMBAT) ||
                creature->GetExactDist2d(player) > DOOMFIRE_DANGER_RADIUS)
            {
                continue;
            }

            botAI->RequestSpellInterrupt();
        }
    }
};

//By leewheel 2026-08-23: the-lab 原用 AC AllSpellScript(本项目兼容层为空壳, 核心无分发,
    // 引发 DBErrors: spell_hyjal_air_burst/inferno referenced but does not exist)——
    // 改用 TC 原生 SpellScript(类名即 DB spell_script_names 的 ScriptName)。
//By leewheel 2026-09-04 修复: 监听时机由 OnCast 迁移到 OnPrecast——AC OnSpellPrepare 在读条开始
    // 时触发, 而 TC OnCast 经查证在 Spell::_cast() 读条计时归零后才调用(效果落地瞬间);
    // 原写法导致气爆的2000ms反应窗从爆炸落地后才开始(预防性散开/打断全部失效, 每次白吃击飞)、
    // Inferno 打断在地狱火落地瞬间才发出(目标bot来不及反应)。
    // OnPrecast 由 Spell::prepare 末尾 CallScriptOnPrecastHandler 调用(读条开始前),
    // 此时 InitExplicitTargets 已执行, GetCaster/GetExplTargetUnit 可用, 与 AC 语义完全对齐。
class spell_hyjal_air_burst : public SpellScript
{
    PrepareSpellScript(spell_hyjal_air_burst);

    void OnPrecast() override
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* target = GetExplTargetUnit() ? GetExplTargetUnit()->ToPlayer() : nullptr;
        if (!target)
            return;

        // 读条开始即记录, 使"boss casting air burst"触发器的2000ms窗口覆盖整段2s读条,
        // bot 拥有完整跑位时间(上游设计意图)
        archimondeAirBurstTargets[caster->GetInstanceId()] =
            AirBurstData{ target->GetGUID(), getMSTime() };

        Map::PlayerList const& players = caster->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            Player* player = it->GetSource();
            if (!player || !player->IsAlive())
                continue;

            PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
            if (!botAI || !botAI->HasStrategy("hyjal", BOT_STATE_COMBAT) ||
                !ShouldInterruptForArchimondeAirBurst(player, caster, target))
            {
                continue;
            }

            botAI->RequestSpellInterrupt();
        }
    }

    void Register() override
    {
    }
};

class spell_hyjal_inferno : public SpellScript
{
    PrepareSpellScript(spell_hyjal_inferno);

    void OnPrecast() override
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* target = GetExplTargetUnit() ? GetExplTargetUnit()->ToPlayer() : nullptr;
        if (!target)
            return;

        // 读条开始即打断目标bot读条并给出3.5s逃生窗(上游OnSpellPrepare语义)
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(target);
        if (!botAI || !botAI->HasStrategy("hyjal", BOT_STATE_COMBAT))
            return;

        botAI->RequestSpellInterrupt();
    }

    void Register() override
    {
    }
};
//End By leewheel

void AddSC_HyjalSummitBotScripts()
{
    new ArchimondeDoomfireTrailCreatureScript();
    //By leewheel 2026-08-23: 两个法术监听器改为 TC SpellScript 注册(类名=ScriptName)
    RegisterSpellScript(spell_hyjal_air_burst);
    RegisterSpellScript(spell_hyjal_inferno);
    //End By leewheel
}
