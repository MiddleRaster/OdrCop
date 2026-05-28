#pragma once

#include <windows.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <exception>
#include <chrono>

class CoffReader
{
    const struct ViewOfObj
    {
        const size_t size{};
        const BYTE* view = nullptr;
        ViewOfObj(const std::filesystem::path& objFile)
            : size([&]()
                {
                    size_t size = 0;
                    HANDLE file = CreateFileW(objFile.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (file != INVALID_HANDLE_VALUE)
                    {
                        LARGE_INTEGER fileSize{};
                        if (GetFileSizeEx(file, &fileSize))
                            size = static_cast<size_t>(fileSize.QuadPart);
                        ::CloseHandle(file);
                    }
                    return size;
                }())
            , view([&]()
                {
                    const BYTE* view = nullptr;
                    HANDLE file = CreateFileA(objFile.generic_string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (file != INVALID_HANDLE_VALUE)
                    {
                        LARGE_INTEGER fileSize{};
                        if (GetFileSizeEx(file, &fileSize))
                        {
                            HANDLE map = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
                            if (map   != nullptr) {
                                view   = (const BYTE*)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
                                ::CloseHandle(map);
                            }
                        }
                        ::CloseHandle(file);
                    }
                    return view;
                }())
        {}
       ~ViewOfObj() { ::UnmapViewOfFile(view); }
    } viewOfObj;

    // per https://www.ti.com/lit/an/spraao8/spraao8.pdf , a COFF file looks like this:

    // File feader
    // Optional file header
    // Section 1 through N headers
    // Section 1 through N raw data
    // Section 1 through N relocation data
    // Symbol table
    // String table

public: // a data object where everything is const
    const BYTE * bytes;
    const size_t size;
    const IMAGE_FILE_HEADER* imageFileHeader;
    // no optional file header
    const std::vector<const IMAGE_SECTION_HEADER*> sectionHeaders;
    // no struct for raw data
    const std::vector<std::vector<const IMAGE_RELOCATION*>> relocations;
    const std::vector<const IMAGE_SYMBOL*> symbolTable;
    const char* startOfStringTable;

public:
    CoffReader(const std::filesystem::path& objFile)
        : viewOfObj(objFile)
        , bytes(viewOfObj.view)
        , size(viewOfObj.size)
        , imageFileHeader([&]()
            {
                if (size < sizeof(IMAGE_FILE_HEADER))
                    throw std::invalid_argument("not an .obj file");

                const IMAGE_FILE_HEADER* ifh = (reinterpret_cast<const IMAGE_FILE_HEADER*>(bytes));
                if ((ifh->Machine != IMAGE_FILE_MACHINE_AMD64) &&
                    (ifh->Machine != IMAGE_FILE_MACHINE_I386 ) )
                    throw std::invalid_argument("not an 0x8664 or i386 .obj file");

                return ifh;
            }())
        , sectionHeaders([&]()
            {
                size_t offset = 0;
                offset += sizeof(IMAGE_FILE_HEADER);
                offset += imageFileHeader->SizeOfOptionalHeader;

                if (offset + imageFileHeader->NumberOfSections*sizeof(IMAGE_SECTION_HEADER) > imageFileHeader->PointerToSymbolTable)
                    throw std::invalid_argument("corrupted .obj file");

                std::vector<const IMAGE_SECTION_HEADER*> ishs;
                for (DWORD i=0; i<imageFileHeader->NumberOfSections; ++i)
                {
                    ishs.push_back(reinterpret_cast<const IMAGE_SECTION_HEADER*>(bytes + offset));
                    offset += sizeof(IMAGE_SECTION_HEADER);
                }
                return ishs;
            }())
        , relocations([&]()
            {
                std::vector<std::vector<const IMAGE_RELOCATION*>> vectorOfVectors;
                vectorOfVectors.resize(imageFileHeader->NumberOfSections);

                for (DWORD i=0; i<imageFileHeader->NumberOfSections; ++i)
                {
                    size_t offset = sectionHeaders[i]->PointerToRelocations;

                    // calculate number of relocations, which has funky overflow mechanism
                    DWORD numberOfRelocations = sectionHeaders[i]->NumberOfRelocations;
                    if (sectionHeaders[i]->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
                    {
                        numberOfRelocations = (reinterpret_cast<const IMAGE_RELOCATION*>(bytes + offset))->RelocCount;
                        offset += sizeof(IMAGE_RELOCATION);   // skip the count record
                    }

                    if (offset + numberOfRelocations*sizeof(IMAGE_RELOCATION) > imageFileHeader->PointerToSymbolTable)
                        throw std::invalid_argument("corrupted .obj file");

                    for(DWORD j=0; j<numberOfRelocations; ++j)
                        vectorOfVectors[i].push_back(reinterpret_cast<const IMAGE_RELOCATION*>(bytes + offset + j*sizeof(IMAGE_RELOCATION)));
                }
                return vectorOfVectors;
            }())
        , symbolTable([&]()
            {
                std::vector<const IMAGE_SYMBOL*> symbols;

                size_t offsetToStringTable = imageFileHeader->PointerToSymbolTable + imageFileHeader->NumberOfSymbols*sizeof(IMAGE_SYMBOL);
                DWORD sizeOfStringTable    = *reinterpret_cast<const DWORD*>(bytes + offsetToStringTable); // first 4 bytes are the size of the string table
                if (offsetToStringTable + sizeOfStringTable > size)
                    throw std::invalid_argument("corrupted .obj file");

                DWORD last = imageFileHeader->PointerToSymbolTable + imageFileHeader->NumberOfSymbols*sizeof(IMAGE_SYMBOL); // each AUX record is the same size as IMAGE_SYMBOL and is included in the NumberOfSymbols
                for(size_t offset = imageFileHeader->PointerToSymbolTable; offset<last; )
                {
                    const IMAGE_SYMBOL* symbol = reinterpret_cast<const IMAGE_SYMBOL*>(bytes + offset);
                    symbols.push_back  (symbol);
                    offset += sizeof(IMAGE_SYMBOL);

                    for (BYTE i=0; i<symbol->NumberOfAuxSymbols; ++i)
                    {
                        auto aux = reinterpret_cast<const IMAGE_SYMBOL*>(bytes + offset);
                        symbols.push_back(aux);
                        offset += sizeof(IMAGE_SYMBOL);
                    }
                }
                return symbols;
            }())
        , startOfStringTable(reinterpret_cast<const char*>(bytes + imageFileHeader->PointerToSymbolTable + imageFileHeader->NumberOfSymbols*sizeof(IMAGE_SYMBOL)))
    {}
};
