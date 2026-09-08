/*
 * 命名对象上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "NamedObjectContext.h"

#include <sstream>
#include <iterator>

void Qualified::Qualify(int qual)
{
    std::ostringstream out;
    out << qual;
    qualifier = out.str();
}

std::string const Qualified::MultiQualify(const std::vector<std::string>& qualifiers, const std::string& separator, const std::string_view brackets)
{
    std::stringstream out;
    for (uint8 i = 0; i < qualifiers.size(); ++i)
    {
        const std::string& qualifier = qualifiers[i];
        if (i == qualifiers.size() - 1)
        {
            out << qualifier;
        }
        else
        {
            out << qualifier << separator;
        }
    }

    if (brackets.empty())
    {
        return out.str();
    }
    else
    {
        return std::string(brackets[0] + out.str() + brackets[1]);
    }
}

std::vector<std::string> Qualified::getMultiQualifiers(const std::string& qualifier1)
{
    std::istringstream iss(qualifier1);
    return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
}

int32 Qualified::getMultiQualifier(const std::string& qualifier1, uint32 pos)
{
    return std::stoi(getMultiQualifiers(qualifier1)[pos]);
}
