/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellGlyphsAction.h"

#include "Event.h"
#include "Playerbots.h"

#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "World.h"

#include <unordered_map>
#include <sstream>

namespace
{
    // -----------------------------------------------------------------
    // Cache : GlyphID (MiscValue) -> ItemTemplate*
    // -----------------------------------------------------------------
    std::unordered_map<uint32, ItemTemplate const*> s_GlyphItemCache;

    void BuildGlyphItemCache()
    {
        if (!s_GlyphItemCache.empty())
            return;

        //By leewheel 2026-07-10 TC中GetItemTemplateStore返回引用而非指针
        ItemTemplateContainer const& store = sObjectMgr->GetItemTemplateStore();
        //End By leewheel

        for (auto const& kv : store)                       // C++17 : range-for sur map
        {
            ItemTemplate const* proto = &kv.second;

            if (!proto || proto->GetClass() != ITEM_CLASS_GLYPH)
                continue;

        for (uint32 i = 0; i < proto->Effects.size(); ++i)
        {
            uint32 spellId = proto->Effects[i]->SpellID;
                if (!spellId)
                    continue;

                SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
                if (!spell)
                    continue;

                for (uint32 eff = 0; eff <= EFFECT_2; ++eff)
                {
                    if (spell->GetEffects()[eff].Effect != SPELL_EFFECT_APPLY_GLYPH)
                        continue;

                    uint32 glyphId = spell->GetEffects()[eff].MiscValue;
                    if (glyphId)
                        s_GlyphItemCache[glyphId] = proto;
                }
            }
        }
    }
} // namespace

// -----------------------------------------------------------------
// Action
// -----------------------------------------------------------------
bool TellGlyphsAction::Execute(Event event)
{
    //-----------------------------------------------------------------
    // 1. who sended the wisp ?  (source of event)
    //-----------------------------------------------------------------
    Player* sender = event.getOwner();          // API Event
    if (!sender)
        return false;

    //-----------------------------------------------------------------
    // 2. Generate glyphId cache -> item
    //-----------------------------------------------------------------
    BuildGlyphItemCache();

    //-----------------------------------------------------------------
    // 3. Look at the 6 glyphs sockets
    //-----------------------------------------------------------------
    std::ostringstream list;
    bool first = true;

    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
    {
        uint32 glyphId = bot->GetGlyph(slot);
        if (!glyphId)
            continue;

        auto it = s_GlyphItemCache.find(glyphId);
        if (it == s_GlyphItemCache.end())
            continue;                                // No glyph found (rare)

        if (!first)
            list << ", ";

        // chat->FormatItem
        list << chat->FormatItem(it->second);
        first = false;
    }

    //-----------------------------------------------------------------
    // 4. Send chat messages
    //-----------------------------------------------------------------
    if (first)                                       // no glyphs
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("没有装备任何雕文");
        //End By leewheel
    else
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster(std::string("雕文：") + list.str());
        //End By leewheel

    return true;
}
