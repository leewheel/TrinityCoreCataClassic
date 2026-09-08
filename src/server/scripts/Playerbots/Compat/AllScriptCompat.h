/*
 * AC AllSpellScript 和 AllCreatureScript 兼容层
 * TrinityCore 没有这两个脚本基类，这里创建兼容版本
 * 使移植的 playerbots 代码能编译通过
 * 注意：实际钩子调用需要通过其他机制实现
 */

#ifndef PLAYERBOTS_ALL_SCRIPT_COMPAT_H
#define PLAYERBOTS_ALL_SCRIPT_COMPAT_H

#include "Scripting/ScriptMgr.h"
#include "Entities/Unit/Unit.h"
#include "Spells/Spell.h"
#include "Spells/SpellInfo.h"
#include "Entities/Creature/Creature.h"
#include "Entities/Object/Object.h"

// 前向声明
class Aura;
class AuraEffect;
class Item;
class GameObject;
class WorldObject;
class Creature;
class Map;
class Player;
class SpellCastTargets;
struct CreatureTemplate;
struct TargetInfo;

// ========== AllSpellScript 兼容类 ==========
// AC 的 AllSpellScript 提供全局法术施放钩子
// TC 没有直接对应的类，这里创建空壳兼容版本
class AllSpellScript : public ScriptObject
{
protected:
    AllSpellScript(const char* name) : ScriptObject(name) { }

public:
    // AC 兼容虚函数 - 子类可重写这些方法
    virtual void OnCalcMaxDuration(Aura const* /*aura*/, int32& /*maxDuration*/) { }
    virtual void OnSpellCheckCast(Spell* /*spell*/, bool /*strict*/, SpellCastResult& /*res*/) { }
    virtual bool CanPrepare(Spell* /*spell*/, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) { return true; }
    virtual bool CanScalingEverything(Spell* /*spell*/) { return false; }
    virtual bool CanSelectSpecTalent(Spell* /*spell*/) { return true; }
    virtual void OnScaleAuraUnitAdd(Spell* /*spell*/, Unit* /*target*/, uint32 /*effectMask*/, bool /*checkIfValid*/, bool /*implicit*/, uint8 /*auraScaleMask*/, TargetInfo& /*targetInfo*/) { }
    virtual void OnRemoveAuraScaleTargets(Spell* /*spell*/, TargetInfo& /*targetInfo*/, uint8 /*auraScaleMask*/, bool& /*needErase*/) { }
    virtual void OnBeforeAuraRankForLevel(SpellInfo const* /*spellInfo*/, SpellInfo const* /*latestSpellInfo*/, uint8 /*level*/) { }
    virtual void OnDummyEffect(WorldObject* /*caster*/, uint32 /*spellID*/, SpellEffIndex /*effIndex*/, GameObject* /*gameObjTarget*/) { }
    virtual void OnDummyEffect(WorldObject* /*caster*/, uint32 /*spellID*/, SpellEffIndex /*effIndex*/, Creature* /*creatureTarget*/) { }
    virtual void OnDummyEffect(WorldObject* /*caster*/, uint32 /*spellID*/, SpellEffIndex /*effIndex*/, Item* /*itemTarget*/) { }
    virtual void OnSpellCastCancel(Spell* /*spell*/, Unit* /*caster*/, SpellInfo const* /*spellInfo*/, bool /*bySelf*/) { }
    virtual void OnSpellCast(Spell* /*spell*/, Unit* /*caster*/, SpellInfo const* /*spellInfo*/, bool /*skipCheck*/) { }
    virtual void OnSpellPrepare(Spell* /*spell*/, Unit* /*caster*/, SpellInfo const* /*spellInfo*/) { }
};

// ========== AllCreatureScript 兼容类 ==========
// AC 的 AllCreatureScript 提供全局生物事件钩子
// TC 没有直接对应的类，这里创建兼容版本
//By leewheel 2026-08-15: 修复——原空壳类OnAllCreatureUpdate/OnCreatureRemoveWorld从未被TC核心调用(无分发器)，
//导致ICC/SWP/Hyjal/RS的生物周期监听(软泥/迷雾/火雨/末日印记/暮光领域buff)全部死代码。
//改为继承CreatureScript并在构造时注册到ScriptMgr的全局生物更新钩子(独立小列表，每tick分发)
class AllCreatureScript : public CreatureScript
{
public:
    AllCreatureScript(const char* name) : CreatureScript(name)
    {
        //By leewheel 2026-08-15: 注册到核心全局生物更新分发器，使OnAllCreatureUpdate真实每tick触发
        sScriptMgr->RegisterCreatureUpdateScript(this);
        //End By leewheel
    }

    CreatureAI* GetAI(Creature* creature) const override { return GetCreatureAI(creature); }

    virtual void OnAllCreatureUpdate(Creature* /*creature*/, uint32 /*diff*/) override { }
    virtual void OnBeforeCreatureSelectLevel(const CreatureTemplate* /*cinfo*/, Creature* /*creature*/, uint8& /*level*/) { }
    virtual void OnCreatureSelectLevel(const CreatureTemplate* /*cinfo*/, Creature* /*creature*/) { }
    virtual void OnCreatureAddWorld(Creature* /*creature*/) { }
    virtual void OnCreatureRemoveWorld(Creature* /*creature*/) { }
    virtual void OnCreatureSaveToDB(Creature* /*creature*/) { }
    virtual bool CanCreatureGossipHello(Player* /*player*/, Creature* /*creature*/) { return false; }
    virtual bool CanCreatureGossipSelect(Player* /*player*/, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) { return false; }
    virtual bool CanCreatureGossipSelectCode(Player* /*player*/, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) { return false; }
    virtual bool CanCreatureQuestAccept(Player* /*player*/, Creature* /*creature*/, Quest const* /*quest*/) { return false; }
    virtual bool CanCreatureQuestReward(Player* /*player*/, Creature* /*creature*/, Quest const* /*quest*/, uint32 /*opt*/) { return false; }
    virtual CreatureAI* GetCreatureAI(Creature* /*creature*/) const { return nullptr; }
    virtual void OnFfaPvpStateUpdate(Creature* /*creature*/, bool /*InPvp*/) {}
};
//End By leewheel

//By leewheel 2026-07-27: 移植ICC地图清理钩子，TC的InstanceMapScript原生支持OnDestroy，比AC的AllMapScript更精准
//End By leewheel

// AC 兼容别名
using SpellSC = AllSpellScript;

//By leewheel 2026-07-09: DynamicObjectScript已由TC的ScriptMgr.h定义，不再重复定义
//End By leewheel

#endif // PLAYERBOTS_ALL_SCRIPT_COMPAT_H
