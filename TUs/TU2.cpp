#define TWO
#include "TestHeader.h"

DifferentSizedMember                  g_2_DifferentSizedMember;
DifferentDataMembers                  g_2_DifferentDataMembers;
DifferentOrderOfDataMembers           g_2_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers            g_2_DifferentTypeOfDataMembers;
DifferentBases                        g_2_DifferentBases;
DifferentDataMemberType               g_2_DifferentDataMemberType;
SameMemberTypesDifferentOrder         g_2_SameMemberTypesDifferentOrder;
DifferentBaseClass                    g_2_DifferentBaseClass;
//SameClassDifferentAccessSpecifier   g_2_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer   g_2_DifferentDefaultMemberInitializer;
//DifferentConstexprValue             g_2_DifferentConstexprValue;
//DifferentConstInit                  g_2_DifferentConstInit;
DifferentConstDataMember              g_2_DifferentConstDataMember;
DifferentVolatileDataMember           g_2_DifferentVolatileDataMember;
StructVsClass                         g_2_StructVsClass;

int g_sink_2 = FunctionsMustBeBitwiseIdentical();
