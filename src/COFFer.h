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

#include "cvinfo.h"

namespace Odr
{
    class FuncInfo
    {
        const std::wstring compiland;
        const std::wstring functionName;
        const ULONGLONG bodyLength;
        const std::vector<BYTE> body;
    public:
        FuncInfo(const std::wstring& compiland, const std::wstring& functionName, ULONGLONG bodyLength, const std::vector<BYTE>& body)
            : compiland(compiland)
            , functionName(functionName)
            , bodyLength(bodyLength)
            , body(body)
        {}
        void Print() const
        {
            PrintCompilandPath();
            std::wcout << L"    function body length: " << bodyLength << L'\n';
            std::wcout << L"    the first few bytes are: " << std::hex;
            auto count = 10<body.size() ? 10 : body.size();
            for(auto i=0; i<count; ++i)
                std::wcout << std::setfill(L'0') << std::setw(2) << body[i] << L' ';

            std::wcout << std::dec << L'\n';
        }
        void PrintCompilandPath() const { std::wcout << L"  [" << compiland << L"]\n"; }

        friend bool operator==(const FuncInfo& a, const FuncInfo& b) { return  a.IsEqualTo(b); }
        friend bool operator!=(const FuncInfo& a, const FuncInfo& b) { return !a.IsEqualTo(b); }
    private:
        bool IsEqualTo(const FuncInfo& other) const
        {
         // if (compiland    != other.compiland)    return false; // compilands must be different for ODR violations
            if (functionName != other.functionName) return false;
            if (bodyLength   != other.bodyLength)   return false;
            if (body         != other.body)         return false;
            return true;
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
        static void Read(const std::wstring& pdbPath, std::map<std::wstring, std::vector<FuncInfo>>& funcMap)
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
            for (IMAGE_SECTION_HEADER* sec : coff.debugS)
            {
                const IMAGE_SECTION_HEADER* textSec = coff.FindTextSectionForDebugS(sec);
                if (!textSec)
                    continue;

                DebugS ds(coff.Data(sec), coff.End(sec));
                for (const PROCSYM32* proc : ds.GetProcs())
                {
                    std::wstring name(proc->name, proc->name + strlen((const char*)proc->name));

                    const BYTE* funcBytes = coff.base + textSec->PointerToRawData + proc->off;
                    funcMap[name].push_back(FuncInfo(objPath, name, proc->len, std::vector<BYTE>(funcBytes, funcBytes + proc->len)));
                }
            }
        }
    private:
        const IMAGE_SYMBOL* SymbolTable() const
        {
            return reinterpret_cast<const IMAGE_SYMBOL*>(base + coffHdr->PointerToSymbolTable);
        }
        const IMAGE_SECTION_HEADER* FindTextSectionForDebugS(const IMAGE_SECTION_HEADER* debugSSec) const
        {
            if (debugSSec->NumberOfRelocations == 0)
                return nullptr;

            const IMAGE_RELOCATION* relocs = reinterpret_cast<const IMAGE_RELOCATION*>(base + debugSSec->PointerToRelocations);

            const IMAGE_SYMBOL* symTab = SymbolTable();
            for (WORD i=0; i<debugSSec->NumberOfRelocations; ++i)
            {
                const IMAGE_RELOCATION& rel = relocs[i];
                const IMAGE_SYMBOL    & sym = symTab[rel.SymbolTableIndex];
                      SHORT           secNo = sym.SectionNumber; // 1-based
                if (secNo < 1 || secNo>(SHORT)sections.size())
                    continue;

                IMAGE_SECTION_HEADER* candidate = sections[secNo-1];
                char name[9] = {};
                memcpy(name, candidate->Name, 8);
                if (strncmp(name, ".text", 5) == 0)
                    return candidate;
            }
            return nullptr;
        }
    };
}