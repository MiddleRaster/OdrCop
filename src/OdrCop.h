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
                        continue; // here only so I can put a breakpoint on it
                        //std::wcout << L"in GetInstanceMembers, not a DataIsMember type, but rather " << dataKind << L"\n";

                        //CComVariant val;
                        //HRESULT hr = child->get_value(&val);
                        //std::wcout << L"get_value hr=0x" << std::hex << hr << std::dec << L"  vt=" << val.vt << L'\n';

                        //auto s = Get(child, &IDiaSymbol::get_value);
                        //std::wcout << L"converted get_value to " << s << L'\n';
                        /*
                        auto lt = Get(child, &IDiaSymbol::get_locationType);
                        switch(lt)
                        {
                        case LocationType::LocIsNull            : std::wcout << L"locationType is " << L"LocIsNull            " << L"\n"; break;
                        case LocationType::LocIsStatic          : std::wcout << L"locationType is " << L"LocIsStatic          " << L"\n"; break;
                        case LocationType::LocIsTLS             : std::wcout << L"locationType is " << L"LocIsTLS             " << L"\n"; break;
                        case LocationType::LocIsRegRel          : std::wcout << L"locationType is " << L"LocIsRegRel          " << L"\n"; break;
                        case LocationType::LocIsThisRel         : std::wcout << L"locationType is " << L"LocIsThisRel         " << L"\n"; break;
                        case LocationType::LocIsEnregistered    : std::wcout << L"locationType is " << L"LocIsEnregistered    " << L"\n"; break;
                        case LocationType::LocIsBitField        : std::wcout << L"locationType is " << L"LocIsBitField        " << L"\n"; break;
                        case LocationType::LocIsSlot            : std::wcout << L"locationType is " << L"LocIsSlot            " << L"\n"; break;
                        case LocationType::LocIsIlRel           : std::wcout << L"locationType is " << L"LocIsIlRel           " << L"\n"; break;
                        case LocationType::LocInMetaData        : std::wcout << L"locationType is " << L"LocInMetaData        " << L"\n"; break;
                        case LocationType::LocIsConstant        : std::wcout << L"locationType is " << L"LocIsConstant        " << L"\n"; break;
                        case LocationType::LocIsRegRelAliasIndir: std::wcout << L"locationType is " << L"LocIsRegRelAliasIndir" << L"\n"; break;
                        case LocationType::LocTypeMax           : std::wcout << L"locationType is " << L"LocTypeMax           " << L"\n"; break;
                        };
                        */
#ifdef KEEP
                            std::wcout << L"get_symIndexId:                        " << Get(child, &IDiaSymbol::get_symIndexId) << L'\n';
                            std::wcout << L"get_symTag:                            " << Get(child, &IDiaSymbol::get_symTag) << L'\n';
                            std::wcout << L"get_name:                              " << Get(child, &IDiaSymbol::get_name) << L'\n';
                            std::wcout << L"get_lexicalParentId:                   " << Get(child, &IDiaSymbol::get_lexicalParentId) << L'\n';
                            std::wcout << L"get_classParentId:                     " << Get(child, &IDiaSymbol::get_classParentId) << L'\n';
                            std::wcout << L"get_typeId:                            " << Get(child, &IDiaSymbol::get_typeId) << L'\n';
                            std::wcout << L"get_dataKind:                          " << Get(child, &IDiaSymbol::get_dataKind) << L'\n';
                            std::wcout << L"get_locationType:                      " << Get(child, &IDiaSymbol::get_locationType) << L'\n';
                            std::wcout << L"get_addressSection:                    " << Get(child, &IDiaSymbol::get_addressSection) << L'\n';
                            std::wcout << L"get_addressOffset:                     " << Get(child, &IDiaSymbol::get_addressOffset) << L'\n';
                            std::wcout << L"get_relativeVirtualAddress:            " << Get(child, &IDiaSymbol::get_relativeVirtualAddress) << L'\n';
                            std::wcout << L"get_virtualAddress:                    " << Get(child, &IDiaSymbol::get_virtualAddress) << L'\n';
                            std::wcout << L"get_registerId:                        " << Get(child, &IDiaSymbol::get_registerId) << L'\n';
                            std::wcout << L"get_offset:                            " << Get(child, &IDiaSymbol::get_offset) << L'\n';
                            std::wcout << L"get_length:                            " << Get(child, &IDiaSymbol::get_length) << L'\n';
                            std::wcout << L"get_slot:                              " << Get(child, &IDiaSymbol::get_slot) << L'\n';
                            std::wcout << L"get_volatileType:                      " << Get(child, &IDiaSymbol::get_volatileType) << L'\n';
                            std::wcout << L"get_constType:                         " << Get(child, &IDiaSymbol::get_constType) << L'\n';
                            std::wcout << L"get_unalignedType:                     " << Get(child, &IDiaSymbol::get_unalignedType) << L'\n';
                            std::wcout << L"get_access:                            " << Get(child, &IDiaSymbol::get_access) << L'\n';
                            std::wcout << L"get_libraryName:                       " << Get(child, &IDiaSymbol::get_libraryName) << L'\n';
                            std::wcout << L"get_platform:                          " << Get(child, &IDiaSymbol::get_platform) << L'\n';
                            std::wcout << L"get_language:                          " << Get(child, &IDiaSymbol::get_language) << L'\n';
                            std::wcout << L"get_editAndContinueEnabled:            " << Get(child, &IDiaSymbol::get_editAndContinueEnabled) << L'\n';
                            std::wcout << L"get_frontEndMajor:                     " << Get(child, &IDiaSymbol::get_frontEndMajor) << L'\n';
                            std::wcout << L"get_frontEndMinor:                     " << Get(child, &IDiaSymbol::get_frontEndMinor) << L'\n';
                            std::wcout << L"get_frontEndBuild:                     " << Get(child, &IDiaSymbol::get_frontEndBuild) << L'\n';
                            std::wcout << L"get_backEndMajor:                      " << Get(child, &IDiaSymbol::get_backEndMajor) << L'\n';
                            std::wcout << L"get_backEndMinor:                      " << Get(child, &IDiaSymbol::get_backEndMinor) << L'\n';
                            std::wcout << L"get_backEndBuild:                      " << Get(child, &IDiaSymbol::get_backEndBuild) << L'\n';
                            std::wcout << L"get_sourceFileName:                    " << Get(child, &IDiaSymbol::get_sourceFileName) << L'\n';
                            std::wcout << L"get_unused:                            " << Get(child, &IDiaSymbol::get_unused) << L'\n';
                            std::wcout << L"get_thunkOrdinal:                      " << Get(child, &IDiaSymbol::get_thunkOrdinal) << L'\n';
                            std::wcout << L"get_thisAdjust:                        " << Get(child, &IDiaSymbol::get_thisAdjust) << L'\n';
                            std::wcout << L"get_virtualBaseOffset:                 " << Get(child, &IDiaSymbol::get_virtualBaseOffset) << L'\n';
                            std::wcout << L"get_virtual:                           " << Get(child, &IDiaSymbol::get_virtual) << L'\n';
                            std::wcout << L"get_intro:                             " << Get(child, &IDiaSymbol::get_intro) << L'\n';
                            std::wcout << L"get_pure:                              " << Get(child, &IDiaSymbol::get_pure) << L'\n';
                            std::wcout << L"get_callingConvention:                 " << Get(child, &IDiaSymbol::get_callingConvention) << L'\n';
                            std::wcout << L"get_value:                             " << Get(child, &IDiaSymbol::get_value) << L'\n';
                            std::wcout << L"get_baseType:                          " << Get(child, &IDiaSymbol::get_baseType) << L'\n';
                            std::wcout << L"get_token:                             " << Get(child, &IDiaSymbol::get_token) << L'\n';
                            std::wcout << L"get_timeStamp:                         " << Get(child, &IDiaSymbol::get_timeStamp) << L'\n';
                            std::wcout << L"get_symbolsFileName:                   " << Get(child, &IDiaSymbol::get_symbolsFileName) << L'\n';
                            std::wcout << L"get_reference:                         " << Get(child, &IDiaSymbol::get_reference) << L'\n';
                            std::wcout << L"get_count:                             " << Get(child, &IDiaSymbol::get_count) << L'\n';
                            std::wcout << L"get_bitPosition:                       " << Get(child, &IDiaSymbol::get_bitPosition) << L'\n';
                            std::wcout << L"get_arrayIndexTypeId:                  " << Get(child, &IDiaSymbol::get_arrayIndexTypeId) << L'\n';
                            std::wcout << L"get_packed:                            " << Get(child, &IDiaSymbol::get_packed) << L'\n';
                            std::wcout << L"get_constructor:                       " << Get(child, &IDiaSymbol::get_constructor) << L'\n';
                            std::wcout << L"get_overloadedOperator:                " << Get(child, &IDiaSymbol::get_overloadedOperator) << L'\n';
                            std::wcout << L"get_nested:                            " << Get(child, &IDiaSymbol::get_nested) << L'\n';
                            std::wcout << L"get_hasNestedTypes:                    " << Get(child, &IDiaSymbol::get_hasNestedTypes) << L'\n';
                            std::wcout << L"get_hasAssignmentOperator:             " << Get(child, &IDiaSymbol::get_hasAssignmentOperator) << L'\n';
                            std::wcout << L"get_hasCastOperator:                   " << Get(child, &IDiaSymbol::get_hasCastOperator) << L'\n';
                            std::wcout << L"get_scoped:                            " << Get(child, &IDiaSymbol::get_scoped) << L'\n';
                            std::wcout << L"get_virtualBaseClass:                  " << Get(child, &IDiaSymbol::get_virtualBaseClass) << L'\n';
                            std::wcout << L"get_indirectVirtualBaseClass:          " << Get(child, &IDiaSymbol::get_indirectVirtualBaseClass) << L'\n';
                            std::wcout << L"get_virtualBasePointerOffset:          " << Get(child, &IDiaSymbol::get_virtualBasePointerOffset) << L'\n';
                            std::wcout << L"get_virtualTableShapeId:               " << Get(child, &IDiaSymbol::get_virtualTableShapeId) << L'\n';
                            std::wcout << L"get_code:                              " << Get(child, &IDiaSymbol::get_code) << L'\n';
                            std::wcout << L"get_function:                          " << Get(child, &IDiaSymbol::get_function) << L'\n';
                            std::wcout << L"get_managed:                           " << Get(child, &IDiaSymbol::get_managed) << L'\n';
                            std::wcout << L"get_msil:                              " << Get(child, &IDiaSymbol::get_msil) << L'\n';
                            std::wcout << L"get_virtualBaseDispIndex:              " << Get(child, &IDiaSymbol::get_virtualBaseDispIndex) << L'\n';
                            std::wcout << L"get_undecoratedName:                   " << Get(child, &IDiaSymbol::get_undecoratedName) << L'\n';
                            std::wcout << L"get_age:                               " << Get(child, &IDiaSymbol::get_age) << L'\n';
                            std::wcout << L"get_signature:                         " << Get(child, &IDiaSymbol::get_signature) << L'\n';
                            std::wcout << L"get_compilerGenerated:                 " << Get(child, &IDiaSymbol::get_compilerGenerated) << L'\n';
                            std::wcout << L"get_addressTaken:                      " << Get(child, &IDiaSymbol::get_addressTaken) << L'\n';
                            std::wcout << L"get_rank:                              " << Get(child, &IDiaSymbol::get_rank) << L'\n';
                            std::wcout << L"get_lowerBoundId:                      " << Get(child, &IDiaSymbol::get_lowerBoundId) << L'\n';
                            std::wcout << L"get_upperBoundId:                      " << Get(child, &IDiaSymbol::get_upperBoundId) << L'\n';
                            std::wcout << L"get_offsetInUdt:                       " << Get(child, &IDiaSymbol::get_offsetInUdt) << L'\n';
                            std::wcout << L"get_paramBasePointerRegisterId:        " << Get(child, &IDiaSymbol::get_paramBasePointerRegisterId) << L'\n';
                            std::wcout << L"get_localBasePointerRegisterId:        " << Get(child, &IDiaSymbol::get_localBasePointerRegisterId) << L'\n';
                            std::wcout << L"get_isLocationControlFlowDependent:    " << Get(child, &IDiaSymbol::get_isLocationControlFlowDependent) << L'\n';
                            std::wcout << L"get_stride:                            " << Get(child, &IDiaSymbol::get_stride) << L'\n';
                            std::wcout << L"get_numberOfRows:                      " << Get(child, &IDiaSymbol::get_numberOfRows) << L'\n';
                            std::wcout << L"get_numberOfColumns:                   " << Get(child, &IDiaSymbol::get_numberOfColumns) << L'\n';
                            std::wcout << L"get_isMatrixRowMajor:                  " << Get(child, &IDiaSymbol::get_isMatrixRowMajor) << L'\n';
                            std::wcout << L"get_isReturnValue:                     " << Get(child, &IDiaSymbol::get_isReturnValue) << L'\n';
                            std::wcout << L"get_isOptimizedAway:                   " << Get(child, &IDiaSymbol::get_isOptimizedAway) << L'\n';
                            std::wcout << L"get_builtInKind:                       " << Get(child, &IDiaSymbol::get_builtInKind) << L'\n';
                            std::wcout << L"get_registerType:                      " << Get(child, &IDiaSymbol::get_registerType) << L'\n';
                            std::wcout << L"get_baseDataSlot:                      " << Get(child, &IDiaSymbol::get_baseDataSlot) << L'\n';
                            std::wcout << L"get_baseDataOffset:                    " << Get(child, &IDiaSymbol::get_baseDataOffset) << L'\n';
                            std::wcout << L"get_textureSlot:                       " << Get(child, &IDiaSymbol::get_textureSlot) << L'\n';
                            std::wcout << L"get_samplerSlot:                       " << Get(child, &IDiaSymbol::get_samplerSlot) << L'\n';
                            std::wcout << L"get_uavSlot:                           " << Get(child, &IDiaSymbol::get_uavSlot) << L'\n';
                            std::wcout << L"get_sizeInUdt:                         " << Get(child, &IDiaSymbol::get_sizeInUdt) << L'\n';
                            std::wcout << L"get_memorySpaceKind:                   " << Get(child, &IDiaSymbol::get_memorySpaceKind) << L'\n';
                            std::wcout << L"get_unmodifiedTypeId:                  " << Get(child, &IDiaSymbol::get_unmodifiedTypeId) << L'\n';
                            std::wcout << L"get_subTypeId:                         " << Get(child, &IDiaSymbol::get_subTypeId) << L'\n';
                            std::wcout << L"get_numberOfModifiers:                 " << Get(child, &IDiaSymbol::get_numberOfModifiers) << L'\n';
                            std::wcout << L"get_numberOfRegisterIndices:           " << Get(child, &IDiaSymbol::get_numberOfRegisterIndices) << L'\n';
                            std::wcout << L"get_isHLSLData:                        " << Get(child, &IDiaSymbol::get_isHLSLData) << L'\n';
                            std::wcout << L"get_isPointerToDataMember:             " << Get(child, &IDiaSymbol::get_isPointerToDataMember) << L'\n';
                            std::wcout << L"get_isPointerToMemberFunction:         " << Get(child, &IDiaSymbol::get_isPointerToMemberFunction) << L'\n';
                            std::wcout << L"get_isSingleInheritance:               " << Get(child, &IDiaSymbol::get_isSingleInheritance) << L'\n';
                            std::wcout << L"get_isMultipleInheritance:             " << Get(child, &IDiaSymbol::get_isMultipleInheritance) << L'\n';
                            std::wcout << L"get_isVirtualInheritance:              " << Get(child, &IDiaSymbol::get_isVirtualInheritance) << L'\n';
                            std::wcout << L"get_restrictedType:                    " << Get(child, &IDiaSymbol::get_restrictedType) << L'\n';
                            std::wcout << L"get_isPointerBasedOnSymbolValue:       " << Get(child, &IDiaSymbol::get_isPointerBasedOnSymbolValue) << L'\n';
                            std::wcout << L"get_baseSymbolId:                      " << Get(child, &IDiaSymbol::get_baseSymbolId) << L'\n';
                            std::wcout << L"get_objectFileName:                    " << Get(child, &IDiaSymbol::get_objectFileName) << L'\n';
                            std::wcout << L"get_isAcceleratorGroupSharedLocal:     " << Get(child, &IDiaSymbol::get_isAcceleratorGroupSharedLocal) << L'\n';
                            std::wcout << L"get_isAcceleratorPointerTagLiveRange:  " << Get(child, &IDiaSymbol::get_isAcceleratorPointerTagLiveRange) << L'\n';
                            std::wcout << L"get_isAcceleratorStubFunction:         " << Get(child, &IDiaSymbol::get_isAcceleratorStubFunction) << L'\n';
                            std::wcout << L"get_numberOfAcceleratorPointerTags:    " << Get(child, &IDiaSymbol::get_numberOfAcceleratorPointerTags) << L'\n';
                            std::wcout << L"get_isSdl:                             " << Get(child, &IDiaSymbol::get_isSdl) << L'\n';
                            std::wcout << L"get_isWinRTPointer:                    " << Get(child, &IDiaSymbol::get_isWinRTPointer) << L'\n';
                            std::wcout << L"get_isRefUdt:                          " << Get(child, &IDiaSymbol::get_isRefUdt) << L'\n';
                            std::wcout << L"get_isValueUdt:                        " << Get(child, &IDiaSymbol::get_isValueUdt) << L'\n';
                            std::wcout << L"get_isInterfaceUdt:                    " << Get(child, &IDiaSymbol::get_isInterfaceUdt) << L'\n';
                            std::wcout << L"get_isPGO:                             " << Get(child, &IDiaSymbol::get_isPGO) << L'\n';
                            std::wcout << L"get_hasValidPGOCounts:                 " << Get(child, &IDiaSymbol::get_hasValidPGOCounts) << L'\n';
                            std::wcout << L"get_isOptimizedForSpeed:               " << Get(child, &IDiaSymbol::get_isOptimizedForSpeed) << L'\n';
                            std::wcout << L"get_PGOEntryCount:                     " << Get(child, &IDiaSymbol::get_PGOEntryCount) << L'\n';
                            std::wcout << L"get_PGOEdgeCount:                      " << Get(child, &IDiaSymbol::get_PGOEdgeCount) << L'\n';
                            std::wcout << L"get_PGODynamicInstructionCount:        " << Get(child, &IDiaSymbol::get_PGODynamicInstructionCount) << L'\n';
                            std::wcout << L"get_staticSize:                        " << Get(child, &IDiaSymbol::get_staticSize) << L'\n';
                            std::wcout << L"get_finalLiveStaticSize:               " << Get(child, &IDiaSymbol::get_finalLiveStaticSize) << L'\n';
                            std::wcout << L"get_phaseName:                         " << Get(child, &IDiaSymbol::get_phaseName) << L'\n';
                            std::wcout << L"get_hasControlFlowCheck:               " << Get(child, &IDiaSymbol::get_hasControlFlowCheck) << L'\n';
                            std::wcout << L"get_constantExport:                    " << Get(child, &IDiaSymbol::get_constantExport) << L'\n';
                            std::wcout << L"get_dataExport:                        " << Get(child, &IDiaSymbol::get_dataExport) << L'\n';
                            std::wcout << L"get_privateExport:                     " << Get(child, &IDiaSymbol::get_privateExport) << L'\n';
                            std::wcout << L"get_noNameExport:                      " << Get(child, &IDiaSymbol::get_noNameExport) << L'\n';
                            std::wcout << L"get_exportHasExplicitlyAssignedOrdinal:" << Get(child, &IDiaSymbol::get_exportHasExplicitlyAssignedOrdinal) << L'\n';
                            std::wcout << L"get_exportIsForwarder:                 " << Get(child, &IDiaSymbol::get_exportIsForwarder) << L'\n';
                            std::wcout << L"get_ordinal:                           " << Get(child, &IDiaSymbol::get_ordinal) << L'\n';
                            std::wcout << L"get_frameSize:                         " << Get(child, &IDiaSymbol::get_frameSize) << L'\n';
                            std::wcout << L"get_exceptionHandlerAddressSection:    " << Get(child, &IDiaSymbol::get_exceptionHandlerAddressSection) << L'\n';
                            std::wcout << L"get_exceptionHandlerAddressOffset:     " << Get(child, &IDiaSymbol::get_exceptionHandlerAddressOffset) << L'\n';
                            std::wcout << L"get_exceptionHandlerRelativeVirtualAddress:" << Get(child, &IDiaSymbol::get_exceptionHandlerRelativeVirtualAddress) << L'\n';
                            std::wcout << L"get_exceptionHandlerVirtualAddress:    " << Get(child, &IDiaSymbol::get_exceptionHandlerVirtualAddress) << L'\n';
                            std::wcout << L"get_characteristics:                   " << Get(child, &IDiaSymbol::get_characteristics) << L'\n';
                            std::wcout << L"get_bindID:                            " << Get(child, &IDiaSymbol::get_bindID) << L'\n';
                            std::wcout << L"get_bindSpace:                         " << Get(child, &IDiaSymbol::get_bindSpace) << L'\n';
                            std::wcout << L"get_bindSlot:                          " << Get(child, &IDiaSymbol::get_bindSlot) << L'\n';

                            // Methods skipped — require special handling:
                            // get_guid          — returns GUID, needs custom formatting
                            // get_lexicalParent — returns IDiaSymbol*, would recurse
                            // get_classParent   — returns IDiaSymbol*, would recurse
                            // get_type          — returns IDiaSymbol*, would recurse
                            // get_arrayIndexType       — returns IDiaSymbol*, would recurse
                            // get_virtualTableShape    — returns IDiaSymbol*, would recurse
                            // get_lowerBound           — returns IDiaSymbol*, would recurse
                            // get_upperBound           — returns IDiaSymbol*, would recurse
                            // get_unmodifiedType       — returns IDiaSymbol*, would recurse
                            // get_container            — returns IDiaSymbol*, would recurse
                            // get_virtualBaseTableType — returns IDiaSymbol*, would recurse
                            // get_baseSymbol           — returns IDiaSymbol*, would recurse
                            // get_subType              — returns IDiaSymbol*, would recurse
                            // get_coffGroup            — returns IDiaSymbol*, would recurse
                            // get_noReturn             — BOOL, included via get_ pattern above? check compilation
                            // get_dataBytes            — variable-length, two-call pattern
                            // get_types                — array out-param
                            // get_typeIds              — array out-param
                            // get_numericProperties    — array out-param
                            // get_modifierValues       — array out-param
                            // get_acceleratorPointerTags — array out-param
                            // get_undecoratedNameEx    — takes an input param
#endif
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

    struct MappedExe
    {
        const BYTE* base = nullptr;

        MappedExe(const wchar_t* path)
        {
            HANDLE hFile    = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
            base = static_cast<const BYTE*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
            CloseHandle(hMapping);
            CloseHandle(hFile);
        }
       ~MappedExe() { if (base) UnmapViewOfFile(base); }

        static const BYTE* FuncBytes(const BYTE* base, DWORD rva)
        {
            auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
            auto* sect = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sect++)
                if (rva >= sect->VirtualAddress && rva < sect->VirtualAddress + sect->Misc.VirtualSize)
                    return base + rva - sect->VirtualAddress + sect->PointerToRawData;
            return nullptr;
        }
    };

    class FuncInfo
    {
        const std::wstring functionName;
        const std::wstring compiland;
        const ULONGLONG bodyLength;
        const BYTE* base;
        std::vector<BYTE> body;
    public:
        FuncInfo(IDiaSymbol* pFunc, const std::wstring& functionName, const std::wstring& compiland,   ULONGLONG bodyLength, const BYTE* base)
                                        : functionName(functionName),          compiland(compiland), bodyLength(bodyLength),       base(base)
        {
            DWORD rva = 0;
            pFunc->get_relativeVirtualAddress(&rva);

            const BYTE* bytes = MappedExe::FuncBytes(base, rva);
            if (bytes != nullptr)
                body.assign(bytes, bytes + bodyLength);
            else
                std::wcout << L"could not get body for " << functionName << L'\n';
        }
        void Print() const
        {
            PrintCompilandPath();
            std::wcout << L"   function body length: " << bodyLength << L'\n';
            std::wcout << L"   the first few bytes are: " << std::hex;
            auto count = 10<body.size() ? 10 : body.size();
            for(auto i=0; i<count; ++i)
                std::wcout << std::setfill(L'0') << std::setw(2) << body[i] << L' ';

            std::wcout << std::dec << L'\n';
        }
        void PrintCompilandPath() const { std::wcout << L"   [" << compiland << L"]\n"; }

        friend bool operator==(const FuncInfo& a, const FuncInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const FuncInfo& a, const FuncInfo& b) { return !a.IsEqualTo(b); }
    private:
        bool IsEqualTo(const FuncInfo& other) const
        {
         // if (compiland    != other.compiland)    return false; // compilands will always be different
            if (functionName != other.functionName) return false;
            if (bodyLength   != other.bodyLength)   return false;
            if (body         != other.body)         return false;
            return true;
        }
    };

    class Cop
    {
        std::map<std::wstring, std::vector<UdtInfo >>  udtMap;
        std::map<std::wstring, std::vector<FuncInfo>> funcMap;

    public:
        Cop() { CoInitialize(nullptr); }
       ~Cop() { CoUninitialize(); }

        HRESULT LoadPdb(const std::wstring& path, const BYTE* base)
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
                                    if (SUCCEEDED(udt->get_name(&name)) && name && name[0] != L'\0') {
                                        // Skip compiler-generated internal types and anonymous/unnamed UDTs
                                        std::wstring key(name);
                                        if (key.find(L'<') == std::wstring::npos && key.find(L"lambda") == std::wstring::npos)
                                            udtMap[key].push_back(UdtInfo(udt, path));
                                    }
                                }
                            }

                            // Functions
                            CComPtr<IDiaEnumSymbols> compilands;
                            if (SUCCEEDED(global->findChildren(SymTagCompiland, nullptr, nsNone, &compilands)))
                            {
                                LONG count = 0L;
                                compilands->get_Count(&count);
                                if (count == -37)
                                    count = -38;

                                while (true)
                                {
                                    ULONG fetched = 0;
                                    CComPtr<IDiaSymbol> compiland;
                                    if (FAILED(compilands->Next(1, &compiland, &fetched)) || fetched == 0)
                                        break;

                                    // is it my compiland?
                                    CComBSTR compilandName;
                                    compiland->get_name(&compilandName);

                                    std::wstring comp(compilandName.m_str);
                                    if (comp.substr(comp.rfind(L'\\')+1, comp.rfind(L'.')-comp.rfind(L'\\')-1) !=
                                        path.substr(path.rfind(L'\\')+1, path.rfind(L'.')-path.rfind(L'\\')-1))
                                        continue;

                                 // std::wcout << L"getting functions from compiland: " << compilandName.m_str << L'\n';

                                    CComPtr<IDiaEnumSymbols> funcs;
                                    if (SUCCEEDED(compiland->findChildren(SymTagFunction, nullptr, nsNone, &funcs)))
                                    {

                                        static const std::wstring prefixes[] = {
                                            L"D:\\a\\OdrCop\\",                 //    GitHub Actions' folder structure
                                            L"C:\\Users\\Bill\\source\\repos\\" // my local machine's folder structure 
                                        };
                                        for (const auto& prefix : prefixes) {
                                            if (_wcsnicmp(comp.c_str(), prefix.c_str(), prefix.size()) == 0) {
                                                comp = comp.substr(prefix.size());
                                                break;
                                            }
                                        }

                                        while (true)
                                        {
                                            ULONG got = 0;
                                            CComPtr<IDiaSymbol> func;
                                            if (FAILED(funcs->Next(1, &func, &got)) || got == 0)
                                                break;

                                            CComBSTR funcName;
                                            if (SUCCEEDED(func->get_name(&funcName)) && funcName)
                                            {
                                                ULONGLONG bodyLength = 0;
                                                func->get_length(&bodyLength);
                                                if (bodyLength == 0)
                                                {
                                                    std::wcout << funcName.m_str << L" has length 0, which is either inlined-away or pure virtual; skipping\n";
                                                    continue;   // inlined-away or pure virtual, skip
                                                }

                                                funcMap[std::wstring(funcName)].push_back(FuncInfo(func, std::wstring(funcName), comp, bodyLength, base));

                                             // std::wcout << L"   " << funcName.m_str << '\n';
                                            }
                                        }
                                    }
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

            return violationCount;
        }
    };
}