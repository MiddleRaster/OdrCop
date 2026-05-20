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
        const BasicType    underlyingType;
        const std::vector<std::pair<std::wstring,std::wstring>> values; // eg, pairs of {A,1}, {B,2}, etc.
    public:
        EnumInfo(bool b, const std::wstring& pdbPath, const std::wstring& name, IDiaSymbol* sym)
            : AnonInfo(b)
            , pdbPath (pdbPath)
            , name    (Odr::MakeAnonymousNamespaceTuSpecific(name, pdbPath))
            , underlyingType((BasicType)Get(sym, &IDiaSymbol::get_baseType))
            , values(MakeVectorOfEnumValues(sym))
        {}
        void Print(int depth) const
        {
            std::wstring indent = Odr::Indent(depth);
            if (depth == 0)
                std::wcout << indent << L"  [" << pdbPath << L"]\n";
            std::wcout     << indent << L"    kind=enum\n";
            std::wcout     << indent << L"    underlying type=" << GetBasicType() << L'\n';
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
            if (           name != other.name            ) return false;
            if ( underlyingType != other.underlyingType  ) return false;
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
        std::wstring GetBasicType() const
        {
            switch(underlyingType)
            { 
            case btNoType:   return L"btNoType";
            case btVoid:     return L"btVoid";
            case btChar:     return L"btChar";
            case btWChar:    return L"btWChar";
            case btInt:      return L"btInt";
            case btUInt:     return L"btUInt";
            case btFloat:    return L"btFloat";
            case btBCD:      return L"btBCD";
            case btBool:     return L"btBool";
            case btLong:     return L"btLong";
            case btULong:    return L"btULong";
            case btCurrency: return L"btCurrency";
            case btDate:     return L"btDate";
            case btVariant:  return L"btVariant";
            case btComplex:  return L"btComplex";
            case btBit:      return L"btBit";
            case btBSTR:     return L"btBSTR";
            case btHresult:  return L"btHresult";
            case btChar16:   return L"btChar16";
            case btChar32:   return L"btChar32";
            case btChar8:    return L"btChar8";
        //  case btVector:   return L"btVector";
            default:         return L"unknown";
            };
        }

    };
}

