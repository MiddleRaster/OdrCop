// Some things are not recorded in the PDB debug info, making some ODR violations undetecable by DIA.
// Here are some known undetectable ODR violations:
//   - Default argument differences
//   - static constexpr / static const member value differences
//   - static constexpr / consteval / constinit
//   - default template arguments
//   - constexpr / inline differences on member functions
//   - Attribute differences (__declspec etc.)
//   - override specifier
//   - friend specifier

#ifdef KNOWN_LIMITATIONS_OF_PDB_DIA

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

    struct DataMemberIsStaticConstOrStaticConstexpr
    {
    #ifdef ONE
        static const
    #else
        static constexpr
    #endif
            int a = 1;
    };

    // Same template but different default template arguments
    #ifdef ONE
    template<typename T = char>
    #else
    template<typename T = long>
    #endif
    struct SameTemplateDifferentDefaultTemplateArguments
    {
        T value;
    };

    struct SameClassDifferentInlinenessOnFunction
    {
    #ifdef ONE
                     inline  void InlineOrNot() {}
    #else
        __declspec(noinline) void InlineOrNot() {}
    #endif
    };

    struct SameClassDifferentConstexpressOnFunction
    {
    #ifdef ONE
        constexpr void ConstexpreOrNot() {}
    #else
                  void ConstexpreOrNot() {}
    #endif
    };

    // Same class but different final / override usage
    // These change the virtual table.
    struct BaseForOverride { virtual void OverrideOrNot() {} };
    struct SameClassDifferentOverrideSpecifier : BaseForOverride
    {
        void OverrideOrNot()
    #ifdef ONE
        override
    #endif
        {}
    };

#endif // cannot be seen by DIA or COFF


// Same class but different noexcept on member functions
struct SameClassDifferentNoExceptOnMethod
{
    void NoExceptMethod()
#ifdef ONE
        noexcept
#endif
    {}
};

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

struct StructContainingPointerToDifferentTypes
{
#ifdef ONE
    char
#else
    DifferentTypeOfDataMembers
#endif
    * ptr;
};

// Same class but different data-member access specifiers
struct SameClassDifferentDataMemberAccessSpecifier
{
#ifdef ONE
public:
#else
private:
#endif
    int a;
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

struct BaseClassesInDifferentOrder
#ifdef ONE
    : Base1, Base2
#else
    : Base2, Base1
#endif
{};

struct BaseClassVirtualOrNot :
#ifdef ONE
virtual
#endif
Base1
{};

struct DifferentAccessSpecifiersOnBaseClass :
#ifdef ONE
    public
#else
    private
#endif
Base1
{};

struct DifferentAccessSpecifiersOnMethod
{
#ifdef ONE
public:
#else
private:
#endif
    void Foo() {}
};


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

struct SameClassDifferentVirtualnessOnFunction
{
#ifdef ONE
    virtual void VirtualOrNot() {}
#else
            void VirtualOrNot() {}
#endif
};

struct StaticFunctionOrMethod
{
#ifdef ONE
    static
#endif
        void Foo() {}
};

// Same class but different member static vs non‑static
struct SameClassDifferentStaticOnDataMember
{
#ifdef ONE
    static
#endif
    int a;
};
struct SameClassDifferentStaticConstOnDataMember
{
#ifdef ONE
    static const
#else
    static
#endif
    int a;
};

struct SameClassDifferentStaticVolatileOnDataMember
{
#ifdef ONE
    static volatile
#else
    static
#endif
    int a;
};


// Same class but different bitfield layout
struct SameClassDifferentBitfieldLayout
{
#ifdef ONE
    unsigned a : 3; unsigned b : 5; 
#else
    unsigned a : 4; unsigned b : 4;
#endif
};

#pragma warning(push)
#pragma warning(disable: 4201)
// Same class but different member order inside anonymous struct / union
struct SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion
{
    union
    {
        struct
        {
#ifdef ONE
            int a; int b;
#else
            int b; int a;
#endif
        };
        int x;
    };
};
// Same class but different presence / absence of anonymous members
struct SameClassDifferentPresenceOfAnonymousMembers
{
    union
    { 
        int a;
#ifdef ONE
        struct
        {
            int b;
        };
#endif
    };
};
#pragma warning(pop)


//Same typedef or using but different underlying type
struct SomeStructForTypedefTesting1 { int x1; };
struct SomeStructForTypedefTesting2 { int x2; };
struct EnclosingTypedefDefinition
{
    typedef
#ifdef ONE
    SomeStructForTypedefTesting1
#else
    SomeStructForTypedefTesting2
#endif
    SameTypedefDifferentUnderlyingType;
};


template<typename Outer>
struct NamelessEnum
{
    template<typename T>
    struct Inner
    {
        enum { value = sizeof(T) == sizeof(char) ? 1 : 0 };
    };
};







inline void FunctionUsing_DifferentDataMembers(DifferentDataMembers) {}

enum SameEnumNameDifferentEnumerators
{
#ifdef ONE
    sendeA
#else
    sendeA, sendeB
#endif
};
inline void FunctionUsing_SameEnumNameDifferentEnumerators(SameEnumNameDifferentEnumerators) {}

template<class T> struct SameTemplateDifferentDefinition
{
#ifdef ONE
    int a;
#else
    int a;  int b;
#endif
};
inline void FunctionUsing_SameTemplateDifferentDefinition(SameTemplateDifferentDefinition<int>) {}

//#ifdef ONE
//inline int InlinedBody() { return 1; }
//#else
//inline int InlinedBody() { return 2; }
//#endif
//inline void FunctionUsing_InlinedBody() { InlinedBody(); }

union SameUnionNameDifferentElements
{
#ifdef ONE
    int i; float f; 
#else
    int i; float f; double d;
#endif
};
inline void FunctionUsing_SameUnionNameDifferentElements(SameUnionNameDifferentElements) {}


struct StaticMethodsBodiesDiffer
{
    static int Foo()
    {
#ifdef ONE
        return 1;
#else
        return 2;
#endif
    }
};

// a friend function vs. non-friend method
struct ClassForFriend
{
#ifdef ONE
    friend
#endif
        int TheFriendFunction(ClassForFriend) { return 2; }
};

inline int g_aGlobal1 = 1;
inline int g_aGlobal2 = 2;
struct MakeSureRelocs
{
    int HaveBeenAppliedToBodies()
    {
#ifdef ONE
        return g_aGlobal1;
#else
        return g_aGlobal2;
#endif
    }
};
