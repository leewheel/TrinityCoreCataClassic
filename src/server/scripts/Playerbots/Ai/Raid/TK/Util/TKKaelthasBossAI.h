/*
 * 风暴要塞 - 凯尔萨斯（Kael'thas）Boss AI 声明
 * 集中维护四顾问阶段、凤凰/奥/剑等技能的协同行为
 */

#ifndef PLAYERBOTS_TKKAELTHASBOSSAI_H
#define PLAYERBOTS_TKKAELTHASBOSSAI_H

#include "ScriptedCreature.h"

enum KTYells
{
};

//By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——KTPhases 阶段枚举已迁至 TKHelpers.h,
//读取阶段统一走 TkHelpers::GetKaelthasPhase()
//End By leewheel

enum KTActions
{
};

struct boss_kaelthas : public BossAI
{
    boss_kaelthas(Creature* creature);

    void PrepareAdvisors();
    void SetRoomState(GOState state);
    void Reset() override;
    void AttackStart(Unit* who) override;
    void MoveInLineOfSight(Unit* who) override;
    void KilledUnit(Unit* victim) override;
    void JustSummoned(Creature* summon) override;
    //By leewheel 2026-07-26: TC的CreatureAI::SpellHit参数为WorldObject*而非Unit*
    void SpellHit(WorldObject* caster, SpellInfo const* spell) override;
    //End By leewheel
    void MovementInform(uint32 type, uint32 point) override;
    void ExecuteMiddleEvent();
    void IntroduceNewAdvisor(KTYells talkIntroduction, KTActions kaelAction);
    void PhaseEnchantedWeaponsExecute();
    void PhaseAllAdvisorsExecute();
    void PhaseKaelExecute();
    void UpdateAI(uint32 diff) override;
    //By leewheel 2026-07-26: TC的CreatureAI没有CheckEvadeIfOutOfCombatArea，移除override
    bool CheckEvadeIfOutOfCombatArea() const;
    //End By leewheel
    void JustDied(Unit* killer) override;

    uint32 GetPhase() const { return _phase; } // This is the only addition to the existing class

private:
    uint32 _phase;
    uint8 _advisorsAlive;
    bool _transitionSceneReached = false;
};

#endif
