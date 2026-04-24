// odrmap.cpp
// ODR violation finder using DIA SDK.
// Usage: odrmap <a.pdb> <b.pdb> [c.pdb ...]
//
// Build:
//   cl /std:c++17 /EHsc /W4
//      /I"<DIA SDK>\include"
//      odrmap.cpp
//      /link "<DIA SDK>\lib\amd64\diaguids.lib" ole32.lib oleaut32.lib

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

struct MemberInfo {
    std::wstring name;
    std::wstring typeName;   // resolved recursively
    DWORD        offset;     // byte offset within UDT
    ULONGLONG    bitSize;    // 0 means "not a bitfield"
    DWORD        bitPos;     // valid only when bitSize != 0
};

struct UdtInfo {
    std::wstring             pdbPath;
    std::wstring             name;
    ULONGLONG                size;        // total size in bytes
    DWORD                    udtKind;     // UdtStruct / UdtClass / UdtUnion
    std::vector<MemberInfo>  members;     // data members in offset order
    std::vector<std::wstring> baseNames;  // base class names in order
};

// name → all UdtInfo records seen across all PDBs
using UdtMap = std::map<std::wstring, std::vector<UdtInfo>>;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static bool isUnion(const UdtInfo& u)
{
    return u.udtKind == UdtUnion;
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

static std::wstring BstrToWstr(BSTR b)
{
    return b ? std::wstring(b) : std::wstring(L"<unnamed>");
}

// Forward declaration — resolveTypeName is mutually recursive with itself
// for pointer/array/function types.
static std::wstring resolveTypeName(IDiaSymbol* type);

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

// ─────────────────────────────────────────────────────────────────────────────
// Load one UDT symbol into a UdtInfo
// ─────────────────────────────────────────────────────────────────────────────

static UdtInfo materializeUdt(IDiaSymbol* sym, const std::wstring& pdbPath)
{
    UdtInfo info;
    info.pdbPath = pdbPath;

    CComBSTR n;
    sym->get_name(&n);
    info.name = BstrToWstr(n);

    sym->get_length(&info.size);

    DWORD kind = 0;
    sym->get_udtKind(&kind);
    info.udtKind = kind;

    // ── Base classes ──────────────────────────────────────────────────────────
    CComPtr<IDiaEnumSymbols> bases;
    if (SUCCEEDED(sym->findChildren(SymTagBaseClass, NULL, nsNone, &bases))) {
        ULONG fetched = 0;
        CComPtr<IDiaSymbol> base;
        while (SUCCEEDED(bases->Next(1, &base, &fetched)) && fetched == 1) {
            CComPtr<IDiaSymbol> baseType;
            if (SUCCEEDED(base->get_type(&baseType))) {
                CComBSTR bn;
                baseType->get_name(&bn);
                info.baseNames.push_back(BstrToWstr(bn));
            }
            base.Release();
        }
    }

    // ── Data members ──────────────────────────────────────────────────────────
    CComPtr<IDiaEnumSymbols> children;
    if (SUCCEEDED(sym->findChildren(SymTagData, NULL, nsNone, &children))) {
        ULONG fetched = 0;
        CComPtr<IDiaSymbol> child;
        while (SUCCEEDED(children->Next(1, &child, &fetched)) && fetched == 1) {

            // Only instance data members (not statics)
            DWORD dataKind = 0;
            child->get_dataKind(&dataKind);
            if (dataKind == DataIsMember) {

                MemberInfo m;
                CComBSTR mn;
                child->get_name(&mn);
                m.name = BstrToWstr(mn);

                LONG offset = 0;
                child->get_offset(&offset);
                m.offset = static_cast<DWORD>(offset < 0 ? 0 : offset);

                // Bitfield?
                ULONGLONG bits = 0;
                child->get_length(&bits);
                DWORD bitPos = 0;
                child->get_bitPosition(&bitPos);
                m.bitSize = bits;   // 0 for non-bitfield apparently — but
                m.bitPos = bitPos; // DIA always returns something; rely on
                // the type tag to distinguish.
// Cleaner: check if type is SymTagBaseType and bits < byte_size
                CComPtr<IDiaSymbol> mtype;
                if (SUCCEEDED(child->get_type(&mtype)))
                 m.typeName = resolveTypeName(mtype);

                info.members.push_back(std::move(m));
            }

            child.Release();
        }
    }

    // Sort by offset so comparison is order-independent w.r.t. declaration
    std::sort(info.members.begin(), info.members.end(),
        [](const MemberInfo& a, const MemberInfo& b) {
            return a.offset < b.offset;
        });

    return info;
}

// ─────────────────────────────────────────────────────────────────────────────
// Load all UDTs from one PDB into udtMap
// ─────────────────────────────────────────────────────────────────────────────

static bool loadPdb(const std::wstring& path, UdtMap& udtMap)
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
                                std::wstring key(name, SysStringLen(name));
                                // Skip compiler-generated internal types
                                if (key.find(L'<') == std::wstring::npos && key.find(L"lambda") == std::wstring::npos)
                                    udtMap[key].push_back(materializeUdt(udt, path));
                            }
                            udt.Release();
                        }
                    }
                } else std::wcerr <<                  L"get_globalScope failed with 0x" << std::hex << hr << std::dec << L'\n';
            }     else std::wcerr <<                      L"openSession failed with 0x" << std::hex << hr << std::dec << L'\n';
        }         else std::wcerr << L"loadDataFromPdb failed: " << path << L" with 0x" << std::hex << hr << std::dec << L'\n';
    }             else std::wcerr <<  L"CoCreateInstance(DiaSource) failed for with 0x" << std::hex << hr << std::dec << L'\n';

    return hr == S_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// Comparison helpers
// ─────────────────────────────────────────────────────────────────────────────

static const wchar_t* udtKindName(DWORD k)
{
    switch (k) {
    case UdtStruct: return L"struct";
    case UdtClass:  return L"class";
    case UdtUnion:  return L"union";
    default:        return L"unknown";
    }
}

// Returns true if two UdtInfo records are identical for ODR purposes.
static bool udtEqual(const UdtInfo& a, const UdtInfo& b)
{
    if (a.size != b.size)    return false;
    if (a.udtKind != b.udtKind) return false;
    if (a.baseNames != b.baseNames) return false;
    if (a.members.size() != b.members.size()) return false;
    for (size_t i=0; i<a.members.size(); ++i) {
        const auto& ma = a.members[i];
        const auto& mb = b.members[i];
        if (ma.name   != mb.name)   return false;
     // if (ma.typeName != mb.typeName) return false;
        if (ma.typeName != mb.typeName)
        {
            if (isAnonymous(ma.typeName) && isAnonymous(mb.typeName))
                continue; // treat as equal
            return false;
        }
        if (ma.offset  != mb.offset )   return false;
        if (ma.bitSize != mb.bitSize)   return false;
        if (ma.bitSize && 
            ma.bitPos  != mb.bitPos )   return false;
    }
    return true;
}

static void printUdt(const UdtInfo& u)
{
    std::wcout << L"  [" << u.pdbPath << L"]\n";
    std::wcout << L"    kind=" << udtKindName(u.udtKind) << L"  size=" << u.size << L'\n';

    if (!u.baseNames.empty()) {
        std::wcout << L"    bases:";
        for (auto& b : u.baseNames) std::wcout << L" " << b;
        std::wcout << L'\n';
    }

    for (auto& m : u.members) {
        std::wcout << L"    +" << m.offset
            << L"  " << m.typeName
            << L"  " << m.name;
        if (m.bitSize)
            std::wcout << L"  :" << m.bitSize << L"@bit" << m.bitPos;
        std::wcout << L'\n';
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3) {
        std::wcout << L"Usage: odrcop <a.pdb> <b.pdb> [c.pdb ...] where each .cpp file/TU is built with /Zi and /Fd\"x64\\Debug\\%(Filename).pdb\"\n";
        return -1;
    }

    CoInitialize(nullptr);

    UdtMap udtMap;

    for (int i=1; i<argc; ++i)
    {
        std::wstring path(argv[i]);
        std::wcout << L"Loading: " << path << L'\n';
        if (!loadPdb(path, udtMap))
            std::wcerr << L"Skipping: " << path << L'\n';
    }
    std::wcout << L'\n';

    // ── Report ────────────────────────────────────────────────────────────────
    int violationCount = 0;
    for (auto& [name, definitions] : udtMap) {

        // Only care about UDTs seen in more than one PDB
        if (definitions.size() < 2) continue;

        // Check whether all definitions are identical
        bool allSame = true;
        for (size_t i=1; i<definitions.size(); ++i) {
            if (!udtEqual(definitions[0], definitions[i])) {
                allSame = false;
                break;
            }
        }
        if (allSame) continue;

        ++violationCount;
        std::wcout << L"ODR VIOLATION: " << name << L'\n';

        // Group identical definitions together so output is readable
        // when there are many PDBs.
        std::vector<bool> printed(definitions.size(), false);
        for (size_t i=0; i<definitions.size(); ++i) {
            if (printed[i]) continue;
            printUdt(definitions[i]);
            printed[i] = true;
            // Mark all later ones that match this one
            for (size_t j=i+1; j<definitions.size(); ++j) {
                if (!printed[j] && udtEqual(definitions[i], definitions[j])) {
                    std::wcout << L"  [" << definitions[j].pdbPath << L"] (same as above)\n";
                    printed[j] = true;
                }
            }
        }
        std::wcout << L'\n';
    }

    if (violationCount == 0)
        std::wcout << L"No ODR violations found.\n";
    else
        std::wcout << violationCount << L" ODR violation(s) found.\n";

    CoUninitialize();
    return violationCount > 0 ? 1 : 0;
}
