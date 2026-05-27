#pragma warning(push)
#pragma warning(disable: 4201) // Intentionally test MSVC nameless struct/union debug records.

namespace OdrCopTU34Tests
{
    // TU34-001: Identical external-linkage struct definitions.
    // Expected ODR violation: NO.
    struct IdenticalStruct
    {
        int a;
        double b;
    };
    IdenticalStruct g_4_001{ 1, 2.0 };

    // TU34-002: Same external-linkage struct name, different member type.
    // Expected ODR violation: YES.
    struct DifferentMemberType
    {
        long value;
    };
    DifferentMemberType g_4_002{ 3L };

    // TU34-003: Same external-linkage struct name, same members in a different order.
    // Expected ODR violation: YES.
    struct DifferentMemberOrder
    {
        double second;
        int first;
    };
    DifferentMemberOrder g_4_003{ 5.0, 4 };

    // TU34-004: Same external-linkage struct name, different member count.
    // Expected ODR violation: YES.
    struct DifferentMemberCount
    {
        int only;
        int extra;
    };
    DifferentMemberCount g_4_004{ 6, 7 };

    // TU34-005: Same external-linkage enum name, identical enumerators and values.
    // Expected ODR violation: NO.
    enum class IdenticalScopedEnum : int
    {
        zero = 0,
        one = 1
    };
    IdenticalScopedEnum g_4_005 = IdenticalScopedEnum::one;

    // TU34-006: Same external-linkage enum name, different enumerator values.
    // Expected ODR violation: YES.
    enum class DifferentEnumeratorValues : int
    {
        zero = 0,
        one = 2
    };
    DifferentEnumeratorValues g_4_006 = DifferentEnumeratorValues::one;

    // TU34-007: Same external-linkage enum name, different underlying type.
    // Expected ODR violation: YES.
    enum class DifferentEnumUnderlyingType : unsigned int
    {
        value = 1U
    };
    DifferentEnumUnderlyingType g_4_007 = DifferentEnumUnderlyingType::value;

    // TU34-008: Same external-linkage union name, identical members.
    // Expected ODR violation: NO.
    union IdenticalUnion
    {
        int i;
        float f;
    };
    IdenticalUnion g_4_008{ 8 };

    // TU34-009: Same external-linkage union name, different member type.
    // Expected ODR violation: YES.
    union DifferentUnionMember
    {
        int i;
        double f;
    };
    DifferentUnionMember g_4_009{ 9 };

    // TU34-010: Same external-linkage class name, struct vs class default access differs.
    // Expected ODR violation: YES.
    class StructVsClassDefaultAccess
    {
        int value;
    public:
        StructVsClassDefaultAccess() : value(10) {}
        int get() const { return value; }
    };
    StructVsClassDefaultAccess g_4_010;
    int g_4_010_use = g_4_010.get();

    // TU34-011: Same external-linkage class name, explicit member access differs.
    // Expected ODR violation: YES.
    class DifferentMemberAccess
    {
    private:
        int publicValue;
    public:
        int privateValue;
        DifferentMemberAccess() : publicValue(11), privateValue(12) {}
        int getPublicValue() const { return publicValue; }
    };
    DifferentMemberAccess g_4_011;
    int g_4_011_use = g_4_011.getPublicValue();

    // TU34-012: Same external-linkage class name, identical access and members.
    // Expected ODR violation: NO.
    class IdenticalClass
    {
    public:
        int value;
        IdenticalClass() : value(12) {}
    };
    IdenticalClass g_4_012;

    // TU34-013: Same derived class name, different direct base class.
    // Expected ODR violation: YES.
    struct BaseAForDifferentBase
    {
        int a;
    };
    struct BaseBForDifferentBase
    {
        int b;
    };
    struct DifferentBaseClass : BaseBForDifferentBase
    {
        int own;
    };
    DifferentBaseClass g_4_013{ { 13 }, 14 };

    // TU34-014: Same derived class name, same bases but in a different order.
    // Expected ODR violation: YES.
    struct BaseAForBaseOrder
    {
        int a;
    };
    struct BaseBForBaseOrder
    {
        int b;
    };
    struct DifferentBaseOrder : BaseBForBaseOrder, BaseAForBaseOrder
    {
        int own;
    };
    DifferentBaseOrder g_4_014{ { 16 }, { 15 }, 17 };

    // TU34-015: Same derived class name, non-virtual base vs virtual base.
    // Expected ODR violation: YES.
    struct BaseForVirtualBase
    {
        int base;
    };
    struct DifferentVirtualBase : virtual BaseForVirtualBase
    {
        int own;
        DifferentVirtualBase() : BaseForVirtualBase{ 18 }, own(19) {}
    };
    DifferentVirtualBase g_4_015;

    // TU34-016: Same external-linkage class name, different virtual function table shape.
    // Expected ODR violation: YES.
    struct DifferentVirtualShape
    {
        virtual int first() { return 16; }
        virtual int second() { return 160; }
        int data;
    };
    DifferentVirtualShape g_4_016;
    int g_4_016_use = g_4_016.first() + g_4_016.second();

    // TU34-017: Same external-linkage class name, virtual function name differs.
    // Expected ODR violation: YES.
    struct DifferentVirtualFunctionName
    {
        virtual int beta() { return 17; }
        int data;
    };
    DifferentVirtualFunctionName g_4_017;
    int g_4_017_use = g_4_017.beta();

    // TU34-018: Same external-linkage class name, virtualness of a method differs.
    // Expected ODR violation: YES.
    struct DifferentMethodVirtualness
    {
        int value() { return 18; }
        int data;
    };
    DifferentMethodVirtualness g_4_018;
    int g_4_018_use = g_4_018.value();

    // TU34-019: Same external-linkage class name, const qualification of a method differs.
    // Expected ODR violation: YES.
    struct DifferentMethodConstness
    {
        int value() { return 19; }
        int data;
    };
    DifferentMethodConstness g_4_019{ 20 };
    int g_4_019_use = g_4_019.value();

    // TU34-020: Same external-linkage class name, noexcept specification differs.
    // Expected ODR violation: YES.
    struct DifferentMethodNoexcept
    {
        int value() { return 20; }
        int data;
    };
    DifferentMethodNoexcept g_4_020{ 21 };
    int g_4_020_use = g_4_020.value();

    // TU34-021: Same external-linkage class name, static data member type differs.
    // Expected ODR violation: YES.
    struct DifferentStaticDataMember
    {
        static const long value = 21L;
        int payload;
    };
    DifferentStaticDataMember g_4_021{ static_cast<int>(DifferentStaticDataMember::value) };

    // TU34-022: Same external-linkage class name, static member function return type differs.
    // Expected ODR violation: YES.
    struct DifferentStaticMemberFunction
    {
        static long value() { return 22L; }
        int payload;
    };
    DifferentStaticMemberFunction g_4_022{ static_cast<int>(DifferentStaticMemberFunction::value()) };
    auto* g_4_022_address = &DifferentStaticMemberFunction::value;

    // TU34-023: Same external-linkage struct name, bitfield width differs.
    // Expected ODR violation: YES.
    struct DifferentBitfieldWidth
    {
        unsigned int a : 4;
        unsigned int b : 4;
    };
    DifferentBitfieldWidth g_4_023{ 3U, 5U };

    // TU34-024: Same external-linkage struct name, bitfield signedness differs.
    // Expected ODR violation: YES.
    struct DifferentBitfieldSignedness
    {
        unsigned int a : 4;
    };
    DifferentBitfieldSignedness g_4_024{ 4U };

    // TU34-025: Same external-linkage struct name, anonymous union member type differs.
    // Expected ODR violation: YES.
    struct DifferentAnonymousUnionMember
    {
        int tag;
        union
        {
            int i;
            double f;
        };
    };
    DifferentAnonymousUnionMember g_4_025{ 25, { 26 } };

    // TU34-026: Same external-linkage struct name, anonymous struct member order differs.
    // Expected ODR violation: YES.
    struct DifferentAnonymousStructOrder
    {
        int tag;
        struct
        {
            double y;
            int x;
        };
    };
    DifferentAnonymousStructOrder g_4_026{ 26, { 28.0, 27 } };

    // TU34-027: Same external-linkage struct name, identical anonymous union.
    // Expected ODR violation: NO.
    struct IdenticalAnonymousUnion
    {
        int tag;
        union
        {
            int i;
            float f;
        };
    };
    IdenticalAnonymousUnion g_4_027{ 27, { 28 } };

    // TU34-028: Same external-linkage struct name, typedef target differs.
    // Expected ODR violation: YES.
    struct DifferentTypedefTarget
    {
        typedef long Alias;
        Alias value;
    };
    DifferentTypedefTarget g_4_028{ 28L };

    // TU34-029: Same external-linkage struct name, using-alias target differs.
    // Expected ODR violation: YES.
    struct DifferentUsingAliasTarget
    {
        using Alias = long;
        Alias value;
    };
    DifferentUsingAliasTarget g_4_029{ 29L };

    // TU34-030: Same external-linkage class template specialization, identical definition.
    // Expected ODR violation: NO.
    template<typename T>
    struct IdenticalTemplate
    {
        T value;
    };
    IdenticalTemplate<int> g_4_030{ 30 };

    // TU34-031: Same external-linkage class template specialization, different member type.
    // Expected ODR violation: YES.
    template<typename T>
    struct DifferentTemplateDefinition
    {
        T value;
        long extra;
    };
    DifferentTemplateDefinition<int> g_4_031{ 31, 32L };

    // TU34-032: Same external-linkage template with non-type argument, definition differs.
    // Expected ODR violation: YES.
    template<int N>
    struct DifferentNonTypeTemplateDefinition
    {
        long values[N];
    };
    DifferentNonTypeTemplateDefinition<2> g_4_032{ { 32L, 33L } };

    // TU34-033: Same external-linkage inline function name and signature, identical body.
    // Expected ODR violation: NO.
    inline int IdenticalInlineFunction(int x)
    {
        return x + 33;
    }
    int g_4_033 = IdenticalInlineFunction(1);
    auto* g_4_033_address = &IdenticalInlineFunction;

    // TU34-034: Same external-linkage inline function name and signature, different body.
    // Expected ODR violation: YES.
    inline int DifferentInlineFunctionBody(int x)
    {
        return x + 340;
    }
    int g_4_034 = DifferentInlineFunctionBody(1);
    auto* g_4_034_address = &DifferentInlineFunctionBody;

    // TU34-035: Same internal-linkage static function name, different body.
    // Expected ODR violation: NO.
    static int SameStaticFunctionNameDifferentBody(int x)
    {
        return x + 350;
    }
    int g_4_035 = SameStaticFunctionNameDifferentBody(1);
    auto* g_4_035_address = &SameStaticFunctionNameDifferentBody;

    // TU34-036: Same anonymous-namespace function name, different body.
    // Expected ODR violation: NO.
    namespace
    {
        int SameAnonymousNamespaceFunctionNameDifferentBody(int x)
        {
            return x + 360;
        }
    }
    int g_4_036 = SameAnonymousNamespaceFunctionNameDifferentBody(1);
    auto* g_4_036_address = &SameAnonymousNamespaceFunctionNameDifferentBody;

    // TU34-037: Same anonymous-namespace type name at namespace scope, different layout.
    // Expected ODR violation: NO.
    namespace
    {
        struct SameAnonymousNamespaceTypeNameDifferentLayout
        {
            double y;
        };
    }
    SameAnonymousNamespaceTypeNameDifferentLayout g_4_037{ 37.0 };

    // TU34-038: Anonymous-namespace type embedded in external-linkage type, different layout.
    // Expected ODR violation: YES.
    namespace
    {
        struct InternalPartForExternalCarrier
        {
            double y;
        };
    }
    struct ExternalCarrierOfInternalType
    {
        InternalPartForExternalCarrier part;
    };
    ExternalCarrierOfInternalType g_4_038{ { 38.0 } };

    // TU34-039: Anonymous-namespace type used only by another internal-linkage type, different layout.
    // Expected ODR violation: NO.
    namespace
    {
        struct InternalOnlyPart
        {
            double y;
        };
        struct InternalOnlyCarrier
        {
            InternalOnlyPart part;
        };
        InternalOnlyCarrier g_4_039_internal{ { 39.0 } };
    }
    int g_4_039 = static_cast<int>(g_4_039_internal.part.y);

    // TU34-040: Same external-linkage function pointer member type, pointee signature differs.
    // Expected ODR violation: YES.
    struct DifferentFunctionPointerMember
    {
        long (*callback)(long);
    };
    long CallbackForTU4(long x)
    {
        return x + 40L;
    }
    DifferentFunctionPointerMember g_4_040{ &CallbackForTU4 };

    // TU34-041: Same external-linkage pointer-to-member type, member type differs.
    // Expected ODR violation: YES.
    struct MemberPointerTarget
    {
        long value;
    };
    struct DifferentPointerToMember
    {
        long MemberPointerTarget::* member;
    };
    DifferentPointerToMember g_4_041{ &MemberPointerTarget::value };

    // TU34-042: Same external-linkage array member type, bound differs.
    // Expected ODR violation: YES.
    struct DifferentArrayBound
    {
        int values[4];
    };
    DifferentArrayBound g_4_042{ { 42, 43, 44, 45 } };

    // TU34-043: Same external-linkage volatile/const qualification on member differs.
    // Expected ODR violation: YES.
    struct DifferentCvQualifiedMember
    {
        volatile int value;
    };
    DifferentCvQualifiedMember g_4_043{ 43 };

    // TU34-044: Same external-linkage nested class name, nested member differs.
    // Expected ODR violation: YES.
    struct DifferentNestedClass
    {
        struct Nested
        {
            double y;
        };
        Nested nested;
    };
    DifferentNestedClass g_4_044{ { 44.0 } };

    // TU34-045: Same external-linkage nested enum name, nested enumerators differ.
    // Expected ODR violation: YES.
    struct DifferentNestedEnum
    {
        enum Kind
        {
            alpha = 1,
            gamma = 3
        };
        Kind kind;
    };
    DifferentNestedEnum g_4_045{ DifferentNestedEnum::alpha };

    // TU34-046: Same external-linkage empty class definition.
    // Expected ODR violation: NO.
    struct IdenticalEmpty
    {
    };
    IdenticalEmpty g_4_046;

    // TU34-047: Same external-linkage empty class vs non-empty class.
    // Expected ODR violation: YES.
    struct EmptyVsNonEmpty
    {
        int value;
    };
    EmptyVsNonEmpty g_4_047{ 47 };

    // TU34-048: Same external-linkage class name, default constructor member initializer differs.
    // Expected ODR violation: YES if constructor bodies/debug records are compared.
    struct DifferentConstructorInitializer
    {
        int value;
        DifferentConstructorInitializer() : value(480) {}
    };
    DifferentConstructorInitializer g_4_048;
    int g_4_048_use = g_4_048.value;

    // TU34-049: Same external-linkage lambda closure use through an inline function, identical shape.
    // Expected ODR violation: NO.
    inline int IdenticalLambdaUser(int x)
    {
        auto lambda = [](int y) { return y + 49; };
        return lambda(x);
    }
    int g_4_049 = IdenticalLambdaUser(1);
    auto* g_4_049_address = &IdenticalLambdaUser;

    // TU34-050: Same external-linkage lambda closure use through an inline function, different body.
    // Expected ODR violation: YES if function/lambda bodies are compared.
    inline int DifferentLambdaUser(int x)
    {
        auto lambda = [](int y) { return y + 500; };
        return lambda(x);
    }
    int g_4_050 = DifferentLambdaUser(1);
    auto* g_4_050_address = &DifferentLambdaUser;
}

#pragma warning(pop)
