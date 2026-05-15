#pragma once

#include <sal.h>
#pragma warning(push)
#pragma warning(disable: 4201)
#include "cvinfo.h"
#pragma warning(pop)

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <iosfwd>
#include <unordered_map>
#include <map>
#include <cstdint>
#include <string_view>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>
#include <array>
#include <fstream>

namespace cvdump
{
    constexpr std::uint32_t imageDebugTypeCodeView = 2;

    constexpr std::uint32_t pdbStreamPdb = 1;
    constexpr std::uint32_t pdbStreamTpi = 2;
    constexpr std::uint32_t pdbStreamDbi = 3;
    constexpr std::uint32_t pdbStreamIpi = 4;

    [[nodiscard]] inline static std::string_view machine_name(const std::uint16_t machine)
    {
        switch (machine) {
        case 0x014c: return "x86";
        case 0x01c0: return "ARM";
        case 0x01c4: return "ARMv7";
        case 0x01f0: return "PowerPC";
        case 0x0200: return "IA64";
        case 0x8664: return "x64";
        case 0xaa64: return "ARM64";
        case 0x0ebc: return "EFI bytecode";
        default    : return "unknown";
        }
    }

    [[nodiscard]] inline static std::string_view debug_type_name(const std::uint32_t type)
    {
        switch (type) {
        case  0: return "Unknown";
        case  1: return "COFF";
        case  2: return "CodeView";
        case  3: return "FPO";
        case  4: return "Misc";
        case  5: return "Exception";
        case  6: return "Fixup";
        case  9: return "Borland";
        case 10: return "Reserved10";
        case 11: return "CLSID";
        case 12: return "VC Feature";
        case 13: return "POGO";
        case 14: return "ILTCG";
        case 15: return "MPX";
        case 16: return "Repro";
        case 17: return "Embedded Portable PDB";
        case 19: return "Extended DLL Characteristics";
        default: return "unknown";
        }
    }

    [[nodiscard]] inline static std::string_view TypeLeafName(const std::uint16_t leaf)
    {
        switch (leaf) {
        case LF_MODIFIER:      return "LF_MODIFIER";
        case LF_POINTER:       return "LF_POINTER";
        case LF_ARRAY_ST:      return "LF_ARRAY_ST";
        case LF_PROCEDURE:     return "LF_PROCEDURE";
        case LF_MFUNCTION:     return "LF_MFUNCTION";
        case LF_ARGLIST:       return "LF_ARGLIST";
        case LF_FIELDLIST:     return "LF_FIELDLIST";
        case LF_BITFIELD:      return "LF_BITFIELD";
        case LF_METHODLIST:    return "LF_METHODLIST";
        case LF_BCLASS:        return "LF_BCLASS";
        case LF_VFUNCTAB:      return "LF_VFUNCTAB";
        case LF_ONEMETHOD:     return "LF_ONEMETHOD";
        case LF_ENUMERATE:     return "LF_ENUMERATE";
        case LF_ARRAY:         return "LF_ARRAY";
        case LF_CLASS:         return "LF_CLASS";
        case LF_STRUCTURE:     return "LF_STRUCTURE";
        case LF_UNION:         return "LF_UNION";
        case LF_ENUM:          return "LF_ENUM";
        case LF_PRECOMP:       return "LF_PRECOMP";
        case LF_ALIAS:         return "LF_ALIAS";
        case LF_TYPESERVER2:   return "LF_TYPESERVER2";
        case LF_VFTABLE:       return "LF_VFTABLE";
        case LF_FUNC_ID:       return "LF_FUNC_ID";
        case LF_MFUNC_ID:      return "LF_MFUNC_ID";
        case LF_BUILDINFO:     return "LF_BUILDINFO";
        case LF_SUBSTR_LIST:   return "LF_SUBSTR_LIST";
        case LF_STRING_ID:     return "LF_STRING_ID";
        case LF_UDT_SRC_LINE:  return "LF_UDT_SRC_LINE";
        case LF_VBCLASS:       return "LF_VBCLASS";
        case LF_VTSHAPE:       return "LF_VTSHAPE";
        case LF_MEMBER:        return "LF_MEMBER";
        case LF_METHOD:        return "LF_METHOD";
        case LF_NESTTYPE:      return "LF_NESTTYPE";
        case LF_STMEMBER:      return "LF_STMEMBER";
        default:               return "LF_?";
        }
    }

    [[nodiscard]] inline static std::string_view SymbolKindName(const std::uint16_t kind)
    {
        switch (kind) {
        case S_END:            return "S_END";
        case S_OBJNAME:        return "S_OBJNAME";
        case S_COMPILE2:       return "S_COMPILE2";
        case S_THUNK32:        return "S_THUNK32";
        case S_BLOCK32:        return "S_BLOCK32";
        case S_WITH32:         return "S_WITH32";
        case S_LABEL32:        return "S_LABEL32";
        case S_REGISTER:       return "S_REGISTER";
        case S_CONSTANT:       return "S_CONSTANT";
        case S_UDT:            return "S_UDT";
        case S_COBOLUDT:       return "S_COBOLUDT";
        case S_MANYREG:        return "S_MANYREG";
        case S_BPREL32:        return "S_BPREL32";
        case S_LDATA32:        return "S_LDATA32";
        case S_GDATA32:        return "S_GDATA32";
        case S_PUB32:          return "S_PUB32";
        case S_LPROC32:        return "S_LPROC32";
        case S_GPROC32:        return "S_GPROC32";
        case S_VFTABLE32:      return "S_VFTABLE32";
        case S_REGREL32:       return "S_REGREL32";
        case S_LTHREAD32:      return "S_LTHREAD32";
        case S_GTHREAD32:      return "S_GTHREAD32";
        case S_FRAMEPROC:      return "S_FRAMEPROC";
        case S_COMPILE3:       return "S_COMPILE3";
        case S_LOCAL:          return "S_LOCAL";
        case S_DEFRANGE:       return "S_DEFRANGE";
        case S_INLINESITE:     return "S_INLINESITE";
        case S_HEAPALLOCSITE:  return "S_HEAPALLOCSITE";
        case S_FRAMECOOKIE:    return "S_FRAMECOOKIE";
        case S_BUILDINFO:      return "S_BUILDINFO";
        default:               return "S_?";
        }
    }

    class ByteView
    {
        std::span<const std::byte> data{};

        void Read_into(const std::size_t offset, void* const destination, const std::size_t count) const
        {
            if (offset > data.size() || count > data.size() - offset)
                throw std::runtime_error("file is truncated or has an invalid offset");

            std::copy_n(data.data() + offset, count, static_cast<std::byte*>(destination));
        }

    public:
        ByteView() = default;
        explicit ByteView(std::span<const std::byte> data) : data(data) {}

        [[nodiscard]] std::size_t                Size () const { return data.size(); }
        [[nodiscard]] const std::byte*           Data () const { return data.data(); }
        [[nodiscard]] std::span<const std::byte> Span () const { return data; }
        [[nodiscard]] ByteView Subview(std::size_t offset, std::size_t count) const
        {
            if (offset > data.size() || count > data.size() - offset)
                throw std::runtime_error("file is truncated or has an invalid offset");
            return ByteView(data.subspan(offset, count));
        }
        [[nodiscard]] bool Starts_with(std::string_view text) const
        {
            if (text.size() > data.size())
                return false;
            for (std::size_t i=0; i<text.size(); ++i)
                if (static_cast<char>(data[i]) != text[i])
                    return false;
            return true;
        }

        template <typename T>
        [[nodiscard]] T Read(std::size_t offset) const
        {
            T value{};
            Read_into(offset, &value, sizeof(T));
            return value;
        }
        [[nodiscard]] std::string Read_c_string(const std::size_t offset, const std::size_t maxCount) const
        {
            if (offset > data.size())
                throw std::runtime_error("string offset is outside the file");

            const std::size_t limit = std::min(maxCount, data.size() - offset);
            std::string result;
            result.reserve(limit);
            for (std::size_t i=0; i<limit; ++i)
            {
                const char ch = static_cast<char>(data[offset + i]);
                if (ch == '\0')
                    break;
                result.push_back(ch);
            }
            return result;
        }
    };

    class OwnedFile
    {
        std::filesystem::path  path;
        std::vector<std::byte> bytes;
    public:
        static OwnedFile Load(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary | std::ios::ate);
            if (!input)
                throw std::runtime_error("could not open input file: " + path.string());

            const std::streamoff fileSize = input.tellg();
            if (fileSize < 0)
                throw std::runtime_error("could not determine input file size");

            OwnedFile file;
            file.path = path;
            file.bytes.resize(static_cast<std::size_t>(fileSize));
            input.seekg(0, std::ios::beg);
            if (!file.bytes.empty()) {
                input.read(reinterpret_cast<char*>(file.bytes.data()), static_cast<std::streamsize>(file.bytes.size()));
                if (!input)
                    throw std::runtime_error("could not read input file");
            }
            return file;
        }

        [[nodiscard]] ByteView                     View() const { return ByteView(bytes); }
        [[nodiscard]] const std::filesystem::path& Path() const { return path; }
    };

    [[nodiscard]] inline static std::string Narrow(const std::wstring& text)
    {
        std::string result;
        result.reserve(text.size());
        for (const wchar_t ch : text)
            result.push_back(ch >= 0 && ch <= 0x7f ? static_cast<char>(ch) : '?');
        return result;
    }
    [[nodiscard]] inline static std::string Hex32(const std::uint32_t value)
    {
        std::ostringstream out;
        out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
        return out.str();
    }
    [[nodiscard]] inline static std::string Hex16(const std::uint16_t value)
    {
        std::ostringstream out;
        out << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
        return out.str();
    }
    [[nodiscard]] inline static std::string Guid_to_string(const std::byte* const bytes)
    {
        const auto u8 = [bytes](const std::size_t index) { return static_cast<unsigned>(std::to_integer<unsigned char>(bytes[index])); };

        std::ostringstream out;
        out << std::hex << std::nouppercase << std::setfill('0')
            << std::setw(2) << u8( 3) << std::setw(2) << u8( 2) << std::setw(2) << u8(1) << std::setw(2) << u8(0) << '-'
            << std::setw(2) << u8( 5) << std::setw(2) << u8( 4) << '-'
            << std::setw(2) << u8( 7) << std::setw(2) << u8( 6) << '-'
            << std::setw(2) << u8( 8) << std::setw(2) << u8( 9) << '-'
            << std::setw(2) << u8(10) << std::setw(2) << u8(11) << std::setw(2) << u8(12)
            << std::setw(2) << u8(13) << std::setw(2) << u8(14) << std::setw(2) << u8(15);
        return out.str();
    }
    [[nodiscard]] inline static std::string Printable_name(const ByteView view)
    {
        std::string result;
        result.reserve(view.Size());
        for (const std::byte b : view.Span())
        {
            const unsigned char ch = std::to_integer<unsigned char>(b);
            if (ch == '\0')
                break;
            result.push_back(ch >= 32 && ch < 127 ? static_cast<char>(ch) : '.');
        }
        return result;
    }

    namespace
    {
        constexpr char             pdbMagicBytes[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0";
        constexpr std::string_view pdbMagic(pdbMagicBytes, 32);
        constexpr std::uint32_t   streamNil        = 0xffffffffU;

        static_assert(sizeof(TYPTYPE)                 == 4);
        static_assert(sizeof(lfModifier)              == 8);
        static_assert(sizeof(lfPointerBody)           == 10);
        static_assert(sizeof(lfBitfield)              == 8);
        static_assert(sizeof(lfVFuncTab)              == 8);
        static_assert(sizeof(mlMethod)                == 8);
        static_assert(offsetof(PROCSYM32, name)       == 39);
        static_assert(offsetof(BPRELSYM32, name)      == 12);
        static_assert(offsetof(DATASYM32, name)       == 14);
        static_assert(offsetof(PUBSYM32, name)        == 14);
        static_assert(offsetof(REGREL32, name)        == 14);
        static_assert(offsetof(LOCALSYM, name)        == 10);

        inline static std::uint32_t Align4       (const std::uint32_t value) { return (value + 3U) & ~3U; }
        inline static std::uint32_t BlockCountFor(const std::uint32_t byteCount, const std::uint32_t blockSize) { return (byteCount + blockSize - 1U)/blockSize; }

        inline static std::size_t CStringLength(const ByteView view, const std::size_t offset)
        {
            std::size_t current = offset;
            while (current < view.Size() && view.Read<char>(current) != '\0')
                ++current;
            if (current >= view.Size())
                throw std::runtime_error("unterminated string in PDB stream");
            return current - offset;
        }
        inline static std::size_t SkipNumeric(const ByteView view, const std::size_t offset)
        {
            const std::uint16_t kind = view.Read<std::uint16_t>(offset);
            if (kind < LF_NUMERIC)
                return offset + sizeof(std::uint16_t);
            switch (kind) {
            case LF_CHAR:      return offset + 3;
            case LF_SHORT:
            case LF_USHORT:    return offset + 4;
            case LF_LONG:
            case LF_ULONG:
            case LF_REAL32:    return offset + 6;
            case LF_REAL64:    return offset + 10;
            case LF_QUADWORD:
            case LF_UQUADWORD: return offset + 10;
            default:           return offset + 2;
            }
        }
        inline static std::string ReadRecordString(const ByteView view, const std::size_t offset)
        {
            if (offset >= view.Size())
                return {};
            return view.Read_c_string(offset, view.Size() - offset);
        }
        inline static std::string BuiltinTypeName(const std::uint32_t typeIndex)
        {
            switch (typeIndex) {
            case 0x0000: return "T_NOTYPE";
            case 0x0003: return "void";
            case 0x0010: return "char";
            case 0x0011: return "short";
            case 0x0012: return "long";
            case 0x0013: return "__int64";
            case 0x0020: return "unsigned char";
            case 0x0021: return "unsigned short";
            case 0x0022: return "unsigned long";
            case 0x0023: return "unsigned __int64";
            case 0x0030: return "bool";
            case 0x0040: return "float";
            case 0x0041: return "double";
            case 0x0070: return "char";
            case 0x0074: return "int";
            case 0x0075: return "unsigned int";
            case 0x0103: return "void*";
            case 0x0603: return "void __ptr64*";
            case 0x0670: return "char __ptr64*";
            case 0x0674: return "int __ptr64*";
            default:
                if (typeIndex < CV_FIRST_NONPRIM)
                    return Hex32(typeIndex);
                return {};
            }
        }
        inline static std::string CompactHex(const std::uint32_t value)
        {
            std::ostringstream out;
            out << "0x" << std::uppercase << std::hex << value;
            return out.str();
        }
        inline static std::string JoinArguments(const std::vector<std::string>& arguments)
        {
            std::ostringstream out;
            for (std::size_t i=0; i<arguments.size(); ++i)
            {
                if (i != 0)
                    out << ", ";
                out << arguments[i];
            }
            return out.str();
        }

        [[nodiscard]] inline static bool        IsFieldListPadding  (const std::uint16_t leaf) { return leaf >= LF_PAD0 && leaf <= LF_PAD15; }
        [[nodiscard]] inline static std::size_t SkipFieldListPadding(const ByteView view, std::size_t cursor)
        {
            while (cursor < view.Size()) {
                const std::uint8_t leaf = view.Read<std::uint8_t>(cursor);
                if (leaf < LF_PAD0 || leaf > LF_PAD15)
                    break;
                cursor += std::max<std::size_t>(1, leaf - LF_PAD0);
            }
            return cursor;
        }

        struct NumericValue
        {
            std::size_t next{};
            std::string text;
        };

        [[nodiscard]] inline static NumericValue ReadNumericValue(const ByteView view, const std::size_t offset)
        {
            const std::uint16_t kind = view.Read<std::uint16_t>(offset);
            if (kind < LF_NUMERIC)
                return { offset + sizeof(std::uint16_t), std::to_string(kind) };

            switch (kind) {
            case LF_CHAR:     { const auto value = view.Read<std::int8_t  >(offset + 2); return { offset + 3, std::to_string(value) }; }
            case LF_SHORT:    { const auto value = view.Read<std::int16_t >(offset + 2); return { offset + 4, std::to_string(value) }; }
            case LF_USHORT:   { const auto value = view.Read<std::uint16_t>(offset + 2); return { offset + 4, std::to_string(value) }; }
            case LF_LONG:     { const auto value = view.Read<std::int32_t >(offset + 2); return { offset + 6, std::to_string(value) }; }
            case LF_ULONG:    { const auto value = view.Read<std::uint32_t>(offset + 2); return { offset + 6, std::to_string(value) }; }
            case LF_QUADWORD: { const auto value = view.Read<std::int64_t >(offset + 2); return { offset +10, std::to_string(value) }; }
            case LF_UQUADWORD:{ const auto value = view.Read<std::uint64_t>(offset + 2); return { offset +10, std::to_string(value) }; }
            default:  return { SkipNumeric(view, offset), TypeLeafName(kind).data() };
            }
        }

        [[nodiscard]] inline static std::string AccessName(const CV_fldattr_t attr)
        {
            switch (attr.access) {
            case 1:  return "private";
            case 2:  return "protected";
            case 3:  return "public";
            default: return "none";
            }
        }
        [[nodiscard]] inline static bool HasIntroVirtualOffset(const CV_fldattr_t attr) { return attr.mprop == CV_MTintro || attr.mprop == CV_MTpureintro; }
    }

struct DumpOptions
{
    bool summary = true;
    bool streams = false;
    bool headers = false;
    bool ids     = false;
    bool types   = false;
    bool symbols = false;
};

class PdbParser
{
    inline static void PrintNew(std::ostream& out, unsigned short leaf=0)
    {
//#define DONOTWANTPRINTNEW
#ifndef DONOTWANTPRINTNEW
        if (leaf)
            out << "Leaf = " << Hex16(leaf) << " " << TypeLeafName(leaf) << '\n';
        else
            out << "no leaf yet: \n";
#else
        (void)out;
        (void)leaf;
#endif
    }

    struct StreamInfo
    {
        std::uint32_t size{};
        std::vector<std::uint32_t> blocks;
    };

    struct DbiModule
    {
        std::uint32_t opened{};
        std::uint16_t section{};
        std::uint32_t offset{};
        std::uint32_t size{};
        std::uint16_t moduleStream{};
        std::uint32_t symbolBytes{};
        std::uint32_t c11Bytes{};
        std::uint32_t c13Bytes{};
        std::string   moduleName;
        std::string   objectName;
    };

    struct TypeInfo
    {
        std::string name;
        std::vector<std::uint32_t> arguments;
        std::uint32_t returnType{};
    };

#pragma region LeafClasses
    struct LfLeaf
    {
        const unsigned short leaf;
        LfLeaf(const unsigned short leaf) : leaf(leaf) {}
        virtual ~LfLeaf() {}
    };
    struct LfBitfield : LfLeaf
    {
        const CV_typ_t     typeIndex;
        const std::uint8_t length;
        const std::uint8_t position;
        explicit LfBitfield(const unsigned short leaf, const ByteView& record)
            : LfLeaf(leaf)
            , typeIndex(record.Read<    CV_typ_t>(4))
            , length   (record.Read<std::uint8_t>(8))
            , position (record.Read<std::uint8_t>(9))
        {}
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    bitfield: " << pThis->type_name(typeIndex)
                << " length="       << static_cast<unsigned>(length)
                << " position="     << static_cast<unsigned>(position) << "\n";
        }
    };
    struct LfFieldList : LfLeaf
    {
        std::vector<std::unique_ptr<LfLeaf>> fields;
        explicit LfFieldList(std::vector<std::unique_ptr<LfLeaf>>&& fields)
            : LfLeaf(LF_FIELDLIST)
            , fields(std::move(fields))
        {}
    };
    class LfMember : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     typeIndex;
        const NumericValue offset;
        const std::string  name;
        LfMember(const unsigned short leaf, CV_fldattr_t a, CV_typ_t t, NumericValue o, std::string n) : LfLeaf(leaf), attr(a), typeIndex(t), offset(std::move(o)), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    member: " << name
                << " : "          << pThis->type_name(typeIndex)
                << " offset="     << offset.text
                << " access="     << AccessName(attr) << "\n";
        }
        static LfMember Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr      =                  record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     typeIndex =                  record.Read<CV_typ_t    >(*cursor + 4);
            const NumericValue offset    = ReadNumericValue(record,                   *cursor + offsetof(lfMember, offset));
            const std::string  name      = ReadRecordString(record,                   offset.next);
            *cursor                      = offset.next + name.size() + 1;
            return LfMember(leaf, attr, typeIndex, offset, name);
        }
    };
    class LfBClass : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     baseType;
        const NumericValue offset;
        LfBClass(const unsigned short leaf, CV_fldattr_t a, CV_typ_t b, NumericValue o) : LfLeaf(leaf), attr(a), baseType(b), offset(std::move(o)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    base: " << pThis->type_name(baseType)
                << " offset="   << offset.text
                << " access="   << AccessName(attr) << "\n";
        }
        static LfBClass Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr     = record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     baseType = record.Read<CV_typ_t>    (*cursor + 4);
            const NumericValue offset   = ReadNumericValue(record,  *cursor + offsetof(lfBClass, offset));
            *cursor                     = offset.next;
            return LfBClass(leaf, attr, baseType, offset);
        }
    };
    class LfVBClass : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     index;
        const CV_typ_t     vbptr;
        const NumericValue vbpoff;
        const NumericValue vboff;
        LfVBClass(const unsigned short leaf, CV_fldattr_t a, CV_typ_t i, CV_typ_t v, NumericValue vbpoff, NumericValue vboff) : LfLeaf(leaf), attr(a), index(i), vbptr(v), vbpoff(std::move(vbpoff)), vboff(std::move(vboff)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out, leaf);
            out << "    vbase: "  << pThis->type_name(index)
                << " vbptr="      << pThis->type_name(vbptr)
                << " vbpoff="     << vbpoff.text
                << " vboff="      << vboff.text
                << " access="     << AccessName(attr) << "\n";
        }
        static LfVBClass Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr  = record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     index = record.Read<CV_typ_t>    (*cursor + 4);
            const CV_typ_t     vbptr = record.Read<CV_typ_t>    (*cursor + 8);
            const NumericValue vbpoff = ReadNumericValue(record, *cursor + offsetof(lfVBClass, vbpoff));
            const NumericValue vboff  = ReadNumericValue(record, vbpoff.next);
            *cursor                   = vboff.next;
            return LfVBClass(leaf, attr, index, vbptr, vbpoff, vboff);
        }
    };
    class LfVFuncTab : public LfLeaf
    {
        const CV_typ_t typeIndex;
        explicit LfVFuncTab(const unsigned short leaf, CV_typ_t t) : LfLeaf(leaf), typeIndex(t) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    vfunctab: " << pThis->type_name(typeIndex) << "\n"; }
        static LfVFuncTab Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_typ_t typeIndex = record.Read<CV_typ_t>(*cursor + offsetof(lfVFuncTab, type));
            *cursor                 += sizeof(lfVFuncTab);
            return LfVFuncTab(leaf, typeIndex);
        }
    };
    class LfOneMethod : public LfLeaf
    {
        const CV_fldattr_t                 attr;
        const CV_typ_t                     typeIndex;
        const std::optional<std::uint32_t> vftableOffset;
        const std::string                  name;
        LfOneMethod(const unsigned short leaf, CV_fldattr_t a, CV_typ_t t, std::optional<std::uint32_t> v, std::string n) : LfLeaf(leaf), attr(a), typeIndex(t), vftableOffset(v), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    one method: ";
            if (vftableOffset.has_value())
                out << "vftable-offset=" << vftableOffset.value() << " ";
            out << name
                << " : " << pThis->procedure_signature(typeIndex)
                << " access=" << AccessName(attr) << "\n";
        }
        static LfOneMethod Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr      = record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     typeIndex = record.Read<CV_typ_t    >(*cursor + 4);
            std::size_t        nameOff   = *cursor + offsetof(lfOneMethod, vbaseoff);
            std::optional<std::uint32_t> vftableOffset;
            if (HasIntroVirtualOffset(attr) && nameOff + sizeof(std::uint32_t) <= record.Size())
            {
                vftableOffset = record.Read<std::uint32_t>(nameOff);
                nameOff += sizeof(std::uint32_t);
            }
            const std::string name = ReadRecordString(record, nameOff);
            *cursor = nameOff + name.size() + 1;

            return LfOneMethod(leaf, attr, typeIndex, vftableOffset, name);
        }
    };
    class LfEnumerate : public LfLeaf
    {
        const CV_fldattr_t attr;
        const NumericValue value;
        const std::string  name;
        LfEnumerate(const unsigned short leaf, CV_fldattr_t a, NumericValue v, std::string n) : LfLeaf(leaf), attr(a), value(std::move(v)), name(std::move(n)) {}
    public:
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const { PrintNew(out,leaf); out << "    enumerate: " << name << " = " << value.text << " access=" << AccessName(attr) << "\n"; }
        static LfEnumerate Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr  =                  record.Read<CV_fldattr_t>(*cursor + 2);
            const NumericValue value = ReadNumericValue(record,                   *cursor + offsetof(lfEnumerate, value));
            const std::string  name  = ReadRecordString(record,                   value.next);
            *cursor = value.next + name.size() + 1;
            return LfEnumerate(leaf, attr, value, name);
        }
    };
    class LfSTMember : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     typeIndex;
        const std::string  name;
        LfSTMember(const unsigned short leaf, CV_fldattr_t a, CV_typ_t t, std::string n) : LfLeaf(leaf), attr(a), typeIndex(t), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    static member: " << name << " : "                 << pThis->type_name(typeIndex) << " access="            << AccessName(attr) << "\n"; }
        static LfSTMember Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr      = record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     typeIndex = record.Read<CV_typ_t>    (*cursor + 4);
            const std::string  name      = ReadRecordString(record, *cursor + offsetof(lfSTMember, Name));
            *cursor                      = *cursor + offsetof(lfSTMember, Name) + name.size() + 1;
            return LfSTMember(leaf, attr, typeIndex, name);
        }
    };
    class LfMethod : public LfLeaf
    {
        const std::uint16_t count;
        const CV_typ_t      methodList;
        const std::string   name;
        LfMethod(const unsigned short leaf, std::uint16_t c, CV_typ_t m, std::string n) : LfLeaf(leaf), count(c), methodList(m), name(std::move(n)) {}
    public:
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const { PrintNew(out,leaf); out << "    overloads: " << name << " count="         << count << " list="          << Hex32(methodList) << "\n"; }
        static LfMethod Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const std::uint16_t count      = record.Read<std::uint16_t>(*cursor + offsetof(lfMethod, count));
            const CV_typ_t      methodList = record.Read<CV_typ_t     >(*cursor + offsetof(lfMethod, mList));
            const std::string   name       = ReadRecordString(record,   *cursor + offsetof(lfMethod, Name));
            *cursor                        =                            *cursor + offsetof(lfMethod, Name) + name.size() + 1;
            return LfMethod(leaf, count, methodList, name);
        }
    };
    class LfNestType : public LfLeaf
    {
        const CV_typ_t    typeIndex;
        const std::string name;
        LfNestType(const unsigned short leaf, CV_typ_t t, std::string n) : LfLeaf(leaf), typeIndex(t), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    nested type: " << name << " : " << pThis->type_name(typeIndex) << "\n"; }
        static LfNestType Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_typ_t    typeIndex =                  record.Read<CV_typ_t>(*cursor + offsetof(lfNestType, index));
            const std::string name      = ReadRecordString(record,               *cursor + offsetof(lfNestType, Name));
            *cursor                     =                                        *cursor + offsetof(lfNestType, Name) + name.size() + 1;
            return LfNestType(leaf, typeIndex, name);
        }
    };
    class LfNestTypeEx : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     typeIndex;
        const std::string  name;
        LfNestTypeEx(const unsigned short leaf, CV_fldattr_t a, CV_typ_t t, std::string n) : LfLeaf(leaf), attr(a), typeIndex(t), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    nested type: " << name << " : "               << pThis->type_name(typeIndex) << " access="          << AccessName(attr) << "\n"; }
        static LfNestTypeEx Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr      =                  record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     typeIndex =                  record.Read<CV_typ_t    >(*cursor + 4);
            const std::string  name      = ReadRecordString(record,                   *cursor + offsetof(lfNestTypeEx, Name));
            *cursor                      =                                            *cursor + offsetof(lfNestTypeEx, Name) + name.size() + 1;
            return LfNestTypeEx(leaf, attr, typeIndex, name);
        }
    };
    class LfIndex : public LfLeaf
    {
        const CV_typ_t typeIndex;
        explicit LfIndex(const unsigned short leaf, CV_typ_t t) : LfLeaf(leaf), typeIndex(t) {}
    public:
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const { PrintNew(out,leaf); out << "    continuation: " << Hex32(typeIndex) << "\n"; }
        static LfIndex Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_typ_t typeIndex = record.Read<CV_typ_t>(*cursor + offsetof(lfIndex, index));
            *cursor                 += sizeof(lfIndex);
            return LfIndex(leaf, typeIndex);
        }
    };
    class LfVFuncOff : public LfLeaf
    {
        const CV_typ_t   typeIndex;
        const CV_off32_t offset;
        LfVFuncOff(const unsigned short leaf, CV_typ_t t, CV_off32_t o) : LfLeaf(leaf), typeIndex(t), offset(o) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    vfuncoff: " << pThis->type_name(typeIndex) << " offset=" << offset << "\n"; }
        static LfVFuncOff Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_typ_t   typeIndex = record.Read<CV_typ_t  >(*cursor + offsetof(lfVFuncOff, type));
            const CV_off32_t offset    = record.Read<CV_off32_t>(*cursor + offsetof(lfVFuncOff, offset));
            *cursor                   += sizeof(lfVFuncOff);
            return LfVFuncOff(leaf, typeIndex, offset);
        }
    };
    class LfMemberModify : public LfLeaf
    {
        const CV_fldattr_t attr;
        const CV_typ_t     typeIndex;
        const std::string  name;
        LfMemberModify(const unsigned short leaf, CV_fldattr_t a, CV_typ_t t, std::string n) : LfLeaf(leaf), attr(a), typeIndex(t), name(std::move(n)) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const { PrintNew(out,leaf); out << "    modified member: " << name << " : "                   << pThis->type_name(typeIndex) << " access="              << AccessName(attr) << "\n"; }
        static LfMemberModify Parse(const unsigned short leaf, const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr      =                  record.Read<CV_fldattr_t>(*cursor + 2);
            const CV_typ_t     typeIndex =                  record.Read<CV_typ_t    >(*cursor + 4);
            const std::string  name      = ReadRecordString(record,                   *cursor + offsetof(lfMemberModify, Name));
            *cursor                      =                                            *cursor + offsetof(lfMemberModify, Name) + name.size() + 1;
            return LfMemberModify(leaf, attr, typeIndex, name);
        }
    };
    class MlMethod : public LfLeaf
    {
        const CV_fldattr_t             attr;
        const CV_typ_t                 index;
        const std::optional<std::uint32_t> vftableOffset;
        MlMethod(CV_fldattr_t a, CV_typ_t i, std::optional<std::uint32_t> v) : LfLeaf(LF_METHODLIST), attr(a), index(i), vftableOffset(v) {}
    public:
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    method: " << pThis->type_name(index) << " access=" << AccessName(attr);
            if (vftableOffset.has_value())
                out << " vftable-offset=" << vftableOffset.value();
            out << "\n";
        }
        static MlMethod Parse(const ByteView& record, std::size_t* cursor)
        {
            const CV_fldattr_t attr  = record.Read<CV_fldattr_t>(*cursor);
            const CV_typ_t     index = record.Read<CV_typ_t>    (*cursor + 4);
            *cursor                 += sizeof(mlMethod);

            std::optional<std::uint32_t> vftableOffset;
            if (HasIntroVirtualOffset(attr) && *cursor + sizeof(std::uint32_t) <= record.Size())
            {
                vftableOffset  = record.Read<std::uint32_t>(*cursor);
                *cursor       += sizeof(std::uint32_t);
            }
            return MlMethod(attr, index, vftableOffset);
        }
    };
    struct LfMethodList : public LfLeaf
    {
        std::vector<std::unique_ptr<LfLeaf>> methods;
        explicit LfMethodList(std::vector<std::unique_ptr<LfLeaf>>&& methods)
            : LfLeaf(LF_METHODLIST)
            , methods(std::move(methods))
        {}
    };
    struct LfVTShape : LfLeaf
    {
        const std::uint16_t count;
        const std::vector<std::uint8_t> descriptors; // 4-bit each, two per byte

        explicit LfVTShape(const unsigned short leaf, const ByteView& record)
            : LfLeaf(leaf)
            , count      (record.Read<std::uint16_t>(4))
            , descriptors([&record]() {
                                            const std::uint16_t count = record.Read<std::uint16_t>(4);
                                            const std::size_t   bytes = (count + 1) / 2;
                                            std::vector<std::uint8_t> desc;
                                            desc.reserve(bytes);
                                            for (std::size_t i=0; i<bytes && 6 + i < record.Size(); ++i)
                                                desc.push_back(record.Read<std::uint8_t>(6 + i));
                                            return desc;
                                        }())
        {}
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const
        {
            PrintNew(out,leaf);
            out << "    vtshape: count=" << count;
            for (std::size_t i=0; i<count; ++i) {
                const std::size_t byteIndex = i/2;
                if (byteIndex >= descriptors.size())
                    break;
                const std::uint8_t nibble = (descriptors[byteIndex] >> ((i%2)*4)) & 0x0f;
                out << " [" << i << "]=" << static_cast<unsigned>(nibble);
            }
            out << "\n";
        }
    };
    class LfVftable : public LfLeaf
    {
        const CV_typ_t              completeClass;
        const CV_typ_t              baseClass;
        const uint32_t              offset;
        const uint32_t              size;
        const std::vector<CV_typ_t> entries;
    public:
        LfVftable(const unsigned short leaf, const ByteView& record)
            : LfLeaf(leaf)
            , completeClass(record.Read<CV_typ_t>(4))
            , baseClass    (record.Read<CV_typ_t>(4))
            , offset       (record.Read<uint32_t>(6))
            , size         (record.Read<uint32_t>(10))
            , entries([&] {
                            std::vector<CV_typ_t> v;
                            const std::size_t count = size/sizeof(CV_typ_t);
                            v.reserve(count);
                            std::size_t pos = 14;
                            for (std::size_t i=0; i<count; ++i)
                                v.push_back(record.Read<CV_typ_t>(pos + i*4));
                            return v;
                            }())
        {}
        void Print(const PdbParser* pThis, std::ostream& out) const
        {
            PrintNew(out, leaf);
            out << "    vftable: complete=" << pThis->type_name(completeClass)
                << " base="   << pThis->type_name(baseClass)
                << " offset=" << offset
                << " size="   << size << "\n";
            for (std::size_t i=0; i<entries.size(); ++i)
                out << "        [" << i << "] " << pThis->procedure_signature(entries[i]) << "\n";
        }
    };
    class LfClass : public LfLeaf
    {
        LfClass(std::uint16_t leaf, std::uint16_t memberCount, CV_prop_t properties, CV_typ_t fieldList, CV_typ_t derivedFrom, CV_typ_t vtShape, std::string name, std::string uniqueName, bool isForwardDeclaration)
            : LfLeaf          (leaf)
            , memberCount     (memberCount)
            , properties      (properties)
            , fieldList       (fieldList)
            , derivedFrom     (derivedFrom)
            , vtShape         (vtShape)
            , name            (std::move(name))
            , uniqueName      (std::move(uniqueName))
            , isForwardDeclaration(isForwardDeclaration)
        {}
    public:
        const std::uint16_t memberCount;
        const CV_prop_t     properties;
        const CV_typ_t      fieldList;
        const CV_typ_t      derivedFrom;
        const CV_typ_t      vtShape;
        const std::string   name;
        const std::string   uniqueName;
        const bool          isForwardDeclaration;
        static LfClass Parse(const ByteView& record, const std::uint16_t leaf)
        {
            const std::uint16_t memberCount  = record.Read<std::uint16_t>(offsetof(lfClass, count)    + sizeof(std::uint16_t));
            const CV_prop_t     properties   = record.Read<CV_prop_t>    (offsetof(lfClass, property) + sizeof(std::uint16_t));
            const CV_typ_t      fieldList    = record.Read<CV_typ_t>     (offsetof(lfClass, field)    + sizeof(std::uint16_t));
            const CV_typ_t      derivedFrom  = record.Read<CV_typ_t>     (offsetof(lfClass, derived)  + sizeof(std::uint16_t));
            const CV_typ_t      vtShape      = record.Read<CV_typ_t>     (offsetof(lfClass, vshape)   + sizeof(std::uint16_t));
            const NumericValue  size         = ReadNumericValue(record,   offsetof(lfClass, data)     + sizeof(std::uint16_t));
            const std::string   name         = ReadRecordString(record, size.next);
            const std::string   uniqueName   = ReadRecordString(record, size.next + name.size() + 1);
            const bool          isFwd        = properties.fwdref != 0;
            return LfClass(leaf, memberCount, properties, fieldList, derivedFrom, vtShape, name, uniqueName, isFwd);
        }
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const
        {
            PrintNew(out, leaf);
            const char* prefix = leaf == LF_CLASS ? "class " : "struct ";
            out << "    " << prefix << name << " members="    << memberCount << " fieldList="  << Hex32(fieldList) << (isForwardDeclaration ? " [fwd]" : "") << "\n";
        }
    };
    class LfUnion : public LfLeaf
    {
        LfUnion(std::uint16_t leaf, std::uint16_t memberCount, CV_prop_t properties, CV_typ_t fieldList, std::string name, std::string uniqueName, bool isForwardDeclaration)
            : LfLeaf              (leaf)
            , memberCount         (memberCount)
            , properties          (properties)
            , fieldList           (fieldList)
            , name                (std::move(name))
            , uniqueName          (std::move(uniqueName))
            , isForwardDeclaration(isForwardDeclaration)
        {}
    public:
        const std::uint16_t memberCount;
        const CV_prop_t     properties;
        const CV_typ_t      fieldList;
        const std::string   name;
        const std::string   uniqueName;
        const bool          isForwardDeclaration;
        static LfUnion Parse(const ByteView& record, const std::uint16_t leaf)
        {
            const std::uint16_t memberCount = record.Read<std::uint16_t>(offsetof(lfUnion, count)    + sizeof(std::uint16_t));
            const CV_prop_t     properties  = record.Read<CV_prop_t>    (offsetof(lfUnion, property) + sizeof(std::uint16_t));
            const CV_typ_t      fieldList   = record.Read<CV_typ_t>     (offsetof(lfUnion, field)    + sizeof(std::uint16_t));
            const NumericValue  size        = ReadNumericValue(record,   offsetof(lfUnion, data)     + sizeof(std::uint16_t));
            const std::string   name        = ReadRecordString(record, size.next);
            const std::string   uniqueName  = ReadRecordString(record, size.next + name.size() + 1);
            const bool          isFwd       = properties.fwdref != 0;
            return LfUnion(leaf, memberCount, properties, fieldList, name, uniqueName, isFwd);
        }
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const
        {
            PrintNew(out, leaf);
            out << "    union "  << name << " members="   << memberCount << " fieldList=" << Hex32(fieldList) << (isForwardDeclaration ? " [fwd]" : "") << "\n";
        }
    };
    class LfEnum : public LfLeaf
    {
        LfEnum(std::uint16_t leaf, std::uint16_t count, CV_prop_t properties, CV_typ_t underlyingType, CV_typ_t fieldList, std::string name, std::string uniqueName, bool isForwardDeclaration)
            : LfLeaf              (leaf)
            , count               (count)
            , properties          (properties)
            , underlyingType      (underlyingType)
            , fieldList           (fieldList)
            , name                (std::move(name))
            , uniqueName          (std::move(uniqueName))
            , isForwardDeclaration(isForwardDeclaration)
        {}
    public:
        const std::uint16_t count;
        const CV_prop_t     properties;
        const CV_typ_t      underlyingType;
        const CV_typ_t      fieldList;
        const std::string   name;
        const std::string   uniqueName;
        const bool          isForwardDeclaration;
        static LfEnum Parse(const ByteView& record, const std::uint16_t leaf)
        {
            const std::uint16_t count          = record.Read<std::uint16_t>(offsetof(lfEnum, count)    + sizeof(std::uint16_t));
            const CV_prop_t     properties     = record.Read<CV_prop_t>    (offsetof(lfEnum, property) + sizeof(std::uint16_t));
            const CV_typ_t      underlyingType = record.Read<CV_typ_t>     (offsetof(lfEnum, utype)    + sizeof(std::uint16_t));
            const CV_typ_t      fieldList      = record.Read<CV_typ_t>     (offsetof(lfEnum, field)    + sizeof(std::uint16_t));
            const std::string   name           = ReadRecordString(record,   offsetof(lfEnum, Name)     + sizeof(std::uint16_t));
            const std::string   uniqueName     = ReadRecordString(record,   offsetof(lfEnum, Name)     + sizeof(std::uint16_t) + name.size() + 1);
            const bool          isFwd          = properties.fwdref != 0;
            return LfEnum(leaf, count, properties, underlyingType, fieldList, name, uniqueName, isFwd);
        }
        void Print(const PdbParser* /*pThis*/, std::ostream& out) const
        {
            PrintNew(out, leaf);
            out << "    enum " << name << " count="        << count << " underlying="   << Hex32(underlyingType) << " fieldList="    << Hex32(fieldList) << (isForwardDeclaration ? " [fwd]" : "") << "\n";
        }
    };
    struct LfProcedure : public LfLeaf
    {
        const CV_typ_t       returnType;
        const unsigned char  callType;
        const CV_funcattr_t  funcAttr;
        const unsigned short paramCount;
        const CV_typ_t       argList;
        LfProcedure(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , returnType(record.Read<      CV_typ_t>(offsetof(lfProc,    rvtype) + sizeof(std::uint16_t)))
            , callType  (record.Read< unsigned char>(offsetof(lfProc,  calltype) + sizeof(std::uint16_t)))
            , funcAttr  (record.Read< CV_funcattr_t>(offsetof(lfProc,  funcattr) + sizeof(std::uint16_t)))
            , paramCount(record.Read<unsigned short>(offsetof(lfProc, parmcount) + sizeof(std::uint16_t)))
            , argList   (record.Read<      CV_typ_t>(offsetof(lfProc,   arglist) + sizeof(std::uint16_t)))
        {}
    };
    struct LfMFunction : public LfLeaf
    {
        const CV_typ_t       returnType;
        const CV_typ_t       classType;
        const CV_typ_t       thisType;
        const unsigned char  callType;
        const CV_funcattr_t  funcAttr;
        const unsigned short paramCount;
        const CV_typ_t       argList;
        const long           thisAdjust;
        LfMFunction(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , returnType(record.Read<      CV_typ_t>(offsetof(lfMFunc, rvtype    ) + sizeof(std::uint16_t)))
            , classType (record.Read<      CV_typ_t>(offsetof(lfMFunc, classtype ) + sizeof(std::uint16_t)))
            , thisType  (record.Read<      CV_typ_t>(offsetof(lfMFunc, thistype  ) + sizeof(std::uint16_t)))
            , callType  (record.Read< unsigned char>(offsetof(lfMFunc, calltype  ) + sizeof(std::uint16_t)))
            , funcAttr  (record.Read< CV_funcattr_t>(offsetof(lfMFunc, funcattr  ) + sizeof(std::uint16_t)))
            , paramCount(record.Read<unsigned short>(offsetof(lfMFunc, parmcount ) + sizeof(std::uint16_t)))
            , argList   (record.Read<      CV_typ_t>(offsetof(lfMFunc, arglist   ) + sizeof(std::uint16_t)))
            , thisAdjust(record.Read<          long>(offsetof(lfMFunc, thisadjust) + sizeof(std::uint16_t)))
        {}
    };
    struct LfArgList : public LfLeaf
    {
        const std::vector<std::uint32_t> args;
        LfArgList(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , args([&](){
                            std::vector<std::uint32_t> v;
                            const std::uint32_t count = record.Read<std::uint32_t>(offsetof(lfArgList, count) + sizeof(std::uint16_t));
                            v.reserve(count);
                            for (std::uint32_t i=0; i<count; ++i) {
                                const std::size_t off = offsetof(lfArgList, arg) + sizeof(std::uint16_t) + i*sizeof(CV_typ_t);
                                if (off + sizeof(CV_typ_t) > record.Size())
                                    break;
                                v.push_back(record.Read<CV_typ_t>(off));
                            }
                            return v;
                        }())
        {}
    };
    struct LfArray : public LfLeaf
    {
        const CV_typ_t elementType;
        const CV_typ_t indexType;
        const std::string name;
        LfArray(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , elementType          (record.Read<CV_typ_t>(      offsetof(lfArray, elemtype) + sizeof(std::uint16_t)))
            , indexType            (record.Read<CV_typ_t>(      offsetof(lfArray,  idxtype) + sizeof(std::uint16_t)))
            , name(ReadRecordString(record, SkipNumeric(record, offsetof(lfArray,     data) + sizeof(std::uint16_t))))
        {}
    };
    struct LfFuncId : public LfLeaf
    {
        const std::uint32_t scopeId;
        const std::uint32_t typeIndex;
        const std::string   name;
        LfFuncId(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , scopeId  (            record.Read<std::uint32_t>(offsetof(lfFuncId, scopeId) + sizeof(std::uint16_t)))
            , typeIndex(            record.Read<std::uint32_t>(offsetof(lfFuncId,    type) + sizeof(std::uint16_t)))
            , name(ReadRecordString(record,                    offsetof(lfFuncId,    name) + sizeof(std::uint16_t)))
        {}
    };
    struct LfMFuncId : public LfLeaf
    {
        const std::uint32_t parentType;
        const std::uint32_t typeIndex;
        const std::string   name;
        LfMFuncId(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , parentType(           record.Read<std::uint32_t>(offsetof(lfMFuncId, parentType) + sizeof(std::uint16_t)))
            , typeIndex (           record.Read<std::uint32_t>(offsetof(lfMFuncId, type) + sizeof(std::uint16_t)))
            , name(ReadRecordString(record,                    offsetof(lfMFuncId, name) + sizeof(std::uint16_t)))
        {}
    };
    struct LfStringId : public LfLeaf
    {
        const std::uint32_t id;
        const std::string   name;
        LfStringId(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , id(                   record.Read<std::uint32_t>(offsetof(lfStringId, id) + sizeof(std::uint16_t)))
            , name(ReadRecordString(record,                    offsetof(lfStringId, name) + sizeof(std::uint16_t)))
        {}
    };
    struct LfPointer : public LfLeaf
    {
        const std::uint32_t               utype;
        const lfPointerBody::lfPointerAttr attr;
        union PBase
        {
            struct {
                std::uint32_t pmclass;
                std::uint16_t pmenum;
            } pm;                    // for pointer-to-member (not discriminated by CV_ptrtype_e)
            std::uint16_t bseg;      // CV_PTR_BASE_SEG
            struct {
                std::uint32_t index; // CV_PTR_BASE_TYPE
                // name is stored outside the union
            } btype;
            PBase() : pm{0,0} {}
        } pbase;
        const std::string btype_name;

        LfPointer(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , utype(record.Read<std::uint32_t>(               offsetof(lfPointerBody, utype) + sizeof(std::uint16_t)))
            , attr (record.Read<lfPointerBody::lfPointerAttr>(offsetof(lfPointerBody,  attr) + sizeof(std::uint16_t)))
            , pbase()
            , btype_name([&]() -> std::string
                {
                    const std::size_t base = offsetof(::lfPointer, pbase) + sizeof(std::uint16_t);
                    if (base >= record.Size())
                        return {}; // Bounds check: if even the first byte of pbase is outside, bail.

                    switch (attr.ptrtype)
                    {
                    case CV_PTR_BASE_SEG:
                        if (base + sizeof(std::uint16_t) <= record.Size())
                            pbase.bseg = record.Read<std::uint16_t>(base);
                        return {};

                    case CV_PTR_BASE_TYPE:
                    {
                        const std::size_t idxOff = base;
                        const std::size_t nameOff = base + sizeof(std::uint32_t);
                        if (nameOff > record.Size())
                            return {};
                        pbase.btype.index = record.Read<std::uint32_t>(idxOff);
                        return ReadRecordString(record, nameOff);
                    }

                    // BASE_SELF and all the “normal” pointer kinds do not carry extra data
                    case CV_PTR_BASE_SELF:
                    case CV_PTR_NEAR:
                    case CV_PTR_FAR:
                    case CV_PTR_HUGE:
                    case CV_PTR_BASE_VAL:
                    case CV_PTR_BASE_SEGVAL:
                    case CV_PTR_BASE_ADDR:
                    case CV_PTR_BASE_SEGADDR:
                    case CV_PTR_NEAR32:
                    case CV_PTR_FAR32:
                    case CV_PTR_64:
                    case CV_PTR_UNUSEDPTR:
                    default:
                        return {};
                    }
                }())
        {}
    };
    struct LfModifier : public LfLeaf
    {
        const std::uint32_t   type;
        const CV_modifier_t   attr;
        LfModifier(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , type(record.Read<std::uint32_t>(offsetof(lfModifier, type) + sizeof(std::uint16_t)))
            , attr(record.Read<CV_modifier_t>(offsetof(lfModifier, attr) + sizeof(std::uint16_t)))
        {}
    };
    struct LfAlias : public LfLeaf
    {
        const std::uint32_t utype;
        const std::string   name;
        LfAlias(std::uint16_t leaf, const ByteView& record)
            : LfLeaf(leaf)
            , utype(                 record.Read<std::uint32_t>(offsetof(lfAlias, utype) + sizeof(std::uint16_t)))
            , name (ReadRecordString(record,                    offsetof(lfAlias,  Name) + sizeof(std::uint16_t)))
        {}
    };


#pragma endregion

    ByteView file;
    std::uint32_t blockSize{};
    std::uint32_t blockCount{};
    std::uint32_t directorySize{};
    std::uint32_t directoryRootBlock{};
    std::vector<StreamInfo> streams;
            std::unordered_map<std::uint32_t, TypeInfo> typeInfo;
            std::unordered_map<std::uint32_t, TypeInfo> idInfo;
    mutable std::vector<std::vector<std::byte>> streamCache;
    mutable std::unordered_map<std::uint32_t, std::unique_ptr<LfLeaf>> typeIndexToLeaf;

    [[nodiscard]] ByteView stream(const std::uint32_t index) const
    {
        if (index >= streamCache.size())
            throw std::runtime_error("requested PDB stream is outside the stream table");

        if (streamCache[index].empty() && streams[index].size != 0)
            streamCache[index] = materialize_stream(index);
        return ByteView(streamCache[index]);
    }
    [[nodiscard]] std::vector<std::byte> materialize_stream(const std::uint32_t index) const
    {
        if (index >= streams.size() || streams[index].size == streamNil)
            throw std::runtime_error("requested PDB stream is absent");

        std::vector<std::byte> bytes(streams[index].blocks.size()*static_cast<std::size_t>(blockSize));
        std::size_t write = 0;
        for (const std::uint32_t block : streams[index].blocks)
        {
            const ByteView source = file.Subview(static_cast<std::size_t>(block)*blockSize, blockSize);
            std::copy(source.Span().begin(), source.Span().end(), bytes.begin() + static_cast<std::ptrdiff_t>(write));
            write += blockSize;
        }
        bytes.resize(streams[index].size);
        return bytes;
    }
    void Parse()
    {
        if (!IsPdb(file))
            throw std::runtime_error("input is not a PDB 7.00 MSF file");

        blockSize          = file.Read<std::uint32_t>(32);
        blockCount         = file.Read<std::uint32_t>(40);
        directorySize      = file.Read<std::uint32_t>(44);
        directoryRootBlock = file.Read<std::uint32_t>(52);

        if (blockSize == 0 || blockSize % 512 != 0)
            throw std::runtime_error("invalid PDB block size");

        const std::uint32_t directoryBlocks = BlockCountFor(directorySize, blockSize);
        const std::uint32_t blockMapBlocks  = BlockCountFor(directoryBlocks*sizeof(std::uint32_t), blockSize);
        if (blockMapBlocks != 1)
            throw std::runtime_error("PDB uses a multi-block directory map; this small dumper supports the common single-map layout");

        const ByteView blockMap = file.Subview(static_cast<std::size_t>(directoryRootBlock)*blockSize, blockSize);
        std::vector<std::uint32_t> directoryBlockNumbers;
        directoryBlockNumbers.reserve(directoryBlocks);
        for (std::uint32_t i = 0; i < directoryBlocks; ++i)
            directoryBlockNumbers.push_back(blockMap.Read<std::uint32_t>(static_cast<std::size_t>(i) * sizeof(std::uint32_t)));

        std::vector<std::byte> directory(directoryBlocks*static_cast<std::size_t>(blockSize));
        std::size_t write = 0;
        for (const std::uint32_t block : directoryBlockNumbers)
        {
            const ByteView source = file.Subview(static_cast<std::size_t>(block)*blockSize, blockSize);
            std::copy(source.Span().begin(), source.Span().end(), directory.begin() + static_cast<std::ptrdiff_t>(write));
            write += blockSize;
        }
        directory.resize(directorySize);
        const ByteView dir(directory);

        const std::uint32_t streamCount = dir.Read<std::uint32_t>(0);
        streams.resize(streamCount);
        std::size_t cursor = sizeof(std::uint32_t);
        for (std::uint32_t i = 0; i < streamCount; ++i)
        {
            streams[i].size = dir.Read<std::uint32_t>(cursor);
            cursor += sizeof(std::uint32_t);
        }

        for (StreamInfo& info : streams)
        {
            if (info.size == streamNil)
                continue;

            const std::uint32_t blocks = BlockCountFor(info.size, blockSize);
            info.blocks.reserve(blocks);
            for (std::uint32_t i=0; i<blocks; ++i)
            {
                info.blocks.push_back(dir.Read<std::uint32_t>(cursor));
                cursor += sizeof(std::uint32_t);
            }
        }
        streamCache.resize(streams.size());

        ensure_type_cache();
        ensure_id_cache();
    }
    void dump_pdb_info(std::ostream& out) const
    {
        if (pdbStreamPdb >= streams.size() || streams[pdbStreamPdb].size < 28)
            return;

        const ByteView s = stream(pdbStreamPdb);
        out << "\nPDB info stream\n";
        out << "  version: "   <<       s.Read<std::uint32_t>(0 ) << "\n";
        out << "  signature: " << Hex32(s.Read<std::uint32_t>(4)) << "\n";
        out << "  age: "       <<       s.Read<std::uint32_t>(8 ) << "\n";
        out << "  guid: "      <<   Guid_to_string(s.Data() + 12) << "\n";
    }
    void dump_tpi_header(std::ostream& out, const std::uint32_t streamIndex, const std::string_view title) const
    {
        if (streamIndex >= streams.size() || streams[streamIndex].size < 56)
            return;

        const ByteView s = stream(streamIndex);
        out << "\n" << title << " stream\n";
        out << "  version: "           <<       s.Read<std::uint32_t>( 0)  << "\n";
        out << "  header bytes: "      <<       s.Read<std::uint32_t>( 4)  << "\n";
        out << "  type index begin: "  << Hex32(s.Read<std::uint32_t>( 8)) << "\n";
        out << "  type index end: "    << Hex32(s.Read<std::uint32_t>(12)) << "\n";
        out << "  type record bytes: " <<       s.Read<std::uint32_t>(16)  << "\n";
        out << "  hash stream: "       <<       s.Read<std::uint16_t>(20)  << "\n";
    }
    void dump_dbi(std::ostream& out, const bool includeModules) const
    {
        if (pdbStreamDbi >= streams.size() || streams[pdbStreamDbi].size < 64)
            return;

        const ByteView s = stream(pdbStreamDbi);
        out << "\nDBI stream\n";
        out << "  version signature: "    << Hex32(s.Read<std::uint32_t>(0)) << "\n";
        out << "  version header: "             << s.Read<std::uint32_t>(4 ) << "\n";
        out << "  age: "                        << s.Read<std::uint32_t>(8 ) << "\n";
        out << "  global symbol stream: "       << s.Read<std::uint16_t>(12) << "\n";
        out << "  public symbol stream: "       << s.Read<std::uint16_t>(16) << "\n";
        out << "  symbol record stream: "       << s.Read<std::uint16_t>(20) << "\n";
        out << "  module info bytes: "          << s.Read<std::uint32_t>(24) << "\n";
        out << "  section contribution bytes: " << s.Read<std::uint32_t>(28) << "\n";
        out << "  section map bytes: "          << s.Read<std::uint32_t>(32) << "\n";
        out << "  file info bytes: "            << s.Read<std::uint32_t>(36) << "\n";

        const std::vector<DbiModule> modules = read_modules();
        out << "  modules: " << modules.size() << "\n";

        if (includeModules) {
            out << "\nModules\n";
            for (std::size_t i = 0; i < modules.size(); ++i) {
                const DbiModule& mod = modules[i];
                out << "  ["    << std::setw(3) << i << "] stream=" << mod.moduleStream
                    << " syms=" << mod.symbolBytes
                    << " c11="  << mod.c11Bytes
                    << " c13="  << mod.c13Bytes
                    << " "      << mod.moduleName << "\n";
                if (!mod.objectName.empty() && mod.objectName != mod.moduleName)
                    out << "        obj: " << mod.objectName << "\n";
            }
        }
    }
    void dump_id_records(std::ostream& out) const
    {
        if (pdbStreamIpi >= streams.size() || streams[pdbStreamIpi].size < 56)
            return;

        const ByteView s = stream(pdbStreamIpi);
        const std::uint32_t headerBytes = s.Read<std::uint32_t>(4);
        const std::uint32_t begin       = s.Read<std::uint32_t>(8);
        const std::uint32_t end         = s.Read<std::uint32_t>(12);
        const std::uint32_t recordBytes = s.Read<std::uint32_t>(16);
        const std::size_t recordsStart  = headerBytes;
        const std::size_t recordsEnd    = recordsStart + recordBytes;
        if (recordsEnd > s.Size())
            throw std::runtime_error("IPI record area is truncated");

        out << "\n*** IDs\n";

        std::size_t cursor = recordsStart;
        std::uint32_t idIndex = begin;
        while (cursor + 4 <= recordsEnd && idIndex < end) {
            const std::uint16_t length = s.Read<std::uint16_t>(cursor);
            if (length < 2 || cursor + sizeof(std::uint16_t) + length > recordsEnd)
                throw std::runtime_error("invalid CodeView ID record length");

            const std::uint16_t leaf = s.Read<std::uint16_t>(cursor + 2);
            const ByteView record = s.Subview(cursor, sizeof(std::uint16_t) + length);
            out << "\n" << CompactHex(idIndex) << " : Length = " << length << ", "; // Leaf = " << CompactHex(leaf) << " " << TypeLeafName(leaf) << "\n";
            switch (leaf) {
            case LF_FUNC_ID: {
                struct LfFuncId : LfLeaf
                {
                    const std::uint32_t scope;
                    const std::uint32_t type;
                    const std::string   name;
                    explicit LfFuncId(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , scope(record.Read<std::uint32_t>(4))
                        , type (record.Read<std::uint32_t>(8))
                        , name (ReadRecordString(record,  12))
                    {}
                    void Print(const PdbParser* pThis, std::ostream& out) const
                    {
                        PrintNew(out, leaf);
                        out << "\tType = " << CompactHex(type)
                            << "\t\tScope = " << (scope == 0 ? "global" : CompactHex(scope))
                            << "\t" << name << "\n";
                        out << "\tSignature = " << pThis->procedure_signature(type) << "\n";
                        if (scope != 0)
                            out << "\tResolved scope = " << pThis->id_name(scope) << "\n";
                    }
                };
                LfFuncId{leaf, record}.Print(this, out);
                break;
            }
            case LF_MFUNC_ID: {
                struct LfMemberFuncId : LfLeaf
                {
                    const std::uint32_t parent;
                    const std::uint32_t type;
                    const std::string   name;
                    explicit LfMemberFuncId(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , parent(record.Read<std::uint32_t>(4))
                        , type  (record.Read<std::uint32_t>(8))
                        , name  (ReadRecordString(record,  12))
                    {}
                    void Print(const PdbParser* pThis, std::ostream& out) const
                    {
                        PrintNew(out, leaf);
                        out << "\tType = "            << CompactHex(type)
                            << "\t\tParent = "        << CompactHex(parent)
                            << "\t"                   << name << "\n";
                        out << "\tSignature = "       << pThis->procedure_signature(type) << "\n";
                        out << "\tResolved parent = " << pThis->type_name(parent) << "\n";
                    }
                };
                LfMemberFuncId{leaf,record}.Print(this, out);
                break;
            }
            case LF_BUILDINFO: {
                struct LfBuildInfo : LfLeaf
                {
                    const std::vector<std::uint32_t> ids;
                    explicit LfBuildInfo(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , ids([&record]() {
                                                std::vector<std::uint32_t> ids;
                                                const std::uint16_t count = record.Read<std::uint16_t>(4);
                                                ids.reserve(count);
                                                for (std::uint16_t i = 0; i < count; ++i) {
                                                    const std::size_t offset = 6 + static_cast<std::size_t>(i)*sizeof(std::uint32_t);
                                                    if (offset + sizeof(std::uint32_t) > record.Size())
                                                        break;
                                                    ids.push_back(record.Read<std::uint32_t>(offset));
                                                }
                                                return ids; 
                                          }())
                    {}
                    void Print(const PdbParser* /*pThis*/, std::ostream& out) const
                    {
                        PrintNew(out, leaf);
                        out << "\tString IDs (count = " << ids.size() << "):";
                        for (const std::uint32_t id : ids)
                            out << " " << CompactHex(id);
                        out << "\n";
                    }
                };
                LfBuildInfo{leaf,record}.Print(this, out);
                break;
            }
            case LF_SUBSTR_LIST: {
                struct LfSubstrList : LfLeaf
                {
                    const std::vector<std::uint32_t> ids;
                    explicit LfSubstrList(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , ids([&record]() {
                                                std::vector<std::uint32_t> ids;
                                                const std::uint32_t count = record.Read<std::uint32_t>(4);
                                                ids.reserve(count);
                                                for (std::uint32_t i = 0; i < count; ++i) {
                                                    const std::size_t offset = 8 + static_cast<std::size_t>(i)*sizeof(std::uint32_t);
                                                    if (offset + sizeof(std::uint32_t) > record.Size())
                                                        break;
                                                    ids.push_back(record.Read<std::uint32_t>(offset));
                                                }
                                                return ids;
                                          }())
                    {}
                    void Print(const PdbParser* /*pThis*/, std::ostream& out) const
                    {
                        PrintNew(out,leaf);
                        out << "\tString IDs (count = " << ids.size() << "):";
                        for (const std::uint32_t id : ids)
                            out << " " << CompactHex(id);
                        out << "\n";
                    }
                };
                LfSubstrList{leaf,record}.Print(this, out);
                break;
            }
            case LF_STRING_ID: {
                struct LfStringId : LfLeaf
                {
                    const std::uint32_t subStringList;
                    const std::string   name;
                    explicit LfStringId(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , subStringList(record.Read<std::uint32_t>(4))
                        , name         (  ReadRecordString(record, 8))
                    {}
                    void Print(const PdbParser* /*pThis*/, std::ostream& out) const
                    {
                        PrintNew(out,leaf);
                        out << "\t" << name << "\n";
                        if (subStringList == 0)
                            out << "\tNo sub string\n";
                        else
                            out << "\tList of sub string IDs = " << CompactHex(subStringList) << "\n";
                    }
                };
                LfStringId{leaf,record}.Print(this, out);
                break;
            }
            case LF_UDT_SRC_LINE: {
                struct LfUdtSrcLine : LfLeaf
                {
                    const std::uint32_t type;
                    const std::uint32_t sourceFile;
                    const std::uint32_t line;
                    explicit LfUdtSrcLine(const unsigned short leaf, const ByteView& record)
                        : LfLeaf(leaf)
                        , type      (record.Read<std::uint32_t>( 4))
                        , sourceFile(record.Read<std::uint32_t>( 8))
                        , line      (record.Read<std::uint32_t>(12))
                    {}
                    void Print(const PdbParser* pThis, std::ostream& out) const
                    {
                        PrintNew(out,leaf);
                        out << "\ttype = "                 << CompactHex(type)
                            << ", source file = "          << CompactHex(sourceFile)
                            << ", line = "                 << line << "\n";
                        out << "\tResolved source file = " << pThis->id_name(sourceFile) << "\n";
                    }
                };
                LfUdtSrcLine{leaf,record}.Print(this, out);
                break;
            }
            default: {
                struct LfDefault : LfLeaf
                {
                    const std::string name;
                    explicit LfDefault(const unsigned short leaf, const std::string& name) : LfLeaf(leaf), name(name) {}
                    void Print(const PdbParser* /*pThis*/, std::ostream& out) const { PrintNew(out, leaf); if (!name.empty()) out << "\t" << name << "\n"; }
                };
                const auto       found = idInfo.find(idIndex);
                const std::string name = (found != idInfo.end() && !found->second.name.empty()) ? found->second.name : std::string{};
                LfDefault{leaf,name}.Print(this, out);
                break;
            }
            }

            cursor += sizeof(std::uint16_t) + length;
            ++idIndex;
        }
    }
    void DumpMethodList(std::ostream& out, const ByteView record, std::vector<std::unique_ptr<LfLeaf>>& methods) const
    {
        std::size_t cursor = sizeof(TYPTYPE);
        while (cursor + sizeof(mlMethod) <= record.Size()) {
            auto m = MlMethod::Parse(record, &cursor);
            m.Print(this, out);
            methods.emplace_back(std::unique_ptr<LfLeaf>(new MlMethod(std::move(m))));
        }
    }

    template<typename T> void AddToFields(std::ostream& out, const std::uint16_t leaf, const ByteView record, std::size_t* cursor, std::vector<std::unique_ptr<LfLeaf>>& fields) const
    {
        auto l = T::Parse(leaf, record, cursor);
        l.Print(this, out);
        fields.emplace_back(std::unique_ptr<LfLeaf>(new T(std::move(l))));
    }
    void DumpFieldList(std::ostream& out, const ByteView record, std::vector<std::unique_ptr<LfLeaf>>& fields) const
    {
        std::size_t cursor = sizeof(TYPTYPE);
        while (cursor + sizeof(std::uint16_t) <= record.Size()) {
            cursor                             = SkipFieldListPadding(record, cursor);
            if (cursor + sizeof(std::uint16_t) > record.Size())
                break;

            const std::uint16_t leaf = record.Read<std::uint16_t>(cursor);
            if (IsFieldListPadding(leaf)) {
                cursor = SkipFieldListPadding(record, cursor);
                continue;
            }

            switch (leaf) {
            case LF_MEMBER: 
                if (cursor + offsetof(lfMember, offset) > record.Size())        return;
                AddToFields<LfMember>(out, leaf, record, &cursor, fields);      break;
            case LF_BCLASS:
                if (cursor + offsetof(lfBClass, offset) > record.Size())        return;
                AddToFields<LfBClass>(out, leaf, record, &cursor, fields);      break;
            case LF_VBCLASS:
                if (cursor + offsetof(lfVBClass, vbpoff) > record.Size())       return;
                AddToFields<LfVBClass>(out, leaf, record, &cursor, fields);     break;
            case LF_VFUNCTAB:
                if (cursor + sizeof(lfVFuncTab) > record.Size())                return;
                AddToFields<LfVFuncTab>(out, leaf, record, &cursor, fields);    break;
            case LF_ONEMETHOD:
                if (cursor + offsetof(lfOneMethod, vbaseoff) > record.Size())   return;
                AddToFields<LfOneMethod>(out, leaf, record, &cursor, fields);   break;
            case LF_ENUMERATE:
                if (cursor + offsetof(lfEnumerate, value) > record.Size())      return;
                AddToFields<LfEnumerate>(out, leaf, record, &cursor, fields);   break;
            case LF_STMEMBER:
                if (cursor + offsetof(lfSTMember, Name) > record.Size())        return;
                AddToFields<LfSTMember>(out, leaf, record, &cursor, fields);    break;
            case LF_METHOD:
                if (cursor + offsetof(lfMethod, Name) > record.Size())          return;
                AddToFields<LfMethod>(out, leaf, record, &cursor, fields);      break;
            case LF_NESTTYPE:
                if (cursor + offsetof(lfNestType, Name) > record.Size())        return;
                AddToFields<LfNestType>(out, leaf, record, &cursor, fields);    break;
            case LF_NESTTYPEEX:
                if (cursor + offsetof(lfNestTypeEx, Name) > record.Size())      return;
                AddToFields<LfNestTypeEx>(out, leaf, record, &cursor, fields);  break;
            case LF_INDEX:
                if (cursor + sizeof(lfIndex) > record.Size())                   return;
                AddToFields<LfIndex>(out, leaf, record, &cursor, fields);       break;
            case LF_VFUNCOFF:
                if (cursor + sizeof(lfVFuncOff) > record.Size())                return;
                AddToFields<LfVFuncOff>(out, leaf, record, &cursor, fields);    break;
            case LF_MEMBERMODIFY:
                if (cursor + offsetof(lfMemberModify, Name) > record.Size())    return;
                AddToFields<LfMemberModify>(out, leaf, record, &cursor, fields);break;
            default:
                out << "    " << TypeLeafName(leaf) << " " << Hex16(leaf) << "\n";
                return;
            }
        }
    }

    template<typename T> void AddToMap(std::uint32_t typeIndex, std::ostream& out, const ByteView record, const std::uint16_t leaf) const
    {
        auto l = T{leaf,record};
        l.Print(this, out);
        typeIndexToLeaf[typeIndex] = std::make_unique<LfLeaf>(l);
    }
    void DumpTypeRecordDetails(std::uint32_t typeIndex, std::ostream& out, const ByteView record, const std::uint16_t leaf) const
    {
        switch (leaf) {
        case LF_FIELDLIST: {
            std::vector<std::unique_ptr<LfLeaf>> fields;
            DumpFieldList(out, record, fields);
            typeIndexToLeaf[typeIndex] = std::unique_ptr<LfLeaf>(new LfFieldList(std::move(fields)));
            break;
        }
        case LF_METHODLIST: {
            std::vector<std::unique_ptr<LfLeaf>> methods;
            DumpMethodList(out, record, methods);
            typeIndexToLeaf[typeIndex] = std::unique_ptr<LfLeaf>(new LfMethodList(std::move(methods)));
            break;
        }
        case LF_BITFIELD: if (record.Size() < sizeof(std::uint16_t) + sizeof(lfBitfield)) return; AddToMap<LfBitfield>(typeIndex, out, record, leaf); break;
        case LF_VTSHAPE:  if (record.Size() < sizeof(std::uint16_t) + sizeof(lfVTShape )) return; AddToMap<LfVTShape >(typeIndex, out, record, leaf); break;
        case LF_VFTABLE:  if (record.Size() < sizeof(std::uint16_t) + sizeof(lfVftable )) return; AddToMap<LfVftable >(typeIndex, out, record, leaf); break;
        default:
            break;
        }
    }
    void dump_type_records(std::ostream& out, const std::uint32_t streamIndex, const std::string_view title) const
    {
        if (streamIndex >= streams.size() || streams[streamIndex].size < 56)
            return;

        const ByteView s = stream(streamIndex);
        const std::uint32_t headerBytes = s.Read<std::uint32_t>(4);
        const std::uint32_t begin       = s.Read<std::uint32_t>(8);
        const std::uint32_t end         = s.Read<std::uint32_t>(12);
        const std::uint32_t recordBytes = s.Read<std::uint32_t>(16);
        const std::size_t recordsStart  = headerBytes;
        const std::size_t recordsEnd    = recordsStart + recordBytes;
        if (recordsEnd > s.Size())
            throw std::runtime_error("TPI/IPI record area is truncated");

        out << "\n" << title << "\n";
        std::size_t cursor = recordsStart;
        std::uint32_t typeIndex = begin;
        while (cursor + 4 <= recordsEnd && typeIndex < end) {
            const std::uint16_t length = s.Read<std::uint16_t>(cursor);
            const std::uint16_t leaf   = s.Read<std::uint16_t>(cursor + 2);
            if (length < 2 || cursor + sizeof(std::uint16_t) + length > recordsEnd)
                throw std::runtime_error("invalid CodeView type record length");

            const ByteView record = s.Subview(cursor, sizeof(std::uint16_t) + length);

            out << "  "     << Hex32(typeIndex)
                << " len="  << std::setw(5) << length
                << " leaf=" << Hex16(leaf)
                << " "      << TypeLeafName(leaf);

            if (streamIndex == pdbStreamIpi) {
                const auto found = idInfo.find(typeIndex);
                if (found != idInfo.end() && !found->second.name.empty())
                    out << "  " << found->second.name;
                if ((leaf == LF_FUNC_ID || leaf == LF_MFUNC_ID) && found != idInfo.end())
                    out << "  " << procedure_signature(found->second.returnType);
            } else {
                const auto found = typeInfo.find(typeIndex);
                if (found != typeInfo.end() && !found->second.name.empty())
                    out << "  " << found->second.name;
                if ((leaf == LF_PROCEDURE || leaf == LF_MFUNCTION) && found != typeInfo.end())
                    out << "  " << procedure_signature(typeIndex);
            }
            out << "\n";
            DumpTypeRecordDetails(typeIndex, out, record, leaf);

            cursor += sizeof(std::uint16_t) + length;
            ++typeIndex;
        }
    }
    void dump_symbol_records(std::ostream& out) const
    {
        const std::vector<DbiModule> modules = read_modules();
        out << "\nModule symbol records\n";

        for (std::size_t moduleIndex = 0; moduleIndex < modules.size(); ++moduleIndex) {
            const DbiModule& mod = modules[moduleIndex];
            if (mod.moduleStream == 0xffff || mod.moduleStream >= streams.size() || mod.symbolBytes == 0)
                continue;

            const ByteView s = stream(mod.moduleStream);
            if (s.Size() < 4)
                continue;

            out << "  module[" << moduleIndex << "] " << mod.moduleName << "\n";
            std::size_t cursor = 4;
            const std::size_t end = std::min<std::size_t>(mod.symbolBytes, s.Size());
            bool inProcedure = false;
            while (cursor + 4 <= end) {
                const std::uint16_t length = s.Read<std::uint16_t>(cursor);
                const std::uint16_t kind   = s.Read<std::uint16_t>(cursor + 2);
                if (length < 2 || cursor + sizeof(std::uint16_t) + length > end) {
                    break;
                }

                const ByteView record = s.Subview(cursor, sizeof(std::uint16_t) + length);
                const char* indent = inProcedure ? "      " : "    ";

                switch (kind) {
                case S_END:
                    inProcedure = false;
                    out << "    END\n";
                    break;
                case S_LPROC32:
                case S_GPROC32: {
                    const CV_typ_t       typeIndex  = record.Read<CV_typ_t>     (offsetof(PROCSYM32, typind));
                    const CV_uoff32_t    codeOffset = record.Read<CV_uoff32_t>  (offsetof(PROCSYM32, off));
                    const std::uint16_t  segment    = record.Read<std::uint16_t>(offsetof(PROCSYM32, seg));
                    const std::string    name       = ReadRecordString(record,   offsetof(PROCSYM32, name));
                    out << "    " << SymbolKindName(kind)
                        << " "    << name
                        << " : "  << procedure_signature(typeIndex)
                        << " ["   << segment << ":" << Hex32(codeOffset) << "]\n";
                    inProcedure = true;
                    break;
                }
                case S_BPREL32: {
                    const CV_off32_t  frameOffset = record.Read<CV_off32_t>( offsetof(BPRELSYM32, off));
                    const CV_typ_t    typeIndex   = record.Read<CV_typ_t  >( offsetof(BPRELSYM32, typind));
                    const std::string name        = ReadRecordString(record, offsetof(BPRELSYM32, name));
                    out << indent << "frame " << name << " : " << type_name(typeIndex) << " [bp" << (frameOffset < 0 ? "" : "+") << frameOffset << "]\n";
                    break;
                }
                case S_LDATA32:
                case S_GDATA32: {
                    const CV_typ_t      typeIndex  = record.Read<CV_typ_t     >(offsetof(DATASYM32, typind));
                    const CV_uoff32_t   dataOffset = record.Read<CV_uoff32_t  >(offsetof(DATASYM32, off));
                    const std::uint16_t segment    = record.Read<std::uint16_t>(offsetof(DATASYM32, seg));
                    const std::string   name       = ReadRecordString(record,   offsetof(DATASYM32, name));
                    out << indent << SymbolKindName(kind) << " " << name << " : " << type_name(typeIndex) << " [" << segment << ":" << Hex32(dataOffset) << "]\n";
                    break;
                }
                case S_PUB32: {
                    const CV_uoff32_t   codeOffset = record.Read<CV_uoff32_t  >(offsetof(PUBSYM32, off));
                    const std::uint16_t segment    = record.Read<std::uint16_t>(offsetof(PUBSYM32, seg));
                    const std::string   name       = ReadRecordString(record,   offsetof(PUBSYM32, name));
                    out << indent << "public " << name << " [" << segment << ":" << Hex32(codeOffset) << "]\n";
                    break;
                }
                case S_REGREL32: {
                    const CV_off32_t    regOffset = record.Read<CV_off32_t   >(offsetof(REGREL32, off));
                    const CV_typ_t      typeIndex = record.Read<CV_typ_t     >(offsetof(REGREL32, typind));
                    const std::uint16_t reg       = record.Read<std::uint16_t>(offsetof(REGREL32, reg));
                    const std::string   name      = ReadRecordString(record,   offsetof(REGREL32, name));
                    out << indent << "regrel " << name << " : " << type_name(typeIndex) << " [reg " << reg << (regOffset < 0 ? "" : "+") << regOffset << "]\n";
                    break;
                }
                case S_LOCAL: {
                    const CV_typ_t      typeIndex = record.Read<CV_typ_t    >(offsetof(LOCALSYM, typind));
                    const CV_LVARFLAGS  flags     = record.Read<CV_LVARFLAGS>(offsetof(LOCALSYM, flags));
                    const std::string   name      = ReadRecordString(record,  offsetof(LOCALSYM, name));
                    out << indent << "local " << name << " : " << type_name(typeIndex);
                    if (flags.fIsParam != 0)
                        out << " (parameter)";
                    out << "\n";
                    break;
                }
                case S_UDT: {
                    const CV_typ_t    typeIndex = record.Read<CV_typ_t>(   offsetof(UDTSYM, typind));
                    const std::string name      = ReadRecordString(record, offsetof(UDTSYM, name));
                    out << indent << "typedef " << name << " : " << type_name(typeIndex) << "\n";
                    break;
                }
                default:
                    if (kind == S_OBJNAME || kind == S_PROCREF || kind == S_FRAMEPROC || kind == S_DEFRANGE || kind == S_COMPILE3)
                        break;
                    out << indent << "@" << Hex32(static_cast<std::uint32_t>(cursor))
                        << " len="       << std::setw(5) << length
                        << " kind="      << Hex16(kind)
                        << " "           << SymbolKindName(kind) << "\n";
                    break;
                }
                cursor += sizeof(std::uint16_t) + length;
            }
        }
    }

    template<typename T, typename F> void AddToMap(const T& l, const std::uint32_t typeIndex, TypeInfo* info, F f)
    {
        f(info, l);
        typeIndexToLeaf[typeIndex] = std::make_unique<T>(l);
    }
    void ensure_type_cache()
    {
        const auto parseStream = [this](const std::uint32_t streamIndex) {
            if (streamIndex >= streams.size() || streams[streamIndex].size < 56)
                return;

            const ByteView s = stream(streamIndex);
            const std::uint32_t headerBytes = s.Read<std::uint32_t>(4);
            const std::uint32_t begin       = s.Read<std::uint32_t>(8);
            const std::uint32_t end         = s.Read<std::uint32_t>(12);
            const std::uint32_t recordBytes = s.Read<std::uint32_t>(16);
            const std::size_t recordsEnd    = static_cast<std::size_t>(headerBytes) + recordBytes;
            if (recordsEnd > s.Size())
                return;

            std::size_t cursor = headerBytes;
            std::uint32_t typeIndex = begin;
            while (cursor + 4 <= recordsEnd && typeIndex < end) {
                const std::uint16_t length = s.Read<std::uint16_t>(cursor);
                if (length < 2 || cursor + sizeof(std::uint16_t) + length > recordsEnd)
                    break;

                const std::uint16_t leaf = s.Read<std::uint16_t>(cursor + 2);
                const ByteView record    = s.Subview(cursor, sizeof(std::uint16_t) + length);

                TypeInfo info;
                switch (leaf) {
                case LF_MODIFIER:
                    AddToMap(LfModifier(leaf, record), typeIndex, &info,    [&](auto* info, auto& l)
                                                                            {
                                                                                std::string prefix;
                                                                                if (l.attr.MOD_const    != 0) prefix += "const ";
                                                                                if (l.attr.MOD_volatile != 0) prefix += "volatile ";
                                                                                info->name = prefix + type_name(l.type);
                                                                            });
                    break;
                case LF_POINTER:
                    AddToMap(LfPointer(leaf, record), typeIndex, &info,     [&](auto* info, auto& l)
                                                                            {
                                                                                const char* suffix = "*";
                                                                                if      (l.attr.ptrmode == CV_PTR_MODE_LVREF)
                                                                                    suffix = "&";
                                                                                else if (l.attr.ptrmode == CV_PTR_MODE_RVREF)
                                                                                    suffix = "&&";
                                                                                if (std::string(suffix) == "*")
                                                                                {
                                                                                    const std::string referentName = type_name(l.utype);
                                                                                    const std::size_t paren        = referentName.find('(');
                                                                                    if (paren != std::string::npos) // is it a pointer to function?
                                                                                        info->name = referentName.substr(0, paren+1) + "*" + referentName.substr(paren + 1);
                                                                                    else
                                                                                        info->name = referentName + "*";
                                                                                } else 
                                                                                    info->name = type_name(l.utype) + suffix;
                                                                            });
                    break;
                case LF_PROCEDURE:
                    AddToMap(LfProcedure(leaf, record), typeIndex, &info,   [&](auto* info, auto& l)
                                                                            {
                                                                                info->returnType    = l.returnType;
                                                                                if (const auto it   = typeInfo.find(l.argList); it != typeInfo.end())
                                                                                    info->arguments = it->second.arguments;
                                                                            });
                    break;
                case LF_MFUNCTION:
                    AddToMap(LfMFunction(leaf, record), typeIndex, &info,   [&](auto* info, auto& l)
                                                                            {
                                                                                info->returnType    = l.returnType;
                                                                                if (const auto it   = typeInfo.find(l.argList); it != typeInfo.end())
                                                                                    info->arguments = it->second.arguments;
                                                                            });
                    break;
                case LF_ARGLIST:
                    AddToMap(    LfArgList(leaf, record), typeIndex, &info, [&](auto* info, auto& l) { info->arguments = l.args; });
                    break;
                case LF_ARRAY:
                    AddToMap(      LfArray(leaf, record), typeIndex, &info, [&](auto* info, auto& l) { info->name = l.name.empty() ? type_name(l.elementType) + "[]" : l.name; });

                    break;
                case LF_STRUCTURE:
                case LF_CLASS:
                    AddToMap(LfClass::Parse(record, leaf), typeIndex, &info,[&](auto* info, auto& l)
                                                                            {
                                                                                if (!l.name.empty())
                                                                                    info->name = std::string(leaf == LF_CLASS ? "class " : "struct ") + l.name;
                                                                            });
                    break;
                case LF_UNION:
                    AddToMap(LfUnion::Parse(record, leaf), typeIndex, &info,[&](auto* info, auto& l) { if (!l.name.empty()) info->name = std::string("union ") + l.name; });
                    break;
                case LF_ENUM:
                    AddToMap( LfEnum::Parse(record, leaf), typeIndex, &info,[&](auto* info, auto& l) { if (!l.name.empty()) info->name = std::string("enum ") + l.name; });
                    break;
                case LF_FUNC_ID:
                    AddToMap(     LfFuncId(leaf, record), typeIndex, &info, [&](auto* info, auto& l)
                                                                            {
                                                                                info->returnType = l.typeIndex;
                                                                                info->name       = (l.scopeId == 0 ? std::string() : type_name(l.scopeId) + "::") + l.name;
                                                                            });
                    break;
                case LF_MFUNC_ID:
                    AddToMap(LfMFuncId(leaf, record), typeIndex, &info,     [&](auto* info, auto& l) {
                                                                                info->returnType = l.typeIndex;
                                                                                info->name       = type_name(l.parentType) + "::" + l.name;
                                                                            });
                    break;
                case LF_STRING_ID:
                    AddToMap(LfStringId(leaf, record), typeIndex, &info,    [&](auto* info, auto& l) {
                                                                                info->returnType = l.id;
                                                                                info->name       = l.name;
                                                                            });
                    break;
                case LF_ALIAS:
                    AddToMap(LfAlias(leaf, record), typeIndex, &info,       [&](auto* info, auto& l) { info->name = "typedef " + type_name(l.utype) + " " + l.name; });
                    break;
                default:
                    break;
                }

                if (!info.name.empty() || !info.arguments.empty() || info.returnType != 0)
                    typeInfo[typeIndex] = std::move(info);
                   
                cursor += sizeof(std::uint16_t) + length;
                ++typeIndex;
            }
        };

        parseStream(pdbStreamTpi);
    }
    void ensure_id_cache()
    {
        if (pdbStreamIpi >= streams.size() || streams[pdbStreamIpi].size < 56)
            return;

        const ByteView s = stream(pdbStreamIpi);
        const std::uint32_t headerBytes = s.Read<std::uint32_t>(4);
        const std::uint32_t begin       = s.Read<std::uint32_t>(8);
        const std::uint32_t end         = s.Read<std::uint32_t>(12);
        const std::uint32_t recordBytes = s.Read<std::uint32_t>(16);
        const std::size_t recordsEnd    = static_cast<std::size_t>(headerBytes) + recordBytes;
        if (recordsEnd > s.Size())
            return;

        std::size_t cursor    = headerBytes;
        std::uint32_t idIndex = begin;
        while (cursor + 4    <= recordsEnd && idIndex < end) {
            const std::uint16_t length = s.Read<std::uint16_t>(cursor);
            if (length < 2 || cursor + sizeof(std::uint16_t) + length > recordsEnd)
                break;

            const std::uint16_t leaf = s.Read<std::uint16_t>(cursor + 2);
            const ByteView record    = s.Subview(cursor, sizeof(std::uint16_t) + length);
            TypeInfo info;
            switch (leaf) {
            case LF_FUNC_ID: {
                const std::uint32_t scope = record.Read<std::uint32_t>(4);
                info.returnType           = record.Read<std::uint32_t>(8);
                const std::string name    = ReadRecordString(record, 12);
                info.name = (scope == 0 ? std::string("global") : id_name(scope)) + "::" + name;
                break;
            }
            case LF_MFUNC_ID: {
                const std::uint32_t parent = record.Read<std::uint32_t>(4);
                info.returnType            = record.Read<std::uint32_t>(8);
                info.name = type_name(parent) + "::" + ReadRecordString(record, 12);
                break;
            }
            case LF_BUILDINFO:
                info.name = "build info";
                break;
            case LF_STRING_ID:
                info.returnType = record.Read<std::uint32_t>(4);
                info.name = ReadRecordString(record, 8);
                break;
            case LF_UDT_SRC_LINE: {
                std::ostringstream text;
                text << "type="   << Hex32  (record.Read<std::uint32_t>(4))
                    << " source=" << id_name(record.Read<std::uint32_t>(8))
                    << " line="   <<         record.Read<std::uint32_t>(12);
                info.name = text.str();
                break;
            }
            default:
                break;
            }

            if (!info.name.empty() || info.returnType != 0)
                idInfo[idIndex] = std::move(info);

            cursor += sizeof(std::uint16_t) + length;
            ++idIndex;
        }
    }
    [[nodiscard]] std::string type_name(const std::uint32_t typeIndex) const
    {
        if (const std::string builtin = BuiltinTypeName(typeIndex); !builtin.empty())
            return builtin;
        if (const auto found = typeInfo.find(typeIndex); found != typeInfo.end()) {
            if (!found->second.name.empty())
                return found->second.name;
            if (found->second.returnType != 0)
                return procedure_signature(typeIndex);
        }
        return Hex32(typeIndex);
    }
    [[nodiscard]] std::string id_name(const std::uint32_t idIndex) const
    {
        if (const auto found = idInfo.find(idIndex); found != idInfo.end() && !found->second.name.empty())
            return found->second.name;
        return Hex32(idIndex);
    }
    [[nodiscard]] std::string procedure_signature(const std::uint32_t typeIndex) const
    {
        const auto found = typeInfo.find(typeIndex);
        if (found == typeInfo.end())
            return type_name(typeIndex);
        std::vector<std::string> args;
        args.reserve(found->second.arguments.size());
        for (const std::uint32_t arg : found->second.arguments)
            args.push_back(type_name(arg));
        return type_name(found->second.returnType) + " (" + JoinArguments(args) + ")";
    }
    [[nodiscard]] std::vector<DbiModule> read_modules() const
    {
        std::vector<DbiModule> modules;
        if (pdbStreamDbi >= streams.size() || streams[pdbStreamDbi].size < 64)
            return modules;

        const ByteView s = stream(pdbStreamDbi);
        const std::uint32_t moduleBytes = s.Read<std::uint32_t>(24);
        const std::size_t moduleStart = 64;
        const std::size_t moduleEnd = moduleStart + moduleBytes;
        if (moduleEnd > s.Size())
            throw std::runtime_error("DBI module info subsection is truncated");

        std::size_t cursor = moduleStart;
        while (cursor + 64 <= moduleEnd) {
            DbiModule mod;
            mod.opened       = s.Read<std::uint32_t>(cursor);
            mod.section      = s.Read<std::uint16_t>(cursor + 8);
            mod.offset       = s.Read<std::uint32_t>(cursor + 12);
            mod.size         = s.Read<std::uint32_t>(cursor + 16);
            mod.moduleStream = s.Read<std::uint16_t>(cursor + 34);
            mod.symbolBytes  = s.Read<std::uint32_t>(cursor + 36);
            mod.c11Bytes     = s.Read<std::uint32_t>(cursor + 40);
            mod.c13Bytes     = s.Read<std::uint32_t>(cursor + 44);

            const std::size_t names        = cursor + 64;
            const std::size_t moduleLen    = CStringLength(s, names);
            mod.moduleName                 = s.Read_c_string(names, moduleEnd - names);
            const std::size_t objectOffset = names + moduleLen + 1;
            const std::size_t objectLen    = CStringLength(s, objectOffset);
            mod.objectName                 = s.Read_c_string(objectOffset, moduleEnd - objectOffset);

            cursor = Align4(static_cast<std::uint32_t>(objectOffset + objectLen + 1));
            modules.push_back(std::move(mod));
        }
        return modules;
    }

public:
    explicit PdbParser(const ByteView file) : file(file) { Parse(); }
    [[nodiscard]] static bool IsPdb(const ByteView file) { return file.Starts_with(pdbMagic); }
    void Dump(std::ostream& out, const DumpOptions& options) const
    {
        out << "PDB/MSF 7.00\n";
        out << "  block size: "      << blockSize      << "\n";
        out << "  blocks: "          << blockCount     << "\n";
        out << "  directory bytes: " << directorySize  << "\n";
        out << "  streams: "         << streams.size() << "\n";

        if (options.streams) {
            out << "\nStreams\n";
            for (std::uint32_t i=0; i<streams.size(); ++i)
            {
                out << "  [" << std::setw(3) << i << "] ";
                if (streams[i].size == streamNil)
                    out << "<nil>\n";
                else
                    out << "size=" << streams[i].size << " blocks=" << streams[i].blocks.size() << "\n";
            }
        }

        if (options.summary || options.headers) {
            dump_pdb_info(out);
            dump_dbi(out, options.headers);
            dump_tpi_header(out, pdbStreamTpi, "TPI");
            if (pdbStreamIpi < streams.size() && streams[pdbStreamIpi].size != streamNil)
                dump_tpi_header(out, pdbStreamIpi, "IPI");
        }

        if (options.ids)
            if (pdbStreamIpi < streams.size() && streams[pdbStreamIpi].size != streamNil)
                dump_id_records(out);

        if (options.types)
            dump_type_records(out, pdbStreamTpi, "TPI type records");

        if (options.symbols)
            dump_symbol_records(out);
    }

};

} // namespace cvdump
