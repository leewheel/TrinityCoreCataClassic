/*
 * 懒计算值模板
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_LAZYCALCULATEDVALUE_H
#define PLAYERBOTS_LAZYCALCULATEDVALUE_H

/**
 * 懒计算辅助类
 *
 * 存储一个函数指针（计算器）及其所有者实例，
 * 仅在首次请求时计算值。结果会被缓存直到调用 Reset()
 */
template <class TValue, class TOwner>
class LazyCalculatedValue
{
public:
    typedef TValue (TOwner::*Calculator)();

public:
    LazyCalculatedValue(TOwner* owner, Calculator calculator) : calculator(calculator), owner(owner) { Reset(); }

public:
    TValue GetValue()
    {
        if (!calculated)
        {
            value = (owner->*calculator)();
            calculated = true;
        }

        return value;
    }

    void Reset() { calculated = false; }

protected:
    Calculator calculator;
    TOwner* owner;
    bool calculated;
    TValue value;
};

#endif // PLAYERBOTS_LAZYCALCULATEDVALUE_H
