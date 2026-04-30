#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

#include "COFFer.h"

namespace Odr
{
    inline std::wstring BstrToWstr(BSTR b) { return b ? std::wstring(b) : std::wstring(L"<unnamed>"); }

    template <typename C, typename R, typename T> T Get(IDiaSymbol* sym, R(C::* m)(T*))
    {
        T value{};
        (sym->*m)(&value);
        return value;
    }
    template <typename R> CComBSTR Get(IDiaSymbol* sym, R (IDiaSymbol::* m)(BSTR*))
    {
        CComBSTR value{};
        (sym->*m)(&value.m_str);
        return value;
    }
    template <typename R> CComPtr<IDiaSymbol> Get(IDiaSymbol* sym, R (IDiaSymbol::* m)(IDiaSymbol*))
    {
        CComPtr<IDiaSymbol> value{};
        (sym->*m)(&value);
        return value;
    }
    template <typename R> std::wstring Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(VARIANT*))
    {
        CComVariant value{};
        HRESULT hr = (sym->*m)(&value);
        if ((hr != S_OK && hr != S_FALSE) || FAILED(value.ChangeType(VT_BSTR)) || !value.bstrVal)
            return L"?";
        return std::wstring(value.bstrVal);
    }

    class UdtInfo
    {
        template <typename Derived>
        class MemberInfoBase
        {
            const std::wstring name;
            const std::wstring typeName;
        public:
            MemberInfoBase(const std::wstring& name, IDiaSymbol* sym) : name(name), typeName(resolveTypeName(Get(sym, &IDiaSymbol::get_type))) {}
            bool IsEqualTo(const Derived& other) const
            {
                if (name     != other.name    ) return false;
                if (typeName != other.typeName)
                    if (!(isAnonymous(typeName) && isAnonymous(other.typeName)))
                        return false;
                    // else treat as equal
                return static_cast<const Derived*>(this)->IsEqualToImpl(other);
            }
            void Print() const
            {
                static_cast<const Derived*>(this)->PrintPrefix();
                std::wcout << L"  " << typeName << L"  " << name;
                static_cast<const Derived*>(this)->PrintSuffix();
            }
            friend bool operator==(const Derived& a, const Derived& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const Derived& a, const Derived& b) { return !a.IsEqualTo(b); }

        private:
            static bool isAnonymous(const std::wstring& name)
            {
                // Extract tail after last "::"
                size_t pos = name.rfind(L"::");
                std::wstring tail = (pos == std::wstring::npos) ? name : name.substr(pos + 2);

                // Strict rules:
                // Anonymous ONLY if tail begins with "<unnamed"
                // Examples:
                //   <unnamed>
                //   <unnamed-tag>
                //   <unnamed-type-u>
                //   <unnamed-type-$S1>
                //   <unnamed-type-$T1>
                //
                // NOT anonymous:
                //   <anonymous>
                //   Foo<unnamed-type-u>
                //   MyStruct_unnamed_type

                if (tail.rfind(L"<unnamed", 0) == 0)
                    return true;

                return false;
            }
            static std::wstring resolveTypeName(IDiaSymbol* type)
            {
                if (!type) return L"<null>";

                DWORD tag = 0;
                type->get_symTag(&tag);

                switch (tag)
                {
                case SymTagBaseType: 
                {
                    DWORD baseType = 0;
                    ULONGLONG len = 0;
                    type->get_baseType(&baseType);
                    type->get_length(&len);
                    switch (baseType) 
                    {
                    case btVoid:    return L"void";
                    case btChar:    return L"char";
                    case btWChar:   return L"wchar_t";
                    case btInt:
                        switch (len) 
                        {
                        case 1:  return L"int8_t";
                        case 2:  return L"int16_t";
                        case 4:  return L"int32_t";
                        case 8:  return L"int64_t";
                        default: return L"int";
                        }
                    case btUInt:
                        switch (len) 
                        {
                        case 1:  return L"uint8_t";
                        case 2:  return L"uint16_t";
                        case 4:  return L"uint32_t";
                        case 8:  return L"uint64_t";
                        default: return L"unsigned";
                        }
                    case btFloat:   return len == 4 ? L"float" : L"double";
                    case btBool:    return L"bool";
                    case btLong:    return L"long";
                    case btULong:   return L"unsigned long";
                    default:        return L"<basetype:" + std::to_wstring(baseType) + L">";
                    }
                }
                case SymTagPointerType:
                {
                    CComPtr<IDiaSymbol> inner;
                    BOOL isRef = FALSE;
                    type->get_type(&inner);
                    type->get_reference(&isRef);
                    std::wstring inner_name = resolveTypeName(inner);
                    return isRef ? inner_name + L"&" : inner_name + L"*";
                }
                case SymTagArrayType:
                {
                    CComPtr<IDiaSymbol> elem;
                    DWORD               count = 0;
                    type->get_type (&elem);
                    type->get_count(&count);
                    return resolveTypeName(elem) + L"[" + std::to_wstring(count) + L"]";
                }
                case SymTagFunctionType:
                {
                    CComPtr<IDiaSymbol> ret;
                    type->get_type(&ret);
                    return resolveTypeName(ret) + L"(*)()";  // simplified
                }
                case SymTagUDT:
                case SymTagEnum:
                case SymTagTypedef:
                {
                    CComBSTR n;
                    type->get_name(&n);
                    return BstrToWstr(n);
                }
                //case SymTagConstType:
                //{
                //    CComPtr<IDiaSymbol> inner;
                //    type->get_type(&inner);
                //    return L"const " + resolveTypeName(inner);
                //}
                //case SymTagVolatileType:
                //{
                //    CComPtr<IDiaSymbol> inner;
                //    type->get_type(&inner);
                //    return L"volatile " + resolveTypeName(inner);
                //}
                default:
                    return L"<tag:" + std::to_wstring(tag) + L">";
                }
            }
        };

        class InstanceMember : public MemberInfoBase<InstanceMember>
        {
            const LONG      offset;    // byte offset within UDT
            const ULONGLONG bitSize;   // 0 means "not a bitfield"
            const DWORD     bitPos;    // valid only when bitSize != 0
            const BOOL      bConst;    // is "const"
            const BOOL      bVolatile; // is "volatile"

            InstanceMember(IDiaSymbol* child, const std::wstring& name, LONG offset, ULONGLONG bitSize,   DWORD bitPos,    BOOL bConst,       BOOL bVolatile)
                                       : MemberInfoBase(name, child), offset(offset),  bitSize(bitSize), bitPos(bitPos), bConst(bConst), bVolatile(bVolatile)
            {
                //    auto lt = Get(child, &IDiaSymbol::get_locationType);
                //    switch(lt)
                //    {
                //    case LocationType::LocIsNull            : std::wcout << L"locationType is " << L"LocIsNull            " << L"\n"; break;
                //    case LocationType::LocIsStatic          : std::wcout << L"locationType is " << L"LocIsStatic          " << L"\n"; break;
                //    case LocationType::LocIsTLS             : std::wcout << L"locationType is " << L"LocIsTLS             " << L"\n"; break;
                //    case LocationType::LocIsRegRel          : std::wcout << L"locationType is " << L"LocIsRegRel          " << L"\n"; break;
                //    case LocationType::LocIsThisRel         : std::wcout << L"locationType is " << L"LocIsThisRel         " << L"\n"; break;
                //    case LocationType::LocIsEnregistered    : std::wcout << L"locationType is " << L"LocIsEnregistered    " << L"\n"; break;
                //    case LocationType::LocIsBitField        : std::wcout << L"locationType is " << L"LocIsBitField        " << L"\n"; break;
                //    case LocationType::LocIsSlot            : std::wcout << L"locationType is " << L"LocIsSlot            " << L"\n"; break;
                //    case LocationType::LocIsIlRel           : std::wcout << L"locationType is " << L"LocIsIlRel           " << L"\n"; break;
                //    case LocationType::LocInMetaData        : std::wcout << L"locationType is " << L"LocInMetaData        " << L"\n"; break;
                //    case LocationType::LocIsConstant        : std::wcout << L"locationType is " << L"LocIsConstant        " << L"\n"; break;
                //    case LocationType::LocIsRegRelAliasIndir: std::wcout << L"locationType is " << L"LocIsRegRelAliasIndir" << L"\n"; break;
                //    case LocationType::LocTypeMax           : std::wcout << L"locationType is " << L"LocTypeMax           " << L"\n"; break;
                //    };
            }

        public:
            static InstanceMember Make(IDiaSymbol* child)
            {
                auto name      = BstrToWstr(Get(child, &IDiaSymbol::get_name));
                auto offset    =            Get(child, &IDiaSymbol::get_offset);
                auto bitSize   =            Get(child, &IDiaSymbol::get_length);
                auto bitPos    =            Get(child, &IDiaSymbol::get_bitPosition);
                BOOL bConst    =    GetFromType(child, &IDiaSymbol::get_constType);
                BOOL bVolatile =    GetFromType(child, &IDiaSymbol::get_volatileType);

                return InstanceMember{child, name, offset, bitSize, bitPos, bConst, bVolatile};
            }
            static std::vector<InstanceMember> MakeSortedCopy(std::vector<InstanceMember>& members)
            {
                std::vector<size_t> idx(members.size());
                std::iota(idx.begin(), idx.end(), 0);
                std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return members[a].offset < members[b].offset; });

                std::vector<InstanceMember> sorted;
                sorted.reserve(members.size());
                for (size_t i : idx)
                    sorted.push_back(members[i]);
                return sorted;
            }
        private:
            friend MemberInfoBase<InstanceMember>;

            static BOOL GetFromType(IDiaSymbol* child, HRESULT(IDiaSymbol::* m)(BOOL*))
            {
                CComPtr<IDiaSymbol> type;
                if (SUCCEEDED(child->get_type(&type)))
                    return Get(type, m);
                return FALSE;
            }
            void PrintPrefix() const
            {
                std::wcout << L"    +" << offset << (bConst ? L" const" : L"") << (bVolatile ? L" volatile" : L"");
            }
            void PrintSuffix() const
            {
                if (bitSize)
                    std::wcout << L"  :" << bitSize << L"@bit" << bitPos;
                std::wcout << L'\n';
            }
            bool IsEqualToImpl(const InstanceMember& other) const
            {
                if (offset    != other.offset   ) return false;
                if (bitSize   != other.bitSize  ) return false;
                if (bitSize   && 
                    bitPos    != other.bitPos   ) return false;
                if (bConst    != other.bConst   ) return false;
                if (bVolatile != other.bVolatile) return false;
                return true;
            }
        };

        // does not work. Yet.
        class ConstantMember : public MemberInfoBase<ConstantMember>
        {
            std::wstring constValue;
        public:
            ConstantMember(std::wstring name, IDiaSymbol* pType, std::wstring constValue) : MemberInfoBase(name,pType), constValue(constValue) {}
        private:
            friend MemberInfoBase<ConstantMember>;

            bool IsEqualToImpl(const ConstantMember& other) const { return constValue == other.constValue; }
            void PrintPrefix() const { std::wcout << L"    constexpr/const static value  "; }
            void PrintSuffix() const { std::wcout << L'\n'; }
        };
        class StaticMember : public MemberInfoBase<StaticMember>
        {
            DWORD locationType;
            BOOL  isConstant;
            BOOL  isVolatile;
        public:
            StaticMember(std::wstring name, IDiaSymbol* pType, DWORD locationType, BOOL isConstant, BOOL isVolatile)
                : MemberInfoBase(name,pType)
                , locationType  (locationType)
                , isConstant    (isConstant)
                , isVolatile    (isVolatile)
            {}
        private:
            friend MemberInfoBase<StaticMember>;

            bool IsEqualToImpl(const StaticMember& other) const
            {
                if (locationType == other.locationType)
                if (isConstant   == other.isConstant  )
                if (isVolatile   == other.isVolatile  )
                    return true;
                return false;
            }
            void PrintPrefix() const
            {
                std::wcout << L"   " << (isConstant ? L"const" : L"") << L" " << (isVolatile ? L"volatile" : L"");
            };
            void PrintSuffix() const
            {
                std::wcout << L"   locationType: " << locationType << L", isConstant: " << (isConstant ? L"true" : L"false") << L", isVolatile: " << (isVolatile ? L"true" : L"false");
                std::wcout << L'\n';
            }
        };

        const std::wstring                 pdbPath;
        const std::wstring                 name;
        const ULONGLONG                    size;      // total size in bytes
        const UdtKind                      udtKind;   // UdtStruct / UdtClass / UdtUnion
        const std::tuple<
              std::vector<InstanceMember>,            // data members in offset order
              std::vector<ConstantMember>,            // constexpr/const static values
              std::vector<StaticMember >>             // static/constinit/consteval values
                                           members;
        const std::vector<std::wstring  >  baseNames; // base class names in order
    public:
        UdtInfo(IDiaSymbol* sym, const std::wstring& pdbPath) : pdbPath  (pdbPath)
                                                              , name(             BstrToWstr(Get(sym, &IDiaSymbol::get_name)))
                                                              , size(                        Get(sym, &IDiaSymbol::get_length))
                                                              , udtKind(static_cast<UdtKind>(Get(sym, &IDiaSymbol::get_udtKind)))
                                                              , members(GetMembers(sym))
                                                              , baseNames(GetBaseNames(sym))
        {}
        void Print() const
        {
            std::wcout << L"  [" << pdbPath << L"]\n";
            std::wcout << L"    kind=" << UdtKindToString() << L"  size=" << size << L'\n';
            if (!baseNames.empty())
            {
                std::wcout << L"    bases:";
                for (auto& b : baseNames) std::wcout << L" " << b;
                std::wcout << L'\n';
            }
            for (auto& i : std::get<0>(members)) i.Print();
            for (auto& c : std::get<1>(members)) c.Print();
            for (auto& s : std::get<2>(members)) s.Print();
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }

        friend bool operator==(const UdtInfo& a, const UdtInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const UdtInfo& a, const UdtInfo& b) { return !a.IsEqualTo(b); }

    private:
        bool IsEqualTo(const UdtInfo& other) const
        {
            if (                         size != other.size                         ) return false;
            if (                      udtKind != other.udtKind                      ) return false;
            if (                    baseNames != other.baseNames                    ) return false;
            if (  std::get<0>(members).size() != std::get<0>(other.members).size()  ) return false;
            if (  std::get<1>(members).size() != std::get<1>(other.members).size()  ) return false;
            if (  std::get<2>(members).size() != std::get<2>(other.members).size()  ) return false;
            if(!std::ranges::equal(std::get<0>(members), std::get<0>(other.members))) return false;
            if(!std::ranges::equal(std::get<1>(members), std::get<1>(other.members))) return false;
            if(!std::ranges::equal(std::get<2>(members), std::get<2>(other.members))) return false;
            return true;
        }
        const wchar_t* UdtKindToString() const
        {
            switch (udtKind) {
            case UdtStruct:      return L"struct";
            case UdtClass:       return L"class";
            case UdtUnion:       return L"union";
            case UdtInterface:   return L"interface";
            case UdtTaggedUnion: return L"taggedUnion";
            default:             return L"unknown";
            }
        }

        static std::tuple<std::vector<InstanceMember>, std::vector<ConstantMember>, std::vector<StaticMember>> GetMembers(IDiaSymbol* sym)
        {
            std::vector<InstanceMember> members;
            std::vector<ConstantMember> constants;
            std::vector<  StaticMember> statics;

            CComPtr<IDiaEnumSymbols> children;
            if (SUCCEEDED(sym->findChildren(SymTagData, NULL, nsNone, &children)))
            {
                while (true)
                {
                    ULONG fetched{};
                    CComPtr<IDiaSymbol> child;
                    children->Next(1, &child, &fetched);
                    if (fetched != 1)
                        break;

                    // Only instance data members (not statics) for now
                    DWORD dataKind = 0;
                    child->get_dataKind(&dataKind);
                    if (DataIsMember == static_cast<DataKind>(dataKind))
                    {
                        members.push_back(InstanceMember::Make(child));
                    }
                    else if (DataIsStaticMember == static_cast<DataKind>(dataKind))
                    {
                        statics.push_back(StaticMember(BstrToWstr(Get(child, &IDiaSymbol::get_name)),
                                                                      child, // let ctor do the IDiaSymbol::get_type call
                                                                  Get(child, &IDiaSymbol::get_locationType),
                                                                  Get(child, &IDiaSymbol::get_constType),
                                                                  Get(child, &IDiaSymbol::get_volatileType)));

                    } else {
                        // add other DataKind types
                        continue; // here only so I can put a breakpoint on it
                    }
                }
            }
            return {InstanceMember::MakeSortedCopy(members), constants, statics};
        }
        static std::vector<ConstantMember> GetConstantMembers(IDiaSymbol* sym)
        {
            std::vector<ConstantMember> members;

            CComPtr<IDiaEnumSymbols> children;
            if (SUCCEEDED(sym->findChildren(SymTagData, NULL, nsNone, &children)))
            {
                while (true)
                {
                    ULONG fetched{};
                    CComPtr<IDiaSymbol> child;
                    children->Next(1, &child, &fetched);
                    if (fetched != 1)
                        break;

                    DWORD dataKind = 0;
                    child->get_dataKind(&dataKind);
                    if (DataIsConstant == static_cast<DataKind>(dataKind))
                    {   // only constant members
                        members.push_back(ConstantMember(BstrToWstr(Get(child, &IDiaSymbol::get_name)),
                                                                    Get(child, &IDiaSymbol::get_type ),
                                                                    Get(child, &IDiaSymbol::get_value)));
                    }
                }
            }
            return members;
        }

        static std::vector<std::wstring> GetBaseNames(IDiaSymbol* sym)
        {
            std::vector<std::wstring> baseNames;

            // base classes
            CComPtr<IDiaEnumSymbols> bases;
            if (SUCCEEDED(sym->findChildren(SymTagBaseClass, NULL, nsNone, &bases)))
            {
                while (true)
                {
                    ULONG fetched{};
                    CComPtr<IDiaSymbol> base;
                    bases->Next(1, &base, &fetched);
                    if (fetched == 0)
                        break;
                    CComPtr<IDiaSymbol> baseType;
                    if (SUCCEEDED(base->get_type(&baseType)))
                    {
                        baseNames.push_back(BstrToWstr(Get(baseType, &IDiaSymbol::get_name)));
                    }
                }
            }
            return baseNames;
        }
    };

    class EnumInfo
    {
        const std::wstring pdbPath;
        const std::wstring name;
        const std::vector<std::pair<std::wstring,std::wstring>> values; // eg, pairs of {A,1}, {B,2}, etc.
    public:
        EnumInfo(IDiaSymbol* sym, const std::wstring& pdbPath) : pdbPath(pdbPath)
                                                               , name(BstrToWstr(Get(sym, &IDiaSymbol::get_name)))
                                                               , values(MakeVectorOfEnumValues(sym))
        {}
        void Print() const
        {
            std::wcout << L"  [" << pdbPath << L"]\n";
            std::wcout << L"    kind=enum\n";
            std::wcout << L"    the first few values are: ";
            
            for(size_t i=0; i<min(values.size(),10); ++i)
                std::wcout << values[i].first << L"=" << values[i].second << L", ";
            std::wcout << L'\n';
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }

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

    class Cop
    {
        std::map<std::wstring, std::vector<UdtInfo >>  udtMap; // user-defined types
        std::map<std::wstring, std::vector<EnumInfo>> enumMap; // enums
        std::map<std::wstring, std::vector<FuncInfo>> funcMap; // functions

    public:
        Cop() { CoInitialize(nullptr); }
       ~Cop() { CoUninitialize(); }

        HRESULT LoadPdb(const std::wstring& path)
        {
            HRESULT hr;

            CComPtr<IDiaDataSource> source;
            if (SUCCEEDED(hr = CoCreateInstance(__uuidof(DiaSource), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), reinterpret_cast<void**>(&source))))
            {
                if (SUCCEEDED(hr = source->loadDataFromPdb(path.c_str())))
                {
                    CComPtr<IDiaSession> session;
                    if (SUCCEEDED(hr = source->openSession(&session)))
                    {
                        CComPtr<IDiaSymbol> global;
                        if (SUCCEEDED(hr = session->get_globalScope(&global)))
                        {
                            // UDTs
                            CComPtr<IDiaEnumSymbols> udts;
                            if (SUCCEEDED(hr = global->findChildren(SymTagUDT, NULL, nsNone, &udts)))
                            {
                                while(true)
                                {
                                    ULONG fetched = 0;
                                    CComPtr<IDiaSymbol> udt;
                                    if (FAILED(udts->Next(1, &udt, &fetched)) || fetched == 0)
                                        break;

                                    CComBSTR name;
                                    if (SUCCEEDED(udt->get_name(&name)) && name && name[0] != L'\0')
                                    {
                                        std::wstring key = BuildUdtKey(udt);
                                        udtMap[key].push_back(UdtInfo(udt, path));
                                    }
                                }
                            }

                            // enums
                            CComPtr<IDiaEnumSymbols> enums;
                            if (SUCCEEDED(hr = global->findChildren(SymTagEnum, NULL, nsNone, &enums)))
                            {
                                while (true)
                                {
                                    ULONG fetched = 0;
                                    CComPtr<IDiaSymbol> sym;
                                    if (FAILED(enums->Next(1, &sym, &fetched)) || fetched == 0)
                                        break;

                                    CComBSTR name;
                                    if (SUCCEEDED(sym->get_name(&name)) && name && name[0] != L'\0')
                                    {
                                        std::wstring key(name);
                                        enumMap[key].push_back(EnumInfo(sym, path));
                                    }
                                }
                            }

                            // Functions
                            COFF::Read(path, funcMap);

                        } else std::wcerr <<                  L"get_globalScope failed with 0x" << std::hex << hr << std::dec << L'\n';
                    }     else std::wcerr <<                      L"openSession failed with 0x" << std::hex << hr << std::dec << L'\n';
                }         else std::wcerr << L"loadDataFromPdb failed: " << path << L" with 0x" << std::hex << hr << std::dec << L'\n';
            }             else std::wcerr <<  L"CoCreateInstance(DiaSource) failed for with 0x" << std::hex << hr << std::dec << L'\n';
            return hr;
        }
        int ReportViolations() const
        {
            int violationCount = 0;
            for (auto& [name, definitions] : udtMap)
            {
                if (definitions.size() < 2)
                    continue;

                // Check whether all definitions are identical
                if (std::all_of(definitions.begin()+1, definitions.end(), [&](const auto& d) { return d == definitions[0]; }))
                    continue;

                ++violationCount;
                std::wcout << L"ODR VIOLATION: " << name << L'\n';

                // Group identical definitions together so output is readable when there are many PDBs.
                std::vector<bool> printed(definitions.size(), false);
                for (size_t i=0; i<definitions.size(); ++i)
                {
                    if (printed[i]) continue;
                    definitions[i].Print();
                    printed[i] = true;
                
                    // Mark all later ones that match this one
                    for (size_t j=i+1; j<definitions.size(); ++j)
                    {
                        if (!printed[j] && (definitions[i] == definitions[j]))
                        {
                            definitions[j].PrintPdbPath();
                            printed[j] = true;
                        }
                    }
                }
                std::wcout << L'\n';
            }

            for (auto& [funcName, funcInfos] : funcMap)
            {
                if (funcInfos.size() < 2)
                    continue;

                // Check whether all definitions are identical
                if (std::all_of(funcInfos.begin() + 1, funcInfos.end(), [&](const auto& fi) { return fi == funcInfos[0]; }))
                    continue;

                ++violationCount;
                std::wcout << L"ODR VIOLATION: " << funcName << L'\n';

                // Group identical funcInfos together so output is readable when there are many PDBs.
                std::vector<bool> printed(funcInfos.size(), false);
                for (size_t i=0; i< funcInfos.size(); ++i)
                {
                    if (printed[i]) continue;
                    funcInfos[i].Print();
                    printed[i] = true;
                
                    // Mark all later ones that match this one
                    for (size_t j=i+1; j< funcInfos.size(); ++j)
                    {
                        if (!printed[j] && (funcInfos[i] == funcInfos[j]))
                        {
                            funcInfos[j].PrintCompilandPath();
                            printed[j] = true;
                        }
                    }
                }
                std::wcout << L'\n';
            }

            for (auto& [enumName, enumInfos] : enumMap)
            {
                if (enumInfos.size() < 2)
                    continue;

                // Check whether all definitions are identical
                if (std::all_of(enumInfos.begin() + 1, enumInfos.end(), [&](const auto& ei) { return ei == enumInfos[0]; }))
                    continue;

                ++violationCount;
                std::wcout << L"ODR VIOLATION: " << enumName << L'\n';

                // Group identical funcInfos together so output is readable when there are many PDBs.
                std::vector<bool> printed(enumInfos.size(), false);
                for (size_t i=0; i<enumInfos.size(); ++i)
                {
                    if (printed[i]) continue;
                    enumInfos[i].Print();
                    printed[i] = true;

                    // Mark all later ones that match this one
                    for (size_t j=i+1; j<enumInfos.size(); ++j)
                    {
                        if (!printed[j] && (enumInfos[i] == enumInfos[j]))
                        {
                            printed[j] = true;
                        }
                    }
                }
                std::wcout << L'\n';
            }

            return violationCount;
        }

    private:
        static std::wstring BuildUdtKey(IDiaSymbol* sym)
        {
            CComBSTR name;
            sym->get_name(&name);
            std::wstring key(name ? name.m_str : L"");

            if (key.find(L"<unnamed") != std::wstring::npos)
            {
                /*
                The problem being solved here is when there is more than nested one unnamed struct/class/union
                DIA just calls them "<unnamed" but then two different ones would collide.  For example:
                struct tagDEC {
                    WORD wReserved;
                    union {
                        struct { BYTE scale; BYTE sign; };
                        USHORT signscale;
                    };
                    ULONG Hi32;
                    union {
                        struct { ULONG Lo32; ULONG Mid32; };
                        ULONGLONG Lo64;
                    };
                };
                tagDEC contains two unnamed unions, each containing an unnamed struct.
                Without this fixup code, the first struct, tagDEC::<unnamed-tag>::<unnamed-rag> which is 2 bytes long,
                would collide with the second struct, having exactly the same "name"           but being 8 bytes long.

                The solution is to append the type and size to the end of the same, thus making them unique.

                N.B.: it could still fail, if the two types were the same size. Appending the name of the first data-member would solve this last problem.
                */

                ULONGLONG size = Get(sym, &IDiaSymbol::get_length);
                DWORD     kind = Get(sym, &IDiaSymbol::get_udtKind);
                key += L"[" + std::wstring(kind == UdtUnion  ? L"union" :
                                           kind == UdtClass  ? L"class" :
                                                               L"struct") + L"]";
                key += L"[size=" + std::to_wstring(size)                  + L"]";
            }
            return key;
        }
    };
}