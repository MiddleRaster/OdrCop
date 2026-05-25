#pragma once

#include "CoffReader.h"
#include "..\src\cvinfo.h"

#include <map>

struct CoffDumper
{
    static void Dump(const std::filesystem::path& objFile, std::wostream& out)
    {
        CoffReader coffReader(objFile);

        out << L"Dumping data in file order\n";
        DumpHeader        (coffReader, out);
        DumpSectionHeaders(coffReader, out);
        DumpRawData       (coffReader, out);
        DumpRelocationData(coffReader, out);
        DumpSymbols       (coffReader, out);
        DumpStringTable   (coffReader, out);

        DumpDataPerSection(coffReader, out);
    }
private:
    static void DumpHeader        (const CoffReader& coffReader, std::wostream& out)
    {
        auto imageFileHeader = coffReader.imageFileHeader;

        out << L"IMAGE_FILE_HEADER:\n";
        out << L"  WORD    Machine             : 0x" << std::hex <<                       imageFileHeader->Machine         << std::dec << L'\n';
        out << L"  WORD    NumberOfSections    : "               <<                       imageFileHeader->NumberOfSections            << L'\n';
        out << L"  DWORD   TimeDateStamp       : "               << TimeDateStampToString(imageFileHeader->TimeDateStamp)              << L'\n';
        out << L"  DWORD   PointerToSymbolTable: "               <<                       imageFileHeader->PointerToSymbolTable        << L'\n';
        out << L"  DWORD   NumberOfSymbols     : "               <<                       imageFileHeader->NumberOfSymbols             << L'\n';
        out << L"  DWORD   SizeOfOptionalHeader: "               <<                       imageFileHeader->SizeOfOptionalHeader        << L'\n';
        out << L"  DWORD   Characteristics     : 0x" << std::hex <<                       imageFileHeader->Characteristics << std::dec << L'\n';
    }
    static void DumpSectionHeaders(const CoffReader& coffReader, std::wostream& out)
    {
        auto sectionHeaders = coffReader.sectionHeaders;
        auto bytes          = coffReader.bytes;

        int count = 0;
        out << L"IMAGE_SECTION_HEADERs:\n";
        for (auto ish : sectionHeaders)
        {
            out << count++;
            DumpSectionHeader(ish, bytes, out);
        }
    }
    static void DumpRawData       (const CoffReader& coffReader, std::wostream& out)
    {
        auto imageFileHeader = coffReader.imageFileHeader;
        auto sectionHeaders  = coffReader.sectionHeaders;
        const BYTE* bytes    = coffReader.bytes;

        for(WORD i=0; i<imageFileHeader->NumberOfSections; ++i)
        {
            out << L"\nraw data for section " << i << L":\n" << std::hex << std::uppercase << std::setfill(L'0');
            DumpRawData(sectionHeaders[i], bytes, out);
            out << std::dec << L"\n";
        }
    }
    static void DumpRelocationData(const CoffReader& coffReader, std::wostream& out)
    {
        auto imageFileHeader = coffReader.imageFileHeader;
        auto sectionHeaders  = coffReader.sectionHeaders;
        auto relocations     = coffReader.relocations;
        auto symbolTable     = coffReader.symbolTable;
        auto stringTable     = coffReader.startOfStringTable;
        for (DWORD i=0; i<imageFileHeader->NumberOfSections; ++i)
        {
            out << L"\nSection " << i << L" has " << relocations[i].size() << L" relocation entries\n";
            DumpRelocations(sectionHeaders[i], relocations[i], symbolTable, stringTable, out, [](const IMAGE_SYMBOL*, const char*, std::wostream&) {});
            out << L'\n';
        }
    }
    static void DumpSymbols       (const CoffReader& coffReader, std::wostream& out)
    {
        auto symbolTable = coffReader.symbolTable;
        auto stringTable = coffReader.startOfStringTable;

        out << L"Symbols:\n";

        bool isAux = false;
        int auxRemaining = 0;
        for (auto symbol : symbolTable)
        {
            if (auxRemaining > 0)
            {   // This is an AUX record
                isAux = true;
                auxRemaining--;
            }
            else
            {   // This is a primary symbol
                isAux = false;
                auxRemaining = symbol->NumberOfAuxSymbols;
            }

            if (isAux)
                continue; // don't dump aux records

            out << L'\n';
            DumpSymbol(symbol, stringTable, out);
        }
    }
    void static DumpStringTable   (const CoffReader& coffReader, std::wostream& out)
    {
        auto imageFileHeader = coffReader.imageFileHeader;
        const BYTE* bytes    = coffReader.bytes;

        size_t offsetToStringTable = imageFileHeader->PointerToSymbolTable + imageFileHeader->NumberOfSymbols*sizeof(IMAGE_SYMBOL);
        DWORD sizeOfStringTable    = *reinterpret_cast<const DWORD*>(bytes + offsetToStringTable); // first 4 bytes are the size of the string table

        out << L"\nString Table\nSize: " << sizeOfStringTable << L'\n';

        offsetToStringTable += 4;
        sizeOfStringTable   -= 4;
        for (size_t sizeSoFar=0; sizeSoFar<sizeOfStringTable;)
        {
            const char* aString = reinterpret_cast<const char*>(bytes + offsetToStringTable + sizeSoFar);
            out << aString << L'\n';
            
            sizeSoFar += std::strlen(aString) + 1; // for null
        }
    }
    void static DumpDataPerSection(const CoffReader& coffReader, std::wostream& out)
    {
        out << L"\nDumping data per section, rather than in file order\n\n";

        DumpHeader(coffReader, out);

        auto imageFileHeaders   = coffReader.imageFileHeader;
        auto sectionHeaders     = coffReader.sectionHeaders;
        const BYTE* bytes       = coffReader.bytes;
        auto relocations        = coffReader.relocations;
        auto symbolTable        = coffReader.symbolTable;
        const char* stringTable = coffReader.startOfStringTable;
        for (WORD i=0; i<imageFileHeaders->NumberOfSections; ++i)
        {
            out << L"\nSection: " << i;

            // first dump section header
            DumpSectionHeader(sectionHeaders[i], bytes, out);

            // raw bytes
            out << L"\nRaw data for section " << i << L":\n" << std::hex << std::uppercase << std::setfill(L'0');
            DumpRawData(sectionHeaders[i], bytes, out);
            out << std::dec << L"\n";

            // relocations
            out << L"\nRelocations for section " << i << L":";

            std::wostringstream oss;
            DumpRelocations(sectionHeaders[i], relocations[i], symbolTable, stringTable, oss, 
                [](const IMAGE_SYMBOL*symbol, const char* stringTable, std::wostream& out)
                {
                    DumpSymbol(symbol, stringTable, out);
                });
            out << oss.str();
            if (oss.str() == L"")
                out << L" none";
            out << L'\n';

            // symbols are dumped with relocations
            // not dumping string table either
        }
        // not per section
        DumpFunctions(coffReader, out);
    }
    static void DumpSectionHeader (const IMAGE_SECTION_HEADER* ish, const BYTE* bytes, std::wostream& out)
    {
        std::string name;
        if (ish->Misc.VirtualSize == 0)
            name = std::string(ish->Name, ish->Name+8);
        else
        {
            name = std::string(bytes + ish->Misc.PhysicalAddress, bytes + ish->Misc.PhysicalAddress + ish->Misc.VirtualSize);
        }
        out << L'\t' << L"Name: " << std::wstring(name.begin(), name.end()) << L'\n';

        out << L'\t' << L"DWORD   VirtualAddress: "                << ish->VirtualAddress              << L'\n';
        out << L'\t' << L"DWORD   SizeOfRawData: "                 << ish->SizeOfRawData               << L'\n';
        out << L'\t' << L"DWORD   PointerToRawData: "              << ish->PointerToRawData            << L'\n';
        out << L'\t' << L"DWORD   PointerToRelocations: "          << ish->PointerToRelocations        << L'\n';
        out << L'\t' << L"DWORD   PointerToLinenumbers: "          << ish->PointerToLinenumbers        << L'\n';
        out << L'\t' << L"DWORD   NumberOfRelocations: "           << ish->NumberOfRelocations         << L'\n';
        out << L'\t' << L"DWORD   NumberOfLinenumbers: "           << ish->NumberOfLinenumbers         << L'\n';
        out << L'\t' << L"DWORD   Characteristics: 0x" << std::hex << ish->Characteristics << std::dec << L'\n';
        out << L"\t\t" << L"( ";

        // these are commented out in winnt.h
        constexpr DWORD IMAGE_SCN_TYPE_REG      = 0x00000000; // this will always be false
        constexpr DWORD IMAGE_SCN_TYPE_DSECT    = 0x00000001;
        constexpr DWORD IMAGE_SCN_TYPE_NOLOAD   = 0x00000002;
        constexpr DWORD IMAGE_SCN_TYPE_GROUP    = 0x00000004;
        constexpr DWORD IMAGE_SCN_TYPE_COPY     = 0x00000010;
        constexpr DWORD IMAGE_SCN_TYPE_OVER     = 0x00000400;
        constexpr DWORD IMAGE_SCN_MEM_PROTECTED = 0x00004000;
        constexpr DWORD IMAGE_SCN_MEM_SYSHEAP   = 0x00010000;

        if (ish->Characteristics & IMAGE_SCN_TYPE_REG               ) out << L"IMAGE_SCN_TYPE_REG ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_DSECT             ) out << L"IMAGE_SCN_TYPE_DSECT ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_NOLOAD            ) out << L"IMAGE_SCN_TYPE_NOLOAD ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_GROUP             ) out << L"IMAGE_SCN_TYPE_GROUP ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_NO_PAD            ) out << L"IMAGE_SCN_TYPE_NO_PAD ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_COPY              ) out << L"IMAGE_SCN_TYPE_COPY ";
        if (ish->Characteristics & IMAGE_SCN_CNT_CODE               ) out << L"IMAGE_SCN_CNT_CODE ";
        if (ish->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ) out << L"IMAGE_SCN_CNT_INITIALIZED_DATA ";
        if (ish->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ) out << L"IMAGE_SCN_CNT_UNINITIALIZED_DATA ";
        if (ish->Characteristics & IMAGE_SCN_LNK_OTHER              ) out << L"IMAGE_SCN_LNK_OTHER ";
        if (ish->Characteristics & IMAGE_SCN_LNK_INFO               ) out << L"IMAGE_SCN_LNK_INFO ";
        if (ish->Characteristics & IMAGE_SCN_TYPE_OVER              ) out << L"IMAGE_SCN_TYPE_OVER ";
        if (ish->Characteristics & IMAGE_SCN_LNK_REMOVE             ) out << L"IMAGE_SCN_LNK_REMOVE ";
        if (ish->Characteristics & IMAGE_SCN_LNK_COMDAT             ) out << L"IMAGE_SCN_LNK_COMDAT ";
        if (ish->Characteristics & 0x00002000                       ) out << L"0x00002000 (reserved) ";
        if (ish->Characteristics & IMAGE_SCN_MEM_PROTECTED          ) out << L"IMAGE_SCN_MEM_PROTECTED ";
        if (ish->Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC      ) out << L"IMAGE_SCN_NO_DEFER_SPEC_EXC ";
        if (ish->Characteristics & IMAGE_SCN_GPREL                  ) out << L"IMAGE_SCN_GPREL ";
        if (ish->Characteristics & IMAGE_SCN_MEM_FARDATA            ) out << L"IMAGE_SCN_MEM_FARDATA ";
        if (ish->Characteristics & IMAGE_SCN_MEM_SYSHEAP            ) out << L"IMAGE_SCN_MEM_SYSHEAP ";
        if (ish->Characteristics & IMAGE_SCN_MEM_PURGEABLE          ) out << L"IMAGE_SCN_MEM_PURGEABLE ";
        if (ish->Characteristics & IMAGE_SCN_MEM_16BIT              ) out << L"IMAGE_SCN_MEM_16BIT ";
        if (ish->Characteristics & IMAGE_SCN_MEM_LOCKED             ) out << L"IMAGE_SCN_MEM_LOCKED ";
        if (ish->Characteristics & IMAGE_SCN_MEM_PRELOAD            ) out << L"IMAGE_SCN_MEM_PRELOAD ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_1BYTES           ) out << L"IMAGE_SCN_ALIGN_1BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_2BYTES           ) out << L"IMAGE_SCN_ALIGN_2BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_4BYTES           ) out << L"IMAGE_SCN_ALIGN_4BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_8BYTES           ) out << L"IMAGE_SCN_ALIGN_8BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_16BYTES          ) out << L"IMAGE_SCN_ALIGN_16BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_32BYTES          ) out << L"IMAGE_SCN_ALIGN_32BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_64BYTES          ) out << L"IMAGE_SCN_ALIGN_64BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_128BYTES         ) out << L"IMAGE_SCN_ALIGN_128BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_256BYTES         ) out << L"IMAGE_SCN_ALIGN_256BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_512BYTES         ) out << L"IMAGE_SCN_ALIGN_512BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_1024BYTES        ) out << L"IMAGE_SCN_ALIGN_1024BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_2048BYTES        ) out << L"IMAGE_SCN_ALIGN_2048BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_4096BYTES        ) out << L"IMAGE_SCN_ALIGN_4096BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_8192BYTES        ) out << L"IMAGE_SCN_ALIGN_8192BYTES ";
        // if (ish->Characteristics & IMAGE_SCN_ALIGN_MASK             ) out << L"IMAGE_SCN_ALIGN_MASK ";
        if (ish->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL        ) out << L"IMAGE_SCN_LNK_NRELOC_OVFL ";
        if (ish->Characteristics & IMAGE_SCN_MEM_DISCARDABLE        ) out << L"IMAGE_SCN_MEM_DISCARDABLE ";
        if (ish->Characteristics & IMAGE_SCN_MEM_NOT_CACHED         ) out << L"IMAGE_SCN_MEM_NOT_CACHED ";
        if (ish->Characteristics & IMAGE_SCN_MEM_NOT_PAGED          ) out << L"IMAGE_SCN_MEM_NOT_PAGED ";
        if (ish->Characteristics & IMAGE_SCN_MEM_SHARED             ) out << L"IMAGE_SCN_MEM_SHARED ";
        if (ish->Characteristics & IMAGE_SCN_MEM_EXECUTE            ) out << L"IMAGE_SCN_MEM_EXECUTE ";
        if (ish->Characteristics & IMAGE_SCN_MEM_READ               ) out << L"IMAGE_SCN_MEM_READ ";
        if (ish->Characteristics & IMAGE_SCN_MEM_WRITE              ) out << L"IMAGE_SCN_MEM_WRITE ";
        out << L")\n";

        // fix up number of relocations if IMAGE_SCN_LNK_NRELOC_OVFL is set.
        if (ish->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
        {
            // ish->PointerToRelocations points to the 0th IMAGE_RELOCATION struct, whose DUMMYUNIONNAME::RelocCount is the real count of relocations
            out << L"IMAGE_SCN_LNK_NRELOC_OVFL is set. The relocation count is not " << ish->NumberOfRelocations << L", but rather ";

            const IMAGE_RELOCATION* pRelocation = reinterpret_cast<const IMAGE_RELOCATION*>(bytes + ish->PointerToRelocations);
            out << pRelocation->RelocCount << L'\n';
        }
    }
    static void DumpRawData       (const IMAGE_SECTION_HEADER* ish, const BYTE* bytes, std::wostream& out)
    {
        auto data = bytes + ish->PointerToRawData;
        for (DWORD j = 0; j < ish->SizeOfRawData; ++j)
            out << std::setw(2) << static_cast<unsigned>(*data++) << L' ';
    }
    template <typename Fn> static void DumpRelocations(const IMAGE_SECTION_HEADER* ish, const std::vector<const IMAGE_RELOCATION*>& relocations, const std::vector<const IMAGE_SYMBOL*>& symbolTable, const char* stringTable, std::wostream& out, Fn&& fn)
    {
        // from winnt.h
        //typedef struct _IMAGE_RELOCATION {
        //    union {
        //        DWORD   VirtualAddress;
        //        DWORD   RelocCount;             // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
        //    } DUMMYUNIONNAME;
        //    DWORD   SymbolTableIndex;
        //    WORD    Type;
        //} IMAGE_RELOCATION;

        bool hasOverflow = ish->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL;

        for(DWORD j=0; j<relocations.size(); ++j)
        {
            out << L"\nrelocation " << j << L":\n";

            auto relocation = relocations[j];
            if (hasOverflow)
                out << L"DWORD RelocCount:       " << relocation->RelocCount       << L'\n';
            else
                out << L"DWORD VirtualAddress:   " << relocation->VirtualAddress   << L'\n';

            out     << L"WORD  Type:             ";
            switch(relocation->Type)
            {
            case IMAGE_REL_AMD64_ABSOLUTE                  : out << L"IMAGE_REL_AMD64_ABSOLUTE                  "; break;
            case IMAGE_REL_AMD64_ADDR64                    : out << L"IMAGE_REL_AMD64_ADDR64                    "; break;
            case IMAGE_REL_AMD64_ADDR32                    : out << L"IMAGE_REL_AMD64_ADDR32                    "; break;
            case IMAGE_REL_AMD64_ADDR32NB                  : out << L"IMAGE_REL_AMD64_ADDR32NB                  "; break;
            case IMAGE_REL_AMD64_REL32                     : out << L"IMAGE_REL_AMD64_REL32                     "; break;
            case IMAGE_REL_AMD64_REL32_1                   : out << L"IMAGE_REL_AMD64_REL32_1                   "; break;
            case IMAGE_REL_AMD64_REL32_2                   : out << L"IMAGE_REL_AMD64_REL32_2                   "; break;
            case IMAGE_REL_AMD64_REL32_3                   : out << L"IMAGE_REL_AMD64_REL32_3                   "; break;
            case IMAGE_REL_AMD64_REL32_4                   : out << L"IMAGE_REL_AMD64_REL32_4                   "; break;
            case IMAGE_REL_AMD64_REL32_5                   : out << L"IMAGE_REL_AMD64_REL32_5                   "; break;
            case IMAGE_REL_AMD64_SECTION                   : out << L"IMAGE_REL_AMD64_SECTION                   "; break;
            case IMAGE_REL_AMD64_SECREL                    : out << L"IMAGE_REL_AMD64_SECREL                    "; break;
            case IMAGE_REL_AMD64_SECREL7                   : out << L"IMAGE_REL_AMD64_SECREL7                   "; break;
            case IMAGE_REL_AMD64_TOKEN                     : out << L"IMAGE_REL_AMD64_TOKEN                     "; break;
            case IMAGE_REL_AMD64_SREL32                    : out << L"IMAGE_REL_AMD64_SREL32                    "; break;
            case IMAGE_REL_AMD64_PAIR                      : out << L"IMAGE_REL_AMD64_PAIR                      "; break;
            case IMAGE_REL_AMD64_SSPAN32                   : out << L"IMAGE_REL_AMD64_SSPAN32                   "; break;
            case IMAGE_REL_AMD64_EHANDLER                  : out << L"IMAGE_REL_AMD64_EHANDLER                  "; break;
            case IMAGE_REL_AMD64_IMPORT_BR                 : out << L"IMAGE_REL_AMD64_IMPORT_BR                 "; break;
            case IMAGE_REL_AMD64_IMPORT_CALL               : out << L"IMAGE_REL_AMD64_IMPORT_CALL               "; break;
            case IMAGE_REL_AMD64_CFG_BR                    : out << L"IMAGE_REL_AMD64_CFG_BR                    "; break;
            case IMAGE_REL_AMD64_CFG_BR_REX                : out << L"IMAGE_REL_AMD64_CFG_BR_REX                "; break;
            case IMAGE_REL_AMD64_CFG_CALL                  : out << L"IMAGE_REL_AMD64_CFG_CALL                  "; break;
            case IMAGE_REL_AMD64_INDIR_BR                  : out << L"IMAGE_REL_AMD64_INDIR_BR                  "; break;
            case IMAGE_REL_AMD64_INDIR_BR_REX              : out << L"IMAGE_REL_AMD64_INDIR_BR_REX              "; break;
            case IMAGE_REL_AMD64_INDIR_CALL                : out << L"IMAGE_REL_AMD64_INDIR_CALL                "; break;
            case IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_FIRST: out << L"IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_FIRST"; break;
            case IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_LAST : out << L"IMAGE_REL_AMD64_INDIR_BR_SWITCHTABLE_LAST "; break;
            default                                        : out << relocation->Type;                              break;
            }
            out << L'\n';
            out << L"DWORD SymbolTableIndex: " << relocation->SymbolTableIndex << L'\n';

            fn(symbolTable[relocation->SymbolTableIndex], stringTable, out);
        }
    }
    static void DumpSymbol        (const IMAGE_SYMBOL* symbol, const char* stringTable, std::wostream& out)
    {
        //typedef struct _IMAGE_SYMBOL {
        //    union {
        //        BYTE    ShortName[8];
        //        struct {
        //            DWORD   Short;     // if 0, use LongName
        //            DWORD   Long;      // offset into string table
        //        } Name;
        //        DWORD   LongName[2];    // PBYTE [2]
        //    } N;
        //    DWORD   Value;
        //    SHORT   SectionNumber;
        //    WORD    Type;
        //    BYTE    StorageClass;
        //    BYTE    NumberOfAuxSymbols;
        //} IMAGE_SYMBOL;

        out << L"Name: ";
        std::string name;
        if (symbol->N.Name.Short != 0)
            name = std::string(symbol->N.ShortName, symbol->N.ShortName + 8);
        else
            name = std::string(stringTable + symbol->N.Name.Long);
        out << std::wstring(name.begin(), name.end()) << L'\n';

        out << L"DWORD   Value:              " << symbol->Value << L'\n';
        out << L"SHORT   SectionNumber:      ";
        switch (symbol->SectionNumber)
        {
        case IMAGE_SYM_DEBUG:     out << L"IMAGE_SYM_DEBUG";     break;
        case IMAGE_SYM_ABSOLUTE:  out << L"IMAGE_SYM_ABSOLUTE";  break;
        case IMAGE_SYM_UNDEFINED: out << L"IMAGE_SYM_UNDEFINED"; break;
        default:                  out << symbol->SectionNumber;  break;
        }
        out << L'\n';
        out << L"WORD    Type:               ";
        switch (symbol->Type)
        {
        case 0x20: out << L"IMAGE_SYM_TYPE_FUNCTION";   break;
        default  : out << symbol->Type;                 break;
        }
        out << L'\n';

        out << L"BYTE    StorageClass:       ";
        switch(symbol->StorageClass)
        {
        case IMAGE_SYM_CLASS_END_OF_FUNCTION : out << L"IMAGE_SYM_CLASS_END_OF_FUNCTION ";  break;
        case IMAGE_SYM_CLASS_NULL            : out << L"IMAGE_SYM_CLASS_NULL            ";  break;
        case IMAGE_SYM_CLASS_AUTOMATIC       : out << L"IMAGE_SYM_CLASS_AUTOMATIC       ";  break;
        case IMAGE_SYM_CLASS_EXTERNAL        : out << L"IMAGE_SYM_CLASS_EXTERNAL        ";  break;
        case IMAGE_SYM_CLASS_STATIC          : out << L"IMAGE_SYM_CLASS_STATIC          ";  break;
        case IMAGE_SYM_CLASS_REGISTER        : out << L"IMAGE_SYM_CLASS_REGISTER        ";  break;
        case IMAGE_SYM_CLASS_EXTERNAL_DEF    : out << L"IMAGE_SYM_CLASS_EXTERNAL_DEF    ";  break;
        case IMAGE_SYM_CLASS_LABEL           : out << L"IMAGE_SYM_CLASS_LABEL           ";  break;
        case IMAGE_SYM_CLASS_UNDEFINED_LABEL : out << L"IMAGE_SYM_CLASS_UNDEFINED_LABEL ";  break;
        case IMAGE_SYM_CLASS_MEMBER_OF_STRUCT: out << L"IMAGE_SYM_CLASS_MEMBER_OF_STRUCT";  break;
        case IMAGE_SYM_CLASS_ARGUMENT        : out << L"IMAGE_SYM_CLASS_ARGUMENT        ";  break;
        case IMAGE_SYM_CLASS_STRUCT_TAG      : out << L"IMAGE_SYM_CLASS_STRUCT_TAG      ";  break;
        case IMAGE_SYM_CLASS_MEMBER_OF_UNION : out << L"IMAGE_SYM_CLASS_MEMBER_OF_UNION ";  break;
        case IMAGE_SYM_CLASS_UNION_TAG       : out << L"IMAGE_SYM_CLASS_UNION_TAG       ";  break;
        case IMAGE_SYM_CLASS_TYPE_DEFINITION : out << L"IMAGE_SYM_CLASS_TYPE_DEFINITION ";  break;
        case IMAGE_SYM_CLASS_UNDEFINED_STATIC: out << L"IMAGE_SYM_CLASS_UNDEFINED_STATIC";  break;
        case IMAGE_SYM_CLASS_ENUM_TAG        : out << L"IMAGE_SYM_CLASS_ENUM_TAG        ";  break;
        case IMAGE_SYM_CLASS_MEMBER_OF_ENUM  : out << L"IMAGE_SYM_CLASS_MEMBER_OF_ENUM  ";  break;
        case IMAGE_SYM_CLASS_REGISTER_PARAM  : out << L"IMAGE_SYM_CLASS_REGISTER_PARAM  ";  break;
        case IMAGE_SYM_CLASS_BIT_FIELD       : out << L"IMAGE_SYM_CLASS_BIT_FIELD       ";  break;
        case IMAGE_SYM_CLASS_FAR_EXTERNAL    : out << L"IMAGE_SYM_CLASS_FAR_EXTERNAL    ";  break;
        case IMAGE_SYM_CLASS_BLOCK           : out << L"IMAGE_SYM_CLASS_BLOCK           ";  break;
        case IMAGE_SYM_CLASS_FUNCTION        : out << L"IMAGE_SYM_CLASS_FUNCTION        ";  break;
        case IMAGE_SYM_CLASS_END_OF_STRUCT   : out << L"IMAGE_SYM_CLASS_END_OF_STRUCT   ";  break;
        case IMAGE_SYM_CLASS_FILE            : out << L"IMAGE_SYM_CLASS_FILE            ";  break;
        case IMAGE_SYM_CLASS_SECTION         : out << L"IMAGE_SYM_CLASS_SECTION         ";  break;
        case IMAGE_SYM_CLASS_WEAK_EXTERNAL   : out << L"IMAGE_SYM_CLASS_WEAK_EXTERNAL   ";  break;
        case IMAGE_SYM_CLASS_CLR_TOKEN       : out << L"IMAGE_SYM_CLASS_CLR_TOKEN       ";  break;
        default                              : out << symbol->StorageClass;                 break;
        }
        out << L'\n';
        out << L"BYTE    NumberOfAuxSymbols: " << symbol->NumberOfAuxSymbols << L'\n';
    }

    static void DumpFunctions(const CoffReader& coffReader, std::wostream& out)
    {
        struct Function
        {
            const std::wstring name;
            const std::vector<BYTE> body;
            const bool hasInternalLinkage;
        };
        std::vector<Function> functions;

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

        // dump 'em; just to see what we've got ('cuz it looks like some are missing)
        out << L"\nOutputting function names sorted by section and offset:\n";
        for (auto& [sec, vec] : sectionIndexToOffsetFunction)
        {
            out << L"section number: " << sec << L'\n';
            for (auto& [offset, name] : vec)
                out << L"offset: " << offset << L" function: " << name << L'\n';
        }

        // use .debug$S sections' data (with relocation fixups) to get the bodies' lengths
        const BYTE* bytes = coffReader.bytes;
        for(SHORT i=0; i<sectionHeaders.size(); ++i)
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
                using VirtualAddressType = DWORD;
                using SectionIndexType   = SHORT;
                std::map<VirtualAddressType,SectionIndexType> fixups;
                auto relocations         = coffReader.relocations[i];
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
                                                auto startOfBody = bytes + coffReader.sectionHeaders[sectionIndex]->PointerToRawData + offset;
                                                std::vector<BYTE> body(startOfBody, startOfBody + procsym32->len);
                                                functions.push_back({decoratedName, body, rec->rectyp == S_LPROC32 || rec->rectyp == S_LPROC32_ID});
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


        out << L"\nFunctions and their bytecode:\n";
        for (const auto& [name, body, linkage]: functions)
        {
            out << name << L" with " << (linkage ? L"internal " : L"external ") << L"linkage\n";
            for (auto b : body)
                out << std::hex << std::uppercase << std::setfill(L'0') << b << L' ';
            out << L'\n';
        }
    }


private:
    static std::wstring TimeDateStampToString(DWORD stamp)
    {
        std::chrono::sys_seconds tp{std::chrono::seconds{stamp}};

        auto dp = floor<std::chrono::days>(tp);
        std::chrono::year_month_day ymd{dp};

        auto secs = duration_cast<std::chrono::seconds>(tp - dp);
        auto h = std::chrono::duration_cast<std::chrono::hours>(secs);
        secs  -= h;
        auto m = std::chrono::duration_cast<std::chrono::minutes>(secs);
        secs  -= m;
        auto s = secs;

        int      year  = int     (ymd.year ());
        unsigned month = unsigned(ymd.month());
        unsigned day   = unsigned(ymd.day  ());

        return std::format(L"{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC", year, month, day, h.count(), m.count(), s.count());
    }
};
