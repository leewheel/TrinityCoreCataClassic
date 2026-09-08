/* 太阳之井高地 机器人策略 */
//By leewheel 2026-08-15: 修复——原AllSpellScript空壳钩子从未被TC核心调用(无OnSpellCast/OnSpellPrepare调用点)，
//SWP各BOSS(卡雷苟斯/菲米丝/双子/基尔加丹)的施法监听全部死代码，机器人对光谱冲击/包裹/烈焰/千魂无感知。
//施法监听改造为TC原生SpellScript(经spell_script_names表绑定)；SunwellBossUpdate/Armageddon两个
//AllCreatureScript类(生物周期更新)在后续适配中单独处理
#include "SpellScript.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SWPSharedConstants.h"
#include "SWPEncounter_Brut.h"
#include "SWPEncounter_Felmyst.h"
#include "SWPEncounter_Kalec.h"
#include "SWPEncounter_KJ.h"
#include "SWPEncounter_Muru.h"
#include "SWPEncounter_Twins.h"
#include <list>
#include <vector>

using namespace SwpHelpers;

static PlayerbotAI* FindFirstSunwellCombatBotInGroup(Player* referencePlayer)
{
    if (!referencePlayer)
        return nullptr;

    if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(referencePlayer);
        botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
    {
        return botAI;
    }

    Group* group = referencePlayer->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == referencePlayer || member->GetMapId() != SWP_MAP_ID)
            continue;

        if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(member);
            botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
        {
            return botAI;
        }
    }

    return nullptr;
}

static void RequestInterruptForBotsNeedingFelmystFogMovement(
    Unit* contextUnit, Player* groupReference)
{
    if (!contextUnit)
        return;

    Group* group = nullptr;
    if (groupReference)
        group = groupReference->GetGroup();

    Map::PlayerList const& players = contextUnit->GetMap()->GetPlayers();

    for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        Player* player = it->GetSource();
        if (!player || !player->IsAlive() || (group && player->GetGroup() != group))
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (!botAI || !botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
            continue;

        Unit* felmyst = PAI_VALUE2(Unit*, "find target", "25038");
        if (!felmyst || !felmyst->IsFlying())
            continue;

        FogOfCorruptionState fogState;
        if (!TryGetActiveFogOfCorruptionState(player, felmyst, fogState))
            continue;

        Position ignored;
        //By leewheel 2026-09-04: 上游70808114——TryGetFelmystFogSafeDestination 改名 TryGetFelmystFogCrossingDestination
        if (!TryGetFelmystFogCrossingDestination(player, fogState.lane, ignored))
            continue;

        botAI->RequestSpellInterrupt();
    }
}

void RequestInterruptForBotsWithFelmystEncapsulate(Creature* felmyst)
{
    if (!felmyst || felmyst->IsFlying())
        return;

    Map::PlayerList const& players = felmyst->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        Player* player = it->GetSource();
        if (!player || !player->IsAlive())
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (!botAI || !botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
            continue;

        if (!player->GetCurrentSpell(CURRENT_GENERIC_SPELL) &&
            !player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            continue;
        }

        Player* encapsulateTarget = GetFelmystEncapsulateTarget(player);
        if (!encapsulateTarget)
            continue;

        constexpr float safeDistance = 20.0f;
        if (player != encapsulateTarget &&
            player->GetExactDist2d(encapsulateTarget) > safeDistance)
        {
            continue;
        }

        botAI->RequestSpellInterrupt();
    }
}

static void RequestInterruptForEredarTwinsAlythessTargets(Creature* alythess)
{
    if (!alythess)
        return;

    Map::PlayerList const& players = alythess->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        Player* player = it->GetSource();
        if (!player || !player->IsAlive())
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (!botAI || !botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
            continue;

        if (!player->GetCurrentSpell(CURRENT_GENERIC_SPELL) &&
            !player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            continue;
        }

        if (GetEredarTwinsConflagrationTarget(player) == player ||
            (GetEredarTwinsBlazeTarget(player) == player && PlayerbotAI::IsRanged(player)))
        {
            botAI->RequestSpellInterrupt();
        }
    }
}

//By leewheel 2026-08-15: SpellScript内用GetExplTargetUnit()获取显式目标(DoCastRandomTarget提供)
static Player* GetSpellTargetPlayer(SpellScript* script)
{
    if (Unit* unitTarget = script->GetExplTargetUnit())
        return unitTarget->ToPlayer();
    return nullptr;
}
//End By leewheel

//By leewheel 2026-08-15
class spell_swp_kalecgos : public SpellScript
{
public:
    spell_swp_kalecgos() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({
            Id(SwpSpells::SPELL_SPECTRAL_BLAST_PORTAL),
            Id(SwpSpells::SPELL_TELEPORT_SPECTRAL)
        });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player)
            return;

        switch (GetSpellInfo()->Id)
        {
            case Id(SwpSpells::SPELL_SPECTRAL_BLAST_PORTAL):
                if (PlayerbotAI* botAI = FindFirstSunwellCombatBotInGroup(player))
                    RecordSpectralBlastTarget(player, botAI);
                break;

            case Id(SwpSpells::SPELL_TELEPORT_SPECTRAL):
                if (FindFirstSunwellCombatBotInGroup(player))
                    RecordSpectralRealmEnter(player);
                break;

            default:
                break;
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_swp_kalecgos::HandleOnCast);
    }
};

class spell_swp_felmyst : public SpellScript
{
public:
    spell_swp_felmyst() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({
            Id(SwpSpells::SPELL_ENCAPSULATE),
            Id(SwpSpells::SPELL_FOG_OF_CORRUPTION),
            Id(SwpSpells::SPELL_FELMYST_STRAFE_TOP),
            Id(SwpSpells::SPELL_FELMYST_STRAFE_MIDDLE),
            Id(SwpSpells::SPELL_FELMYST_STRAFE_BOTTOM),
            Id(SwpSpells::SPELL_SUMMON_DEMONIC_VAPOR)
        });
    }

    // OnPrecast在读条开始(Spell::prepare)触发，包裹(Encapsulate)是2s读条需提前预警
    void OnPrecast() override
    {
        if (GetSpellInfo()->Id != Id(SwpSpells::SPELL_ENCAPSULATE))
            return;

        Unit* caster = GetCaster();
        Player* target = GetSpellTargetPlayer(this);
        if (!caster || !target)
            return;

        if (!FindFirstSunwellCombatBotInGroup(target))
            return;

        RecordFelmystIncomingEncapsulateTarget(target);
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (GetSpellInfo()->Id == Id(SwpSpells::SPELL_FOG_OF_CORRUPTION) ||
            GetSpellInfo()->Id == Id(SwpSpells::SPELL_FELMYST_STRAFE_TOP) ||
            GetSpellInfo()->Id == Id(SwpSpells::SPELL_FELMYST_STRAFE_MIDDLE) ||
            GetSpellInfo()->Id == Id(SwpSpells::SPELL_FELMYST_STRAFE_BOTTOM))
        {
            Player* targetPlayer = GetSpellTargetPlayer(this);
            Player* groupReference = targetPlayer;
            if (Player* casterPlayer = caster->ToPlayer())
                groupReference = casterPlayer;

            if (groupReference)
            {
                if (!FindFirstSunwellCombatBotInGroup(groupReference))
                    return;
            }
            else
            {
                bool hasSunwellStrategy = false;
                Map::PlayerList const& players = caster->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator it = players.begin();
                     it != players.end(); ++it)
                {
                    Player* player = it->GetSource();
                    if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
                        botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
                    {
                        hasSunwellStrategy = true;
                        break;
                    }
                }

                if (!hasSunwellStrategy)
                    return;
            }

            RequestInterruptForBotsNeedingFelmystFogMovement(caster, groupReference);

            return;
        }

        Player* target = GetSpellTargetPlayer(this);
        if (!target)
            return;

        switch (GetSpellInfo()->Id)
        {
            case Id(SwpSpells::SPELL_SUMMON_DEMONIC_VAPOR):
                if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(target);
                    botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
                {
                    botAI->RequestSpellInterrupt();
                }
                break;

            default:
                break;
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_swp_felmyst::HandleOnCast);
    }
};

class spell_swp_eredar_twins : public SpellScript
{
public:
    spell_swp_eredar_twins() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({
            Id(SwpSpells::SPELL_CONFLAGRATION),
            Id(SwpSpells::SPELL_BLAZE)
        });
    }

    // 烈焰/引燃是读条，OnPrecast在读条开始触发
    void OnPrecast() override
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != Id(SwpNpcs::NPC_GRAND_WARLOCK_ALYTHESS))
            return;

        Player* target = GetSpellTargetPlayer(this);
        if (!target || !FindFirstSunwellCombatBotInGroup(target))
            return;

        if (GetSpellInfo()->Id == Id(SwpSpells::SPELL_CONFLAGRATION))
            RecordIncomingEredarTwinsConflagrationTarget(target);
        else if (GetSpellInfo()->Id == Id(SwpSpells::SPELL_BLAZE))
            RecordEredarTwinsBlazeTarget(target);
    }

    // Register是纯虚必须实现(OnPrecast为基类虚函数，无hook列表注册项)
    void Register() override { }
};

class spell_swp_kiljaeden : public SpellScript
{
public:
    spell_swp_kiljaeden() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Id(SwpSpells::SPELL_DARKNESS_OF_A_THOUSAND_SOULS) });
    }

    void HandleOnCast()
    {
        if (GetSpellInfo()->Id != Id(SwpSpells::SPELL_DARKNESS_OF_A_THOUSAND_SOULS))
            return;

        Unit* caster = GetCaster();
        if (!caster)
            return;

        Map::PlayerList const& players = caster->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            Player* player = it->GetSource();
            if (!player || !player->IsAlive() || HasKiljaedenDragonAura(player))
                continue;

            PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
            if (!botAI || !botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
                continue;

            if (PAI_VALUE2(Unit*, "find target", "25608") != caster)
                continue;

            botAI->RequestSpellInterrupt();
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_swp_kiljaeden::HandleOnCast);
    }
};
//End By leewheel

//By leewheel 2026-08-22: 引入 brighton-chi the-lab 新提交(9e8e7718..70b60954)——熵魔黑暗施法监听
// 上游为 AllSpellScript(OnSpellCast), 本项目已确认 TC 核心无该调用点(见文件头2026-08-15说明),
// 按既定模式改造为原生 SpellScript, 经 spell_script_names 表绑定 46269,
// 绑定 SQL 见 SqlUpdate/20260822_绑定熵魔黑暗施法监听.sql
class spell_swp_muru_void_zone : public SpellScript
{
public:
    spell_swp_muru_void_zone() = default;

    // 熵魔选中玩家施放, 导弹落点即其当前站位, 区域落地即永久——
    // 在这里打断读条可争取导弹飞行时间让 bot 离开
    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        Player* target = GetSpellTargetPlayer(this);
        if (!caster || !target)
            return;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(target);
        if (botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
            botAI->RequestSpellInterrupt();
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_swp_muru_void_zone::HandleOnCast);
    }
};
//End By leewheel

//By leewheel 2026-09-04: 上游——SunwellBossUpdateScript 改名 SunwellPlateauBossUpdateScript
class SunwellPlateauBossUpdateScript : public AllCreatureScript
{
public:
    SunwellPlateauBossUpdateScript() : AllCreatureScript("SunwellPlateauBossUpdateScript") {}
//End By leewheel

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        switch (creature->GetEntry())
        {
            case Id(SwpNpcs::NPC_FELMYST):
                RequestInterruptForBotsNeedingFelmystFogMovement(creature, nullptr);
                RequestInterruptForBotsWithFelmystEncapsulate(creature);
                break;

            case Id(SwpNpcs::NPC_GRAND_WARLOCK_ALYTHESS):
                RequestInterruptForEredarTwinsAlythessTargets(creature);
                break;

            default:
                break;
        }
    }
};

class KiljaedenArmageddonTargetTrackerScript : public AllCreatureScript
{
public:
    KiljaedenArmageddonTargetTrackerScript()
        : AllCreatureScript("KiljaedenArmageddonTargetTrackerScript") {}

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != Id(SwpNpcs::NPC_ARMAGEDDON_TARGET))
            return;

        if (kiljaedenTrackedArmageddonTargets.count(creature->GetGUID()))
            return;

        bool hasSunwellStrategy = false;
        std::vector<PlayerbotAI*> botsToInterrupt;
        Map::PlayerList const& players = creature->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            Player* player = it->GetSource();
            if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
                botAI && botAI->HasStrategy("sunwell", BOT_STATE_COMBAT))
            {
                hasSunwellStrategy = true;

                if (!player->IsAlive() || HasKiljaedenDragonAura(player))
                    continue;

                if (creature->GetExactDist2d(player) > KILJAEDEN_ARMAGEDDON_SAFE_DISTANCE)
                    continue;

                botsToInterrupt.push_back(botAI);
            }
        }

        if (!hasSunwellStrategy)
            return;

        kiljaedenTrackedArmageddonTargets.insert(creature->GetGUID());

        AddKiljaedenArmageddon(
            creature->GetInstanceId(), creature->GetPosition(),
            KILJAEDEN_ARMAGEDDON_HAZARD_DURATION_MS, KILJAEDEN_ARMAGEDDON_SAFE_DISTANCE);

        for (PlayerbotAI* botAI : botsToInterrupt)
            botAI->RequestSpellInterrupt();
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != Id(SwpNpcs::NPC_ARMAGEDDON_TARGET))
            return;

        kiljaedenTrackedArmageddonTargets.erase(creature->GetGUID());
    }
};

void AddSC_SunwellPlateauBotScripts()
{
    //By leewheel 2026-08-15: AllSpellScript改SpellScript后改用RegisterSpellScript注册(经spell_script_names表绑定)
    RegisterSpellScript(spell_swp_kalecgos);
    RegisterSpellScript(spell_swp_felmyst);
    RegisterSpellScript(spell_swp_eredar_twins);
    RegisterSpellScript(spell_swp_kiljaeden);
    //End By leewheel
    //By leewheel 2026-08-22: 新增熵魔黑暗(虚空区域)施法监听
    RegisterSpellScript(spell_swp_muru_void_zone);
    //End By leewheel
    new SunwellPlateauBossUpdateScript(); //By leewheel 2026-09-04: 上游——跟随改名
    new KiljaedenArmageddonTargetTrackerScript();
}
