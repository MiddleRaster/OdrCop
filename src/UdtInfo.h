#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include <string>
#include <vector>
#include <algorithm>

#include "DiaGetters.h"
#include "AnonInfo.h"

namespace Odr
{
    static std::wstring Indent(int depth) { return std::wstring(depth * 4, L' '); }

    class UdtInfo : public AnonInfo
    {
        template <typename Derived>
        class MemberInfoBase
        {
            const std::wstring       name;
            const std::wstring       typeName;
            std::shared_ptr<UdtInfo> nestedUdt; // populated only for anonymous-namespace UDT members
        public:
            MemberInfoBase(IDiaSession* session, const std::wstring& name, IDiaSymbol* sym, const std::wstring& pdbPath)
                : name(name)
                , typeName (resolveTypeName (Get(sym, &IDiaSymbol::get_type)))
                , nestedUdt(resolveNestedUdt(session, Get(sym, &IDiaSymbol::get_type), pdbPath))
            {}
            MemberInfoBase(      MemberInfoBase&&) = default;
            MemberInfoBase(const MemberInfoBase &) = default;
            bool IsEqualTo(const Derived& other) const
            {
                if (name     != other.name    ) return false;
                if (typeName != other.typeName)
                    if (!(isAnonymous(typeName) && isAnonymous(other.typeName)))
                        return false;

                // If both sides have a nested UDT, compare it structurally
                if     ( nestedUdt ||  other.nestedUdt) {
                    if (!nestedUdt || !other.nestedUdt)
                        return false;
                    if (*nestedUdt != *other.nestedUdt)
                        return false;
                }
                return static_cast<const Derived*>(this)->IsEqualToImpl(other);
            }
            void Print(int depth) const
            {
                std::wstring indent = Indent(depth);

                static_cast<const Derived*>(this)->PrintPrefix(depth);
                std::wcout << L" " << typeName << L" " << name;
                static_cast<const Derived*>(this)->PrintSuffix(depth);

                if (nestedUdt)
                    nestedUdt->Print(depth+1);
            }
            std::wstring GetName() const { return name; }
            template <typename Fn> void CollectSubItems(Fn&& fn) const
            {
                if (nestedUdt)
                    fn(nestedUdt->GetName(), *nestedUdt);
            }

            friend bool operator==(const Derived& a, const Derived& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const Derived& a, const Derived& b) { return !a.IsEqualTo(b); }
        private:
            static std::shared_ptr<UdtInfo> resolveNestedUdt(IDiaSession* session, IDiaSymbol* type, const std::wstring& pdbPath)
            {
                if (!type || !session)
                    return nullptr;

                if (SymTagUDT != static_cast<enum SymTagEnum>(Get(type, &IDiaSymbol::get_symTag)))
                    return nullptr;

                // Only recurse into anonymous-namespace UDTs;
                std::wstring name = BstrToWstr(Get(type, &IDiaSymbol::get_name));
                if (name.find(L"anonymous-namespace'") == std::wstring::npos)
                    return nullptr;

                // Check if this is already the defining symbol
                if (!Get(type, &IDiaSymbol::get_exportIsForwarder))
                    return std::make_shared<UdtInfo>(true, session, type, pdbPath, name);

                // It's a forwarder — search the lexical parent scope for the defining symbol
                DWORD lexParentId = Get(type, &IDiaSymbol::get_lexicalParentId);
                CComPtr<IDiaSymbol> lexParent;
                if (FAILED(session->symbolById(lexParentId, &lexParent)) || !lexParent)
                    return nullptr;

                // trying tail end of the name as the name
                std::wstring tail = name;
                size_t pos = name.rfind(L"::");
                if (pos != std::wstring::npos)
                    tail = name.substr(pos + 2);

                CComPtr<IDiaEnumSymbols> found;
                if (FAILED(lexParent->findChildren(SymTagUDT, /*name*/ tail.c_str(), nsCaseSensitive, &found)) || !found)
                    return nullptr;
                while (true)
                {
                    ULONG fetched = 0;
                    CComPtr<IDiaSymbol> candidate;
                    if (FAILED(found->Next(1, &candidate, &fetched)) || fetched == 0)
                        break;

                    if (!Get(candidate, &IDiaSymbol::get_exportIsForwarder))
                        return std::make_shared<UdtInfo>(true, session, candidate, pdbPath, name);
                }
                return nullptr;
            }

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

        struct ToStringBase
        {
            static std::wstring ToString(CV_access_e access)
            {
                switch(access)
                {
                case CV_private  : return L"private";   break;
                case CV_protected: return L"protected"; break;
                case CV_public   : return L"public";    break;
                default          : return L"impossible access type"; break;
                }
            }
        };
        class InstanceMember : public MemberInfoBase<InstanceMember>, private ToStringBase
        {
            const LONG        offset;    // byte offset within UDT
            const ULONGLONG   bitSize;   // 0 means "not a bitfield"
            const DWORD       bitPos;    // valid only when bitSize != 0
            const BOOL        bConst;    // is "const"
            const BOOL        bVolatile; // is "volatile"
            const CV_access_e access;    // private/protected/public

            InstanceMember(IDiaSession* session, IDiaSymbol* child, const std::wstring& name, const std::wstring& pdbPath, LONG offset, ULONGLONG bitSize,   DWORD bitPos,    BOOL bConst,       BOOL bVolatile, CV_access_e access)
                : MemberInfoBase(session, name, child, pdbPath)
                , offset   (offset)
                , bitSize  (bitSize)
                , bitPos   (bitPos)
                , bConst   (bConst)
                , bVolatile(bVolatile)
                , access   (access)
            {}
        public:
            InstanceMember(      InstanceMember&&) = default;
            InstanceMember(const InstanceMember &) = default;
            static InstanceMember Make(IDiaSession* session, IDiaSymbol* child, const std::wstring& pdbPath)
            {
                auto name          =               BstrToWstr(Get(child, &IDiaSymbol::get_name));
                auto offset        =                          Get(child, &IDiaSymbol::get_offset);
                auto bitSize       =                          Get(child, &IDiaSymbol::get_length);
                auto bitPos        =                          Get(child, &IDiaSymbol::get_bitPosition);
                BOOL bConst        =                  GetFromType(child, &IDiaSymbol::get_constType);
                BOOL bVolatile     =                  GetFromType(child, &IDiaSymbol::get_volatileType);
                CV_access_e access = static_cast<CV_access_e>(Get(child, &IDiaSymbol::get_access));

                return InstanceMember{session, child, name, pdbPath, offset, bitSize, bitPos, bConst, bVolatile, access};
            }
            static std::vector<InstanceMember> MakeSortedCopy(std::vector<InstanceMember>& members)
            {
                std::vector<size_t> idx(members.size());
                std::iota(idx.begin(), idx.end(), 0);
                std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return members[a].offset < members[b].offset; });

                std::vector<InstanceMember> sorted;
                sorted.reserve(members.size());
                for (size_t i : idx)
                    sorted.push_back(std::move(members[i]));
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
            void PrintPrefix(int   depth  ) const { std::wcout << Indent(depth) << L"    +" << offset << L' ' << ToString(access) << (bConst ? L" const" : L"") << (bVolatile ? L" volatile" : L""); }
            void PrintSuffix(int /*depth*/) const
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
                if (access    != other.access   ) return false;
                return true;
            }
        };

        // does not work. Yet.
        class ConstantMember : public MemberInfoBase<ConstantMember>
        {
            std::wstring constValue;
        public:
            ConstantMember(IDiaSession* session, std::wstring name, IDiaSymbol* pType, const std::wstring& pdbPath, std::wstring constValue)
                : MemberInfoBase(session, name,pType,pdbPath)
                , constValue(constValue)
            {}
            ConstantMember(      ConstantMember&&) = default;
            ConstantMember(const ConstantMember &) = default;
        private:
            friend MemberInfoBase<ConstantMember>;

            bool IsEqualToImpl(const ConstantMember& other) const { return constValue == other.constValue; }
            void PrintPrefix(int   depth  ) const { std::wcout << Indent(depth) << L"    constexpr/const static value  "; }
            void PrintSuffix(int /*depth*/) const { std::wcout << L'\n'; }
        };
        class StaticMember : public MemberInfoBase<StaticMember>
        {
         // const DWORD locationType; // will be LocationType::LocIsStatic OR LocationType::LocIsConstant, which looks like a difference but isn't
            const BOOL  isConstant;
            const BOOL  isVolatile;
        public:
            StaticMember(IDiaSession* session, const std::wstring& name, IDiaSymbol* pType, const std::wstring& pdbPath)
                : MemberInfoBase(session, name,pType,pdbPath)
                , isConstant    (GetFromType(pType, &IDiaSymbol::get_constType))
                , isVolatile    (GetFromType(pType, &IDiaSymbol::get_volatileType))
            {}
            StaticMember(      StaticMember&&) = default;
            StaticMember(const StaticMember &) = default;
        private:
            friend MemberInfoBase<StaticMember>;
            bool IsEqualToImpl(const StaticMember& other) const
            {
                if (isConstant == other.isConstant)
                if (isVolatile == other.isVolatile)
                    return true;
                return false;
            }
            void PrintPrefix(int   depth  ) const { std::wcout << Indent(depth) << L"    static " << (isConstant ? L"const " : L"") << (isVolatile ? L"volatile" : L""); };
            void PrintSuffix(int /*depth*/) const { std::wcout << L'\n'; }
            static BOOL GetFromType(IDiaSymbol* sym, HRESULT(IDiaSymbol::* m)(BOOL*))
            {
                CComPtr<IDiaSymbol> type;
                if (SUCCEEDED(sym->get_type(&type)))
                    return Get(type, m);
                return FALSE;
            }
        };

        class MethodInfo : private ToStringBase
        {
            const CV_access_e  access;    // private/protected/public
            const bool         isVirtual;
            const bool         isStatic;
            const std::wstring name;
            const bool         isNoExcept;
        public:
            MethodInfo(CV_access_e access,       bool isVirtual,      bool isStatic, const std::wstring& name,        bool isNoExcept)
                          : access(access), isVirtual(isVirtual), isStatic(isStatic),               name(name), isNoExcept(isNoExcept) {}
            void Print(int depth) const
            {
                std::wcout << Indent(depth)
                           << L"      " << ToString(access) << L": " 
                           << (isStatic   ? L"static "  : L"") 
                           << (isVirtual  ? L"virtual " : L"")
                           << name                          << L" "
                           << (isNoExcept ? L"noexcept ": L"")
                           << L'\n'; 
            }
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
            template <typename Fn> void CollectSubItems(Fn&&) const {}

            friend bool operator==(const MethodInfo& a, const MethodInfo& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const MethodInfo& a, const MethodInfo& b) { return !a.IsEqualTo(b); }
        private:
            bool IsEqualTo(const MethodInfo& other) const
            {
                if (   access   != other.access    ) return false;
                if (isVirtual   != other.isVirtual ) return false;
                if ( isStatic   != other.isStatic  ) return false;
                if (     name   != other.name      ) return false;
                if ( isNoExcept != other.isNoExcept) return false;
                return true;
            }
        };

        class BaseInfo : private ToStringBase
        {
            const std::wstring       name;
            const CV_access_e        access;    // private/protected/public
            const bool               isVirtual;
            std::shared_ptr<UdtInfo> nestedUdt; // populated only for anonymous-namespace bases
        public:
            BaseInfo(IDiaSession* session, const std::wstring& name, CV_access_e access, bool isVirtual, IDiaSymbol* baseType, const std::wstring& pdbPath)
                : name(name)
                , access(access)
                , isVirtual(isVirtual)
                , nestedUdt(resolveNestedUdt(session, baseType, pdbPath))
            {}
            BaseInfo(      BaseInfo&&) = default;
            BaseInfo(const BaseInfo &) = default;
            void Print(int depth) const
            {
                std::wcout << Indent(depth) << ToString(access) << L" " << (isVirtual ? L"virtual " : L"") << name << L'\n';
                if (nestedUdt)
                    nestedUdt->Print(depth);
            }
            template <typename Fn> void CollectSubItems(Fn&& fn) const
            {
                if (nestedUdt)
                    fn(nestedUdt->GetName(), *nestedUdt);
            }

            friend bool operator==(const BaseInfo& a, const BaseInfo& b) { return  a.IsEqualTo(b); }
            friend bool operator!=(const BaseInfo& a, const BaseInfo& b) { return !a.IsEqualTo(b); }
        private:
            bool IsEqualTo(const BaseInfo& other) const
            {
                if (     name != other.name     ) return false;
                if (   access != other.access   ) return false;
                if (isVirtual != other.isVirtual) return false;

                if     ( nestedUdt ||  other.nestedUdt) {
                    if (!nestedUdt || !other.nestedUdt)
                        return false;
                    if (*nestedUdt != *other.nestedUdt)
                        return false;
                }
                return true;
            }
        private:
            static std::shared_ptr<UdtInfo> resolveNestedUdt(IDiaSession* session, IDiaSymbol* type, const std::wstring& pdbPath)
            {
                if (!type)
                    return nullptr;

                CComBSTR n;
                type->get_name(&n);
                std::wstring name = BstrToWstr(n);
                if (name.find(L"`anonymous-namespace'") == std::wstring::npos)
                    return nullptr;

                return std::make_shared<UdtInfo>(true, session, type, pdbPath, name);
            }
        };

        const std::wstring                 pdbPath;
        const std::wstring                 name;
        const ULONGLONG                    size;      // total size in bytes
        const UdtKind                      udtKind;   // UdtStruct / UdtClass / UdtUnion
              std::tuple<
              std::vector<InstanceMember>,            // data members in offset order
              std::vector<ConstantMember>,            // constexpr/const static values
              std::vector<StaticMember >>             // static/constinit/consteval values
                                           members;
              std::vector<BaseInfo>        bases;     // base class names+access in order
        const std::pair<std::vector<MethodInfo>,
                        std::vector<MethodInfo>> methodsAndCtors; // method and ctor names
    public:
        UdtInfo(bool b, IDiaSession* session, IDiaSymbol* sym, const std::wstring& pdbPath, const std::wstring& name)
            : AnonInfo(b)
            , pdbPath(pdbPath)
            , name   (name)
            , size   (                     Get(sym, &IDiaSymbol::get_length))
            , udtKind(static_cast<UdtKind>(Get(sym, &IDiaSymbol::get_udtKind)))
            , members(     GetMembers(session, sym, pdbPath))
            , bases  (    GetBaseInfo(session, sym, pdbPath))
            , methodsAndCtors(      GetMethods(sym, name))
        {}
        UdtInfo(      UdtInfo&&) = default;
        UdtInfo(const UdtInfo &) = default;
        void Print(int depth) const
        {
            std::wstring indent = Indent(depth);
            if (depth == 0)
                std::wcout << indent << L"  [" << pdbPath << L"]\n";

            std::wcout <<indent << L"    kind=" << UdtKindToString() << L"  size=" << size << L'\n';
            if (!bases.empty())
            {
                std::wcout << indent << L"    bases:\n";
                for(auto i=0; i<bases.size(); ++i)
                    bases[i].Print(depth+2);
            }
            for (auto& i : std::get<0>(members)) i.Print(depth);
            for (auto& c : std::get<1>(members)) c.Print(depth);
            for (auto& s : std::get<2>(members)) s.Print(depth);

            const auto& ctors = std::get<1>(methodsAndCtors);
            if (ctors.size() > 0)
            {
                if (ctors.size() == 1)
                    std::wcout << indent << L"    1 ctor:\n";
                else
                    std::wcout << indent << L"    " << ctors.size() << L" ctors:\n";
                for (auto& c : ctors) c.Print(depth);

            }
            const auto& methods = std::get<0>(methodsAndCtors);
            if (methods.size() > 0)
            {
                if (methods.size() == 1)
                    std::wcout << indent << L"    1 method:\n";
                else 
                    std::wcout << indent << L"    " << methods.size() << L" methods:\n";
                for(auto& m : methods) m.Print(depth);
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

        std::wstring GetName() const { return name; }
        template <typename Fn> void CollectSubItems(Fn&& fn) const
        {
            for(auto& b : bases                       ) b.CollectSubItems(fn);
            for(auto& m : std::get<0>(members)        ) m.CollectSubItems(fn);
            for(auto& m : std::get<1>(members)        ) m.CollectSubItems(fn);
            for(auto& m : std::get<2>(members)        ) m.CollectSubItems(fn);
            for(auto& m : std::get<0>(methodsAndCtors)) m.CollectSubItems(fn);
            for(auto& c : std::get<1>(methodsAndCtors)) c.CollectSubItems(fn);
        }

        friend bool operator==(const UdtInfo& a, const UdtInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const UdtInfo& a, const UdtInfo& b) { return !a.IsEqualTo(b); }

    private:
        bool IsEqualTo(const UdtInfo& other) const
        {
            if (                        size != other.size                        ) return false;
            if (                     udtKind != other.udtKind                     ) return false;
            if (                       bases != other.bases                       ) return false;
            if (       std ::get<0>(members) != std::get<0>(other.members)        ) return false;
            if (        std::get<1>(members) != std::get<1>(other.members)        ) return false;
            if (        std::get<2>(members) != std::get<2>(other.members)        ) return false;
            if (std::get<1>(methodsAndCtors) != std::get<1>(other.methodsAndCtors)) return false;
            
            // the previous line was commented out with this comment:  // all ctors may or may not be emitted: C++20 modules have them, TUs may not. Not an ODR violation
            // This might be wrong. Will try ignore noexcept when compiler-generated

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

        static std::tuple<std::vector<InstanceMember>, std::vector<ConstantMember>, std::vector<StaticMember>> GetMembers(IDiaSession* session, IDiaSymbol* sym, const std::wstring& pdbPath)
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
                        members.push_back(InstanceMember::Make(session, child, pdbPath));
                    }
                    else if (DataIsStaticMember == static_cast<DataKind>(dataKind))
                    {
                        statics.push_back(StaticMember(session, BstrToWstr(Get(child, &IDiaSymbol::get_name)), child, pdbPath));
                    } else {
                        // add other DataKind types
                        continue; // here only so I can put a breakpoint on it
                    }
                }
            }
            return {std::move(InstanceMember::MakeSortedCopy(members)), std::move(constants), std::move(statics)};
        }
        static std::vector<ConstantMember> GetConstantMembers(IDiaSession* session, IDiaSymbol* sym, const std::wstring& pdbPath)
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
                        members.push_back(ConstantMember(session,
                                                         BstrToWstr(Get(child, &IDiaSymbol::get_name)),
                                                                    Get(child, &IDiaSymbol::get_type ),
                                                                    pdbPath,
                                                                    Get(child, &IDiaSymbol::get_value)));
                    }
                }
            }
            return members;
        }
        static std::vector<BaseInfo> GetBaseInfo(IDiaSession* session, IDiaSymbol* sym, const std::wstring& pdbPath)
        {
            std::vector<BaseInfo> baseInfos;

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
                        baseInfos.emplace_back(BaseInfo(session,
                                                        BstrToWstr(Get(baseType, &IDiaSymbol::get_name)),
                                          static_cast<CV_access_e>(Get(base,     &IDiaSymbol::get_access)),
                                                                (!!Get(base,     &IDiaSymbol::get_virtualBaseClass)),
                                                                       baseType,
                                                                       pdbPath));
                    }
                }
            }
            return baseInfos;
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

                    CComBSTR functionName  = Get(function, &IDiaSymbol::get_undecoratedName); // prefer this one
                    if (functionName.m_str == NULL)
                        functionName       = Get(function, &IDiaSymbol::get_name); // but use this one if need be

                    bool isVirtual     =                          Get(function, &IDiaSymbol::get_virtual);
                    CV_access_e access = static_cast<CV_access_e>(Get(function, &IDiaSymbol::get_access));
                    bool isStatic      = false;
                    {
                        CComPtr<IDiaSymbol6> function6;
                        function->QueryInterface<IDiaSymbol6>(&function6);
                        if (function6)
                            isStatic   = !!GetN(function6, &IDiaSymbol6::get_isStaticMemberFunc); // could also query type's objectPointerType's nullness
                    }
                    bool isNoExcept = false;
                    {
                        CComPtr<IDiaSymbol> type;
                        function->get_type(&type);
                        if (type)
                        {
                            CComPtr<IDiaSymbol4> function4;
                            type->QueryInterface<IDiaSymbol4>(&function4);
                            if (function4)
                                isNoExcept = !!GetN(function4, &IDiaSymbol4::get_noexcept);
                        }
                    }

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
                              ctors.push_back({access, false,     isStatic, Trim::Static(methodName, true,      function), isNoExcept});
                        else
                            methods.push_back({access, isVirtual, isStatic, Trim::Static(methodName, isVirtual, function), isNoExcept}); // isVirtual and isStatic had better not both be true
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
