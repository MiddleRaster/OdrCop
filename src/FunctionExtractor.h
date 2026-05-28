#pragma once

#include "CoffReader.h"
#include "cvinfo.h"

#include <map>
#include <variant>
#include <fstream>

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

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
        const bool         isStatic;
        const ULONGLONG    bodyLength;
        const std::vector<BYTE> body;
        const std::vector<std::pair<std::wstring,std::variant<NullInfo,UdtInfo,EnumInfo>>> args;
        const             std::pair<std::wstring,std::variant<NullInfo,UdtInfo,EnumInfo>>  returnType;
    public:
        FuncInfo(bool b, const std::wstring& compiland, const std::wstring& decorated, ULONGLONG bodyLength, const std::vector<BYTE>& body, bool isStatic, const PerTuTypes& perTU)
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
            , isStatic  (isStatic)
            , bodyLength(bodyLength)
            , body      (body)
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
            std::wcout << L"    unmangled name:  "  << (isStatic ? L"static " : L"") << unmangled << L'\n';

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
            auto end = (int)min((size_t)(11 + mismatch), body.size());

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
            if (isStatic || other.isStatic)
                return true; // never compare static free functions because they're internal-linkage, TU-specific

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


    class FilesAndLinesCache
    {
        std::map<std::wstring, std::map<DWORD, std::string>> cache;
    public:
        bool IsStaticFreeFunction(const std::wstring& decorated, const std::wstring& filename, DWORD linenumber)
        {
            if (IsFreeFunction(decorated) == false)
                return false;

            if ((filename == L"") || (linenumber == 0))
                return false; // compiler-generated; don't care if it's static or not

            if (cache.find(filename) == cache.end())
                cache[filename] = ReadLinesFromFile(filename);

            auto lines = cache[filename];
            if (lines.find(linenumber) == lines.end())
                return false;
            auto line  = lines[linenumber];

            if (true == HasLeadingKeyword(line, "static"))
                return true;

            // try backing up one line at a time, but if we find a ; or }, we've gone too far
            while (--linenumber > 0)
            {
                line = lines[linenumber];
                if (PreviousDeclarationEnds(line))
                    break;
                if (true == HasLeadingKeyword(line, "static"))
                    return true;
            }

            return false;
        }
    private:
        static auto ReadLinesFromFile(const std::wstring& path)
        {
            std::map<DWORD, std::string> lines;

            std::ifstream in(path, std::ios::binary);
            if (in)
            {
                std::string line;
                DWORD lineno = 1;
                while (std::getline(in, line))
                    lines[lineno++] = line;
            }
            return lines;
        }
        static bool HasLeadingKeyword(const std::string& line, const std::string& keyword)
        {
            size_t pos = line.find(keyword);
            if (pos == std::string::npos)
                return false;
            bool  leftOk = (pos == 0)                            || std::isspace((unsigned char)line[pos-1]);
            bool rightOk = (pos + keyword.size() == line.size()) || std::isspace((unsigned char)line[pos + keyword.size()]);
            return leftOk && rightOk;
        }
        static bool PreviousDeclarationEnds(const std::string& line)
        {
            for (char ch : line)
                if (ch == ';' ||    // end of statement
                    ch == '}' ||    // end of block/namespace
                    ch == '{' ||    // start of block (we've gone too far)
                    ch == '>')      // end of #include<> or template argument
                    return true;
            return false;
        }
        static bool IsFreeFunction(const std::wstring& decorated)
        {
            size_t pos = decorated.find(L"@@");
            if (pos == std::wstring::npos)
                return false;

            pos += 2;
            if (pos >= decorated.size())
                return false;

            return decorated[pos] == L'Y';
        }
    };

    struct FunctionExtractor
    {
        struct Function
        {
            const std::wstring      decorated, undecorated;
            const std::vector<BYTE> body;
            const std::wstring      filename;
            const DWORD             linenumber;
        };

        static void Extract(const std::filesystem::path& pdbPath, bool excludeStdlib, std::map<std::wstring, std::vector<FuncInfo>>& funcMap, const PerTuTypes& perTU)
        {
            auto objPath = std::filesystem::path(pdbPath).replace_extension(L".obj");
            if (!std::filesystem::exists(objPath))
            {
                // modules don't build like regular .cpp files:
                // their .obj files are /not/ in the same folder.
                // For example, the std module ends up like this:
                //    C:\Users\Bill\source\repos\katas\cxx\8Queens\8Queens\x64\Debug\std.pdb
                //    C:\Users\Bill\source\repos\katas\cxx\8Queens\8Queens\x64\Debug\microsoft\STL\std.ixx.obj

                // non-std modules end up in the same folder, but named like this:
                //    C:\Users\Bill\source\repos\TestProjectForClaudomatic2\TestProjectForClaudomatic2\x64\Debug\tdd20.ixx.obj
                //    C:\Users\Bill\source\repos\TestProjectForClaudomatic2\TestProjectForClaudomatic2\x64\Debug\tdd20.pdb

                objPath = std::filesystem::path(pdbPath).replace_extension(L".ixx.obj");
            }
            if (!std::filesystem::exists(objPath))
            {
                auto stem = pdbPath.stem().string();
                if (stem != "std" && stem != "std.compat")
                    return;

                if (excludeStdlib)
                    return;

                objPath = pdbPath.parent_path() / "microsoft" / "STL" / (stem + ".ixx.obj");
                if (!std::filesystem::exists(objPath))
                    return;
            }
            std::wcout << L"Loading: " << objPath << L"\n";

            std::vector<Function> functions = Odr::FunctionExtractor::ExtractFunctions(objPath);

            FilesAndLinesCache flc;

            for (const auto& function : functions)
            {
                if (excludeStdlib == true) // return values are not included in undecoratedName so this check is sufficient
                    if (function.undecorated.starts_with(L"std::"))
                        continue;

                if (function.undecorated.find(L"<lambda_") != std::wstring::npos)
                {
                    // as I expected, it's not that simple. Lambdas that are local to a function or assigned to a UDT's data-member can be involved in ODR violations.
                    // But if it's not enclosed by a function or UDT, then it can't be ODR-relevant.
                    // So filter those out immediately

                    if (function.undecorated.starts_with(L"<lambda_"))
                        continue;
                    if (function.undecorated.starts_with(L"`<lambda_"))
                        continue;

                    // TODO:  look for procsym32's parent field, and look for enclosing UDT.
                    // TODO:  also, do relocation fixups for bytecode
                }
            
                // Note how there is no - between anonymous and namespace; evidently MSVC does this for functions, but everthing else gets the dash
                bool b        = function.undecorated.find(L"`anonymous namespace'") != std::wstring::npos;
                bool isStatic = flc.IsStaticFreeFunction(function.decorated, function.filename, function.linenumber);
                funcMap[MakeAnonymousNamespaceTuSpecific(b, NormalizeAnonNsCookies(function.decorated), pdbPath)].push_back(FuncInfo(b, objPath.c_str(), function.decorated, function.body.size(), function.body, isStatic, perTU));
            }
        }

    private:
        static std::vector<Function> ExtractFunctions(const std::filesystem::path& objFile)
        {
            // Sheesh, why do I have to define this myself?
            struct CV_Checksum_t
            {
                DWORD  strOffset;   // byte offset of the filename into the DEBUG_S_STRINGTABLE payload
                BYTE   cbChecksum;  // length of the checksum data that follows
                BYTE   kind;        // checksum algorithm: 0=none, 1=MD5, 2=SHA1, 3=SHA256
                                    // followed by cbChecksum bytes of checksum data, then padded to 4-byte alignment
            };

            std::vector<Function> functions;

            try
            {
                CoffReader coffReader(objFile);


                // first, from the symbol table, build up a map from sectionNumber to pair of offset and decorated name. 
                using SectionIndex = SHORT;
                using Offset       = DWORD;
                std::map<SectionIndex, std::vector<std::pair<Offset,std::wstring>>> sectionIndexToOffsetFunction;

                auto sectionHeaders = coffReader.sectionHeaders;
                auto stringTable    = coffReader.startOfStringTable;
                auto symbolTable    = coffReader.symbolTable;
                for(size_t i=0; i<symbolTable.size(); )
                {
                    auto symbol = symbolTable[i];

                    if (symbol->SectionNumber > 0)                                                                                  // not undefined, not absolute (#define)
                    if (ISFCN(symbol->Type))                                                                                        // is a function
                    if ((symbol->StorageClass & IMAGE_SYM_CLASS_EXTERNAL) || (symbol->StorageClass & IMAGE_SYM_CLASS_STATIC))       // is either external-linkage or internal-linkage
                    if (auto section = sectionHeaders[symbol->SectionNumber-1]; section->Characteristics & IMAGE_SCN_MEM_EXECUTE)   // is executable
                    {
                        std::string name;
                        if (symbol->N.Name.Short != 0)
                            name = std::string(symbol->N.ShortName, symbol->N.ShortName + 8);
                        else
                            name = std::string(stringTable + symbol->N.Name.Long);

                        sectionIndexToOffsetFunction[symbol->SectionNumber-1].push_back({symbol->Value, std::wstring(name.begin(), name.end())});
                    }
                    i += 1 + symbol->NumberOfAuxSymbols;
                }
                // sort each section's vector by offset.
                for (auto& [sec, vec] : sectionIndexToOffsetFunction)
                    std::sort(vec.begin(), vec.end());


                // use .debug$S sections' data (with relocation fixups) to get the bodies' lengths
                const BYTE* bytes = coffReader.bytes;

                // pre-pass: walk all .debug$S sections to build a map from sectionIndex to line records.
                // each line record holds a code offset (absolute within the section), line number, and file checksum offset.
                // in COFF .obj files offCon is always 0, so codeOffset from the line record is already absolute within the section.
                using VirtualAddressType = DWORD;
                using SectionIndexType   = SHORT;
                struct LineRecord { const unsigned long codeOffset; CV_off32_t lineNumber; std::wstring path; };
                struct SourceLocation { std::wstring path; CV_off32_t lineNumber; };
                std::map<SectionIndexType, std::vector<LineRecord>> linesPerSection;
                std::map<DWORD, std::wstring>                       fileChecksumOffsetToPath;

                for(SHORT i=0; i<(SHORT)sectionHeaders.size(); ++i)
                {
                    auto section = sectionHeaders[i];

                    std::string name;
                    if (section->Misc.VirtualSize == 0)
                        name = std::string(section->Name, section->Name + 8);
                    else
                        name = std::string(bytes + section->Misc.PhysicalAddress, bytes + section->Misc.PhysicalAddress + section->Misc.VirtualSize);

                    if (name == ".debug$S")
                    {
                        std::map<VirtualAddressType,SectionIndexType> fixups;
                        auto relocations = coffReader.relocations[i];
                        for(auto relocation : relocations)
                        {
                            auto symbol = symbolTable[relocation->SymbolTableIndex];
                            if (symbol->SectionNumber > 0)
                                fixups[relocation->VirtualAddress] = symbol->SectionNumber-1;
                        }

                        const BYTE* raw = bytes + section->PointerToRawData;
                        const BYTE* end = raw   + section->SizeOfRawData;
                        if (CV_SIGNATURE_C13 == *reinterpret_cast<const DWORD*>(raw))
                        {
                            // single pre-pass loop: collect string table, checksum entries, and line records together
                            const BYTE*                          stringTableBase = nullptr;
                            std::vector<std::pair<DWORD,DWORD>>  rawChecksumEntries; // (byteOffset, strOffset)

                            struct RawLineContrib { SectionIndexType secIdx; CV_off32_t offFile; const unsigned long codeOffset; CV_off32_t lineNumber; };
                            std::vector<RawLineContrib> rawLineContribs;

                            {
                                const BYTE* p = raw + sizeof(DWORD);
                                while (p < end)
                                {
                                    auto* hdr = reinterpret_cast<const CV_DebugSSubsectionHeader_t*>(p);
                                    p += sizeof(CV_DebugSSubsectionHeader_t);

                                    if (hdr->type == DEBUG_S_STRINGTABLE)
                                    {
                                        stringTableBase = p;
                                    }
                                    else if (hdr->type == DEBUG_S_FILECHKSMS)
                                    {
                                        const BYTE* q          = p;
                                        const BYTE* qEnd       = p + hdr->cbLen;
                                        DWORD       byteOffset = 0;
                                        while (q < qEnd)
                                        {

                                            auto* entry = reinterpret_cast<const CV_Checksum_t*>(q);
                                            rawChecksumEntries.push_back({byteOffset, entry->strOffset});
                                            DWORD entrySize = (sizeof(CV_Checksum_t) + entry->cbChecksum + 3u) & ~3u;
                                            q          += entrySize;
                                            byteOffset += entrySize;
                                        }
                                    }
                                    else if (hdr->type == DEBUG_S_LINES)
                                    {
                                        auto* linesHdr     = reinterpret_cast<const CV_DebugSLinesHeader_t*>(p);
                                        DWORD segConOffset = static_cast<DWORD>(reinterpret_cast<const BYTE*>(linesHdr) + offsetof(CV_DebugSLinesHeader_t, segCon) - (bytes + section->PointerToRawData));

                                        auto fit = fixups.find(segConOffset);
                                        if (fit != fixups.end())
                                        {
                                            SectionIndexType secIdx  = fit->second;
                                            const BYTE*      pBlock  = p + sizeof(CV_DebugSLinesHeader_t);
                                            const BYTE*      pEnd    = p + hdr->cbLen;
                                            while (pBlock < pEnd)
                                            {
                                                auto* fileBlock = reinterpret_cast<const CV_DebugSLinesFileBlockHeader_t*>(pBlock);
                                                auto* lineEntry = reinterpret_cast<const CV_Line_t*>(pBlock + sizeof(CV_DebugSLinesFileBlockHeader_t));
                                                for (CV_off32_t j=0; j<fileBlock->nLines; ++j)
                                                    rawLineContribs.push_back({secIdx, fileBlock->offFile, lineEntry[j].offset, lineEntry[j].linenumStart});
                                                pBlock += fileBlock->cbBlock;
                                            }
                                        }
                                    }

                                    p += (hdr->cbLen + 3u) & ~3u;
                                }
                            }

                            // resolve checksum offsets to paths now that we have the string table
                            if (stringTableBase && fileChecksumOffsetToPath.empty())
                            {
                                for (auto& [byteOffset, strOffset] : rawChecksumEntries)
                                {
                                    const char* path = reinterpret_cast<const char*>(stringTableBase + strOffset);
                                    fileChecksumOffsetToPath[byteOffset] = std::wstring(path, path + std::strlen(path));
                                }
                            }

                            // populate linesPerSection, resolving offFile to path is deferred to main pass
                            for (auto& [secIdx, offFile, codeOffset, lineNumber] : rawLineContribs)
                            {
                                std::wstring path;
                                auto pathIt = fileChecksumOffsetToPath.find(offFile);
                                if (pathIt != fileChecksumOffsetToPath.end())
                                    path = pathIt->second;
                                linesPerSection[secIdx].push_back({codeOffset, lineNumber, path});
                            }
                        }
                    }
                }

                // main pass: same structure as before, but look up source file and line number
                for(SHORT i=0; i<(SHORT)sectionHeaders.size(); ++i)
                {
                    auto section = sectionHeaders[i];

                    std::string name;
                    if (section->Misc.VirtualSize == 0)
                        name = std::string(section->Name, section->Name + 8);
                    else
                        name = std::string(bytes + section->Misc.PhysicalAddress, bytes + section->Misc.PhysicalAddress + section->Misc.VirtualSize);

                    if (name == ".debug$S")
                    {
                        // find the relocation data associated with this section, create a mapping from VirtualAddress to section, to be used to fixup the "seg" field
                        std::map<VirtualAddressType,SectionIndexType> fixups;
                        auto relocations = coffReader.relocations[i];
                        for(auto relocation : relocations)
                        {
                            auto symbol = symbolTable[relocation->SymbolTableIndex];
                            if (symbol->SectionNumber > 0)
                                fixups[relocation->VirtualAddress] = symbol->SectionNumber-1;
                        }

                        const BYTE* raw = bytes + section->PointerToRawData;
                        const BYTE* end = raw   + section->SizeOfRawData;
                        if (CV_SIGNATURE_C13 == *reinterpret_cast<const DWORD*>(raw))
                        {
                            raw += sizeof(DWORD);

                            // we need this since PROCSYM32's seg AND offset are both set to 0 (seg gets fixed up via relocation data, offset doesn't).
                            // We can't find the bodies by offset, so using a counter instead.
                            std::map<SectionIndexType,size_t> countOfProcsPerSection;

                            for (;;)
                            {
                                auto subSectionHeader = reinterpret_cast<const CV_DebugSSubsectionHeader_t*>(raw);
                                raw += sizeof(CV_DebugSSubsectionHeader_t);

                                const BYTE* payload = raw; // start of this subsection's data

                                if (subSectionHeader->type == DEBUG_S_SYMBOLS)
                                {
                                    const BYTE* record    = payload;
                                    const BYTE* recordEnd = payload + subSectionHeader->cbLen;
                                    while (record < recordEnd)
                                    {
                                        auto* rec = reinterpret_cast<const SYMTYPE*>(record);
                                        if (rec->rectyp == S_GPROC32    || rec->rectyp == S_LPROC32 ||
                                            rec->rectyp == S_GPROC32_ID || rec->rectyp == S_LPROC32_ID)
                                        {
                                            auto  procsym32 = reinterpret_cast<const PROCSYM32*>(record);
                                            DWORD segOffset = static_cast<DWORD>(reinterpret_cast<const BYTE*>(procsym32) + offsetof(PROCSYM32, seg) - (coffReader.bytes + section->PointerToRawData));

                                            auto it = fixups.find(segOffset);
                                            if (it != fixups.end())
                                            {   // found it
                                                auto& sectionIndex = it->second;

                                                // find the decorated function name in sectionIndexToOffsetFunction by the section we just found in our fixups map
                                                auto it2 = sectionIndexToOffsetFunction.find(sectionIndex);
                                                if (it2 != sectionIndexToOffsetFunction.end())
                                                {
                                                    auto& vectorOfOffsetAndName = it2->second;
                                                    size_t count = countOfProcsPerSection[sectionIndex]++;
                                                    if (count < vectorOfOffsetAndName.size())
                                                    {
                                                        auto& [offset, decoratedName] = vectorOfOffsetAndName[count];

                                                        // range-based lookup: find the first line record within [offset, offset+len).
                                                        // offCon is always 0 in COFF .obj files, so line record codeOffsets are absolute within the section.
                                                        std::wstring sourceFile;
                                                        DWORD        lineNumber = 0;
                                                        auto linesIt = linesPerSection.find(sectionIndex);
                                                        if (linesIt != linesPerSection.end())
                                                        {
                                                            size_t zeroth = 0;
                                                            for (auto& lr : linesIt->second)
                                                            {
                                                                if (lr.codeOffset == 0)
                                                                {
                                                                    if (zeroth == count)
                                                                    {
                                                                        sourceFile = lr.path;
                                                                        lineNumber = lr.lineNumber;
                                                                        break;
                                                                    }
                                                                    ++zeroth;
                                                                }
                                                            }
                                                        }

                                                        // filter out extern "C" functions and compiler-generated functions, as neither of these can be ODR violations
                                                        std::wstring undec = Undecorate(decoratedName);
                                                        if (IsPlainCFunction(undec))
                                                            continue; // ignore C functions entirely
                                                        if (IsCompilerGenerated(decoratedName))
                                                            continue; // compiler-generated can't be ODR-relevant

                                                        auto startOfBody = bytes + coffReader.sectionHeaders[sectionIndex]->PointerToRawData + offset;
                                                        std::vector<BYTE> body(startOfBody, startOfBody + procsym32->len);
                                                        functions.push_back({decoratedName,
                                                                             std::wstring(procsym32->name, procsym32->name + std::strlen((const char*)procsym32->name)),
                                                                             body,
                                                                             sourceFile,
                                                                             lineNumber});
                                                    }
                                                }
                                            }
                                        }
                                        record += sizeof(WORD) + rec->reclen;
                                    }
                                }

                                // round payload size up to 4-byte boundary
                                auto length = (subSectionHeader->cbLen + 3u) & ~3u;
                                raw += length;
                                if (raw == end)
                                    break;
                                if (raw > end)
                                    break; // uh oh, outa synch
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::wcout << L"caught exception - " << objFile.c_str() << L": " << e.what() << L'\n';
            }
            catch(...)
            {
                std::wcout << L"unknown exception - " << objFile.c_str() << L": " << L"probably not a.obj file or doesn't exist\n";
            }
            return functions;
        }
    private:
        static std::wstring Undecorate(const std::wstring& decorated)
        {
            wchar_t buffer[16384];
            if (UnDecorateSymbolNameW(decorated.c_str(), buffer, _countof(buffer), UNDNAME_COMPLETE))
                return buffer;
            return decorated;
        }
        static bool IsPlainCFunction(const std::wstring& undec)
        {
            if (undec.find(L'?') != std::wstring::npos)
                return false; // No MSVC C++ mangling marker

            for (wchar_t ch : undec)
            {   // Only [A-Za-z0-9_]
                if(!(ch == L'_'                ||
                    (ch >= L'0' && ch <= L'9') ||
                    (ch >= L'A' && ch <= L'Z') ||
                    (ch >= L'a' && ch <= L'z') ))
                    return false;
            }
            return true;
        }
        static bool IsCompilerGenerated(const std::wstring& decorated)
        {
            static const std::wstring decoratedMarkers[] =
            { // Strong compiler-generated markers in the *decorated* name
                L"$dtor$",       // destructor helpers
                L"$ctor$",       // constructor helpers
                L"$fin$",        // EH finally helpers
                L"$catch$",      // EH catch block helpers
                L"$handlerMap",  // EH handler tables
                L"$TSS",         // thread-safe static initialization
                L"$TLS",         // thread-local storage helpers
                L"$ILT",         // incremental linker thunks
                L"$RTC",         // run-time check helpers
            };
            for (const auto& m : decoratedMarkers)
            {
                if (decorated.find(m) != std::wstring::npos)
                    return true;
            }

            static const std::wstring undecMarkers[] =
            { // EH / vcall / dynamic-init helpers visible in *undecorated* names
                L"`EH",                              // EH internal helpers
                L"`vcall'",                          // virtual call thunks
                L"`dynamic initializer for '",       // dynamic init
                L"`dynamic atexit destructor for '", // dynamic dtor
            };
            std::wstring undec = Undecorate(decorated);
            for (const auto& m : undecMarkers)
            {
                if (undec.find(m) != std::wstring::npos)
                    return true;
            }

            // Anonymous-namespace lambdas: decorated contains ?A0x########@?1
            if (decorated.find(L"?A0x")     != std::wstring::npos &&
                    undec.find(L"<lambda_") != std::wstring::npos)
                return true;

            return false; // Everything else is potentially ODR-relevant
        }

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