/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "ItemVisitors.h"

#include "Playerbots.h"

bool FindUsableItemVisitor::Visit(Item* item)
{
    if (bot->CanUseItem(item->GetTemplate()) == EQUIP_ERR_OK)
        return FindItemVisitor::Visit(item);

    return true;
}

bool FindPotionVisitor::Accept(ItemTemplate const* proto)
{
    if (proto->GetClass() == ITEM_CLASS_CONSUMABLE &&
        (proto->GetSubClass() == ITEM_SUBCLASS_POTION || proto->GetSubClass() == ITEM_SUBCLASS_FLASK))
    {
        for (uint8 j = 0; j < proto->Effects.size(); j++)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Effects[j]->SpellID);
            if (!spellInfo)
                return false;

            for (uint8 i = 0; i < 3; i++)
            {
                //By leewheel 2026-09-03 修复C4389警告：Effect为枚举(SpellEffectName)，effectId为uint32，比较前显式转换
                if (static_cast<uint32>(spellInfo->GetEffects()[i].Effect) == effectId)
                    return true;
            }
        }
    }

    return false;
}

bool FindMountVisitor::Accept(ItemTemplate const* proto)
{
    for (uint8 j = 0; j < proto->Effects.size(); j++)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Effects[j]->SpellID);
        if (!spellInfo)
            return false;

        for (uint8 i = 0; i < 3; i++)
        {
            if (spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOUNTED)
                return true;
        }
    }

    return false;
}

bool FindPetVisitor::Accept(ItemTemplate const* proto)
{
    if (proto->GetClass() == ITEM_CLASS_MISC)
    {
        for (uint8 j = 0; j < proto->Effects.size(); j++)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Effects[j]->SpellID);
            if (!spellInfo)
                return false;

            for (uint8 i = 0; i < 3; i++)
            {
                if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_SUMMON_PET)
                    return true;
            }
        }
    }

    return false;
}

FindItemUsageVisitor::FindItemUsageVisitor(Player* bot, ItemUsage usage) : FindUsableItemVisitor(bot), usage(usage)
{
    context = GET_PLAYERBOT_AI(bot)->GetAiObjectContext();
};

bool FindItemUsageVisitor::Accept(ItemTemplate const* proto)
{
    if (AI_VALUE2(ItemUsage, "item usage", proto->GetId()) == usage)
        return true;

    return false;
}

bool FindUsableNamedItemVisitor::Accept(ItemTemplate const* proto)
{
    return proto && *proto->GetName(DEFAULT_LOCALE) && strstri(proto->GetName(DEFAULT_LOCALE), name.c_str());
}
