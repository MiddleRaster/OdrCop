// Some things are not recorded in the PDB debug info, making some ODR violations undetecable by DIA.
// Here are some known undetectable ODR violations:
//   - Access specifier differences (public/private/protected)
//   - Default argument differences
//   - static constexpr / static const member value differences
//   - static constexpr / consteval / constinit
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

#endif



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

// let's try two of our own
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

#ifdef ALL_ODR_VIOLATIONS
    10. Same inline function but different bodies
    Inline functions must be bit‑for‑bit identical across TUs.

    TU1.cpp
    cpp
    inline int f() { return 1; }
    TU2.cpp
    cpp
    inline int f() { return 2; }   // ODR violation
    11. Same template specialization but different definitions
    TU1.cpp
    cpp
    template<> int f<int>() { return 1; }
    TU2.cpp
    cpp
    template<> int f<int>() { return 2; }   // ODR violation
    12. Same constexpr function but different bodies
    TU1.cpp
    cpp
    constexpr int f() { return 1; }
    TU2.cpp
    cpp
    constexpr int f() { return 2; }   // ODR violation
    13. Same enum but different enumerator values
    TU1.cpp
    cpp
    enum E { A = 1, B = 2 };
    TU2.cpp
    cpp
    enum E { A = 1, B = 3 };   // ODR violation
    14. Same typedef or using but different underlying type
    TU1.cpp
    cpp
    typedef int T;
    TU2.cpp
    cpp
    typedef long T;   // ODR violation
    15. Same template but different default template arguments
    TU1.cpp
    cpp
    template<typename T = int>
    struct S {};
    TU2.cpp
    cpp
    template<typename T = long>
    struct S {};   // ODR violation
    16. Same class but different alignment
    TU1.cpp
    cpp
    struct alignas(4) S { int a; };
    TU2.cpp
    cpp
    struct alignas(8) S { int a; };   // ODR violation
    17. Same class but different virtual function table shape
    TU1.cpp
    cpp
    struct S { virtual void f(); };
    TU2.cpp
    cpp
    struct S {};   // ODR violation
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