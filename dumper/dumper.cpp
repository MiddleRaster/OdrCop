#include <iostream>
#include <filesystem>
#include <set>

#include <windows.h>
#include <dia2.h>
#include <atlbase.h>

#include "magic_enum.h"

template <typename C, typename R, typename T> std::pair<HRESULT, T> Get(IDiaSymbol* sym, R(C::* m)(T*))
{
    T value{};
    HRESULT hr = (sym->*m)(&value);
    return {hr, value};
}
template <typename R> std::pair<HRESULT,std::wstring> Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(BSTR*))
{
    CComBSTR value{};
    HRESULT hr = (sym->*m)(&value.m_str);
    if (hr == S_OK)
        return {hr, value.m_str};
    return {hr, L""};
}
template <typename R> std::pair<HRESULT,CComPtr<IDiaSymbol>> Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(IDiaSymbol*))
{
    CComPtr<IDiaSymbol> value{};
    HRESULT hr = (sym->*m)(&value);
    return {hr,value};
}
template <typename R> std::pair<HRESULT,std::wstring> Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(VARIANT*))
{
    CComVariant value{};
    HRESULT hr = (sym->*m)(&value);
    if ((hr != S_OK && hr != S_FALSE) || FAILED(value.ChangeType(VT_BSTR)) || !value.bstrVal)
        return {hr,L"?"};
    return {hr,std::wstring(value.bstrVal)};
}

template <typename C, typename R, typename T> void Print(const std::wstring& tab, IDiaSymbol* sym, R(C::* m)(T*), const std::wstring& propName)
{
    auto [hr, wstr] = Get(sym, m);
    if (hr == S_OK)
        std::wcout << tab << propName << L": " << wstr << L'\n';
}
template <typename E, typename C, typename R> void Print /*Enum*/ (const std::wstring& tab, IDiaSymbol* sym, R(C::* m)(DWORD*), const std::wstring& propName)
{
    auto [hr, dw] = Get(sym, m);
    if (hr == S_OK)
        std::wcout << tab << propName << L": " << enum_name<E>((E)dw) << L'\n';
}

void PrintUdtKind(const std::wstring& tab, IDiaSymbol* sym)
{ // N.B.: not using magic_enum here, as I want the output to be slightly different
    auto [hr, dw] = Get(sym, &IDiaSymbol::get_udtKind);
    if (hr == S_OK)
    {
        std::wcout << tab << L"udtKind: ";
        switch ((enum UdtKind)dw)
        {
        case UdtKind::UdtStruct:      std::wcout << L"struct";      break;
        case UdtKind::UdtClass:       std::wcout << L"class";       break;
        case UdtKind::UdtUnion:       std::wcout << L"union";       break;
        case UdtKind::UdtInterface:   std::wcout << L"interface";   break;
        case UdtKind::UdtTaggedUnion: std::wcout << L"taggedUnion"; break;
        default:                      std::wcout << L"unknown    "; break;
        };
        std::wcout << L'\n';
    }
}
void PrintAccess(const std::wstring& tab, IDiaSymbol* sym) // Print(tab, item, &IDiaSymbol::get_access, L"access");
{ // N.B.: not using magic_enum here, as I want the output to be slightly different
    auto [hr, dw] = Get(sym, &IDiaSymbol::get_access);
    if (hr == S_OK)
    {
        std::wcout << tab << L"access: ";
        switch ((enum CV_access_e)dw)
        {
        case CV_access_e::CV_private:   std::wcout << L"private";   break;
        case CV_access_e::CV_protected: std::wcout << L"protected"; break;
        case CV_access_e::CV_public:    std::wcout << L"public";    break;
        default:                        std::wcout << L"unknown";   break;
        };
        std::wcout << L'\n';
    }
}

void PrintAllProps(std::wstring tab, const std::wstring& itemName, IDiaSymbol* item, std::set<DWORD>& visited)
{
    std::wcout << tab << itemName << L":\n";
    tab += L"  ";

    // move an important few up top
    PrintUdtKind          (tab, item);
    Print<       DataKind>(tab, item, &IDiaSymbol::get_dataKind,                             L"dataKind");
    Print<enum SymTagEnum>(tab, item, &IDiaSymbol::get_symTag,                               L"symTag");
    PrintAccess           (tab, item);

    Print              (tab, item, &IDiaSymbol::get_name,                                    L"name");
    Print              (tab, item, &IDiaSymbol::get_symIndexId,                              L"symIndexId");
    Print              (tab, item, &IDiaSymbol::get_lexicalParent,                           L"lexicalParent");
    Print              (tab, item, &IDiaSymbol::get_classParent,                             L"classParent");
    Print              (tab, item, &IDiaSymbol::get_type,                                    L"type");
    Print<LocationType>(tab, item, &IDiaSymbol::get_locationType,                            L"locationType");
    Print              (tab, item, &IDiaSymbol::get_addressSection,                          L"addressSection");
    Print              (tab, item, &IDiaSymbol::get_addressOffset,                           L"addressOffset");
    Print              (tab, item, &IDiaSymbol::get_relativeVirtualAddress,                  L"relativeVirtualAddress");
    Print              (tab, item, &IDiaSymbol::get_virtualAddress,                          L"virtualAddress");
    Print              (tab, item, &IDiaSymbol::get_registerId,                              L"registerId");
    Print              (tab, item, &IDiaSymbol::get_offset,                                  L"offset");
    Print              (tab, item, &IDiaSymbol::get_length,                                  L"length");
    Print              (tab, item, &IDiaSymbol::get_slot,                                    L"slot");
    Print              (tab, item, &IDiaSymbol::get_volatileType,                            L"volatileType");
    Print              (tab, item, &IDiaSymbol::get_constType,                               L"constType");
    Print              (tab, item, &IDiaSymbol::get_unalignedType,                           L"unalignedType");
    Print              (tab, item, &IDiaSymbol::get_libraryName,                             L"libraryName");
    Print              (tab, item, &IDiaSymbol::get_platform,                                L"platform");
    Print<CV_CFL_LANG> (tab, item, &IDiaSymbol::get_language,                                L"language");
    Print              (tab, item, &IDiaSymbol::get_editAndContinueEnabled,                  L"editAndContinueEnabled");
    Print              (tab, item, &IDiaSymbol::get_frontEndMajor,                           L"frontEndMajor");
    Print              (tab, item, &IDiaSymbol::get_frontEndMinor,                           L"frontEndMinor");
    Print              (tab, item, &IDiaSymbol::get_frontEndBuild,                           L"frontEndBuild");
    Print              (tab, item, &IDiaSymbol::get_backEndMajor,                            L"backEndMajor");
    Print              (tab, item, &IDiaSymbol::get_backEndMinor,                            L"backEndMinor");
    Print              (tab, item, &IDiaSymbol::get_backEndBuild,                            L"backEndBuild");
    Print              (tab, item, &IDiaSymbol::get_sourceFileName,                          L"sourceFileName");
    Print              (tab, item, &IDiaSymbol::get_unused,                                  L"unused");
    Print              (tab, item, &IDiaSymbol::get_thunkOrdinal,                            L"thunkOrdinal");
    Print              (tab, item, &IDiaSymbol::get_thisAdjust,                              L"thisAdjust");
    Print              (tab, item, &IDiaSymbol::get_virtualBaseOffset,                       L"virtualBaseOffset");
    Print              (tab, item, &IDiaSymbol::get_virtual,                                 L"virtual");
    Print              (tab, item, &IDiaSymbol::get_intro,                                   L"intro");
    Print              (tab, item, &IDiaSymbol::get_pure,                                    L"pure");
    Print<CV_call_e>   (tab, item, &IDiaSymbol::get_callingConvention,                       L"callingConvention");
    Print              (tab, item, &IDiaSymbol::get_value,                                   L"value");
    Print<BasicType>   (tab, item, &IDiaSymbol::get_baseType,                                L"baseType");
    Print              (tab, item, &IDiaSymbol::get_token,                                   L"token");
    Print              (tab, item, &IDiaSymbol::get_timeStamp,                               L"timeStamp");
 // Print              (tab, item, &IDiaSymbol::get_guid,                                    L"guid"); 
    Print              (tab, item, &IDiaSymbol::get_symbolsFileName,                         L"symbolsFileName");
    Print              (tab, item, &IDiaSymbol::get_reference,                               L"reference");
    Print              (tab, item, &IDiaSymbol::get_count,                                   L"count");
    Print              (tab, item, &IDiaSymbol::get_bitPosition,                             L"bitPosition");
    Print              (tab, item, &IDiaSymbol::get_arrayIndexType,                          L"arrayIndexType");
    Print              (tab, item, &IDiaSymbol::get_packed,                                  L"packed");
    Print              (tab, item, &IDiaSymbol::get_constructor,                             L"constructor");
    Print              (tab, item, &IDiaSymbol::get_overloadedOperator,                      L"overloadedOperator");
    Print              (tab, item, &IDiaSymbol::get_nested,                                  L"nested");
    Print              (tab, item, &IDiaSymbol::get_hasNestedTypes,                          L"hasNestedTypes");
    Print              (tab, item, &IDiaSymbol::get_hasAssignmentOperator,                   L"hasAssignmentOperator");
    Print              (tab, item, &IDiaSymbol::get_hasCastOperator,                         L"hasCastOperator");
    Print              (tab, item, &IDiaSymbol::get_scoped,                                  L"scoped");
    Print              (tab, item, &IDiaSymbol::get_virtualBaseClass,                        L"virtualBaseClass");
    Print              (tab, item, &IDiaSymbol::get_indirectVirtualBaseClass,                L"indirectVirtualBaseClass");
    Print              (tab, item, &IDiaSymbol::get_virtualBasePointerOffset,                L"virtualBasePointerOffset");
    Print              (tab, item, &IDiaSymbol::get_virtualTableShape,                       L"virtualTableShape");
    Print              (tab, item, &IDiaSymbol::get_lexicalParentId,                         L"lexicalParentId");
    Print              (tab, item, &IDiaSymbol::get_classParentId,                           L"classParentId");
    Print              (tab, item, &IDiaSymbol::get_typeId,                                  L"typeId");
    Print              (tab, item, &IDiaSymbol::get_arrayIndexTypeId,                        L"arrayIndexTypeId");
    Print              (tab, item, &IDiaSymbol::get_virtualTableShapeId,                     L"virtualTableShapeId");
    Print              (tab, item, &IDiaSymbol::get_code,                                    L"code");
    Print              (tab, item, &IDiaSymbol::get_function,                                L"function");
    Print              (tab, item, &IDiaSymbol::get_managed,                                 L"managed");
    Print              (tab, item, &IDiaSymbol::get_msil,                                    L"msil");
    Print              (tab, item, &IDiaSymbol::get_virtualBaseDispIndex,                    L"virtualBaseDispIndex");
    Print              (tab, item, &IDiaSymbol::get_undecoratedName,                         L"undecoratedName");
    Print              (tab, item, &IDiaSymbol::get_age,                                     L"age");
    Print              (tab, item, &IDiaSymbol::get_signature,                               L"signature");
    Print              (tab, item, &IDiaSymbol::get_compilerGenerated,                       L"compilerGenerated");
    Print              (tab, item, &IDiaSymbol::get_addressTaken,                            L"addressTaken");
    Print              (tab, item, &IDiaSymbol::get_rank,                                    L"rank");
    Print              (tab, item, &IDiaSymbol::get_lowerBound,                              L"lowerBound");
    Print              (tab, item, &IDiaSymbol::get_upperBound,                              L"upperBound");
    Print              (tab, item, &IDiaSymbol::get_lowerBoundId,                            L"lowerBoundId");
    Print              (tab, item, &IDiaSymbol::get_upperBoundId,                            L"upperBoundId");
    Print              (tab, item, &IDiaSymbol::get_targetSection,                           L"targetSection");
    Print              (tab, item, &IDiaSymbol::get_targetOffset,                            L"targetOffset");
    Print              (tab, item, &IDiaSymbol::get_targetRelativeVirtualAddress,            L"targetRelativeVirtualAddress");
    Print              (tab, item, &IDiaSymbol::get_targetVirtualAddress,                    L"targetVirtualAddress");
    Print              (tab, item, &IDiaSymbol::get_machineType,                             L"machineType");
    Print              (tab, item, &IDiaSymbol::get_oemId,                                   L"oemId");
    Print              (tab, item, &IDiaSymbol::get_oemSymbolId,                             L"oemSymbolId");
    Print              (tab, item, &IDiaSymbol::get_objectPointerType,                       L"get_objectPointerType");
    Print              (tab, item, &IDiaSymbol::get_noReturn,                                L"noReturn");
    Print              (tab, item, &IDiaSymbol::get_customCallingConvention,                 L"customCallingConvention");
    Print              (tab, item, &IDiaSymbol::get_noInline,                                L"noInline");
    Print              (tab, item, &IDiaSymbol::get_optimizedCodeDebugInfo,                  L"optimizedCodeDebugInfo");
    Print              (tab, item, &IDiaSymbol::get_notReached,                              L"notReached");
    Print              (tab, item, &IDiaSymbol::get_interruptReturn,                         L"interruptReturn");
    Print              (tab, item, &IDiaSymbol::get_farReturn,                               L"farReturn");
    Print              (tab, item, &IDiaSymbol::get_isStatic,                                L"isStatic");
    Print              (tab, item, &IDiaSymbol::get_hasDebugInfo,                            L"hasDebugInfo");
    Print              (tab, item, &IDiaSymbol::get_isLTCG,                                  L"isLTCG");
    Print              (tab, item, &IDiaSymbol::get_isDataAligned,                           L"isDataAligned");
    Print              (tab, item, &IDiaSymbol::get_hasSecurityChecks,                       L"hasSecurityChecks");
    Print              (tab, item, &IDiaSymbol::get_compilerName,                            L"compilerName");
    Print              (tab, item, &IDiaSymbol::get_hasAlloca,                               L"hasAlloca");
    Print              (tab, item, &IDiaSymbol::get_hasSetJump,                              L"hasSetJump");
    Print              (tab, item, &IDiaSymbol::get_hasLongJump,                             L"hasLongJump");
    Print              (tab, item, &IDiaSymbol::get_hasInlAsm,                               L"hasInlAsm");
    Print              (tab, item, &IDiaSymbol::get_hasEH,                                   L"hasEH");
    Print              (tab, item, &IDiaSymbol::get_hasSEH,                                  L"hasSEH");
    Print              (tab, item, &IDiaSymbol::get_hasEHa,                                  L"hasEHa");
    Print              (tab, item, &IDiaSymbol::get_isNaked,                                 L"isNaked");
    Print              (tab, item, &IDiaSymbol::get_isAggregated,                            L"isAggregated");
    Print              (tab, item, &IDiaSymbol::get_isSplitted,                              L"isSplitted");
    Print              (tab, item, &IDiaSymbol::get_container,                               L"container");
    Print              (tab, item, &IDiaSymbol::get_inlSpec,                                 L"inlSpec");
    Print              (tab, item, &IDiaSymbol::get_noStackOrdering,                         L"noStackOrdering");
    Print              (tab, item, &IDiaSymbol::get_virtualBaseTableType,                    L"virtualBaseTableType");
    Print              (tab, item, &IDiaSymbol::get_hasManagedCode,                          L"hasManagedCode");
    Print              (tab, item, &IDiaSymbol::get_isHotpatchable,                          L"isHotpatchable");
    Print              (tab, item, &IDiaSymbol::get_isCVTCIL,                                L"isCVTCIL");
    Print              (tab, item, &IDiaSymbol::get_isMSILNetmodule,                         L"isMSILNetmodule");
    Print              (tab, item, &IDiaSymbol::get_isCTypes,                                L"isCTypes");
    Print              (tab, item, &IDiaSymbol::get_isStripped,                              L"isStripped");
    Print              (tab, item, &IDiaSymbol::get_frontEndQFE,                             L"frontEndQFE");
    Print              (tab, item, &IDiaSymbol::get_backEndQFE,                              L"backEndQFE");
    Print              (tab, item, &IDiaSymbol::get_wasInlined,                              L"wasInlined");
    Print              (tab, item, &IDiaSymbol::get_strictGSCheck,                           L"strictGSCheck");
    Print              (tab, item, &IDiaSymbol::get_isCxxReturnUdt,                          L"isCxxReturnUdt");
    Print              (tab, item, &IDiaSymbol::get_isConstructorVirtualBase,                L"isConstructorVirtualBase");
    Print              (tab, item, &IDiaSymbol::get_RValueReference,                         L"RValueReference");
    Print              (tab, item, &IDiaSymbol::get_unmodifiedType,                          L"unmodifiedType");
    Print              (tab, item, &IDiaSymbol::get_framePointerPresent,                     L"framePointerPresent");
    Print              (tab, item, &IDiaSymbol::get_isSafeBuffers,                           L"isSafeBuffers");
    Print              (tab, item, &IDiaSymbol::get_intrinsic,                               L"intrinsic");
    Print              (tab, item, &IDiaSymbol::get_sealed,                                  L"sealed");
    Print              (tab, item, &IDiaSymbol::get_hfaFloat,                                L"hfaFloat");
    Print              (tab, item, &IDiaSymbol::get_hfaDouble,                               L"hfaDouble");
    Print              (tab, item, &IDiaSymbol::get_liveRangeStartAddressSection,            L"liveRangeStartAddressSection");
    Print              (tab, item, &IDiaSymbol::get_liveRangeStartAddressOffset,             L"liveRangeStartAddressOffset");
    Print              (tab, item, &IDiaSymbol::get_liveRangeStartRelativeVirtualAddress,    L"liveRangeStartRelativeVirtualAddress");
    Print              (tab, item, &IDiaSymbol::get_countLiveRanges,                         L"countLiveRanges");
    Print              (tab, item, &IDiaSymbol::get_liveRangeLength,                         L"liveRangeLength");
    Print              (tab, item, &IDiaSymbol::get_offsetInUdt,                             L"offsetInUdt");
    Print              (tab, item, &IDiaSymbol::get_paramBasePointerRegisterId,              L"paramBasePointerRegisterId");
    Print              (tab, item, &IDiaSymbol::get_localBasePointerRegisterId,              L"localBasePointerRegisterId");
    Print              (tab, item, &IDiaSymbol::get_isLocationControlFlowDependent,          L"isLocationControlFlowDependent");
    Print              (tab, item, &IDiaSymbol::get_stride,                                  L"stride");
    Print              (tab, item, &IDiaSymbol::get_numberOfRows,                            L"numberOfRows");
    Print              (tab, item, &IDiaSymbol::get_numberOfColumns,                         L"numberOfColumns");
    Print              (tab, item, &IDiaSymbol::get_isMatrixRowMajor,                        L"isMatrixRowMajor");
    Print              (tab, item, &IDiaSymbol::get_isReturnValue,                           L"isReturnValue");
    Print              (tab, item, &IDiaSymbol::get_isOptimizedAway,                         L"isOptimizedAway");
    Print              (tab, item, &IDiaSymbol::get_builtInKind,                             L"builtInKind");
    Print              (tab, item, &IDiaSymbol::get_registerType,                            L"registerType");
    Print              (tab, item, &IDiaSymbol::get_baseDataSlot,                            L"baseDataSlot");
    Print              (tab, item, &IDiaSymbol::get_baseDataOffset,                          L"baseDataOffset");
    Print              (tab, item, &IDiaSymbol::get_textureSlot,                             L"textureSlot");
    Print              (tab, item, &IDiaSymbol::get_samplerSlot,                             L"samplerSlot");
    Print              (tab, item, &IDiaSymbol::get_uavSlot,                                 L"uavSlot");
    Print              (tab, item, &IDiaSymbol::get_sizeInUdt,                               L"sizeInUdt");
    Print              (tab, item, &IDiaSymbol::get_memorySpaceKind,                         L"memorySpaceKind");
    Print              (tab, item, &IDiaSymbol::get_unmodifiedTypeId,                        L"unmodifiedTypeId");
    Print              (tab, item, &IDiaSymbol::get_subTypeId,                               L"subTypeId");
    Print              (tab, item, &IDiaSymbol::get_subType,                                 L"subType");
    Print              (tab, item, &IDiaSymbol::get_numberOfModifiers,                       L"numberOfModifiers");
    Print              (tab, item, &IDiaSymbol::get_numberOfRegisterIndices,                 L"numberOfRegisterIndices");
    Print              (tab, item, &IDiaSymbol::get_isHLSLData,                              L"isHLSLData");
    Print              (tab, item, &IDiaSymbol::get_isPointerToDataMember,                   L"isPointerToDataMember");
    Print              (tab, item, &IDiaSymbol::get_isPointerToMemberFunction,               L"isPointerToMemberFunction");
    Print              (tab, item, &IDiaSymbol::get_isSingleInheritance,                     L"isSingleInheritance");
    Print              (tab, item, &IDiaSymbol::get_isMultipleInheritance,                   L"isMultipleInheritance");
    Print              (tab, item, &IDiaSymbol::get_isVirtualInheritance,                    L"isVirtualInheritance");
    Print              (tab, item, &IDiaSymbol::get_restrictedType,                          L"restrictedType");
    Print              (tab, item, &IDiaSymbol::get_isPointerBasedOnSymbolValue,             L"isPointerBasedOnSymbolValue");
    Print              (tab, item, &IDiaSymbol::get_baseSymbol,                              L"baseSymbol");
    Print              (tab, item, &IDiaSymbol::get_baseSymbolId,                            L"baseSymbolId");
    Print              (tab, item, &IDiaSymbol::get_objectFileName,                          L"objectFileName");
    Print              (tab, item, &IDiaSymbol::get_isAcceleratorGroupSharedLocal,           L"isAcceleratorGroupSharedLocal");
    Print              (tab, item, &IDiaSymbol::get_isAcceleratorPointerTagLiveRange,        L"isAcceleratorPointerTagLiveRange");
    Print              (tab, item, &IDiaSymbol::get_isAcceleratorStubFunction,               L"isAcceleratorStubFunction");
    Print              (tab, item, &IDiaSymbol::get_numberOfAcceleratorPointerTags,          L"numberOfAcceleratorPointerTags");
    Print              (tab, item, &IDiaSymbol::get_isSdl,                                   L"isSdl");
    Print              (tab, item, &IDiaSymbol::get_isWinRTPointer,                          L"isWinRTPointer");
    Print              (tab, item, &IDiaSymbol::get_isRefUdt,                                L"isRefUdt");
    Print              (tab, item, &IDiaSymbol::get_isValueUdt,                              L"isValueUdt");
    Print              (tab, item, &IDiaSymbol::get_isInterfaceUdt,                          L"isInterfaceUdt");
    Print              (tab, item, &IDiaSymbol::findInlineeLines,                            L"InlineeLines");
    Print              (tab, item, &IDiaSymbol::getSrcLineOnTypeDefn,                        L"rcLineOnTypeDefn");
    Print              (tab, item, &IDiaSymbol::get_isPGO,                                   L"isPGO");
    Print              (tab, item, &IDiaSymbol::get_hasValidPGOCounts,                       L"hasValidPGOCounts");
    Print              (tab, item, &IDiaSymbol::get_isOptimizedForSpeed,                     L"isOptimizedForSpeed");
    Print              (tab, item, &IDiaSymbol::get_PGOEntryCount,                           L"PGOEntryCount");
    Print              (tab, item, &IDiaSymbol::get_PGOEdgeCount,                            L"PGOEdgeCount");
    Print              (tab, item, &IDiaSymbol::get_PGODynamicInstructionCount,              L"PGODynamicInstructionCount");
    Print              (tab, item, &IDiaSymbol::get_staticSize,                              L"staticSize");
    Print              (tab, item, &IDiaSymbol::get_finalLiveStaticSize,                     L"finalLiveStaticSize");
    Print              (tab, item, &IDiaSymbol::get_phaseName,                               L"phaseName");
    Print              (tab, item, &IDiaSymbol::get_hasControlFlowCheck,                     L"hasControlFlowCheck");
    Print              (tab, item, &IDiaSymbol::get_constantExport,                          L"constantExport");
    Print              (tab, item, &IDiaSymbol::get_dataExport,                              L"dataExport");
    Print              (tab, item, &IDiaSymbol::get_privateExport,                           L"privateExport");
    Print              (tab, item, &IDiaSymbol::get_noNameExport,                            L"noNameExport");
    Print              (tab, item, &IDiaSymbol::get_exportHasExplicitlyAssignedOrdinal,      L"exportHasExplicitlyAssignedOrdinal");
    Print              (tab, item, &IDiaSymbol::get_exportIsForwarder,                       L"exportIsForwarder");
    Print              (tab, item, &IDiaSymbol::get_ordinal,                                 L"ordinal");
    Print              (tab, item, &IDiaSymbol::get_frameSize,                               L"frameSize");
    Print              (tab, item, &IDiaSymbol::get_exceptionHandlerAddressSection,          L"exceptionHandlerAddressSection");
    Print              (tab, item, &IDiaSymbol::get_exceptionHandlerAddressOffset,           L"exceptionHandlerAddressOffset");
    Print              (tab, item, &IDiaSymbol::get_exceptionHandlerRelativeVirtualAddress,  L"exceptionHandlerRelativeVirtualAddress");
    Print              (tab, item, &IDiaSymbol::get_exceptionHandlerVirtualAddress,          L"exceptionHandlerVirtualAddress");
    Print              (tab, item, &IDiaSymbol::findInputAssemblyFile,                       L"InputAssemblyFile");
    Print              (tab, item, &IDiaSymbol::get_characteristics,                         L"characteristics");
    Print              (tab, item, &IDiaSymbol::get_coffGroup,                               L"coffGroup");
    Print              (tab, item, &IDiaSymbol::get_bindID,                                  L"bindID");
    Print              (tab, item, &IDiaSymbol::get_bindSpace,                               L"bindSpace");
    Print              (tab, item, &IDiaSymbol::get_bindSlot,                                L"bindSlot");

#ifdef KEEP
    IDiaSymbol interface
        virtual HRESULT STDMETHODCALLTYPE get_types(
            /* [in] */ DWORD cTypes,
            /* [out] */ DWORD * pcTypes,
            /* [size_is][size_is][out] */ IDiaSymbol * *pTypes) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_undecoratedNameEx(
        /* [in] */ DWORD undecorateOptions,
        /* [out] */ BSTR * name) = 0;
#endif
}

void PrintPropsAndRecurse(std::wstring tab, const std::wstring& itemName, IDiaSymbol* item, std::set<DWORD>& visited)
{
    auto [_, symIndexId] = Get(item, &IDiaSymbol::get_symIndexId);
    if (!visited.insert(symIndexId).second)
        return; // seen already, don't recurse

    PrintAllProps(tab, itemName, item, visited);

    // now get all children and recurse on each
    tab += L"  ";
    HRESULT hr;
    CComPtr<IDiaEnumSymbols> children;
    if (S_OK == (hr = item->findChildren(SymTagNull, NULL, nsNone, &children)) && (children != nullptr))
    {
        while(true)
        {
            ULONG fetched = 0;
            CComPtr<IDiaSymbol> child;
            if (FAILED(children->Next(1, &child, &fetched)) || fetched == 0)
                break;

            CComBSTR name;
            if (S_FALSE == child->get_name(&name) || !name || name[0] == L'\0')
                name = L"unnamed item";

            PrintPropsAndRecurse(tab, name.m_str, child, visited);
        }
    }
}

template<typename Recurse> void OutputSpecificItem(IDiaSymbol* child, const wchar_t* desiredItem, Recurse recurse)
{
    CComBSTR name;
    if ((S_OK == child->get_name(&name)) && name && name[0] != L'\0')
    {
        if (std::wstring(desiredItem) == name.m_str)
        {
            // found it!
            std::set<DWORD> visited;
            recurse(L"", desiredItem, child, visited);
        }
    }
}

template<typename Recurse> void OutputEvenUnnamed(IDiaSymbol* child, const wchar_t* desiredItem, Recurse recurse)
{
    CComBSTR name;
    if (S_OK != child->get_name(&name))
        name = L"unnamed item";

    std::set<DWORD> visited;
    recurse(L"", name.m_str, child, visited);
}

template<typename DoIt, typename Recurse> HRESULT ForEachSymbol(const std::filesystem::path& path, const wchar_t* desiredItem, DoIt doIt, Recurse recurse)
{
    CoInitialize(nullptr);

    HRESULT hr;
    {
        CComPtr<IDiaDataSource> source;
        if (SUCCEEDED(hr = CoCreateInstance(__uuidof(DiaSource), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), reinterpret_cast<void**>(&source))))
        {
            if (SUCCEEDED(hr = source->loadDataFromPdb(path.c_str())))
            {
                CComPtr<IDiaSession> session;
                if (SUCCEEDED(hr = source->openSession(&session)))
                {
                    CComPtr<IDiaSymbol> global;
                    if (SUCCEEDED(hr = session->get_globalScope(&global)))
                    {
                        CComPtr<IDiaEnumSymbols> children;
                        if (SUCCEEDED(hr = global->findChildren(SymTagNull, NULL, nsNone, &children)) && (children != nullptr))
                        {
                            while(true)
                            {
                                ULONG fetched = 0;
                                CComPtr<IDiaSymbol> child;
                                if (FAILED(children->Next(1, &child, &fetched)) || fetched == 0)
                                    break;

                                doIt(child, desiredItem, recurse);
                            }
                        }

                    } else std::wcerr <<                  L"get_globalScope failed with 0x" << std::hex << hr << std::dec << L'\n';
                }     else std::wcerr <<                      L"openSession failed with 0x" << std::hex << hr << std::dec << L'\n';
            }         else std::wcerr << L"loadDataFromPdb failed: " << path << L" with 0x" << std::hex << hr << std::dec << L'\n';
        }             else std::wcerr <<  L"CoCreateInstance(DiaSource) failed for with 0x" << std::hex << hr << std::dec << L'\n';
    }
    CoUninitialize();
    return hr;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcout << L"Usage: DiaDump <path to a .pdb file> - will dump all types by name in the .pdb\n";
        std::wcout << L"Usage: DiaDump <path to a .pdb file> some_type_name - will dump all properties and child properties, recursively\n";
        return -1;
    }

    std::filesystem::path root = argv[1];

    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        std::wcerr << L"Path not found: " << root.wstring() << L'\n';
        return -1;
    }

    if (argc == 2) {   // dump all IDiaSymbol names in .pdb file
        std::wcout << L"Dumping all types in " << root << L'\n';
        ForEachSymbol(root, nullptr, OutputEvenUnnamed<decltype(PrintPropsAndRecurse)*>,  PrintPropsAndRecurse);
    } else {
        std::wcout << L"Dumping " << argv[2] << L" and sub-elements in " << root << L'\n';
        ForEachSymbol(root, argv[2], OutputSpecificItem<decltype(PrintPropsAndRecurse)*>, PrintPropsAndRecurse);
    }

    return 0;
}

