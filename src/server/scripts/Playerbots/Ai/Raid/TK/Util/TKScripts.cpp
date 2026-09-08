/* 风暴要塞 机器人策略 */
//By leewheel 2026-08-15: 修复——原AllSpellScript空壳钩子从未被TC核心调用(无OnSpellCast调用点)，
//空灵机甲奥术宝珠的施法监听全部死代码，机器人不会躲宝珠。改造为TC原生SpellScript(经spell_script_names表绑定)
#include "SpellScript.h"
//End By leewheel
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "Timer.h"
#include "TKHelpers.h"

using namespace TkHelpers;

//By leewheel 2026-08-15
class spell_tk_arcane_orb : public SpellScript
{
public:
    spell_tk_arcane_orb() = default;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Id(TkSpells::SPELL_ARCANE_ORB) });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsPlayer())
            return;

        //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
        auto& orbs = voidReaverArcaneOrbs[caster->GetInstanceId()];
        //End By leewheel
        uint32 const now = getMSTime();

        ArcaneOrbData orbData;
        orbData.destination = target->GetPosition();
        orbData.castTime = now;

        orbs.push_back(orbData);

        orbs.erase(std::remove_if(orbs.begin(), orbs.end(),
            [now](ArcaneOrbData const& orb) {
                return getMSTimeDiff(orb.castTime, now) > ARCANE_ORB_DURATION_MS;
            }), orbs.end());
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_tk_arcane_orb::HandleOnCast);
    }
};
//End By leewheel

void AddSC_TempestKeepBotScripts()
{
    //By leewheel 2026-08-15: AllSpellScript改SpellScript后改用RegisterSpellScript注册
    RegisterSpellScript(spell_tk_arcane_orb);
    //End By leewheel
}
