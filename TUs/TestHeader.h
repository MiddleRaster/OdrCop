// Some things are not recorded in the PDB debug info, making some ODR violations undetecable by DIA.
// Here are some known undetectable ODR violations:
//   - Access specifier differences (public/private/protected)
//   - Default argument differences
//   - static constexpr / static const member value differences
//   - static constexpr / consteval / constinit
//   - typedefs
//   - default template arguments
// 
// below here is untested (as yet)
//   - constexpr / inline differences on member functions
//   - noexcept differences
//   - Attribute differences (__declspec etc.)

#ifdef KNOWN_LIMITATIONS_OF_PDB_DIA

// Same class but different access specifiers
struct SameClassDifferentAccessSpecifier
{
#ifdef ONE
public:
#else
private:
#endif
    int a;
};

// Same class but different default member initializers
struct DifferentDefaultMemberInitializer
{
#ifdef ONE
    int a = 1;
#else
    int a = 2;
#endif
};

// Same class but different constexpr values
struct DifferentConstexprValue
{
#ifdef ONE
    static constexpr int v = 1;
#else
    static constexpr int v = 2;
#endif
};

// Same class but different constexpr / consteval / constinit
// These affect linkage and initialization.
struct DifferentConstInit
{
#ifdef ONE
    static constinit int x;
#else
    static           int x;
#endif
};

// Same constexpr function but different bodies
constexpr int SameConstexprFunctionDifferentBody()
{
#ifdef ONE
    return 1;
#else
    return 2;
#endif
}


//Same typedef or using but different underlying type
struct SomeStructForTypedefTesting1 { int x1; };
struct SomeStructForTypedefTesting2 { int x2; };
typedef
#ifdef ONE
SomeStructForTypedefTesting1
#else
SomeStructForTypedefTesting2
#endif
SameTypedefDifferentUnderlyingType;

// Same template but different default template arguments
#ifdef ONE
template<typename T = char>
#else
template<typename T = long>
#endif
struct SameTemplateDifferentDefaultTemplateArguments {};

#endif // cannot be seen by DIA or COFF



struct DifferentSizedMember
{
#ifdef ONE
    char a;
#else
    wchar_t* a;
#endif
};


// Multiple definitions of the same class / struct / union with different layout
// Different member lists, order, types, or base classes.
struct DifferentDataMembers
{
    int a;
#ifdef TWO
    int b;
#endif
};
struct DifferentOrderOfDataMembers
{
#ifdef ONE
    int a, b;
#else
    int b, a;
#endif
};
struct DifferentTypeOfDataMembers
{
#ifdef ONE
    signed a;
#else
    unsigned a;
#endif
};

struct Base1 {};
struct Base2 {};
struct DifferentBases :
#ifdef ONE
    Base1
#else
    Base2
#endif
{};

// Same class but different member types
struct DifferentDataMemberType
{
#ifdef ONE
    char a;
#else
    long a;
#endif
};

// Same class but different member order
struct SameMemberTypesDifferentOrder
{
#ifdef ONE
    int a; char b;
#else
    char b; int a;
#endif
};

// Same class but different base classes
#ifdef ONE
struct DifferentBaseClass : Base1
#else
struct DifferentBaseClass
#endif
{};

struct DifferentConstDataMember
{
#ifdef ONE
    const int a{};
#else
          int a{};
#endif
};
struct DifferentVolatileDataMember
{
#ifdef ONE
    volatile int a{};
#else
             int a{};
#endif
};

#ifdef ONE
struct StructVsClass
#else
class  StructVsClass
#endif
{};

// Same inline function but different bodies
// Inline functions must be bit‑for‑bit identical across TUs.
#ifdef ONE
inline int FunctionsMustBeBitwiseIdentical() { return 1; }
#else
inline int FunctionsMustBeBitwiseIdentical() { return 2; }
#endif


// Same template specialization but different definitions
template<typename T> inline T   SameFunctionTemplateSpecializationDifferentDefinitions();
template<          > inline int SameFunctionTemplateSpecializationDifferentDefinitions<int>() 
{
#ifdef ONE
    return 1;
#else
    return 2;
#endif
}


// Same enum but different enumerator values
enum SameEnumButDifferentValues
{
#ifdef ONE
    A = 1, B = 2
#else
    A = 1, B = 3
#endif
};

// Same class but different alignment
#pragma warning(push)
#pragma warning(disable: 4324) // really stupid warning
struct
#ifdef ONE
alignas(4)
#else
alignas(8)
#endif
SameClassDifferentAlignment { int a; };
#pragma warning(pop)


// Same class but different virtual function table shape
struct SameClassDifferentVirtualFunctionTableShape
{
#ifdef ONE
    virtual void f() {}
#endif
};

struct SameClassDifferentVirtualFunctionNames
{
#ifdef ONE
    virtual void Foo() {}
#else
    virtual void Bar() {}
#endif
};




#ifdef ALL_ODR_VIOLATIONS
    18. Same class but different final / override usage
    These change the virtual table.

    TU1.cpp
    cpp
    struct B { virtual void f(); };
    struct S : B { void f() override; };
    TU2.cpp
    cpp
    struct B { virtual void f(); };
    struct S : B { void f(); };   // ODR violation
    19. Same class but different member static vs non‑static
    TU1.cpp
    cpp
    struct S { int a; };
    TU2.cpp
    cpp
    struct S { static int a; };   // ODR violation
    20. Same class but different bitfield layout
    TU1.cpp
    cpp
    struct S { unsigned a : 3; unsigned b : 5; };
    TU2.cpp
    cpp
    struct S { unsigned a : 4; unsigned b : 4; };   // ODR violation
    21. Same class but different member order inside anonymous struct / union
    TU1.cpp
    cpp
    struct S { union { struct { int a; int b; }; int x; }; };
    TU2.cpp
    cpp
    struct S { union { struct { int b; int a; }; int x; }; };   // ODR violation
    22. Same class but different presence / absence of anonymous members
    TU1.cpp
    cpp
    struct S { union { int a; }; };
    TU2.cpp
    cpp
    struct S { union { int a; struct { int b; }; }; };   // ODR violation
    23. Same class but different friend declarations
    Yes, even this is an ODR violation.

    TU1.cpp
    cpp
    struct S { friend void f(); };
    TU2.cpp
    cpp
    struct S {};   // ODR violation

    25. Same class but different noexcept on member functions
    Affects type identity.

    TU1.cpp
    cpp
    struct S { void f() noexcept; };
    TU2.cpp
    cpp
    struct S { void f(); };   // ODR violation
#endif