#include <utility>
#define TWO
#include "TestHeader.h"

DifferentSizedMember                                         g_2_DifferentSizedMember;
DifferentDataMembers                                         g_2_DifferentDataMembers;
DifferentOrderOfDataMembers                                  g_2_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers                                   g_2_DifferentTypeOfDataMembers;
DifferentBases                                               g_2_DifferentBases;
DifferentDataMemberType                                      g_2_DifferentDataMemberType;
SameMemberTypesDifferentOrder                                g_2_SameMemberTypesDifferentOrder;
DifferentBaseClass                                           g_2_DifferentBaseClass;
//SameClassDifferentAccessSpecifier                          g_2_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer                          g_2_DifferentDefaultMemberInitializer;
//DifferentConstexprValue                                    g_2_DifferentConstexprValue;
//DifferentConstInit                                         g_2_DifferentConstInit;
DifferentConstDataMember                                     g_2_DifferentConstDataMember;
DifferentVolatileDataMember                                  g_2_DifferentVolatileDataMember;
StructVsClass                                                g_2_StructVsClass;
//SameTemplateDifferentDefaultTemplateArguments<>            g_2_SameTemplateDifferentDefaultTemplateArguments;
SameClassDifferentAlignment                                  g_2_SameClassDifferentAlignment;
SameClassDifferentVirtualFunctionTableShape                  g_2_SameClassDifferentVirtualFunctionTableShape;
SameClassDifferentVirtualFunctionNames                       g_2_SameClassDifferentVirtualFunctionNames;
SameClassDifferentVirtualnessOnFunction                      g_2_SameClassDifferentVirtualnessOnFunction;
SameClassDifferentNoExceptOnMethod                           g_2_SameClassDifferentNoExceptOnMethod;
//SameClassDifferentInlinenessOnFunction                     g_2_SameClassDifferentInlinenessOnFunction;
//SameClassDifferentConstexpressOnFunction                   g_2_SameClassDifferentConstexpressOnFunction;
//SameClassDifferentOverrideSpecifier                        g_2_SameClassDifferentOverrideSpecifier;
SameClassDifferentStaticOnDataMember                         g_2_SameClassDifferentStaticOnDataMember;
SameClassDifferentStaticConstOnDataMember                    g_2_SameClassDifferentStaticConstOnDataMember;
SameClassDifferentStaticVolatileOnDataMember                 g_2_SameClassDifferentStaticVolatileOnDataMember;
SameClassDifferentBitfieldLayout                             g_2_SameClassDifferentBitfieldLayout;
SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion g_2_SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion;
SameClassDifferentPresenceOfAnonymousMembers                 g_2_SameClassDifferentPresenceOfAnonymousMembers;
//SameClassDifferentFriendDeclaration                        g_2_SameClassDifferentFriendDeclaration;
BaseClassesInDifferentOrder                                  g_2_BaseClassesInDifferentOrder;
EnclosingTypedefDefinition                                   g_2_EnclosingTypedefDefinition;
SameClassDifferentDataMemberAccessSpecifier                  g_2_SameClassDifferentDataMemberAccessSpecifier;
DifferentAccessSpecifiersOnBaseClass                         g_2_DifferentAccessSpecifiersOnBaseClass;
DifferentAccessSpecifiersOnMethod                            g_2_DifferentAccessSpecifiersOnMethod;
StaticFunctionOrMethod                                       g_2_StaticFunctionOrMethod;
BaseClassVirtualOrNot                                        g_2_BaseClassVirtualOrNot;
StructContainingPointerToDifferentTypes                      g_2_StructContainingPointerToDifferentTypes;
//DataMemberIsStaticConstOrStaticConstexpr                   g_2_DataMemberIsStaticConstOrStaticConstexpr;
NamelessEnum<int>::Inner<long>                               g_2_instance;



int g_2_call_FunctionsMustBeBitwiseIdentical                        = FunctionsMustBeBitwiseIdentical();
int g_2_call_SameFunctionTemplateSpecializationDifferentDefinitions = SameFunctionTemplateSpecializationDifferentDefinitions<int>();
//int g_2_call_SameConstexprFunctionDifferentBody                   = SameConstexprFunctionDifferentBody();
//int g_2_call_SameClassDifferentFriendDeclaration                  = Friendly();

auto g_2_enum = SameEnumButDifferentValues::A;



int AnOverloadInTU2(void)   { return 0; }
int AnOverloadInTU2(char a) { return sizeof(a); }

template<class T> auto f() { return [](T* /*p*/) { /* ... */ }; }
auto g_2_f_int   = f<int  >();
auto g_2_f_char  = f<char >();
auto g_2_f_short = f<short>();
auto g_2_f_long  = f<long >();




template<auto Fn>
struct Callable
{
    template<typename... Args> decltype(auto) operator()(Args&&... args) const { return Fn(std::forward<Args>(args)...); }
};

namespace Tests {

    // Test 1 — Top-level anonymous type, different layouts
    // OdrCop should NOT flag
    namespace T1 {
        namespace { struct Empty { double y; }; }
        Empty t1_instance;
    }

    // Test 2 — Anonymous type inside external-linkage struct, different layouts
    // OdrCop SHOULD flag
    namespace T2 {
        namespace { struct Helper { unsigned int y; }; }
        struct Public { Helper h; };
        Public g_2_t2_instance;
    }

    // Test 3 — Anonymous type inside external-linkage struct, identical layouts
    // OdrCop should NOT flag
    namespace T3 {
        namespace { struct Helper { int x; }; }
        struct Public { Helper h; };
        Public g_2_t3_instance;
    }

    // Test 4 — Anonymous type unused by external-linkage struct
    // OdrCop should NOT flag
    namespace T4 {
        namespace { struct Helper { int x; void f(int); }; }
        struct Public { int a; };
        Helper g_2_t4_helper_instance;
        Public g_2_t4_public_instance;
    }

    // Test 5 — Anonymous type used only inside inline function
    // OdrCop should NOT flag
    namespace T5 {
        namespace { struct Local { double y; }; }
        inline int f() { Local l{ 1.0 }; return (int)l.y; }
        Local g_2_t5_instance;
    }

#ifdef DOES_NOT_WORK_ON_GITHUB
    // Test 6 — Anonymous type as template argument, external-linkage instantiation, different layouts
    // OdrCop SHOULD flag
    namespace T6 {
        namespace { struct Tag { double y; }; }
        template<typename T> struct Wrapper { T t; };
        Wrapper<Tag> g_2_w;
    }
#endif

    // Test 7 — Anonymous type as base class of external-linkage struct, different layouts
    // OdrCop SHOULD flag
    namespace T7 {
        namespace { struct Base { double y; }; }
        struct Public : Base {};
        Public g_2_t7_instance;
    }

    // Test 8 — Anonymous type as base class of external-linkage struct, identical layouts
    // OdrCop should NOT flag
    namespace T8 {
        namespace { struct Base { int x; }; }
        struct Public : Base {};
        Public g_2_t8_instance;
    }

    // Test 9 — Anonymous type as parameter of external-linkage function, different layouts
    // OdrCop SHOULD flag
    namespace T9 {
        namespace { struct Arg { double y; }; }
        Arg g_2_t9_instance;
        void function9(Arg a) { (void)a; }
    }
    auto* g_2_addressOfFunctionWithAnonymousNamespaceArg = &T9::function9;   // forces codegen
    namespace T9a {
        namespace { struct Arg { unsigned int x; }; }
        Arg g_2_t9a_instance;
        void function9(Arg a) { (void)a; }
    }
    auto* g_2a_addressOfFunctionWithAnonymousNamespaceArg = &T9a::function9;   // forces codegen

    // Test 10 — Anonymous type as return type of external-linkage function, different layouts
    // OdrCop SHOULD flag
    namespace T10 {
        namespace { struct Result { double y; }; }
        Result ReturnAnAnonymousType() { return {}; };
        Result g_2_t10_instance = ReturnAnAnonymousType();
    }

#ifdef NOT_YET

    // Test 11 — Doubly-nested anonymous namespace, different layouts
    // OdrCop should NOT flag
    namespace T11 {
        namespace { namespace { struct Empty { double y; }; } }
        Empty g_2_t11_instance;
    }

    // Test 12 — TBCI pattern: anonymous Empty as template argument, different layouts
    // OdrCop should NOT flag
    namespace T12 {
        namespace { struct Empty { int payload; }; }
        template<typename T> struct ClassUnderTest { T t; };
        ClassUnderTest<Empty> g_2_cut;
    }

#endif

} // namespace Tests

/*
### ** Expected OdrCop Results**

| Test | Scenario                                                     | OdrCop Flag ? |
|------| -------------------------------------------------------------|---------------|
|  1   | Top - level anonymous type, different layouts                | ❌ No         |
|  2   | Anonymous type inside external - linkage struct, different   | ✔ Yes         |
|  3   | Anonymous type inside external - linkage struct, identical   | ❌ No         |
|  4   | Anonymous type unused by external - linkage struct           | ❌ No         |
|  5   | Anonymous type used only inside inline function              | ❌ No         |
|  6   | Anonymous type in template instantiation, external linkage   | ✔ Yes         |
|  7   | Anonymous type as base class of external struct, different   | ✔ Yes         |
|  8   | Anonymous type as base class of external struct, identical   | ❌ No         |
|  9   | Anonymous type as parameter of external - linkage function   | ✔ Yes         |
| 10   | Anonymous type as return type of external - linkage function | ✔ Yes         |
| 11   | Doubly - nested anonymous namespace, different layouts       | ❌ No         |
| 12   | TBCI pattern : anonymous Empty as template argument          | ❌ No         |
*/

auto* g_2_addressOf_FunctionUsing_DifferentDataMembers             = &FunctionUsing_DifferentDataMembers;
auto* g_2_addressOf_FunctionUsing_SameEnumNameDifferentEnumerators = &FunctionUsing_SameEnumNameDifferentEnumerators;
auto* g_2_addressOf_FunctionUsing_SameTemplateDifferentDefinition  = &FunctionUsing_SameTemplateDifferentDefinition;
//auto* g_2_addressOf_FunctionUsing_InlinedBody                    = &FunctionUsing_InlinedBody;
auto* g_2_addressOf_FunctionUsing_SameUnionNameDifferentElements   = &FunctionUsing_SameUnionNameDifferentElements;
