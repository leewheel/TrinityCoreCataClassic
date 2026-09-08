#include "AutoMaintenanceOnLevelupAction.h"

#include "Playerbots.h"  //By leewheel 2026-07-13: GetSpellNameBestLocale
#include "SpellMgr.h"

#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"
#include "BroadcastHelper.h"

bool AutoMaintenanceOnLevelupAction::Execute(Event /*event*/)
{
    AutoPickTalents();
    AutoLearnSpell();
    AutoTeleportForLevel();
    AutoUpgradeEquip();

    return true;
}

void AutoMaintenanceOnLevelupAction::AutoTeleportForLevel()
{
    if (!sPlayerbotAIConfig.autoTeleportForLevel || !sRandomPlayerbotMgr.IsRandomBot(bot))
        return;

    if (botAI->HasRealPlayerMaster())
        return;

    sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
    return;
}

void AutoMaintenanceOnLevelupAction::AutoPickTalents()
{
    if (!sPlayerbotAIConfig.autoPickTalents || !sRandomPlayerbotMgr.IsRandomBot(bot))
        return;

    uint32 freeTalentPoints = bot->GetLevel() >= 10 ? (uint32)((bot->GetLevel() - 9) * 1) : 0;
    freeTalentPoints = freeTalentPoints > bot->GetSpentTalentPointsCount() ? freeTalentPoints - bot->GetSpentTalentPointsCount() : 0;
    if (freeTalentPoints <= 0)
        return;

    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.InitTalentsTree(true, true, true);
    factory.InitPetTalents();
}

void AutoMaintenanceOnLevelupAction::AutoLearnSpell()
{
    std::ostringstream out;
    LearnSpells(&out);

    if (!out.str().empty())
    {
        std::string const temp = out.str();
        out.seekp(0);
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "学会的法术: ";
        //End By leewheel
        out << temp;
        out.seekp(-2, out.cur);
        out << ".";
        botAI->TellMaster(out);
    }
    return;
}

void AutoMaintenanceOnLevelupAction::LearnSpells(std::ostringstream* out)
{
    BroadcastHelper::BroadcastLevelup(botAI, bot);
    if (sPlayerbotAIConfig.autoLearnTrainerSpells && sRandomPlayerbotMgr.IsRandomBot(bot))
        LearnTrainerSpells(out);

    if (sPlayerbotAIConfig.autoLearnQuestSpells && sRandomPlayerbotMgr.IsRandomBot(bot))
        LearnQuestSpells(out);
}

void AutoMaintenanceOnLevelupAction::LearnTrainerSpells(std::ostringstream* /*out*/)
{
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.InitSkills();
    factory.InitClassSpells();
    factory.InitAvailableSpells();
    factory.InitPet();
}

void AutoMaintenanceOnLevelupAction::LearnQuestSpells(std::ostringstream* out)
{
    auto const& questTemplates = sObjectMgr->GetQuestTemplates();
    for (auto const& [questId, quest] : questTemplates)
    {
        //By leewheel 2026-09-06: 移植到TrinityCore-Cata
        //Cata的任务模板容器为unique_trackable_ptr<Quest>(智能指针)，访问改用箭头语法
        if (!quest->GetAllowableClasses() || quest->IsRepeatable() || quest->GetQuestMinLevel() < 10 ||
            quest->GetQuestMinLevel() > bot->GetLevel())
        {
            continue;
        }

        if (!bot->SatisfyQuestClass(quest.get(), false) || !bot->SatisfyQuestRace(quest.get(), false) ||
            !bot->SatisfyQuestSkill(quest.get(), false))
        {
            continue;
        }

        int32 spellId = quest->GetRewSpell();
        //End By leewheel
        if (!spellId)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        bool found = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_LEARN_SPELL && spellInfo->GetEffects()[i].TriggerSpell &&
                !bot->HasSpell(spellInfo->GetEffects()[i].TriggerSpell))
            {
                if (SpellInfo const* triggeredInfo = sSpellMgr->GetSpellInfo(spellInfo->GetEffects()[i].TriggerSpell, DIFFICULTY_NONE))
                    if (triggeredInfo->GetEffects()[0].Effect == SPELL_EFFECT_TRADE_SKILL)
                        break;

                found = true;
                break;
            }
        }

        if (!found)
            continue;

        bot->CastSpell(bot, spellId, true);

        uint32 rewSpellId = quest->GetRewSpell();
        if (rewSpellId)
        {
            if (SpellInfo const* rewSpellInfo = sSpellMgr->GetSpellInfo(rewSpellId, DIFFICULTY_NONE))
            {
                *out << FormatSpell(rewSpellInfo) << ", ";
                continue;
            }
        }

        *out << FormatSpell(spellInfo) << ", ";
    }
}

std::string const AutoMaintenanceOnLevelupAction::FormatSpell(SpellInfo const* sInfo)
{
    std::ostringstream out;
    uint8 rank = sInfo->GetRank();

    if (rank == 0)
        out << "|cffffffff|Hspell:" << sInfo->Id << "|h[" << GetSpellNameBestLocaleWithCache(sInfo->Id, sInfo->SpellName) << "]|h|r";
    else
        out << "|cffffffff|Hspell:" << sInfo->Id << "|h[" << GetSpellNameBestLocaleWithCache(sInfo->Id, sInfo->SpellName) << " 等级 " << rank << "]|h|r";

    return out.str();
}

void AutoMaintenanceOnLevelupAction::AutoUpgradeEquip()
{
    if (!sRandomPlayerbotMgr.IsRandomBot(bot))
        return;

    PlayerbotFactory factory(bot, bot->GetLevel());

    factory.CleanupConsumables();

    factory.InitAmmo();
    factory.InitReagents();
    factory.InitFood();
    factory.InitConsumables();
    factory.InitPotions();

    if (sPlayerbotAIConfig.autoUpgradeEquip)
        factory.InitEquipment(true);
}
