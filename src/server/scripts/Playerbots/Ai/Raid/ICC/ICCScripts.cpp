/*
 * 冰冠堡垒团本服务端钩子脚本
 * 提供 Putricide/Rotface/LK 机制检测、实例状态管理
 *
 * 作者: leewheel
 */

#include "ICCScripts.h"
//By leewheel 2026-07-27: AllScriptCompat.h提供AC的AllSpellScript/AllCreatureScript兼容基类
#include "AllScriptCompat.h"
//End By leewheel
//By leewheel 2026-08-15: SpellScript基类定义(施法监听改造为TC原生SpellScript)
#include "SpellScript.h"
//End By leewheel
//By leewheel 2026-07-27: InstanceMapScript的InstanceMap*完整定义在Map.h(TC-fork无InstanceMap.h)
#include "Map.h"
//End By leewheel
#include "Creature.h"
#include "Player.h"
#include "ICCTriggers.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Timer.h"
#include <algorithm>
#include <mutex>

namespace
{
    std::unordered_map<uint32, IcecrownHelpers::IccInstanceState> g_state;
    std::mutex g_stateMutex;
}

namespace IcecrownHelpers
{
    IccInstanceState& IccState(uint32 instanceId)
    {
        std::lock_guard lock(g_stateMutex);
        return g_state[instanceId];
    }

    void IccResetInstance(uint32 instanceId)
    {
        std::lock_guard lock(g_stateMutex);
        g_state.erase(instanceId);
    }

    std::vector<Position> ActiveGooPositions(uint32 instanceId, uint32 lifetimeMs)
    {
        std::vector<Position> out;
        IccInstanceState& st = IccState(instanceId);

        uint32 const now = getMSTime();
        for (auto const& impact : st.malleableGoo)
            if (getMSTimeDiff(impact.castTime, now) <= lifetimeMs)
                out.push_back(impact.position);

        return out;
    }
}

//By leewheel 2026-08-15: 修复——原AllSpellScript空壳钩子从未被TC核心调用(无OnSpellCast/OnSpellPrepare调用点)，
//导致普崔塞德软泥/腐面毒气/巫妖王污染的施法监听全部死代码，机器人在ICC战斗中对这些机制"无感知"。
//改造为TC原生SpellScript，通过spell_script_names表绑定到具体spellId，OnCast(施放)/OnPrecast(读条开始)真实触发
class spell_icc_malleable_goo : public SpellScript
{
public:
    spell_icc_malleable_goo() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        //By leewheel 2026-08-17 修复: 移除不存在的 10H=74280/25H=74281(3.4.3 DB2 无此 ID),
        //英雄模式施放复用 10N/25N ID, 仅校验 DB2 真实存在的法术
        return ValidateSpellInfo({
            SPELL_MALLEABLE_GOO_10N, SPELL_MALLEABLE_GOO_25N,
            SPELL_MALLEABLE_GOO_BALCONY
        });
        //End By leewheel
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Malleable Goo is cast triggered, so m_UniqueTargetInfo is not yet
        // populated at this point; read the explicit unit target directly.
        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsPlayer())
            return;

        uint32 now = getMSTime();

        IcecrownHelpers::MalleableGooImpact impact;
        impact.position = target->GetPosition();
        impact.castTime = now;

        auto& impacts = IcecrownHelpers::IccState(caster->GetMap()->GetInstanceId()).malleableGoo;
        impacts.push_back(impact);

        // Evict stale entries to keep the list bounded. Retention covers the
        // longest consumer window (Festergut avoid: 8s) + slack.
        impacts.erase(
            std::remove_if(impacts.begin(), impacts.end(),
                           [now](IcecrownHelpers::MalleableGooImpact const& i)
                           { return getMSTimeDiff(i.castTime, now) > 9000; }),
            impacts.end());
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_icc_malleable_goo::HandleOnCast);
    }
};

class spell_icc_vile_gas : public SpellScript
{
public:
    spell_icc_vile_gas() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VILE_GAS_H });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Professor Putricide casts vile gas from the balcony during Rotface
        // heroic. Filtering on caster entry keeps this hook scoped to the
        // Rotface encounter only (Festergut also uses 'vile gas' as the gas
        // spore aura name but a different spell ID).
        if (caster->GetEntry() != NPC_PROFESSOR_PUTRICIDE)
            return;

        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsPlayer())
            return;

        IcecrownHelpers::VileGasVictim& entry =
            IcecrownHelpers::IccState(caster->GetMap()->GetInstanceId()).rotfaceVileGas;
        entry.victimGuid = target->GetGUID();
        entry.castTime = getMSTime();
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_icc_vile_gas::HandleOnCast);
    }
};

class spell_icc_defile : public SpellScript
{
public:
    spell_icc_defile() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ DEFILE_CAST_ID });
    }

    // OnPrecast fires at cast START (Spell::prepare). OnCast fires at cast
    // END, which for Defile (2s cast time) is too late - the puddle
    // is already spawning and bots have no time to move out.
    void OnPrecast() override
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsPlayer())
            return;

        IcecrownHelpers::DefileCastInfo& entry =
            IcecrownHelpers::IccState(caster->GetMap()->GetInstanceId()).defileCast;
        entry.targetGuid = target->GetGUID();
        entry.castTime = getMSTime();
    }

    // Register是纯虚必须实现(OnPrecast为基类虚函数，无hook列表注册项)
    void Register() override { }
};
//End By leewheel

class IccBossStateResetScript : public AllCreatureScript
{
public:
    IccBossStateResetScript() : AllCreatureScript("IccBossStateResetScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetMapId() != ICC_MAP_ID || !creature->IsDungeonBoss())
            return;

        uint32 const instanceId = creature->GetInstanceId();
        uint32 const now = getMSTime();
        IcecrownHelpers::IccInstanceState& st = IcecrownHelpers::IccState(instanceId);

        if (creature->IsInCombat())
        {
            st.lastBossCombatMs = now;
            return;
        }

        if (st.lastBossCombatMs != 0 && getMSTimeDiff(st.lastBossCombatMs, now) > ICC_RESET_GRACE_MS)
            IcecrownHelpers::IccResetInstance(instanceId);
    }
};

class IccMapCleanupScript : public InstanceMapScript
{
public:
    //By leewheel 2026-07-27: AC的AllMapScript无对应物，TC的InstanceMapScript构造接mapId=631精准绑定ICC地图
    IccMapCleanupScript() : InstanceMapScript("IccMapCleanupScript", ICC_MAP_ID) { }
    //End By leewheel

    //By leewheel 2026-07-27: TC的OnDestroy回调签名是OnDestroy(InstanceMap* map)而非OnDestroyMap(Map*)
    void OnDestroy(InstanceMap* map) override
    //End By leewheel
    {
        IcecrownHelpers::IccResetInstance(map->GetInstanceId());
    }
};

void AddSC_IcecrownBotScripts()
{
    //By leewheel 2026-08-15: AllSpellScript改SpellScript后改用RegisterSpellScript注册(经spell_script_names表绑定)
    RegisterSpellScript(spell_icc_malleable_goo);
    RegisterSpellScript(spell_icc_vile_gas);
    RegisterSpellScript(spell_icc_defile);
    //End By leewheel
    new IccBossStateResetScript();
    new IccMapCleanupScript();
}
