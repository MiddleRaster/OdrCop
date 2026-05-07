#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include <string>
#include <vector>

#include "DiaGetters.h"

namespace Odr
{
    class TDefInfo
    {
        const std::wstring pdbPath;
        const std::wstring name;
        const std::wstring underlyingType;
    public:
        TDefInfo(const std::wstring& name, IDiaSymbol* sym, const std::wstring& pdbPath) : name(name), pdbPath(pdbPath), underlyingType(GetUnderlyingType(sym)) {}
        void Print() const
        {
            std::wcout << L"  [" << pdbPath << L"]\n";
            std::wcout << L"    kind=typedef\n";
            std::wcout << L"    underlying type: " << underlyingType << L'\n';
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }

        friend bool operator==(const TDefInfo& a, const TDefInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const TDefInfo& a, const TDefInfo& b) { return !a.IsEqualTo(b); }

    private:
        bool IsEqualTo(const TDefInfo& other) const
        {
            if (          name != other.name          ) return false;
            if (underlyingType != other.underlyingType) return false;
            return true;
        }
        static std::wstring GetUnderlyingType(IDiaSymbol* sym)
        {
            CComPtr<IDiaSymbol> type = Get(sym, &IDiaSymbol::get_type);
            if (type) {
                CComBSTR name = Get(type, &IDiaSymbol::get_name);
                if (!!name)
                    return std::wstring{name};
            }
            return L"unknown underlying type";
        }
    };
}
