#pragma once

#include <string>

namespace Odr
{
    struct AnonInfo
    {
        const bool excludeFromComparison;
        explicit AnonInfo(const bool b) noexcept : excludeFromComparison(b) {}
    };

    static std::wstring MakeAnonymousNamespaceTuSpecific(std::wstring name, const std::wstring& pdbPath)
    {
        auto pos = name.find(pdbPath);
        if (pos != std::wstring::npos)
            return name; // make specific already

        pos = name.find(L"`anonymous-namespace'");
        if (pos != std::wstring::npos)
        {

            auto first  = name.substr(0, pos);
            auto second = std::wstring(L"`anonymous-namespace'");
            auto third  = L"[" + pdbPath + L"]";
            auto fourth = name.substr(pos + second.length());

            name = first + second + third + fourth;
        }
        return name;
    }
    static std::wstring MakeAnonymousNamespaceTuNonSpecific(std::wstring name)
    {
        auto pos = name.find(L"`anonymous-namespace'[");
        if (pos != std::wstring::npos)
        {
            auto end = name.find(L']', pos+22);
            return name.erase(pos+21, end-pos-20);
        }
        return name;
    }

    static std::wstring CanonicalizeAnonymousNamespace(std::wstring name)
    {
        // MSVC is strangely inconsistent - it's either:  anonymous-namespace or anonymous namespace.
        // when an anonymous namespace type is an argument (including a template argument), MSVC uses a space rather than a -.
        for (;;) {
            auto pos1 = name.find(L"anonymous namespace");
            if (pos1 == std::wstring::npos)
                break;
            name[pos1 + 9] = L'-';
        }
        return name;
    }
}
