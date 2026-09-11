/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "EquipGlyphsAction.h"

#include "Playerbots.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "DBCStores.h"
#include "AiObjectContext.h"
#include "Log.h"

#include <unordered_map>
#include <sstream>
#include <unordered_set>

namespace
{
    // itemId -> GlyphInfo
    std::unordered_map<uint32, EquipGlyphsAction::GlyphInfo> s_GlyphCache;
}

void EquipGlyphsAction::BuildGlyphCache()
{
    if (!s_GlyphCache.empty())
        return;

    //By leewheel 2026-07-10 TC中GetItemTemplateStore返回引用而非指针
    ItemTemplateContainer const& store = sObjectMgr->GetItemTemplateStore();
    //End By leewheel

    for (auto const& kv : store)
    {
        uint32 itemId = kv.first;
        ItemTemplate const* proto = &kv.second;
        if (!proto || proto->GetClass() != ITEM_CLASS_GLYPH)
            continue;

        // inspect item spell
        for (uint32 i = 0; i < proto->Effects.size(); ++i)
        {
            uint32 spellId = proto->Effects[i]->SpellID;
            if (!spellId) continue;

            SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId);
            if (!si)         continue;

            for (uint8 eff = 0; eff <= EFFECT_2; ++eff)
            {
                if (si->GetEffects()[eff].Effect != SPELL_EFFECT_APPLY_GLYPH)
                    continue;

                uint32 glyphId = si->GetEffects()[eff].MiscValue;
                if (!glyphId) continue;

                if (auto const* gp = sGlyphPropertiesStore.LookupEntry(glyphId))
                    s_GlyphCache[itemId] = {gp, proto};
            }
        }
    }
}

EquipGlyphsAction::GlyphInfo const* EquipGlyphsAction::GetGlyphInfo(uint32 itemId)
{
    BuildGlyphCache();
    auto it = s_GlyphCache.find(itemId);
    return (it == s_GlyphCache.end()) ? nullptr : &it->second;
}

/// -----------------------------------------------------------------
///  Validation and collect
/// -----------------------------------------------------------------
bool EquipGlyphsAction::CollectGlyphs(std::vector<uint32> const& itemIds,
                                      std::vector<GlyphInfo const*>& out) const
{
    std::unordered_set<uint32> seen;

    for (uint32 itemId : itemIds)
    {
        if (!seen.insert(itemId).second)
            return false;                                 // double

        auto const* info = GetGlyphInfo(itemId);
        if (!info)                                        // no good glyph
            return false;

        // check class by AllowableClass
        if ((info->proto->GetAllowableClass() & bot->GetClassMask()) == 0)
            return false;

        out.push_back(info);
    }
    return out.size() <= 6 && !out.empty();
}

/// -----------------------------------------------------------------
///  Action
/// -----------------------------------------------------------------
bool EquipGlyphsAction::Execute(Event event)
{
    // 1) parse IDs
    std::vector<uint32> itemIds;
    std::istringstream iss(event.getParam());
    for (uint32 id; iss >> id; ) itemIds.push_back(id);

    std::vector<GlyphInfo const*> glyphs;
    if (!CollectGlyphs(itemIds, glyphs))
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("用法：glyph equip <6个雕文物品ID>（3大雕文，3小雕文）。");
        //End By leewheel
        return false;
    }

    // 2) prepare a empty slots table ?
    bool used[6] = {false,false,false,false,false,false};

    // 3) for each glyph, find the first available and compatible socket
    for (auto const* g : glyphs)
    {
        bool placed = false;

        for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
        {
            if (used[i]) continue;

            uint32 slotId   = bot->GetGlyphSlot(i);
            auto const* gs  = sGlyphSlotStore.LookupEntry(slotId);
            //By leewheel 2025-07-10
            // TC: GlyphSlotEntry有Type字段, GlyphPropertiesEntry用GlyphType
            if (!gs || gs->Type != g->prop->GlyphType)
                continue;
            //End By leewheel                                   // major/minor don't match

            // Remove aura if exist
            uint32 cur = bot->GetGlyph(i);
            if (cur)
                if (auto* old = sGlyphPropertiesStore.LookupEntry(cur))
                //By leewheel 2025-07-10
            // TC使用SpellID（大写），AC使用SpellId（小写）
            bot->RemoveAurasDueToSpell(old->SpellID);
            //End By leewheel

            // Apply new one
            //By leewheel 2025-07-10
            // TC使用SpellID（大写），AC使用SpellId（小写）
            bot->CastSpell(bot, g->prop->SpellID, true);
            //End By leewheel
            //By leewheel 2025-07-10
            // TC使用ID（大写），AC使用Id（小写）
            // TC的SetGlyph只有2个参数，没有第3个bool参数
            bot->SetGlyph(i, g->prop->ID);
            //End By leewheel

            used[i] = true;
            placed  = true;
            break;
        }

        if (!placed)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMaster("没有足够空插槽装下所有雕文。");
            //End By leewheel
            return false;
        }
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("雕文已更新。");
    //End By leewheel

    // Flag for custom glyphs
    botAI->GetAiObjectContext()->GetValue<bool>("custom_glyphs")->Set(true);
    TC_LOG_INFO("playerbots", "Custom Glyph Flag set to ON");

    return true;
}
