#pragma once
// a stripped-down version of magic_enum

#include <array>

#define WIDEN2(x) L##x
#define WIDEN(x)  WIDEN2(x)
template <auto V> constexpr const wchar_t* raw_name() noexcept { return WIDEN(__FUNCSIG__); }
template <auto V> std::wstring enum_name_of()
{
    std::wstring s = raw_name<V>();
    auto end = s.rfind(L'>');
    auto start = s.rfind(L'<', end) + 1;
    return s.substr(start, end - start);
}
template <auto V> bool is_valid() { return enum_name_of<V>()[0] != L'('; }
template <typename E, int Min, int Max> struct EnumTable
{
    static constexpr int     count = Max - Min + 1;
    std::array<std::wstring, count> names{};

    EnumTable() { fill(std::make_integer_sequence<int, count>{}); }
    template <int... Is> void fill(std::integer_sequence<int, Is...>)
    {
        ((names[Is] = is_valid<static_cast<E>(Min + Is)>() ? enum_name_of<static_cast<E>(Min + Is)>() : std::wstring{}), ...);
    }
    std::wstring lookup(int v) const
    {
        if (v < Min || v > Max) return {};
        return names[v - Min];
    }
};
template <typename E, int Min = 0, int Max = 127> std::wstring enum_name(E value)
{
    static const EnumTable<E, Min, Max> table;
    std::wstring name = table.lookup(static_cast<int>(value));
    if (name.empty())
        return L"unknown(" + std::to_wstring(static_cast<int>(value)) + L")";
    return name;
}
