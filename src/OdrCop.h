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

#include "UdtInfo.h"
#include "EnumInfo.h"
#include "TDefInfo.h"
#include "COFFer.h"

namespace Odr
{
    class Cop
    {
        std::map<std::wstring, std::vector<UdtInfo >>  udtMap; // user-defined types
        std::map<std::wstring, std::vector<EnumInfo>> enumMap; // enums
        std::map<std::wstring, std::vector<FuncInfo>> funcMap; // functions
        std::map<std::wstring, std::vector<TDefInfo>> tdefMap; // typedefs

    public:
        Cop() { CoInitialize(nullptr); }
       ~Cop() { CoUninitialize(); }

        HRESULT LoadPdb(const std::wstring& path, bool excludeStdlib)
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
                            CComPtr<IDiaEnumSymbols> syms;
                            if (SUCCEEDED(hr = global->findChildren(SymTagNull, nullptr, nsNone, &syms)))
                            {
                                while(true)
                                {
                                    ULONG fetched = 0;
                                    CComPtr<IDiaSymbol> sym;
                                    if (FAILED(syms->Next(1, &sym, &fetched)) || fetched == 0)
                                        break;

                                    if (TRUE == Get(sym, &IDiaSymbol::get_exportIsForwarder))
                                        continue; // this is a forward declaration; never part of ODR violations

                                    CComBSTR name = Get(sym, &IDiaSymbol::get_name);

                                    if (excludeStdlib == true)
                                    if (name && name[0] != L'\0')
                                    if (QualifiedName(sym).starts_with(L"std::"))
                                        continue;

                                    enum SymTagEnum tag = static_cast<enum SymTagEnum>(Get(sym, &IDiaSymbol::get_symTag));
                                    switch(tag)
                                    {
                                    case SymTagEnum::SymTagUDT:
                                        if (name && name[0] != L'\0')
                                        {
                                            /*
                                            Lambda closure types are never ODR‑relevant
                                                A lambda’s closure type :
                                                    is unnamed
                                                    is unique per expression
                                                    is unique per TU
                                                    is not required to match across TUs
                                                    is not required to have stable layout
                                                    is not governed by the ODR at all
                                            Therefore, comparing lambda closure types across TUs is meaningless.
                                                It will always produce false positives.
                                                Suppressing them is not a heuristic — it is the correct interpretation of the C++ standard.
                                            */
                                            if (std::wstring(name).find(L"<lambda_") != std::wstring::npos)
                                                break;

                                            if (Get(sym, &IDiaSymbol::get_scoped)) // this may not be the right way to see if my type is defined locally
                                                break;                             // in a function or a block but everything else the LLMs suggested failed.

                                            std::wstring qualifiedName = QualifiedName(sym);
                                            // Anonymous-namespace types are TU-unique; never compare at top level
                                            if (qualifiedName.find(L"`anonymous-namespace'") != std::wstring::npos)
                                                break;

                                            UdtInfo udtInfo(session, sym, path, qualifiedName);
                                            std::wstring key = BuildUdtKey(sym, udtInfo.GetFirstMemberName());
                                            udtMap[key].push_back(std::move(udtInfo));
                                        }
                                        break;
                                    case (enum SymTagEnum)::SymTagEnum:
                                        if (name && name[0] != L'\0')
                                        {
                                            std::wstring key(QualifiedName(sym));
                                            enumMap[key].push_back(EnumInfo(path, key, sym));
                                        }
                                        break;
                                    case SymTagEnum::SymTagTypedef:
                                        if (name && name[0] != L'\0')
                                        {
                                            std::wstring key(QualifiedName(sym));
                                            if (excludeStdlib == true)
                                                if (std::wstring(key).starts_with(L"std::"))
                                                    break;

                                            tdefMap[key].push_back(TDefInfo(std::wstring(name), sym, path));
                                        }
                                        break;
                                    default:
                                        break;
                                    }
                                }
                            }

                            // Functions
                            COFF::Read(path, excludeStdlib, funcMap);

                        } else std::wcerr <<                  L"get_globalScope failed with 0x" << std::hex << hr << std::dec << L'\n';
                    }     else std::wcerr <<                      L"openSession failed with 0x" << std::hex << hr << std::dec << L'\n';
                }         else std::wcerr << L"loadDataFromPdb failed: " << path << L" with 0x" << std::hex << hr << std::dec << L'\n';
            }             else std::wcerr <<  L"CoCreateInstance(DiaSource) failed for with 0x" << std::hex << hr << std::dec << L'\n';
            return hr;
        }
        int ReportViolations() const
        {
            int
            violationCount  = ReportMapViolations( udtMap, [](const auto&  d) {  d.PrintPdbPath();       }, [](const auto&,    const auto&   ) -> int { return 0;                    }, [](const auto&,   int  ) {});
            violationCount += ReportMapViolations(funcMap, [](const auto& fi) { fi.PrintCompilandPath(); }, [](const auto& f1, const auto& f2) -> int { return f1.MismatchIndex(f2); }, [](const auto& f, int m) { f.PrintMismatch(m); });
            violationCount += ReportMapViolations(enumMap, [](const auto&   ) {                          }, [](const auto&,    const auto&   ) -> int { return 0;                    }, [](const auto&,   int  ) {});
            violationCount += ReportMapViolations(tdefMap, [](const auto&  t) {  t.PrintPdbPath();       }, [](const auto&,    const auto&   ) -> int { return 0;                    }, [](const auto&,   int  ) {});
            return violationCount;
        }

    private:
        template<typename Map, typename PrintPath, typename GetMismatchIndex, typename PrintMismatch> static int ReportMapViolations(Map& map, PrintPath printPath, GetMismatchIndex&& getMismatchIndex, PrintMismatch&& printMismatch)
        {
            int violationCount = 0;
            for (auto& [name, items] : map)
            {
                if (items.size() < 2)
                    continue;

                if (std::all_of(items.begin() + 1, items.end(), [&](const auto& x) { return x == items[0]; }))
                    continue;

                // find mismatch index
                int mismatch = -1;
                for (size_t m=1; m<items.size(); ++m)
                {
                    if (-1 != (mismatch = getMismatchIndex(items[0], items[m])))
                        break;
                }

                ++violationCount;
                std::wcout << L"ODR VIOLATION: " << name << L'\n';

                std::vector<bool> printed(items.size(), false);
                for (size_t i=0; i<items.size(); ++i)
                {
                    if (printed[i]) continue;
                    items[i].Print(0);
                    printMismatch(items[i], mismatch);
                    printed[i] = true;

                    for (size_t j=i+1; j<items.size(); ++j)
                    {
                        if (!printed[j] && (items[i] == items[j]))
                        {
                            printPath(items[j]);
                            printed[j] = true;
                        }
                    }
                }
                std::wcout << L'\n';
            }
            return violationCount;
        }

        static std::wstring BuildUdtKey(IDiaSymbol* sym, const std::wstring& firstMemberName)
        {
            CComBSTR name = Get(sym, &IDiaSymbol::get_name);
            std::wstring key(name.m_str);

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

                /*
                another example is "<unnamed-type-u>" which even after appending extra uniqueness info, generates this ODR violation:
                ODR VIOLATION: <unnamed-type-u>[struct][size=8][first=HighPart]
                      [TUs\x64\Debug\dllmain.pdb]
                        kind=struct  size=8
                        +0  unsigned long  LowPart
                        +4           long  HighPart
                      [TUs\x64\Debug\dllmain.pdb]
                        kind=struct  size=8
                        +0  unsigned long  LowPart
                        +4  unsigned long  HighPart

                In real life, this is merely an anonymous struct inside an outer struct/class/union, like LARGE_INTEGER or ULARGE_INTEGER.
                E.g.,
                    typedef union _LARGE_INTEGER {
                        struct {
                            DWORD LowPart;
                            LONG HighPart;
                        } DUMMYSTRUCTNAME;
                        struct {
                            DWORD LowPart;
                            LONG HighPart;
                        } u;
                        LONGLONG QuadPart;
                    } LARGE_INTEGER;
                and
                    typedef union _ULARGE_INTEGER {
                        struct {
                            DWORD LowPart;
                            DWORD HighPart;
                        } DUMMYSTRUCTNAME;
                        struct {
                            DWORD LowPart;
                            DWORD HighPart;
                        } u;
                        ULONGLONG QuadPart;
                    } ULARGE_INTEGER;
                */

                if (key.starts_with(L"<unnamed-type"))
                    key = QualifiedName(sym); // get the outer union/struct/class name (go all the way up)

                ULONGLONG size = Get(sym, &IDiaSymbol::get_length);
                DWORD     kind = Get(sym, &IDiaSymbol::get_udtKind);
                key += L"[" + std::wstring(kind == UdtUnion  ? L"union" :
                                           kind == UdtClass  ? L"class" :
                                                               L"struct") + L"]";
                key += L"[size=" + std::to_wstring(size)                  + L"]";
                if (firstMemberName != L"")
                    key += L"[first=" + firstMemberName                   + L"]";

                return key;
            }
            return QualifiedName(sym);
        }
        static std::wstring QualifiedName(IDiaSymbol* sym)
        {
            std::wstring name;

            CComBSTR bstrName;
            if (SUCCEEDED(sym->get_name(&bstrName)) && bstrName)
                name = bstrName;
            else
                name = L"<unnamed>";

            CComPtr<IDiaSymbol> parent;
            if (SUCCEEDED(sym->get_classParent(&parent)) && parent)
            {
                DWORD parentTag = SymTagNull;
                parent->get_symTag(&parentTag);
                if (parentTag == SymTagUDT || parentTag == SymTagEnum)
                    return QualifiedName(parent) + L"::" + name;
            }
            return name;
        }
    };
}
