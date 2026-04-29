#define ONE
#include "TestHeader.h"

DifferentSizedMember                  g_1_DifferentSizedMember;
DifferentDataMembers                  g_1_DifferentDataMembers;
DifferentOrderOfDataMembers           g_1_DifferentOrderOfDataMembers;
DifferentTypeOfDataMembers            g_1_DifferentTypeOfDataMembers;
DifferentBases                        g_1_DifferentBases;
DifferentDataMemberType               g_1_DifferentDataMemberType;
SameMemberTypesDifferentOrder         g_1_SameMemberTypesDifferentOrder;
DifferentBaseClass                    g_1_DifferentBaseClass;
//SameClassDifferentAccessSpecifier   g_1_SameClassDifferentAccessSpecifier;
//DifferentDefaultMemberInitializer   g_1_DifferentDefaultMemberInitializer;
//DifferentConstexprValue             g_1_DifferentConstexprValue;
//DifferentConstInit                  g_1_DifferentConstInit;
DifferentConstDataMember              g_1_DifferentConstDataMember;
DifferentVolatileDataMember           g_1_DifferentVolatileDataMember;
StructVsClass                         g_1_StructVsClass;

void ForceEmit_1()
{
    volatile int sink_1 = FunctionsMustBeBitwiseIdentical();
    (void)sink_1;
}