#define ONE
#include "TestHeader.h"

DifferentSizedMember                              g_1_DifferentSizedMember;
DifferentDataMembers                              g_1_DifferentDataMembers;
DifferentOrderOfDataMembers                       g_1_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers                        g_1_DifferentTypeOfDataMembers;
DifferentBases                                    g_1_DifferentBases;
DifferentDataMemberType                           g_1_DifferentDataMemberType;
SameMemberTypesDifferentOrder                     g_1_SameMemberTypesDifferentOrder;
DifferentBaseClass                                g_1_DifferentBaseClass;
//SameClassDifferentAccessSpecifier               g_1_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer               g_1_DifferentDefaultMemberInitializer;
//DifferentConstexprValue                         g_1_DifferentConstexprValue;
//DifferentConstInit                              g_1_DifferentConstInit;
DifferentConstDataMember                          g_1_DifferentConstDataMember;
DifferentVolatileDataMember                       g_1_DifferentVolatileDataMember;
StructVsClass                                     g_1_StructVsClass;
//SameTemplateDifferentDefaultTemplateArguments<> g_1_SameTemplateDifferentDefaultTemplateArguments;
SameClassDifferentAlignment                       g_1_SameClassDifferentAlignment;
SameClassDifferentVirtualFunctionTableShape       g_1_SameClassDifferentVirtualFunctionTableShape;
SameClassDifferentVirtualFunctionNames            g_1_SameClassDifferentVirtualFunctionNames;
SameClassDifferentVirtualnessOnFunction           g_1_SameClassDifferentVirtualnessOnFunction;


int g_1_call_FunctionsMustBeBitwiseIdentical                        = FunctionsMustBeBitwiseIdentical();
int g_1_call_SameFunctionTemplateSpecializationDifferentDefinitions = SameFunctionTemplateSpecializationDifferentDefinitions<int>();
//int g_1_call_SameConstexprFunctionDifferentBody                   = SameConstexprFunctionDifferentBody();

auto g_1_enum = SameEnumButDifferentValues::A;

//SameTypedefDifferentUnderlyingType g_1_sameTypedefDifferentUnderlyingType{};
