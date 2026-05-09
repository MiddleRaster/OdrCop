#include <utility>
#define ONE
#include "TestHeader.h"

DifferentSizedMember                                         g_1_DifferentSizedMember;
DifferentDataMembers                                         g_1_DifferentDataMembers;
DifferentOrderOfDataMembers                                  g_1_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers                                   g_1_DifferentTypeOfDataMembers;
DifferentBases                                               g_1_DifferentBases;
DifferentDataMemberType                                      g_1_DifferentDataMemberType;
SameMemberTypesDifferentOrder                                g_1_SameMemberTypesDifferentOrder;
DifferentBaseClass                                           g_1_DifferentBaseClass;
//SameClassDifferentAccessSpecifier                          g_1_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer                          g_1_DifferentDefaultMemberInitializer;
//DifferentConstexprValue                                    g_1_DifferentConstexprValue;
//DifferentConstInit                                         g_1_DifferentConstInit;
DifferentConstDataMember                                     g_1_DifferentConstDataMember;
DifferentVolatileDataMember                                  g_1_DifferentVolatileDataMember;
StructVsClass                                                g_1_StructVsClass;
//SameTemplateDifferentDefaultTemplateArguments<>            g_1_SameTemplateDifferentDefaultTemplateArguments;
SameClassDifferentAlignment                                  g_1_SameClassDifferentAlignment;
SameClassDifferentVirtualFunctionTableShape                  g_1_SameClassDifferentVirtualFunctionTableShape;
SameClassDifferentVirtualFunctionNames                       g_1_SameClassDifferentVirtualFunctionNames;
SameClassDifferentVirtualnessOnFunction                      g_1_SameClassDifferentVirtualnessOnFunction;
SameClassDifferentNoExceptOnMethod                           g_1_SameClassDifferentNoExceptOnMethod;
//SameClassDifferentInlinenessOnFunction                     g_1_SameClassDifferentInlinenessOnFunction;
//SameClassDifferentConstexpressOnFunction                   g_1_SameClassDifferentConstexpressOnFunction;
//SameClassDifferentOverrideSpecifier                        g_1_SameClassDifferentOverrideSpecifier;
SameClassDifferentStaticOnDataMember                         g_1_SameClassDifferentStaticOnDataMember;
SameClassDifferentStaticConstOnDataMember                    g_1_SameClassDifferentStaticConstOnDataMember;
SameClassDifferentStaticVolatileOnDataMember                 g_1_SameClassDifferentStaticVolatileOnDataMember;
SameClassDifferentBitfieldLayout                             g_1_SameClassDifferentBitfieldLayout;
SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion g_1_SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion;
SameClassDifferentPresenceOfAnonymousMembers                 g_1_SameClassDifferentPresenceOfAnonymousMembers;
//SameClassDifferentFriendDeclaration                        g_1_SameClassDifferentFriendDeclaration;
BaseClassesInDifferentOrder                                  g_1_BaseClassesInDifferentOrder;
EnclosingTypedefDefinition                                   g_1_EnclosingTypedefDefinition;
SameClassDifferentDataMemberAccessSpecifier                  g_1_SameClassDifferentDataMemberAccessSpecifier;
DifferentAccessSpecifiersOnBaseClass                         g_1_DifferentAccessSpecifiersOnBaseClass;
DifferentAccessSpecifiersOnMethod                            g_1_DifferentAccessSpecifiersOnMethod;
StaticFunctionOrMethod                                       g_1_StaticFunctionOrMethod;
BaseClassVirtualOrNot                                        g_1_BaseClassVirtualOrNot;
StructContainingPointerToDifferentTypes                      g_1_StructContainingPointerToDifferentTypes;
//DataMemberIsStaticConstOrStaticConstexpr                   g_1_DataMemberIsStaticConstOrStaticConstexpr;
NamelessEnum<int>::Inner<char>                               g_1_instance;



int g_1_call_FunctionsMustBeBitwiseIdentical                        = FunctionsMustBeBitwiseIdentical();
int g_1_call_SameFunctionTemplateSpecializationDifferentDefinitions = SameFunctionTemplateSpecializationDifferentDefinitions<int>();
//int g_1_call_SameConstexprFunctionDifferentBody                   = SameConstexprFunctionDifferentBody();
//int g_1_call_SameClassDifferentFriendDeclaration                  = Friendly();

auto g_1_enum = SameEnumButDifferentValues::A;



int AnOverloadInTU1(void)   { return 0; }
int AnOverloadInTU1(char a) { return sizeof(a); }

template<class T> auto f() { return [](T* /*p*/) { /* ... */ }; }
auto g_1_f_int   = f<int  >();
auto g_1_f_char  = f<char >();
auto g_1_f_short = f<short>();
auto g_1_f_long  = f<long >();




template<auto Fn>
struct Callable
{
    template<typename... Args> decltype(auto) operator()(Args&&... args) const { return Fn(std::forward<Args>(args)...); }
};

namespace Tests {

    // Test 1 — Top-level anonymous type, different layouts
    // OdrCop should NOT flag
    namespace T1 {
        namespace { struct Empty { int x; }; }
        Empty t1_instance;
    }

    // Test 2 — Anonymous type inside external-linkage struct, different layouts
    // OdrCop SHOULD flag
    namespace T2 {
        namespace { struct Helper { int x; ~Helper(){} }; }
        struct Public { Helper h; };
        Public t2_instance;
    }

#ifdef NOT_YET

    // Test 3 — Anonymous type inside external-linkage struct, identical layouts
    // OdrCop should NOT flag
    namespace T3 {
        namespace { struct Helper { int x; }; }
        struct Public { Helper h; };
        Public t3_instance;
    }

    // Test 4 — Anonymous type unused by external-linkage struct
    // OdrCop should NOT flag
    namespace T4 {
        namespace { struct Helper { int x; void f(); }; }
        struct Public { int a; };
        Helper t4_helper_instance;
        Public t4_public_instance;
    }

    // Test 5 — Anonymous type used only inside inline function
    // OdrCop should NOT flag
    namespace T5 {
        namespace { struct Local { int x; }; }
        inline int f() { Local l{ 1 }; return l.x; }
        Local t5_instance;
    }

    // Test 6 — Anonymous type as template argument, external-linkage instantiation, different layouts
    // OdrCop SHOULD flag
    namespace T6 {
        namespace { struct Tag { int x; }; }
        template<typename T> struct Wrapper { T t; };
        Wrapper<Tag> w;
    }

    // Test 7 — Anonymous type as base class of external-linkage struct, different layouts
    // OdrCop SHOULD flag
    namespace T7 {
        namespace { struct Base { int x; }; }
        struct Public : Base {};
        Public t7_instance;
    }

    // Test 8 — Anonymous type as base class of external-linkage struct, identical layouts
    // OdrCop should NOT flag
    namespace T8 {
        namespace { struct Base { int x; }; }
        struct Public : Base {};
        Public t8_instance;
    }

    // Test 9 — Anonymous type as parameter of external-linkage function, different layouts
    // OdrCop SHOULD flag
    namespace T9 {
        namespace { struct Arg { int x; }; }
        Arg t9_instance;
        void f(Arg a);
    }

    // Test 10 — Anonymous type as return type of external-linkage function, different layouts
    // OdrCop SHOULD flag
    namespace T10 {
        namespace { struct Result { int x; }; }
        Result t10_instance;
        Result f();
    }

    // Test 11 — Doubly-nested anonymous namespace, different layouts
    // OdrCop should NOT flag
    namespace T11 {
        namespace { namespace { struct Empty { int x; }; } }
        Empty t11_instance;
    }

    // Test 12 — TBCI pattern: anonymous Empty as template argument, different layouts
    // OdrCop should NOT flag
    namespace T12 {
        namespace { struct Empty {}; }
        template<typename T> struct ClassUnderTest { T t; };
        ClassUnderTest<Empty> cut;
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
