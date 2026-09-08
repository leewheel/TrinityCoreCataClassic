/* 玛瑟里顿的巢穴 机器人策略 */
//By leewheel 2026-08-15: 修复——原AllSpellScript空壳钩子从未被TC核心调用(无OnSpellCast调用点)，
//玛瑟里顿碎石/地震的施法监听全部死代码，机器人不会躲碎石、震击计时失真。
//改造为TC原生SpellScript(经spell_script_names表绑定两个spell到同一脚本)
#include "SpellScript.h" //By leewheel 2026-08-15
#include "MagHelpers.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "Timer.h"
//By leewheel 2026-08-21: 移植 brighton-chi c8f273d2——碎石动态对象监听打断所需头文件
#include "DynamicObject.h"
#include "DynamicObjectScript.h"
//End By leewheel

using namespace MagHelpers;

//By leewheel 2026-08-15
class spell_magtheridon_bot : public SpellScript
{
public:
    spell_magtheridon_bot() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Id(MagSpells::SPELL_DEBRIS_SPAWN), static_cast<uint32>(MagSpells::SPELL_QUAKE) });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (GetSpellInfo()->Id == Id(MagSpells::SPELL_DEBRIS_SPAWN))
        {
            //By leewheel 2026-08-21: 移植 brighton-chi 49c3de92——碎石位置改由动态对象实时查询,
            //此钩子仅保留打断功能(不再维护位置记录容器)
            // Interrupt casts for bots that could be standing in incoming debris
            Map::PlayerList const& players = caster->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
            {
                Player* player = it->GetSource();
                if (!player || !player->IsAlive())
                    continue;

                PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
                if (!botAI || !botAI->HasStrategy("magtheridon", BOT_STATE_COMBAT))
                    continue;

                if (IsPositionInActiveDebris(
                        player, player->GetPositionX(), player->GetPositionY()))
                {
                    botAI->RequestSpellInterrupt();
                }
            }
            //End By leewheel
        }
        else if (GetSpellInfo()->Id == static_cast<uint32>(MagSpells::SPELL_QUAKE))
        {
            // To account for Blast Nova delay caused by Quake's DelayAll(6999ms)
            auto it = blastNovaTimer.find(caster->GetMap()->GetInstanceId());
            if (it != blastNovaTimer.end())
                it->second += 7 * IN_MILLISECONDS;
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_magtheridon_bot::HandleOnCast);
    }
};
//End By leewheel

//By leewheel 2026-08-21: 移植 brighton-chi c8f273d2——碎石动态对象存活期间持续打断施法
//(原SpellScript钩子仅在碎石生成瞬间打断一次, 动态对象监听可覆盖bot后续走进碎石的情况)
class MagtheridonDebrisDynamicObjectScript : public DynamicObjectScript
{
public:
    MagtheridonDebrisDynamicObjectScript() :
        DynamicObjectScript("MagtheridonDebrisDynamicObjectScript") {}

    void OnUpdate(DynamicObject* debris, uint32 /*diff*/) override
    {
        if (debris->GetSpellId() != Id(MagSpells::SPELL_DEBRIS_SPAWN))
            return;

        Map::PlayerList const& players = debris->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            Player* player = it->GetSource();
            if (!player || !player->IsAlive())
                continue;

            PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
            if (!botAI || !botAI->HasStrategy("magtheridon", BOT_STATE_COMBAT) ||
                debris->GetExactDist2d(player) > DEBRIS_HAZARD_RADIUS)
            {
                continue;
            }

            botAI->RequestSpellInterrupt();
        }
    }
};
//End By leewheel

void AddSC_MagtheridonBotScripts()
{
    //By leewheel 2026-08-15: AllSpellScript改SpellScript后改用RegisterSpellScript注册
    RegisterSpellScript(spell_magtheridon_bot);
    //End By leewheel
    //By leewheel 2026-08-21: 注册碎石动态对象打断脚本
    new MagtheridonDebrisDynamicObjectScript();
    //End By leewheel
}
