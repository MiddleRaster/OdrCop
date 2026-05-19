#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include <string>
#include <vector>

#include "DiaGetters.h"
#include "AnonInfo.h"

namespace Odr
{
    class EnumInfo : public AnonInfo
    {
        const std::wstring pdbPath;
        const std::wstring name;
        const std::vector<std::pair<std::wstring,std::wstring>> values; // eg, pairs of {A,1}, {B,2}, etc.
    public:
        EnumInfo(bool b, const std::wstring& pdbPath, const std::wstring& name, IDiaSymbol* sym)
            : AnonInfo(b)
            , pdbPath(pdbPath)
            , name(Odr::MakeAnonymousNamespaceTuSpecific(name, pdbPath))
            , values(MakeVectorOfEnumValues(sym))
        {}
        void Print(int depth) const
        {
            std::wstring indent = Odr::Indent(depth);
            if (depth == 0)
                std::wcout << indent << L"  [" << pdbPath << L"]\n";
            std::wcout     << indent << L"    kind=enum\n";
            std::wcout     << indent << L"    the enum values are: ";
            
            for(size_t i=0; i<values.size(); ++i)
                std::wcout << values[i].first << L"=" << values[i].second << L", ";
            std::wcout << L'\n';
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }
        template <typename Fn> void  CollectSubItems(Fn&&) const {}

        friend bool operator==(const EnumInfo& a, const EnumInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const EnumInfo& a, const EnumInfo& b) { return !a.IsEqualTo(b); }
    private:
        bool IsEqualTo(const EnumInfo& other) const
        {
            if (name != other.name)                        return false;
            if (!std::ranges::equal(values, other.values)) return false;
            return true;
        }
        static auto MakeVectorOfEnumValues(IDiaSymbol* sym)
        {
            std::vector<std::pair<std::wstring, std::wstring>> values;

            HRESULT hr;
            CComPtr<IDiaEnumSymbols> children;

            /*
            I'm supposed to use SymTagConstant, but that doesn't exist in my cvconst.h.
            Fortunately, the only thing that can be inside an enum is its values,
            so I can use SymTagNull and just get everything.
            */
            
            if (SUCCEEDED(hr = sym->findChildren(SymTagNull, nullptr, nsNone, &children)))
            {
                while(true)
                {
                    CComPtr<IDiaSymbol> child;
                    ULONG fetched=0;
                    hr = children->Next(1, &child, &fetched);
                    if (fetched == 0)
                        break;

                    CComBSTR  name = Get(child, &IDiaSymbol::get_name);
                    std::wstring v = Get(child, &IDiaSymbol::get_value);
                    values.push_back({std::wstring{name}, v});
                }
            }
            return values;
        }
    };
}

