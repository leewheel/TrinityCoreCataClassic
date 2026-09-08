/*
 * 工具函数
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_HELPERS_H
#define PLAYERBOTS_HELPERS_H

#include <string>
#include <vector>

/**
 * 大小写不敏感的子字符串搜索
 */
char* strstri(char const* haystack, char const* needle);

/**
 * 去除字符串左侧空白
 */
std::string& ltrim(std::string& s);

/**
 * 去除字符串右侧空白
 */
std::string& rtrim(std::string& s);

/**
 * 去除字符串两端空白
 */
std::string& trim(std::string& s);

/**
 * 使用 C 字符串分隔符分割字符串
 */
void split(std::vector<std::string>& dest, std::string const str, char const* delim);

/**
 * 使用单个字符分隔符分割字符串
 */
std::vector<std::string>& split(std::string const s, char delim, std::vector<std::string>& elems);

/**
 * 使用单个字符分隔符分割字符串
 */
std::vector<std::string> split(std::string const s, char delim);

#endif // PLAYERBOTS_HELPERS_H
