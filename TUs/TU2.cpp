#define TWO
#include "TestHeader.h"

DifferentSizedMember                              g_2_DifferentSizedMember;
DifferentDataMembers                              g_2_DifferentDataMembers;
DifferentOrderOfDataMembers                       g_2_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers                        g_2_DifferentTypeOfDataMembers;
DifferentBases                                    g_2_DifferentBases;
DifferentDataMemberType                           g_2_DifferentDataMemberType;
SameMemberTypesDifferentOrder                     g_2_SameMemberTypesDifferentOrder;
DifferentBaseClass                                g_2_DifferentBaseClass;
//SameClassDifferentAccessSpecifier               g_2_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer               g_2_DifferentDefaultMemberInitializer;
//DifferentConstexprValue                         g_2_DifferentConstexprValue;
//DifferentConstInit                              g_2_DifferentConstInit;
DifferentConstDataMember                          g_2_DifferentConstDataMember;
DifferentVolatileDataMember                       g_2_DifferentVolatileDataMember;
StructVsClass                                     g_2_StructVsClass;
//SameTemplateDifferentDefaultTemplateArguments<> g_2_SameTemplateDifferentDefaultTemplateArguments;
SameClassDifferentAlignment                       g_2_SameClassDifferentAlignment;
SameClassDifferentVirtualFunctionTableShape       g_2_SameClassDifferentVirtualFunctionTableShape;
SameClassDifferentVirtualFunctionNames            g_2_SameClassDifferentVirtualFunctionNames;
SameClassDifferentVirtualnessOnFunction           g_2_SameClassDifferentVirtualnessOnFunction;
//SameClassDifferentNoExceptOnMethod              g_2_SameClassDifferentNoExceptOnMethod;
//SameClassDifferentInlinenessOnFunction          g_2_SameClassDifferentInlinenessOnFunction;
//SameClassDifferentConstexpressOnFunction        g_2_SameClassDifferentConstexpressOnFunction;
//SameClassDifferentOverrideSpecifier             g_2_SameClassDifferentOverrideSpecifier;


int g_2_call_FunctionsMustBeBitwiseIdentical                        = FunctionsMustBeBitwiseIdentical();
int g_2_call_SameFunctionTemplateSpecializationDifferentDefinitions = SameFunctionTemplateSpecializationDifferentDefinitions<int>();
//int g_2_call_SameConstexprFunctionDifferentBody                   = SameConstexprFunctionDifferentBody();

auto g_2_enum = SameEnumButDifferentValues::A;

//SameTypedefDifferentUnderlyingType g_2_sameTypedefDifferentUnderlyingType{};
