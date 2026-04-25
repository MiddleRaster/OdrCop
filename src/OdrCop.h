#pragma once

#include <windows.h>
#include <dia2.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <atlbase.h>

namespace {

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

    class UdtInfo
    {
        class MemberInfo
        {
            const std::wstring name;
            const std::wstring typeName;   // resolved recursively
            const DWORD        offset;     // byte offset within UDT
            const ULONGLONG    bitSize;    // 0 means "not a bitfield"
            const DWORD        bitPos;     // valid only when bitSize != 0
        public:
            MemberInfo(const std::wstring& name, IDiaSymbol* pType, DWORD offset, ULONGLONG bits, DWORD bitPos) : name(name), typeName(resolveTypeName(pType)), offset(offset), bitSize(bits), bitPos(bitPos) {}
            void Print() const
            {
                std::wcout << L"    +" << offset << L"  " << typeName << L"  " << name;
                if (bitSize)
                    std::wcout << L"  :" << bitSize << L"@bit" << bitPos;
                std::wcout << L'\n';
            }
            bool IsEqualTo(const MemberInfo& other) const
            {
                if (name     != other.name)   return false;
                if (typeName != other.typeName)
                {
                    if (isAnonymous(typeName) && isAnonymous(other.typeName))
                        return true; // treat as equal
                    return false;
                }
                if (offset  != other.offset )   return false;
                if (bitSize != other.bitSize)   return false;
                if (bitSize && 
                    bitPos  != other.bitPos )   return false;
                return true;
            }
            static std::vector<MemberInfo> MakeSortedCopy(std::vector<MemberInfo>& members)
            {
                std::vector<size_t> idx(members.size());
                std::iota(idx.begin(), idx.end(), 0);
                std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return members[a].offset < members[b].offset; });

                std::vector<MemberInfo> sorted;
                sorted.reserve(members.size());
                for (size_t i : idx)
                    sorted.push_back(members[i]);
                return sorted;
            }
            
            friend bool operator==(const MemberInfo& a, const MemberInfo& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const MemberInfo& a, const MemberInfo& b) { return !a.IsEqualTo(b); }

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

        const std::wstring              pdbPath;
        const std::wstring              name;
        const ULONGLONG                 size;         // total size in bytes
        const UdtKind                   udtKind;      // UdtStruct / UdtClass / UdtUnion
        const std::vector<MemberInfo>   members;      // data members in offset order
        const std::vector<std::wstring> baseNames;    // base class names in order
    public:
        UdtInfo(IDiaSymbol* sym, const std::wstring& pdbPath) : pdbPath  (pdbPath)
                                                              , name(             BstrToWstr(Get(sym, &IDiaSymbol::get_name)))
                                                              , size(                        Get(sym, &IDiaSymbol::get_length))
                                                              , udtKind(static_cast<UdtKind>(Get(sym, &IDiaSymbol::get_udtKind)))
                                                              , members  (GetMembers  (sym))
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
            for (auto& m : members)
            {
                m.Print();
            }
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }

        friend bool operator==(const UdtInfo& a, const UdtInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const UdtInfo& a, const UdtInfo& b) { return !a.IsEqualTo(b); }

    private:
        bool IsEqualTo(const UdtInfo& other) const
        {
            if (size           != other.size)           return false;
            if (udtKind        != other.udtKind)        return false;
            if (baseNames      != other.baseNames)      return false;
            if (members.size() != other.members.size()) return false;
            return std::ranges::equal(members, other.members);
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
        static std::vector<MemberInfo> GetMembers(IDiaSymbol* sym)
        {
            std::vector<MemberInfo> members;

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
                    if (dataKind == DataIsMember)
                    {
                        members.push_back(MemberInfo(BstrToWstr(Get(child, &IDiaSymbol::get_name)),
                                                                Get(child, &IDiaSymbol::get_type),
                                                                Get(child, &IDiaSymbol::get_offset),
                                                                Get(child, &IDiaSymbol::get_length),
                                                                Get(child, &IDiaSymbol::get_bitPosition)));
                    }
                }
            }
            return MemberInfo::MakeSortedCopy(members);
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
}

class OdrCop
{
    std::map<std::wstring, std::vector<UdtInfo>> map;

public:
    OdrCop() { CoInitialize(nullptr); }
   ~OdrCop() { CoUninitialize(); }

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
                        CComPtr<IDiaEnumSymbols> udts;
                        if (SUCCEEDED(hr = global->findChildren(SymTagUDT, NULL, nsNone, &udts)))
                        {
                            ULONG fetched = 0;
                            CComPtr<IDiaSymbol> udt;
                            while (SUCCEEDED(udts->Next(1, &udt, &fetched)) && fetched == 1)
                            {   // Skip anonymous/unnamed UDTs
                                CComBSTR name;
                                if (SUCCEEDED(udt->get_name(&name)) && name && name[0] != L'\0') {
                                    std::wstring key(name);
                                    // Skip compiler-generated internal types
                                    if (key.find(L'<') == std::wstring::npos && key.find(L"lambda") == std::wstring::npos)
                                        map[key].push_back(UdtInfo(udt, path));
                                }
                                udt.Release();
                            }
                        }
                    } else std::wcerr <<                  L"get_globalScope failed with 0x" << std::hex << hr << std::dec << L'\n';
                }     else std::wcerr <<                      L"openSession failed with 0x" << std::hex << hr << std::dec << L'\n';
            }         else std::wcerr << L"loadDataFromPdb failed: " << path << L" with 0x" << std::hex << hr << std::dec << L'\n';
        }             else std::wcerr <<  L"CoCreateInstance(DiaSource) failed for with 0x" << std::hex << hr << std::dec << L'\n';
        return hr;
    }
    int ReportViolations() const
    {
        int violationCount = 0;
        for (auto& [name, definitions] : map)
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
        return violationCount;
    }
};

