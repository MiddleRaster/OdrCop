#pragma once

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <variant>

#include "cvinfo.h"
#include "UdtInfo.h"

inline auto MyMin(auto a, auto b) // stupid windows.h macro gets in the way
{
    if (a < b)
        return a;
    return b;
}

namespace Odr
{
    struct PerTuTypes
    {
        std::map<std::wstring, std::vector<UdtInfo >>  udtMap; // UDTs
        std::map<std::wstring, std::vector<EnumInfo>> enumMap; // enums
        std::map<std::wstring, std::vector<TDefInfo>> tdefMap; // enums
    };

    struct NullInfo
    {
        NullInfo() noexcept = default;
        void Print(int /*depth*/) const {}
        bool operator==(const NullInfo&) const { return true; }
    };
class FuncInfo : public AnonInfo
{
    const std::wstring compiland;
    const std::wstring decorated;
    const std::wstring unmangled;
    const ULONGLONG    bodyLength;
    const std::vector<BYTE> body;
    const std::vector<std::pair<std::wstring,std::variant<NullInfo,UdtInfo,EnumInfo>>> args;
    const             std::pair<std::wstring,std::variant<NullInfo,UdtInfo,EnumInfo>>  returnType;
public:
    FuncInfo(bool b, const std::wstring& compiland, const std::wstring& decorated, ULONGLONG bodyLength, const std::vector<BYTE>& body, const PerTuTypes& perTU)
        : AnonInfo(b)
        , compiland(compiland)
        , decorated(decorated)
        , unmangled([&]() -> std::wstring
                    {
                        std::vector<wchar_t> buffer(65534);
                        if (0 != UnDecorateSymbolNameW(decorated.c_str(), buffer.data(), 65534, UNDNAME_COMPLETE))
                            return std::wstring(buffer.data());
                        return std::wstring(decorated); // my give up
                    }())
        , bodyLength(bodyLength)
        , body(body)
        , args([&](){
                        std::vector<std::pair<std::wstring,std::variant<NullInfo,UdtInfo,EnumInfo>>> theArgs;

                        // given an undecorated name, piece it apart, looking for arguments to the function.
                        // note: there may be extra () as part of the arg names.
                        // So we count backwards from the last ')'.
                        // if we find a ')', we increment a counter,
                        // if we find a '(', we decrement the counter.
                        // if the counter == 0, we've got it.

                        std::wstring argsAsOneString;

                        auto pos = unmangled.rfind(L')');
                        if (pos != std::wstring::npos)
                        {
                            int parens = 1;
                            for(auto i=pos-1; i>0; --i)
                            {
                                if (unmangled[i] == L')') { ++parens; continue; }
                                if (unmangled[i] == L'(') { --parens;
                                    if (parens == 0)
                                    {   // found it!
                                        argsAsOneString = unmangled.substr(i+1, pos-i-1);
                                        break;
                                    }
                                }
                            }
                        }

                        do { // now split by scanning for ',' at depth 0 (respecting nested brackets)
                            std::wstring oneArg;
                            pos = std::wstring::npos;
                            {
                                int depth         = 0;
                                int backtickDepth = 0;
                                for (size_t i=0; i<argsAsOneString.size(); ++i)
                                {
                                    wchar_t  c = argsAsOneString[i];
                                    if      (c == L'`')  { ++backtickDepth; }
                                    else if (c == L'\'') { if (backtickDepth > 0) --backtickDepth; }
                                    else if (c == L'(')  { ++depth; }
                                    else if (c == L')')  { --depth; }
                                    else if (c == L',' && depth == 0 && backtickDepth == 0)
                                    {
                                        pos = i;
                                        break;
                                    }
                                }
                            }
                            if (pos != std::wstring::npos)
                            {
                                oneArg          = argsAsOneString.substr(0, pos);
                                argsAsOneString = argsAsOneString.substr(pos+1);
                                // trim leading space from remainder
                                if (!argsAsOneString.empty() && argsAsOneString.front() == L' ')
                                    argsAsOneString = argsAsOneString.substr(1);
                            }
                            else
                            {
                                oneArg = argsAsOneString;
                            }

                            oneArg = Odr::CanonicalizeAnonymousNamespace  (oneArg);
                            oneArg = Odr::MakeAnonymousNamespaceTuSpecific(oneArg, CompilandToPdbPath(compiland));

                            if (oneArg.starts_with(L"enum "))
                            {
                                oneArg  = oneArg.substr(5);
                                auto it = perTU.enumMap.find(oneArg);
                                if (it != perTU.enumMap.end())
                                {
                                    theArgs.push_back({oneArg,{it->second[0]}}); // there should only be one per TU, but I suppose it's possible. Just grab the first.
                                    continue;
                                }
                                // else fall through to string
                            }

                            // if struct, class or union
                            if (oneArg.starts_with(L"struct ") || oneArg.starts_with(L"class ") || oneArg.starts_with(L"union "))
                            {
                                if (oneArg.starts_with(L"struct ")) oneArg = oneArg.substr(7);
                                if (oneArg.starts_with(L"class " )) oneArg = oneArg.substr(6);
                                if (oneArg.starts_with(L"union " )) oneArg = oneArg.substr(6);

                                auto it = perTU.udtMap.find(oneArg);
                                if (it != perTU.udtMap.end())
                                {
                                    theArgs.push_back({oneArg,{it->second[0]}}); // there should only be one per TU, but I suppose it's possible. Just grab the first.
                                    continue;
                                }
                                // else give up for now. Perhaps try PdbParser.h later
                                // fall through to string
                            }

                            // add a string
                            theArgs.push_back({oneArg,NullInfo()});

                        } while (pos != std::wstring::npos);
                        return theArgs;
                    }())
        , returnType([&]() {

                    // using calling convention to find split point

                    struct ReturnTypeExtractor // poor man's lambda
                    {
                        enum class FunctionKind
                        {
                            Regular,
                            Constructor,
                            Destructor,
                            DeducedLambda
                        };

                        static std::pair<FunctionKind, std::wstring> ExtractReturnTypeAndKind(const std::wstring& unmangled)
                        {
                            std::wstring ret = ExtractBareReturnType(unmangled);
                            FunctionKind kind = ClassifyFunctionKind(unmangled, ret);
                            return { kind, ret };
                        }

                    private:
                        static std::wstring ExtractBareReturnType(const std::wstring& unmangled)
                        {
                            std::wstring ws = extract_return_type(unmangled);

                            static constexpr const wchar_t* junk[] = {
                                L" __cdecl",
                                L" __stdcall",
                                L" __fastcall",
                                L" __thiscall",
                                L" __vectorcall",
                                L" __clrcall",

                                L"static ",
                                L"extern \"C++\" ",
                                L"extern \"C\" ",
                                L"extern ",

                                // spaces         no spaces
                                L"public: ",    L"public:",
                                L"private: ",   L"private:",
                                L"protected: ", L"protected:",

                                L"virtual ",

                                L" __ptr64",
                                L" __ptr32",
                                L" __unaligned",
                                L" __restrict",
                            };
                            for (auto j : junk)
                            {
                                while (true)
                                {
                                    auto p = ws.find(j);
                                    if (p == std::wstring::npos)
                                        break;
                                    ws.erase(p, wcslen(j));
                                }
                            }
                            return ws;
                        }

                        static std::wstring extract_return_type(const std::wstring& unmangled)
                        {
                            static constexpr const wchar_t* CALLING_CONVENTIONS[] =
                            {
                                L"__cdecl",
                                L"__stdcall",
                                L"__fastcall",
                                L"__thiscall",
                                L"__vectorcall",
                                L"__clrcall"
                            };

                            int depth        = 0;
                            int backtickDepth = 0;

                            for (size_t i = 0; i < unmangled.size(); ++i) {
                                wchar_t c = unmangled[i];

                                if      (c == L'`')  { ++backtickDepth; continue; }
                                else if (c == L'\'') { if (backtickDepth > 0) { --backtickDepth; continue; } }
                                else if (c == L'(')  { ++depth; }
                                else if (c == L')')  { --depth; }

                                if (depth == 0 && backtickDepth == 0) {
                                    for (auto cc : CALLING_CONVENTIONS) {
                                        size_t len = wcslen(cc);

                                        if (i + len <= unmangled.size() &&
                                            unmangled.compare(i, len, cc) == 0)
                                        {
                                            // Found the boundary
                                            std::wstring ret = unmangled.substr(0, i);

                                            // Trim trailing spaces
                                            while (!ret.empty() && ret.back() == L' ')
                                                ret.pop_back();

                                            return ret;
                                        }
                                    }
                                }
                            }
                            return L""; // constructors, destructors, or deduced lambdas
                        }

                        static FunctionKind ClassifyFunctionKind(const std::wstring& unmangled, const std::wstring& ret)
                        {
                            // If return type is non-empty → Regular
                            if (!ret.empty())
                                return FunctionKind::Regular;

                            // Find the position just after the function-level calling convention
                            // using the same depth-tracking logic as extract_return_type.
                            size_t pos = std::wstring::npos;

                            static constexpr const wchar_t* CALLING_CONVENTIONS[] = {
                                L"__cdecl", L"__stdcall", L"__fastcall",
                                L"__thiscall", L"__vectorcall", L"__clrcall"
                            };

                            int depth        = 0;
                            int backtickDepth = 0;

                            for (size_t i = 0; i < unmangled.size(); ++i) {
                                wchar_t c = unmangled[i];

                                if      (c == L'`')  { ++backtickDepth; continue; }
                                else if (c == L'\'') { if (backtickDepth > 0) { --backtickDepth; continue; } }
                                else if (c == L'(')  { ++depth; }
                                else if (c == L')')  { --depth; }

                                if (depth == 0 && backtickDepth == 0) {
                                    for (auto cc : CALLING_CONVENTIONS) {
                                        size_t len = wcslen(cc);
                                        if (i + len <= unmangled.size() &&
                                            unmangled.compare(i, len, cc) == 0)
                                        {
                                            pos = i + len;
                                            goto found;
                                        }
                                    }
                                }
                            }
                    found:
                            if (pos == std::wstring::npos)
                                return FunctionKind::Regular; // fallback

                            // Skip whitespace
                            while (pos < unmangled.size() && unmangled[pos] == L' ')
                                pos++;

                            std::wstring_view fn(unmangled.c_str() + pos,
                                                 unmangled.size() - pos);

                            // Destructor: any ::~ in the qualified name
                            if (fn.find(L"::~") != std::wstring_view::npos)
                                return FunctionKind::Destructor;

                            // Constructor: last name component matches the one before it,
                            // confirmed by '(' immediately following — tracked at bracket depth.
                            {
                                // Find all depth-0 '::' separator positions in fn.
                                std::vector<size_t> sepPositions;
                                int d  = 0;
                                int bt = 0;
                                for (size_t i = 0; i + 1 < fn.size(); ++i)
                                {
                                    wchar_t ch = fn[i];
                                    if      (ch == L'`')  { ++bt; }
                                    else if (ch == L'\'') { if (bt > 0) --bt; }
                                    else if (ch == L'(')  { ++d; }
                                    else if (ch == L')')  { --d; }
                                    else if (ch == L':' && fn[i+1] == L':' && d == 0 && bt == 0)
                                    {
                                        sepPositions.push_back(i);
                                        ++i; // skip second ':'
                                    }
                                }

                                if (sepPositions.size() >= 1)
                                {
                                    size_t last = sepPositions[sepPositions.size() - 1] + 2;

                                    // For single '::': className is everything before the last '::'.
                                    // For two or more: className is the second-to-last segment.
                                    std::wstring_view className = (sepPositions.size() >= 2)
                                        ? fn.substr(sepPositions[sepPositions.size() - 2] + 2, last - (sepPositions[sepPositions.size() - 2] + 2) - 2)
                                        : fn.substr(0, sepPositions[0]);

                                    std::wstring_view name = fn.substr(last);

                                    if (name.starts_with(className) &&
                                        name.size() > className.size() &&
                                        name[className.size()] == L'(')
                                        return FunctionKind::Constructor;
                                }
                            }

                            // Empty return type but not ctor/dtor → deduced lambda
                            return FunctionKind::DeducedLambda;
                        }
                    };

                    auto [kind, returnAsOneString] = ReturnTypeExtractor::ExtractReturnTypeAndKind(unmangled);

                    returnAsOneString = Odr::CanonicalizeAnonymousNamespace(returnAsOneString);
                    returnAsOneString = Odr::MakeAnonymousNamespaceTuSpecific(returnAsOneString, CompilandToPdbPath(compiland));

                    if (kind == ReturnTypeExtractor::FunctionKind::Regular)
                    {
                        if (returnAsOneString.starts_with(L"enum "))
                        {
                            returnAsOneString = returnAsOneString.substr(5);
                            auto it = perTU.enumMap.find(returnAsOneString);
                            if (it != perTU.enumMap.end())
                                return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{returnAsOneString,{it->second[0]}};
                            // else fall through to string
                        }

                        // if struct, class or union
                        if (returnAsOneString.starts_with(L"struct ") ||
                            returnAsOneString.starts_with(L"class " ) ||
                            returnAsOneString.starts_with(L"union " ) )
                        {
                            if (returnAsOneString.starts_with(L"struct ")) returnAsOneString = returnAsOneString.substr(7);
                            if (returnAsOneString.starts_with(L"class " )) returnAsOneString = returnAsOneString.substr(6);
                            if (returnAsOneString.starts_with(L"union " )) returnAsOneString = returnAsOneString.substr(6);

                            auto it = perTU.udtMap.find(returnAsOneString);
                            if (it != perTU.udtMap.end())
                                return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{returnAsOneString,{it->second[0]}};
                            // else give up for now. Perhaps try PdbParser.h later
                            // fall through to string
                        }
                    }

                    // if we get here, either we didn't find the type in our udt/enum maps,
                    // OR it's a ctor, dtor or deduced lambda

                    switch (kind)
                    {
                    case ReturnTypeExtractor::FunctionKind::Constructor:
                        return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{returnAsOneString, NullInfo()}; // had better be L""
                    case ReturnTypeExtractor::FunctionKind::Destructor:
                        return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{returnAsOneString, NullInfo()}; // had better be L""
                    case ReturnTypeExtractor::FunctionKind::DeducedLambda:
                        return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{L"no type for deduced lambda", NullInfo()};
                    case ReturnTypeExtractor::FunctionKind::Regular:
                    default:
                        return std::pair<std::wstring, std::variant<NullInfo, UdtInfo, EnumInfo>>{returnAsOneString, NullInfo()}; // int, void, etc.: things not found in the udtMap
                    }
               }())
        {}
        void Print(int depth) const
        {
            std::wcout << L"  [" << compiland << L"]\n";
            std::wcout << L"    unmangled name:  "  << unmangled        << L'\n';

            { // first print return type
                std::wcout << L"    return type:  ";
                auto& [name, argItem] = returnType;
                std::wcout << L"   " << name << L'\n';
                std::visit([depth](auto& arg) { arg.Print(depth+1); }, argItem);
            }

            if (args.size() > 0) {
                if (args.size() == 1)
                    std::wcout << L"    argument:\n";
                else
                    std::wcout << L"    arguments:\n";
                for (auto& [name, argItem] : args)
                {
                    std::wcout << L"      " << name << L'\n';
                    std::visit([depth](auto& arg) { arg.Print(depth+1); }, argItem);
                }
            }
            std::wcout << L"    function body length: " << bodyLength << L'\n';
            // actual bytes are printed in PrintMismatch, below
        }
        void PrintCompilandPath() const { std::wcout << L"  [" << compiland << L"] (same as above)\n"; }
        void PrintMismatch(int mismatch) const
        {
#ifdef WANT_ALL_BYTES
            (void)mismatch;
            std::wcout << L"    all bytes: " << std::hex;
            for(auto b : body)
                std::wcout << std::setfill(L'0') << std::setw(2) << b << L' ';
            std::wcout << std::dec << L'\n';
#else
            std::wcout << L"    bytes at the first mismatch are: " << std::hex;
            int i = mismatch - 10;
            if (i < 0)
                i = 0;
            auto end = (int)MyMin((size_t)(11 + mismatch), body.size());

            for(; i<=mismatch; ++i)
                std::wcout << std::setfill(L'0') << std::setw(2) << body[i] << L' ';
            for (; i<end; ++i)
                std::wcout << std::setfill(L'0') << std::setw(2) << body[i] << L' ';
            std::wcout << std::dec << L'\n';
#endif         
        }
        int MismatchIndex(const FuncInfo& other) const
        {   // find first mismatch that is not a NOP (0x90)
            constexpr BYTE NOP{0x90};

            size_t thisEnd =       body.size();
            size_t thatEnd = other.body.size();
            size_t thisCounter = 0;
            size_t thatCounter = 0;
            for (;(thisCounter < thisEnd) && (thatCounter < thatEnd);)
            {
                if (body[thisCounter] == other.body[thatCounter])
                {
                    ++thisCounter;
                    ++thatCounter;
                    continue;
                }

                // got a mismatch. Check to see if it's a NOP
                if (      body[thisCounter] == NOP) { ++thisCounter; continue; }
                if (other.body[thatCounter] == NOP) { ++thatCounter; continue; }

                // an actual mismatch
                return (int)((thisCounter + thatCounter)/2); // this will fail if there are many more NOPs in one of the bodies, but it'll most likely be in range
            }
            return -1;
        }
        template <typename Fn> void CollectSubItems(Fn&& fn) const
        {
            for (auto& [name, arg] : args)
                std::visit([&](const auto& x) { fn(name, x); }, arg);
            std::visit([&](const auto& x) { fn(returnType.first, x); }, returnType.second);
        }

        friend bool operator==(const FuncInfo& a, const FuncInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const FuncInfo& a, const FuncInfo& b) { return !a.IsEqualTo(b); }
    private:
        bool IsEqualTo(const FuncInfo& other) const
        {
         // if ( compiland  != other.compiland  ) return false; // compilands must be different for ODR violations
         // if ( decorated  != other.decorated  ) return false; // we're tring to catch anonymous namespace args, which always hash to something unique. So skip.
            if ( unmangled  != other.unmangled  ) return false;
            if (returnType  != other.returnType ) return false;
            if (args.size() != other.args.size()) return false;
            for(size_t i=0; i<args.size(); ++i)
                if (args[i] != other.args[i])     return false;

            if (MismatchIndex(other) !=   -1)     return false;
            return true;
        }
        static std::wstring CompilandToPdbPath(std::wstring compiland)
        {
            auto pos = compiland.rfind(L'.');
            if (pos != std::wstring::npos)
            {
                compiland = compiland.substr(0, pos);
                compiland.append(L".pdb");
            }
            return compiland;
        }
    };

    class DebugS
    {
        struct DebugSSubsection
        {
            DWORD       type;
            DWORD       length;
            const BYTE* data;
            const BYTE* end;
        };
        std::vector<DebugSSubsection> subsections;
        std::vector<const PROCSYM32*> procs;
    public:
        const std::vector<const PROCSYM32*>& GetProcs() const { return procs; }

        explicit DebugS(const BYTE* data, const BYTE* end)
        {   // First DWORD is signature, must be 4 (CV_SIGNATURE_C13)
            const BYTE* p = data;
            DWORD sig = *reinterpret_cast<const DWORD*>(p);
            if (sig != 4)
                return;

            p += sizeof(DWORD);

            while (p + 8 <= end)
            {
                DebugSSubsection sub;
                sub.type   = *reinterpret_cast<const DWORD*>(p);  p += sizeof(DWORD);
                sub.length = *reinterpret_cast<const DWORD*>(p);  p += sizeof(DWORD);
                sub.data   = p;
                sub.end    = p + sub.length;
                subsections.push_back(sub);

                // advance past data, aligned to 4 bytes relative to section start
                p                = sub.end;
                uintptr_t offset = p - data;
                offset           = (offset + 3) & ~3;
                p                = data + offset;
            }

            for (const DebugSSubsection& sub : subsections)
            {
                if (sub.type != DEBUG_S_SYMBOLS)
                    continue;

                const BYTE* q = sub.data;
                while (q + 4 <= sub.end)
                {
                    const WORD  recLen  = *reinterpret_cast<const WORD*>(q);
                    const WORD  recType = *reinterpret_cast<const WORD*>(q + 2);
                    const BYTE* recEnd  = q + sizeof(WORD) + recLen;

                    if (recLen == 0)
                        break;

                    if (recType == S_GPROC32    ||
                        recType == S_LPROC32    ||
                        recType == S_GPROC32_ID ||
                        recType == S_LPROC32_ID)
                    {
                        procs.push_back(reinterpret_cast<const PROCSYM32*>(q));
                    }

                    q = recEnd;
                }
            }
        }
    };

    class COFF
    {
        const std::vector<BYTE>  bytes;
        const BYTE             * base    = nullptr;
        const IMAGE_FILE_HEADER* coffHdr = nullptr;
        std::vector<IMAGE_SECTION_HEADER*> sections;
        std::vector<IMAGE_SECTION_HEADER*> debugS;
        std::vector<IMAGE_SECTION_HEADER*> debugT;
        std::vector<IMAGE_SECTION_HEADER*> text;

        explicit COFF(std::vector<BYTE>& bytes)
            : bytes(std::move(bytes))
            , base(this->bytes.data())
            , coffHdr(reinterpret_cast<const IMAGE_FILE_HEADER*>(base))
        {
            auto* secs = reinterpret_cast<IMAGE_SECTION_HEADER*>(const_cast<BYTE*>(base) + sizeof(IMAGE_FILE_HEADER) + coffHdr->SizeOfOptionalHeader);
            for (WORD i=0; i<coffHdr->NumberOfSections; ++i)
            {
                IMAGE_SECTION_HEADER* sec = &secs[i];
                sections.push_back(sec);

                char name[9] = {};
                memcpy(name, sec->Name, 8);
                     if (strcmp (name, ".debug$S") == 0) debugS.push_back(sec);
                else if (strcmp (name, ".debug$T") == 0) debugT.push_back(sec);
                else if (strncmp(name, ".text", 5) == 0)   text.push_back(sec);
            }
        }
    public:
        const BYTE* Data(const IMAGE_SECTION_HEADER* sec) const { return base + sec->PointerToRawData; }
        const BYTE* End (const IMAGE_SECTION_HEADER* sec) const { return base + sec->PointerToRawData + sec->SizeOfRawData; }
        static void Read(const std::wstring& pdbPath, bool excludeStdlib, std::map<std::wstring, std::vector<FuncInfo>>& funcMap, const PerTuTypes& perTU)
        {
            auto objPath = pdbPath.substr(0, pdbPath.rfind(L'.')) + L".obj";
            DWORD size;
            try { size = static_cast<DWORD>(std::filesystem::file_size(objPath)); }
            catch (...) { return; }

            // read the whole .obj into a vector
            std::vector<BYTE> bytes;
            bytes.resize(size);

            HANDLE hFile = CreateFileW(objPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                DWORD bytesRead = 0;
                ReadFile(hFile, bytes.data(), size, &bytesRead, nullptr);
                CloseHandle(hFile);
            }

            COFF coff(bytes);
            auto sectionSyms = coff.BuildSymbolIndex();
            std::map<SHORT, int> procCounter;

            for (IMAGE_SECTION_HEADER* sec : coff.debugS)
            {
                DebugS ds(coff.Data(sec), coff.End(sec));

                // build a relocation map for this .debug$S section
                std::map<DWORD, SHORT> segRelocMap;
                auto* relocs = reinterpret_cast<const IMAGE_RELOCATION*>(coff.base + sec->PointerToRelocations);
                for (WORD r=0; r<sec->NumberOfRelocations; ++r)
                {
                    const IMAGE_RELOCATION& rel = relocs[r];
                    const IMAGE_SYMBOL& sym     = coff.SymbolTable()[rel.SymbolTableIndex];
                    if (sym.SectionNumber > 0)
                        segRelocMap[rel.VirtualAddress] = sym.SectionNumber;
                }

                for (const PROCSYM32* proc : ds.GetProcs())
                {
                    // compute the reloc offset of the `seg` field:
                    DWORD procOffsetInSection = static_cast<DWORD>(reinterpret_cast<const BYTE*>(proc) - coff.Data(sec));

                    // In PROCSYM32, `seg` is at offsetof(PROCSYM32, seg) from the record start, but the record start (q) is offset from section data:
                    DWORD segFieldOffset = procOffsetInSection + offsetof(PROCSYM32, seg);

                    auto relIt = segRelocMap.find(segFieldOffset);
                    if (relIt == segRelocMap.end())
                        continue; // no relocation = can't resolve

                    SHORT actualSection                 = relIt->second; // now this is a valid 1-based section index
                    const IMAGE_SECTION_HEADER* textSec = coff.sections[actualSection - 1];

                    int idx    = procCounter[actualSection]++;
                    auto& syms = sectionSyms[actualSection];
                    if (idx >= (int)syms.size())
                        continue;

                    DWORD     resolvedOff = syms[idx].first;
                    std::string decorated = syms[idx].second;
                    const BYTE* funcBytes = coff.base + textSec->PointerToRawData + resolvedOff;
                    std::wstring   decoratedName(decorated.begin(), decorated.end());
                    std::wstring undecoratedName(proc->name, proc->name + std::strlen((const char*)proc->name));
                    std::wstring  normalizedName(NormalizeAnonNsCookies(decoratedName));

                    if (excludeStdlib == true)
                    {   // return values are not included in undecoratedName so this check is sufficient
                        if (undecoratedName.starts_with(L"std::"))
                            continue;
                    }

                    // Note how there is no - between anonymous and namespace; evidently MSVC does this for functions, but everthing else gets the dash
                    bool b = undecoratedName.find(L"`anonymous namespace'") != std::wstring::npos;
                    funcMap[MakeAnonymousNamespaceTuSpecific(b, normalizedName, pdbPath)].push_back(FuncInfo(b, objPath, decoratedName, proc->len, std::vector<BYTE>(funcBytes, funcBytes + proc->len), perTU));
                }
            }
        }
    private:
        static std::wstring MakeAnonymousNamespaceTuSpecific(bool isInAnonymousNamespace, std::wstring name, const std::wstring& pdbPath)
        {   // Note: this function is similar to Odr::MakeAnonymousNamespaceTuSpecific but instead of looking for L"`anonymous-namespace'",
            // we need to look for L"????????" instead. The rest is the same
            if (isInAnonymousNamespace)
            {
                auto pos = name.find(L"????????");
                if (pos != std::wstring::npos)
                {
                    auto first  = name.substr(0, pos);
                    auto second = std::wstring(L"????????");
                    auto third  = L"[" + pdbPath + L"]";
                    auto fourth = name.substr(pos + second.length());
                    name = first + second + third + fourth;
                }
            }
            return name;
        }
        static std::string GetSymbolName(const IMAGE_SYMBOL& sym, const BYTE* stringTableBase)
        {
            if (sym.N.Name.Short != 0)
            {   // Short name (<= 8 bytes, not null-terminated if exactly 8)
                char buf[9] = {};
                memcpy(buf, sym.N.ShortName, 8);
                return std::string(buf);
            }
            else
            {   // Long name via string table
                DWORD offset = sym.N.Name.Long;
                const char* str = reinterpret_cast<const char*>(stringTableBase + offset);
                return std::string(str);
            }
        }
        auto BuildSymbolIndex() const
        {
            std::map<SHORT, std::vector<std::pair<DWORD, std::string>>> sectionSyms;

            const IMAGE_SYMBOL* symTab = SymbolTable();
            const BYTE* stringTable = reinterpret_cast<const BYTE*>(symTab + coffHdr->NumberOfSymbols);

            DWORD i=0;
            while (i < coffHdr->NumberOfSymbols)
            {
                const IMAGE_SYMBOL& sym = symTab[i];
                if (sym.SectionNumber > 0)
                {
                    std::string name = GetSymbolName(sym, stringTable);
                    if ((sym.StorageClass == IMAGE_SYM_CLASS_EXTERNAL) || // normal external-linkage functions
                        (sym.StorageClass == IMAGE_SYM_CLASS_STATIC && name.find("?A0x") != std::string::npos)) // internal-linkage AND anonymous
                    {
                        sectionSyms[sym.SectionNumber].push_back({ sym.Value, std::move(name) });
                    }
                }

                i += 1+sym.NumberOfAuxSymbols; // critical
            }

            // sort each section's list by offset
            for (auto& [sec, vec] : sectionSyms)
                std::sort(vec.begin(), vec.end());

            return sectionSyms;
        }
        const IMAGE_SYMBOL* SymbolTable() const { return reinterpret_cast<const IMAGE_SYMBOL*>(base + coffHdr->PointerToSymbolTable); }
        static std::wstring NormalizeAnonNsCookies(const std::wstring& decorated)
        {
            std::wstring result = decorated;
            std::wstring::size_type pos = 0;
            while ((pos = result.find(L"?A0x", pos)) != std::wstring::npos)
            {
                auto end = result.find(L'@', pos);
                if (end != std::wstring::npos)
                    result.replace(pos, end - pos, L"?A0x????????");
                pos += 12; // length of L"?A0x????????"
            }
            return result;
        }
    };
}
