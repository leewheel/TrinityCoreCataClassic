/*
 * 工具函数
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "Helpers.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

/**
 * 大小写不敏感的子字符串搜索
 */
char* strstri(char const* haystack, char const* needle)
{
    if (!*needle)
    {
        return (char*)haystack;
    }

    for (; *haystack; ++haystack)
    {
        if (tolower(*haystack) == tolower(*needle))
        {
            char const* h = haystack;
            char const* n = needle;

            for (; *h && *n; ++h, ++n)
            {
                if (tolower(*h) != tolower(*n))
                {
                    break;
                }
            }

            if (!*n)
            {
                return (char*)haystack;
            }
        }
    }

    return 0;
}

/**
 * 去除字符串左侧空白
 */
std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
    return s;
}

/**
 * 去除字符串右侧空白
 */
std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

/**
 * 去除字符串两端空白
 */
std::string& trim(std::string& s) { return ltrim(rtrim(s)); }

/**
 * 使用 C 字符串分隔符分割字符串
 */
void split(std::vector<std::string>& dest, std::string const str, char const* delim)
{
    char* pTempStr = strdup(str.c_str());
    char* pWord = strtok(pTempStr, delim);

    while (pWord != nullptr)
    {
        dest.push_back(pWord);
        pWord = strtok(nullptr, delim);
    }

    free(pTempStr);
}

/**
 * 使用单个字符分隔符分割字符串
 */
std::vector<std::string>& split(std::string const s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim))
    {
        elems.push_back(item);
    }

    return elems;
}

/**
 * 使用单个字符分隔符分割字符串
 */
std::vector<std::string> split(std::string const s, char delim)
{
    std::vector<std::string> elems;
    return split(s, delim, elems);
}
