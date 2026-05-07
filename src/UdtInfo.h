#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include <string>
#include <vector>
#include <algorithm>

#include "DiaGetters.h"

namespace Odr
{
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
            std::wstring GetName() const { return name; }

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
                    std::wcout << L"  : " << bitSize << L" bits at bit " << bitPos;
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
         // const DWORD locationType; // will be LocationType::LocIsStatic OR LocationType::LocIsConstant, which looks like a difference but isn't
            const BOOL  isConstant;
            const BOOL  isVolatile;
        public:
            StaticMember(std::wstring name, IDiaSymbol* pType)
                : MemberInfoBase(name,pType)
                , isConstant    (GetFromType(pType, &IDiaSymbol::get_constType))
                , isVolatile    (GetFromType(pType, &IDiaSymbol::get_volatileType))
            {}
        private:
            friend MemberInfoBase<StaticMember>;
            bool IsEqualToImpl(const StaticMember& other) const
            {
                if (isConstant == other.isConstant)
                if (isVolatile == other.isVolatile)
                    return true;
                return false;
            }
            void PrintPrefix() const { std::wcout << L"    static " << (isConstant ? L"const " : L"") << (isVolatile ? L"volatile" : L""); };
            void PrintSuffix() const { std::wcout << L'\n'; }
            static BOOL GetFromType(IDiaSymbol* sym, HRESULT(IDiaSymbol::* m)(BOOL*))
            {
                CComPtr<IDiaSymbol> type;
                if (SUCCEEDED(sym->get_type(&type)))
                    return Get(type, m);
                return FALSE;
            }
        };

        class MethodInfo
        {
            const std::wstring name;
            const bool isVirtual;
        public:
            MethodInfo(const std::wstring& name, bool isVirtual) : name(name), isVirtual(isVirtual) {}
            void Print() const { std::wcout << L"      " << (isVirtual ? L"virtual " : L"") << name << L'\n'; }
            static std::vector<MethodInfo> MakeSortedCopy(std::vector<MethodInfo> methods)
            {   // all this to avoid removing 'const' from my data-members
                std::vector<size_t> indices(methods.size());
                std::iota(indices.begin(), indices.end(), 0);
                std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) { return methods[a].name < methods[b].name; });

                std::vector<MethodInfo> result;
                result.reserve(methods.size());
                for (size_t i : indices)
                    result.push_back(methods[i]);   // copy-constructs, no assignment needed
                return result;
            }
            friend bool operator==(const MethodInfo& a, const MethodInfo& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const MethodInfo& a, const MethodInfo& b) { return !a.IsEqualTo(b); }
        private:
            bool IsEqualTo(const MethodInfo& other) const
            {
                if (     name != other.name     ) return false;
                if (isVirtual != other.isVirtual) return false;
                return true;
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
        const std::pair<std::vector<MethodInfo>,
                        std::vector<MethodInfo>> methodsAndCtors; // method and ctor names
    public:
        UdtInfo(IDiaSymbol* sym, const std::wstring& pdbPath) 
            : pdbPath  (pdbPath)
            , name(             BstrToWstr(Get(sym, &IDiaSymbol::get_name)))
            , size(                        Get(sym, &IDiaSymbol::get_length))
            , udtKind(static_cast<UdtKind>(Get(sym, &IDiaSymbol::get_udtKind)))
            , members  (GetMembers  (sym))
            , baseNames(GetBaseNames(sym))
            , methodsAndCtors(GetMethods(sym, name))
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

            const auto& ctors = std::get<1>(methodsAndCtors);
            if (ctors.size() > 0)
            {
                if (ctors.size() == 1)
                    std::wcout << L"    1 ctor:\n";
                else
                    std::wcout << L"    " << ctors.size() << L" ctors:\n";
                for (auto& c : ctors) c.Print();

            }
            const auto& methods = std::get<0>(methodsAndCtors);
            if (methods.size() > 0)
            {
                if (methods.size() == 1)
                    std::wcout << L"    1 method:\n";
                else 
                    std::wcout << L"    " << methods.size() << L" methods:\n";
                for(auto& m : methods) m.Print();
            }
        }
        void PrintPdbPath() const { std::wcout << L"  [" << pdbPath << L"] (same as above)\n"; }
        std::wstring GetFirstMemberName() const
        {
            if (std::get<0>(members).size() > 0)
            {
                std::wstring smallest = (std::get<0>(members))[0].GetName();
                for (auto& m : std::get<0>(members))
                {
                    if (smallest > m.GetName())
                        smallest = m.GetName();
                }
                return smallest;
            }
            return L"";
        }

        friend bool operator==(const UdtInfo& a, const UdtInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const UdtInfo& a, const UdtInfo& b) { return !a.IsEqualTo(b); }

    private:
        bool IsEqualTo(const UdtInfo& other) const
        {
            if (                        size != other.size                        ) return false;
            if (                     udtKind != other.udtKind                     ) return false;
            if (                   baseNames != other.baseNames                   ) return false;
            if (       std ::get<0>(members) != std::get<0>(other.members)        ) return false;
            if (        std::get<1>(members) != std::get<1>(other.members)        ) return false;
            if (        std::get<2>(members) != std::get<2>(other.members)        ) return false;
         // if (std::get<1>(methodsAndCtors) != std::get<1>(other.methodsAndCtors)) return false; // all ctors may or may not be emitted: C++20 modules have them, TUs may not. Not an ODR violation
            if (std::get<0>(methodsAndCtors) != std::get<0>(other.methodsAndCtors)) return false;
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
                        statics.push_back(StaticMember(BstrToWstr(Get(child, &IDiaSymbol::get_name)), child));

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
        static std::pair<std::vector<MethodInfo>,std::vector<MethodInfo>> GetMethods(IDiaSymbol* parent, const std::wstring& className)
        {   // get method and ctor names attached to this udt, if any
            std::vector<MethodInfo> methods, ctors;

            CComPtr<IDiaEnumSymbols> functions;
            if (SUCCEEDED(parent->findChildren(SymTagFunction, nullptr, nsNone, &functions)) && functions)
            {
                while (true)
                {
                    ULONG retrieved = 0;
                    CComPtr<IDiaSymbol> function;
                    if (FAILED(functions->Next(1, &function, &retrieved)) || retrieved == 0)
                        break;

                    CComBSTR functionName = Get(function, &IDiaSymbol::get_undecoratedName); // prefer this one
                    if (functionName.m_str == NULL)
                        // function->get_name(&functionName); // but use this one if need be
                        functionName = Get(function, &IDiaSymbol::get_name); // but use this one if need be

                    bool isVirtual = Get(function, &IDiaSymbol::get_virtual);

                    if (functionName.m_str != NULL)
                    {
                        // strip off classname from fully qualified method names
                        std::wstring methodName(functionName.m_str);

                        std::wstring prefix = className + L"::";
                        size_t       pos;
                        while ((pos = methodName.find(prefix)) != std::wstring::npos)
                            methodName.erase(pos, prefix.size());

                        struct Trim
                        {
                            /*
                               The following code is to handle a peculiarity of linker-generated .pdb files (which the users are NOT supposed to pass in).
                               What can happen is that when the linker does its work, it ends up reporting that some methods, even virtual ones, are static
                               and even ctors are sometimes marked as static.

                               So, to keep people from thinking my tool is reporting nonsense, I'll trim off the static keyword when:
                               1. the method is virtual
                               2. it's a constructor
                               3. it has a "this" pointer
                            */
                            static std::wstring Static(const std::wstring& aMethodName, bool isVirtualOrCtor, IDiaSymbol* function)
                            {
                                if (!aMethodName.starts_with(L"static "))
                                    return aMethodName;

                                if (isVirtualOrCtor == false)
                                {
                                    CComPtr<IDiaEnumSymbols> children;
                                    function->findChildren(SymTagData, L"this", nsNone, &children);
                                    if (children)
                                    {
                                        LONG count = 0;
                                        children->get_Count(&count);
                                        if (count == 0) // no this pointer => truly static
                                            return aMethodName;
                                    } else
                                        return aMethodName;
                                }

                                std::wstring trimmed(aMethodName);
                                return trimmed.erase(0, 7); // remove leading "static " which is 7 wide characters
                            }
                        };

                        // follow the type to see if this method is a ctor
                        if (TRUE == GetFromType(function, &IDiaSymbol::get_constructor))
                              ctors.push_back({Trim::Static(methodName, true,      function), false});
                        else
                            methods.push_back({Trim::Static(methodName, isVirtual, function), isVirtual});
                    }
                }
            }
            return {MethodInfo::MakeSortedCopy(methods), ctors};
        }
        static BOOL GetFromType(IDiaSymbol* sym, HRESULT(IDiaSymbol::* m)(BOOL*))
        {
            CComPtr<IDiaSymbol> type;
            if (SUCCEEDED(sym->get_type(&type)))
                return Get(type, m);
            return FALSE;
        }
    };
}
