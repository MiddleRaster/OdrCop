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
    IdenticalStruct g_3_001{ 1, 2.0 };

    // TU34-002: Same external-linkage struct name, different member type.
    // Expected ODR violation: YES.
    struct DifferentMemberType
    {
        int value;
    };
    DifferentMemberType g_3_002{ 3 };

    // TU34-003: Same external-linkage struct name, same members in a different order.
    // Expected ODR violation: YES.
    struct DifferentMemberOrder
    {
        int first;
        double second;
    };
    DifferentMemberOrder g_3_003{ 4, 5.0 };

    // TU34-004: Same external-linkage struct name, different member count.
    // Expected ODR violation: YES.
    struct DifferentMemberCount
    {
        int only;
    };
    DifferentMemberCount g_3_004{ 6 };

    // TU34-005: Same external-linkage enum name, identical enumerators and values.
    // Expected ODR violation: NO.
    enum class IdenticalScopedEnum : int
    {
        zero = 0,
        one = 1
    };
    IdenticalScopedEnum g_3_005 = IdenticalScopedEnum::one;

    // TU34-006: Same external-linkage enum name, different enumerator values.
    // Expected ODR violation: YES.
    enum class DifferentEnumeratorValues : int
    {
        zero = 0,
        one = 1
    };
    DifferentEnumeratorValues g_3_006 = DifferentEnumeratorValues::one;

    // TU34-007: Same external-linkage enum name, different underlying type.
    // Expected ODR violation: YES.
    enum class DifferentEnumUnderlyingType : int
    {
        value = 1
    };
    DifferentEnumUnderlyingType g_3_007 = DifferentEnumUnderlyingType::value;

    // TU34-008: Same external-linkage union name, identical members.
    // Expected ODR violation: NO.
    union IdenticalUnion
    {
        int i;
        float f;
    };
    IdenticalUnion g_3_008{ 8 };

    // TU34-009: Same external-linkage union name, different member type.
    // Expected ODR violation: YES.
    union DifferentUnionMember
    {
        int i;
        float f;
    };
    DifferentUnionMember g_3_009{ 9 };

    // TU34-010: Same external-linkage class name, struct vs class default access differs.
    // Expected ODR violation: YES.
    struct StructVsClassDefaultAccess
    {
        int value;
    };
    StructVsClassDefaultAccess g_3_010{ 10 };

    // TU34-011: Same external-linkage class name, explicit member access differs.
    // Expected ODR violation: YES.
    class DifferentMemberAccess
    {
    public:
        int publicValue;
    private:
        int privateValue;
    public:
        DifferentMemberAccess() : publicValue(11), privateValue(12) {}
        int getPrivateValue() const { return privateValue; }
    };
    DifferentMemberAccess g_3_011;
    int g_3_011_use = g_3_011.getPrivateValue();

    // TU34-012: Same external-linkage class name, identical access and members.
    // Expected ODR violation: NO.
    class IdenticalClass
    {
    public:
        int value;
        IdenticalClass() : value(12) {}
    };
    IdenticalClass g_3_012;

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
    struct DifferentBaseClass : BaseAForDifferentBase
    {
        int own;
    };
    DifferentBaseClass g_3_013{ { 13 }, 14 };

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
    struct DifferentBaseOrder : BaseAForBaseOrder, BaseBForBaseOrder
    {
        int own;
    };
    DifferentBaseOrder g_3_014{ { 15 }, { 16 }, 17 };

    // TU34-015: Same derived class name, non-virtual base vs virtual base.
    // Expected ODR violation: YES.
    struct BaseForVirtualBase
    {
        int base;
    };
    struct DifferentVirtualBase : BaseForVirtualBase
    {
        int own;
    };
    DifferentVirtualBase g_3_015{ { 18 }, 19 };

    // TU34-016: Same external-linkage class name, different virtual function table shape.
    // Expected ODR violation: YES.
    struct DifferentVirtualShape
    {
        virtual int first() { return 16; }
        int data;
    };
    DifferentVirtualShape g_3_016;
    int g_3_016_use = g_3_016.first();

    // TU34-017: Same external-linkage class name, virtual function name differs.
    // Expected ODR violation: YES.
    struct DifferentVirtualFunctionName
    {
        virtual int alpha() { return 17; }
        int data;
    };
    DifferentVirtualFunctionName g_3_017;
    int g_3_017_use = g_3_017.alpha();

    // TU34-018: Same external-linkage class name, virtualness of a method differs.
    // Expected ODR violation: YES.
    struct DifferentMethodVirtualness
    {
        virtual int value() { return 18; }
        int data;
    };
    DifferentMethodVirtualness g_3_018;
    int g_3_018_use = g_3_018.value();

    // TU34-019: Same external-linkage class name, const qualification of a method differs.
    // Expected ODR violation: YES.
    struct DifferentMethodConstness
    {
        int value() const { return 19; }
        int data;
    };
    DifferentMethodConstness g_3_019{ 20 };
    int g_3_019_use = g_3_019.value();

    // TU34-020: Same external-linkage class name, noexcept specification differs.
    // Expected ODR violation: YES.
    struct DifferentMethodNoexcept
    {
        int value() noexcept { return 20; }
        int data;
    };
    DifferentMethodNoexcept g_3_020{ 21 };
    int g_3_020_use = g_3_020.value();

    // TU34-021: Same external-linkage class name, static data member type differs.
    // Expected ODR violation: YES.
    struct DifferentStaticDataMember
    {
        static const int value = 21;
        int payload;
    };
    DifferentStaticDataMember g_3_021{ DifferentStaticDataMember::value };

    // TU34-022: Same external-linkage class name, static member function return type differs.
    // Expected ODR violation: YES.
    struct DifferentStaticMemberFunction
    {
        static int value() { return 22; }
        int payload;
    };
    DifferentStaticMemberFunction g_3_022{ DifferentStaticMemberFunction::value() };
    auto* g_3_022_address = &DifferentStaticMemberFunction::value;

    // TU34-023: Same external-linkage struct name, bitfield width differs.
    // Expected ODR violation: YES.
    struct DifferentBitfieldWidth
    {
        unsigned int a : 3;
        unsigned int b : 5;
    };
    DifferentBitfieldWidth g_3_023{ 3U, 5U };

    // TU34-024: Same external-linkage struct name, bitfield signedness differs.
    // Expected ODR violation: YES.
    struct DifferentBitfieldSignedness
    {
        signed int a : 4;
    };
    DifferentBitfieldSignedness g_3_024{ 4 };

    // TU34-025: Same external-linkage struct name, anonymous union member type differs.
    // Expected ODR violation: YES.
    struct DifferentAnonymousUnionMember
    {
        int tag;
        union
        {
            int i;
            float f;
        };
    };
    DifferentAnonymousUnionMember g_3_025{ 25, { 26 } };

    // TU34-026: Same external-linkage struct name, anonymous struct member order differs.
    // Expected ODR violation: YES.
    struct DifferentAnonymousStructOrder
    {
        int tag;
        struct
        {
            int x;
            double y;
        };
    };
    DifferentAnonymousStructOrder g_3_026{ 26, { 27, 28.0 } };

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
    IdenticalAnonymousUnion g_3_027{ 27, { 28 } };

    // TU34-028: Same external-linkage struct name, typedef target differs.
    // Expected ODR violation: YES.
    struct DifferentTypedefTarget
    {
        typedef int Alias;
        Alias value;
    };
    DifferentTypedefTarget g_3_028{ 28 };

    // TU34-029: Same external-linkage struct name, using-alias target differs.
    // Expected ODR violation: YES.
    struct DifferentUsingAliasTarget
    {
        using Alias = int;
        Alias value;
    };
    DifferentUsingAliasTarget g_3_029{ 29 };

    // TU34-030: Same external-linkage class template specialization, identical definition.
    // Expected ODR violation: NO.
    template<typename T>
    struct IdenticalTemplate
    {
        T value;
    };
    IdenticalTemplate<int> g_3_030{ 30 };

    // TU34-031: Same external-linkage class template specialization, different member type.
    // Expected ODR violation: YES.
    template<typename T>
    struct DifferentTemplateDefinition
    {
        T value;
        int extra;
    };
    DifferentTemplateDefinition<int> g_3_031{ 31, 32 };

    // TU34-032: Same external-linkage template with non-type argument, definition differs.
    // Expected ODR violation: YES.
    template<int N>
    struct DifferentNonTypeTemplateDefinition
    {
        int values[N];
    };
    DifferentNonTypeTemplateDefinition<2> g_3_032{ { 32, 33 } };

    // TU34-033: Same external-linkage inline function name and signature, identical body.
    // Expected ODR violation: NO.
    inline int IdenticalInlineFunction(int x)
    {
        return x + 33;
    }
    int g_3_033 = IdenticalInlineFunction(1);
    auto* g_3_033_address = &IdenticalInlineFunction;

    // TU34-034: Same external-linkage inline function name and signature, different body.
    // Expected ODR violation: YES.
    inline int DifferentInlineFunctionBody(int x)
    {
        return x + 34;
    }
    int g_3_034 = DifferentInlineFunctionBody(1);
    auto* g_3_034_address = &DifferentInlineFunctionBody;

    // TU34-035: Same internal-linkage static function name, different body.
    // Expected ODR violation: NO.
    static int SameStaticFunctionNameDifferentBody(int x)
    {
        return x + 35;
    }
    int g_3_035 = SameStaticFunctionNameDifferentBody(1);
    auto* g_3_035_address = &SameStaticFunctionNameDifferentBody;

    // TU34-036: Same anonymous-namespace function name, different body.
    // Expected ODR violation: NO.
    namespace
    {
        int SameAnonymousNamespaceFunctionNameDifferentBody(int x)
        {
            return x + 36;
        }
    }
    int g_3_036 = SameAnonymousNamespaceFunctionNameDifferentBody(1);
    auto* g_3_036_address = &SameAnonymousNamespaceFunctionNameDifferentBody;

    // TU34-037: Same anonymous-namespace type name at namespace scope, different layout.
    // Expected ODR violation: NO.
    namespace
    {
        struct SameAnonymousNamespaceTypeNameDifferentLayout
        {
            int x;
        };
    }
    SameAnonymousNamespaceTypeNameDifferentLayout g_3_037{ 37 };

    // TU34-038: Anonymous-namespace type embedded in external-linkage type, different layout.
    // Expected ODR violation: YES.
    namespace
    {
        struct InternalPartForExternalCarrier
        {
            int x;
        };
    }
    struct ExternalCarrierOfInternalType
    {
        InternalPartForExternalCarrier part;
    };
    ExternalCarrierOfInternalType g_3_038{ { 38 } };

    // TU34-039: Anonymous-namespace type used only by another internal-linkage type, different layout.
    // Expected ODR violation: NO.
    namespace
    {
        struct InternalOnlyPart
        {
            int x;
        };
        struct InternalOnlyCarrier
        {
            InternalOnlyPart part;
        };
        InternalOnlyCarrier g_3_039_internal{ { 39 } };
    }
    int g_3_039 = g_3_039_internal.part.x;

    // TU34-040: Same external-linkage function pointer member type, pointee signature differs.
    // Expected ODR violation: YES.
    struct DifferentFunctionPointerMember
    {
        int (*callback)(int);
    };
    int CallbackForTU3(int x)
    {
        return x + 40;
    }
    DifferentFunctionPointerMember g_3_040{ &CallbackForTU3 };

    // TU34-041: Same external-linkage pointer-to-member type, member type differs.
    // Expected ODR violation: YES.
    struct MemberPointerTarget
    {
        int value;
    };
    struct DifferentPointerToMember
    {
        int MemberPointerTarget::* member;
    };
    DifferentPointerToMember g_3_041{ &MemberPointerTarget::value };

    // TU34-042: Same external-linkage array member type, bound differs.
    // Expected ODR violation: YES.
    struct DifferentArrayBound
    {
        int values[3];
    };
    DifferentArrayBound g_3_042{ { 42, 43, 44 } };

    // TU34-043: Same external-linkage volatile/const qualification on member differs.
    // Expected ODR violation: YES.
    struct DifferentCvQualifiedMember
    {
        const int value;
    };
    DifferentCvQualifiedMember g_3_043{ 43 };

    // TU34-044: Same external-linkage nested class name, nested member differs.
    // Expected ODR violation: YES.
    struct DifferentNestedClass
    {
        struct Nested
        {
            int x;
        };
        Nested nested;
    };
    DifferentNestedClass g_3_044{ { 44 } };

    // TU34-045: Same external-linkage nested enum name, nested enumerators differ.
    // Expected ODR violation: YES.
    struct DifferentNestedEnum
    {
        enum Kind
        {
            alpha = 1,
            beta = 2
        };
        Kind kind;
    };
    DifferentNestedEnum g_3_045{ DifferentNestedEnum::alpha };

    // TU34-046: Same external-linkage empty class definition.
    // Expected ODR violation: NO.
    struct IdenticalEmpty
    {
    };
    IdenticalEmpty g_3_046;

    // TU34-047: Same external-linkage empty class vs non-empty class.
    // Expected ODR violation: YES.
    struct EmptyVsNonEmpty
    {
    };
    EmptyVsNonEmpty g_3_047;

    // TU34-048: Same external-linkage class name, default constructor member initializer differs.
    // Expected ODR violation: YES if constructor bodies/debug records are compared.
    struct DifferentConstructorInitializer
    {
        int value;
        DifferentConstructorInitializer() : value(48) {}
    };
    DifferentConstructorInitializer g_3_048;
    int g_3_048_use = g_3_048.value;

    // TU34-049: Same external-linkage lambda closure use through an inline function, identical shape.
    // Expected ODR violation: NO.
    inline int IdenticalLambdaUser(int x)
    {
        auto lambda = [](int y) { return y + 49; };
        return lambda(x);
    }
    int g_3_049 = IdenticalLambdaUser(1);
    auto* g_3_049_address = &IdenticalLambdaUser;

    // TU34-050: Same external-linkage lambda closure use through an inline function, different body.
    // Expected ODR violation: YES if function/lambda bodies are compared.
    inline int DifferentLambdaUser(int x)
    {
        auto lambda = [](int y) { return y + 50; };
        return lambda(x);
    }
    int g_3_050 = DifferentLambdaUser(1);
    auto* g_3_050_address = &DifferentLambdaUser;
}

#pragma warning(pop)
