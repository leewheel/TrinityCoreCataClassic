/* 地下城机器人策略 */
//By leewheel 2026-08-24: 随 the-lab 将 SethData.h 重命名为 SethShared.h(头守卫与命名空间同步更名)
#ifndef PLAYERBOTS_SETHSHARED_H
#define PLAYERBOTS_SETHSHARED_H
//End By leewheel

#include "Common.h"
#include "Position.h"
#include <type_traits>

namespace SethShared
{

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr uint32 Id(T value)
{
    return static_cast<uint32>(value);
}

enum class SethSpells : uint32
{
    SPELL_REJUVENATION_RANK_1 = 774,
    SPELL_TREMOR_TOTEM        = 8143,
    SPELL_ARCANE_BUBBLE       = 9438,
    SPELL_SPELL_BOMB          = 40303,
    SPELL_BANISH_ANZU         = 42354,
};

enum class SethNpcs : uint32
{
    NPC_CHARMING_TOTEM        = 20343,
    NPC_HAWK_SPIRIT           = 23134,
    NPC_FALCON_SPIRIT         = 23135,
    NPC_EAGLE_SPIRIT          = 23136,
};

inline constexpr uint32 SETH_MAP_ID = 556;
inline constexpr uint32 REJUVENATION_SPELL_ICON_ID = 64;

inline Position const PILLAR_CENTER   = { 23.730f, 309.230f };
inline Position const PILLAR_POSITION = { 35.538f, 309.573f, 25.086f };

}

#endif
