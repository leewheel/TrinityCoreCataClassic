/*
 * By leewheel 2026-09-06
 * 移植到TrinityCore-Cata：TC-Cata已有完整SkillType枚举(SharedDefines.h)，
 * 但个别WotLK命名常量缺失，此处仅补齐模块用到的缺失项(带#ifndef保护避免重定义)
 * 技能行ID在WotLK 3.3.5与Cataclysm 4.3.4之间保持一致
 */
#ifndef PLAYERBOTS_SKILLDEFINESCOMPAT_H
#define PLAYERBOTS_SKILLDEFINESCOMPAT_H

#ifndef SKILL_FIRST_AID
#define SKILL_FIRST_AID 129  // 急救技能行ID
#endif

#ifndef SKILL_LOCKPICKING
#define SKILL_LOCKPICKING 633  // 开锁技能行ID
#endif

#endif
