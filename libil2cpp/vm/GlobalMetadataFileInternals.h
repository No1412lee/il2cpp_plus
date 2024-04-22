#pragma once

#include "il2cpp-metadata.h"

// This file contains the structures specifying how we store converted metadata.
// These structures have 3 constraints:
// 1. These structures will be stored in an external file, and as such must not contain any pointers.
//    All references to other metadata should occur via an index into a corresponding table.
// 2. These structures are assumed to be const. Either const structures in the binary or mapped as
//    readonly memory from an external file. Do not add any 'calculated' fields which will be written to at runtime.
// 3. These structures should be optimized for size. Other structures are used at runtime which can
//    be larger to store cached information

// Encoded index (1 bit)
// MethodDef - 0
// MethodSpec - 1
// We use the top 3 bits to indicate what table to index into
// Type              Binary            Hex
// Il2CppClass       001               0x20000000
// Il2CppType        010               0x40000000
// MethodInfo        011               0x60000000
// FieldInfo         100               0x80000000
// StringLiteral     101               0xA0000000
// MethodRef         110               0xC0000000
// FieldRVA          111               0xE0000000

typedef uint32_t EncodedMethodIndex;

enum Il2CppMetadataUsage
{
    kIl2CppMetadataUsageInvalid,
    kIl2CppMetadataUsageTypeInfo,
    kIl2CppMetadataUsageIl2CppType,
    kIl2CppMetadataUsageMethodDef,
    kIl2CppMetadataUsageFieldInfo,
    kIl2CppMetadataUsageStringLiteral,
    kIl2CppMetadataUsageMethodRef,
    kIl2CppMetadataUsageFieldRva
};

enum Il2CppInvalidMetadataUsageToken
{
    kIl2CppInvalidMetadataUsageNoData = 0,
    kIl2CppInvalidMetadataUsageAmbiguousMethod = 1,
};

#ifdef __cplusplus
static inline Il2CppMetadataUsage GetEncodedIndexType(EncodedMethodIndex index)
{
    return (Il2CppMetadataUsage)((index & 0xE0000000) >> 29);
}

static inline uint32_t GetDecodedMethodIndex(EncodedMethodIndex index)
{
    return (index & 0x1FFFFFFEU) >> 1;
}

#endif

typedef struct Il2CppInterfaceOffsetPair
{
    TypeIndex interfaceTypeIndex;
    int32_t offset;
} Il2CppInterfaceOffsetPair;

typedef struct Il2CppTypeDefinition
{
    StringIndex nameIndex;
    StringIndex namespaceIndex;
    TypeIndex byvalTypeIndex;

    TypeIndex declaringTypeIndex;
    TypeIndex parentIndex;
    TypeIndex elementTypeIndex; // we can probably remove this one. Only used for enums

    GenericContainerIndex genericContainerIndex;

    uint32_t flags;

    FieldIndex fieldStart;
    MethodIndex methodStart;
    EventIndex eventStart;
    PropertyIndex propertyStart;
    NestedTypeIndex nestedTypesStart;
    InterfacesIndex interfacesStart;
    VTableIndex vtableStart;
    InterfacesIndex interfaceOffsetsStart;

    uint16_t method_count;
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count;
    uint16_t interfaces_count;
    uint16_t interface_offsets_count;

    // bitfield to portably encode boolean values as single bits
    // 01 - valuetype;
    // 02 - enumtype;
    // 03 - has_finalize;
    // 04 - has_cctor;
    // 05 - is_blittable;
    // 06 - is_import_or_windows_runtime;
    // 07-10 - One of nine possible PackingSize values (0, 1, 2, 4, 8, 16, 32, 64, or 128)
    // 11 - PackingSize is default
    // 12 - ClassSize is default
    // 13-16 - One of nine possible PackingSize values (0, 1, 2, 4, 8, 16, 32, 64, or 128) - the specified packing size (even for explicit layouts)
    uint32_t bitfield;
    uint32_t token;
} Il2CppTypeDefinition;

typedef struct Il2CppFieldDefinition
{
    StringIndex nameIndex;
    TypeIndex typeIndex;
    uint32_t token;
} Il2CppFieldDefinition;

typedef struct Il2CppFieldDefaultValue
{
    FieldIndex fieldIndex;
    TypeIndex typeIndex;
    DefaultValueDataIndex dataIndex;
} Il2CppFieldDefaultValue;

typedef struct Il2CppFieldMarshaledSize
{
    FieldIndex fieldIndex;
    TypeIndex typeIndex;
    int32_t size;
} Il2CppFieldMarshaledSize;

typedef struct Il2CppFieldRef
{
    TypeIndex typeIndex;
    FieldIndex fieldIndex; // local offset into type fields
} Il2CppFieldRef;

typedef struct Il2CppParameterDefinition
{
    StringIndex nameIndex;
    uint32_t token;
    TypeIndex typeIndex;
} Il2CppParameterDefinition;

typedef struct Il2CppParameterDefaultValue
{
    ParameterIndex parameterIndex;
    TypeIndex typeIndex;
    DefaultValueDataIndex dataIndex;
} Il2CppParameterDefaultValue;

typedef struct Il2CppMethodDefinition
{
    StringIndex nameIndex;
    TypeDefinitionIndex declaringType;
    TypeIndex returnType;
    ParameterIndex parameterStart;
    GenericContainerIndex genericContainerIndex;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint16_t parameterCount;
} Il2CppMethodDefinition;

typedef struct Il2CppEventDefinition
{
    StringIndex nameIndex;
    TypeIndex typeIndex;
    MethodIndex add;
    MethodIndex remove;
    MethodIndex raise;
    uint32_t token;
} Il2CppEventDefinition;

typedef struct Il2CppPropertyDefinition
{
    StringIndex nameIndex;
    MethodIndex get;
    MethodIndex set;
    uint32_t attrs;
    uint32_t token;
} Il2CppPropertyDefinition;

typedef struct Il2CppStringLiteral
{
    uint32_t length;
    StringLiteralIndex dataIndex;
} Il2CppStringLiteral;

typedef struct Il2CppAssemblyNameDefinition
{
    StringIndex nameIndex;
    StringIndex cultureIndex;
    StringIndex publicKeyIndex;
    uint32_t hash_alg;
    int32_t hash_len;
    uint32_t flags;
    int32_t major;
    int32_t minor;
    int32_t build;
    int32_t revision;
    uint8_t public_key_token[PUBLIC_KEY_BYTE_LENGTH];
} Il2CppAssemblyNameDefinition;

typedef struct Il2CppImageDefinition
{
    StringIndex nameIndex;
    AssemblyIndex assemblyIndex;

    TypeDefinitionIndex typeStart;
    uint32_t typeCount;

    TypeDefinitionIndex exportedTypeStart;
    uint32_t exportedTypeCount;

    MethodIndex entryPointIndex;
    uint32_t token;

    CustomAttributeIndex customAttributeStart;
    uint32_t customAttributeCount;
} Il2CppImageDefinition;

typedef struct Il2CppAssemblyDefinition
{
    ImageIndex imageIndex;
    uint32_t token;
    int32_t referencedAssemblyStart;
    int32_t referencedAssemblyCount;
    Il2CppAssemblyNameDefinition aname;
} Il2CppAssemblyDefinition;

typedef struct Il2CppCustomAttributeDataRange
{
    uint32_t token;
    uint32_t startOffset;
} Il2CppCustomAttributeDataRange;

typedef struct Il2CppMetadataRange
{
    int32_t start;
    int32_t length;
} Il2CppMetadataRange;

typedef struct Il2CppGenericContainer
{
    /* index of the generic type definition or the generic method definition corresponding to this container */
    int32_t ownerIndex; // either index into Il2CppClass metadata array or Il2CppMethodDefinition array
    int32_t type_argc;
    /* If true, we're a generic method, otherwise a generic type definition. */
    int32_t is_method;
    /* Our type parameters. */
    GenericParameterIndex genericParameterStart;
} Il2CppGenericContainer;

typedef struct Il2CppGenericParameter
{
    GenericContainerIndex ownerIndex;  /* Type or method this parameter was defined in. */
    StringIndex nameIndex;
    GenericParameterConstraintIndex constraintsStart;
    int16_t constraintsCount;
    uint16_t num;
    uint16_t flags;
} Il2CppGenericParameter;


typedef struct Il2CppWindowsRuntimeTypeNamePair
{
    StringIndex nameIndex;
    TypeIndex typeIndex;
} Il2CppWindowsRuntimeTypeNamePair;

#pragma pack(push, p1, 4)
typedef struct Il2CppGlobalMetadataHeader1
{
    int32_t sanity;
    int32_t version;
    int32_t stringLiteralOffset; // string data for managed code
    int32_t stringLiteralSize;
    int32_t stringLiteralDataOffset;
    int32_t stringLiteralDataSize;
    int32_t stringOffset; // string data for metadata
    int32_t stringSize;
    int32_t eventsOffset; // Il2CppEventDefinition
    int32_t eventsSize;
    int32_t propertiesOffset; // Il2CppPropertyDefinition
    int32_t propertiesSize;
    int32_t methodsOffset; // Il2CppMethodDefinition
    int32_t methodsSize;
    int32_t parameterDefaultValuesOffset; // Il2CppParameterDefaultValue
    int32_t parameterDefaultValuesSize;
    int32_t fieldDefaultValuesOffset; // Il2CppFieldDefaultValue
    int32_t fieldDefaultValuesSize;
    int32_t fieldAndParameterDefaultValueDataOffset; // uint8_t
    int32_t fieldAndParameterDefaultValueDataSize;
    int32_t fieldMarshaledSizesOffset; // Il2CppFieldMarshaledSize
    int32_t fieldMarshaledSizesSize;
    int32_t parametersOffset; // Il2CppParameterDefinition
    int32_t parametersSize;
    int32_t fieldsOffset; // Il2CppFieldDefinition
    int32_t fieldsSize;
    int32_t genericParametersOffset; // Il2CppGenericParameter
    int32_t genericParametersSize;
    int32_t genericParameterConstraintsOffset; // TypeIndex
    int32_t genericParameterConstraintsSize;
    int32_t genericContainersOffset; // Il2CppGenericContainer
    int32_t genericContainersSize;
    int32_t nestedTypesOffset; // TypeDefinitionIndex
    int32_t nestedTypesSize;
    int32_t interfacesOffset; // TypeIndex
    int32_t interfacesSize;
    int32_t vtableMethodsOffset; // EncodedMethodIndex
    int32_t vtableMethodsSize;
    int32_t interfaceOffsetsOffset; // Il2CppInterfaceOffsetPair
    int32_t interfaceOffsetsSize;
    int32_t typeDefinitionsOffset; // Il2CppTypeDefinition
    int32_t typeDefinitionsSize;
    int32_t imagesOffset; // Il2CppImageDefinition
    int32_t imagesSize;
    int32_t assembliesOffset; // Il2CppAssemblyDefinition
    int32_t assembliesSize;
    int32_t fieldRefsOffset; // Il2CppFieldRef
    int32_t fieldRefsSize;
    int32_t referencedAssembliesOffset; // int32_t
    int32_t referencedAssembliesSize;
    int32_t attributeDataOffset;
    int32_t attributeDataSize;
    int32_t attributeDataRangeOffset;
    int32_t attributeDataRangeSize;
    int32_t unresolvedIndirectCallParameterTypesOffset; // TypeIndex
    int32_t unresolvedIndirectCallParameterTypesSize;
    int32_t unresolvedIndirectCallParameterRangesOffset; // Il2CppMetadataRange
    int32_t unresolvedIndirectCallParameterRangesSize;
    int32_t windowsRuntimeTypeNamesOffset; // Il2CppWindowsRuntimeTypeNamePair
    int32_t windowsRuntimeTypeNamesSize;
    int32_t windowsRuntimeStringsOffset; // const char*
    int32_t windowsRuntimeStringsSize;
    int32_t exportedTypeDefinitionsOffset; // TypeDefinitionIndex
    int32_t exportedTypeDefinitionsSize;
} Il2CppGlobalMetadataHeader1;
#pragma pack(pop, p1)
#pragma pack(push, p1, 4)
typedef struct Il2CppGlobalMetadataHeader2
{
    int32_t attributeDataSize6;
    int32_t attributeDataSize;
    int32_t attributeDataRangeOffset8;
    int32_t fieldMarshaledSizesOffset8;
    int32_t vtableMethodsOffset5;
    int32_t stringLiteralDataSize9;
    int32_t attributeDataSize3;
    int32_t fieldRefsSize3;
    int32_t interfaceOffsetsSize5;
    int32_t nestedTypesOffset6;
    int32_t typeDefinitionsOffset4;
    int32_t fieldAndParameterDefaultValueDataOffset3;
    int32_t genericParameterConstraintsOffset0;
    int32_t unresolvedIndirectCallParameterTypesSize0;
    int32_t referencedAssembliesSize9;
    int32_t methodsSize9;
    int32_t windowsRuntimeStringsOffset5;
    int32_t imagesSize1;
    int32_t exportedTypeDefinitionsSize5;
    int32_t nestedTypesSize8;
    int32_t interfacesSize8;
    int32_t unresolvedIndirectCallParameterRangesOffset8;
    int32_t methodsOffset1;
    int32_t referencedAssembliesSize7;
    int32_t genericParameterConstraintsOffset6;
    int32_t fieldRefsOffset1;
    int32_t fieldMarshaledSizesOffset6;
    int32_t genericParameterConstraintsSize0;
    int32_t fieldMarshaledSizesSize4;
    int32_t windowsRuntimeStringsSize9;
    int32_t fieldDefaultValuesOffset1;
    int32_t nestedTypesSize7;
    int32_t fieldAndParameterDefaultValueDataSize6;
    int32_t fieldMarshaledSizesOffset4;
    int32_t unresolvedIndirectCallParameterRangesOffset9;
    int32_t fieldDefaultValuesSize1;
    int32_t stringOffset5;
    int32_t interfaceOffsetsSize7;
    int32_t windowsRuntimeStringsOffset7;
    int32_t interfaceOffsetsOffset2;
    int32_t genericParametersSize2;
    int32_t stringOffset9;
    int32_t genericParametersOffset; // Il2CppGenericParameter
    int32_t imagesSize6;
    int32_t referencedAssembliesOffset1;
    int32_t stringSize2;
    int32_t stringLiteralSize1;
    int32_t interfaceOffsetsSize;
    int32_t fieldAndParameterDefaultValueDataSize5;
    int32_t typeDefinitionsSize5;
    int32_t unresolvedIndirectCallParameterTypesSize9;
    int32_t imagesOffset5;
    int32_t parametersSize5;
    int32_t fieldDefaultValuesSize5;
    int32_t stringSize0;
    int32_t windowsRuntimeStringsSize5;
    int32_t nestedTypesOffset5;
    int32_t stringLiteralOffset5;
    int32_t windowsRuntimeTypeNamesOffset3;
    int32_t fieldDefaultValuesSize7;
    int32_t assembliesOffset1;
    int32_t stringLiteralDataOffset2;
    int32_t fieldDefaultValuesOffset4;
    int32_t typeDefinitionsOffset0;
    int32_t interfacesOffset8;
    int32_t exportedTypeDefinitionsSize6;
    int32_t interfacesOffset7;
    int32_t methodsOffset; // Il2CppMethodDefinition
    int32_t interfacesOffset3;
    int32_t interfacesSize3;
    int32_t stringSize1;
    int32_t unresolvedIndirectCallParameterRangesSize8;
    int32_t propertiesSize3;
    int32_t unresolvedIndirectCallParameterTypesSize7;
    int32_t stringSize6;
    int32_t fieldRefsOffset2;
    int32_t genericParametersOffset4;
    int32_t parametersSize4;
    int32_t interfacesOffset5;
    int32_t assembliesOffset4;
    int32_t stringLiteralSize4;
    int32_t genericContainersOffset5;
    int32_t genericContainersSize6;
    int32_t imagesSize0;
    int32_t genericParameterConstraintsOffset5;
    int32_t parameterDefaultValuesSize4;
    int32_t vtableMethodsOffset0;
    int32_t eventsOffset7;
    int32_t fieldRefsOffset4;
    int32_t stringLiteralDataSize5;
    int32_t windowsRuntimeTypeNamesOffset6;
    int32_t typeDefinitionsOffset1;
    int32_t exportedTypeDefinitionsSize9;
    int32_t propertiesSize7;
    int32_t vtableMethodsOffset3;
    int32_t fieldAndParameterDefaultValueDataOffset2;
    int32_t genericContainersOffset6;
    int32_t propertiesSize1;
    int32_t stringSize8;
    int32_t stringSize;
    int32_t methodsSize3;
    int32_t fieldRefsOffset8;
    int32_t fieldDefaultValuesOffset9;
    int32_t exportedTypeDefinitionsOffset5;
    int32_t unresolvedIndirectCallParameterTypesSize1;
    int32_t genericContainersSize1;
    int32_t stringOffset8;
    int32_t fieldDefaultValuesOffset0;
    int32_t unresolvedIndirectCallParameterRangesOffset0;
    int32_t fieldsSize9;
    int32_t attributeDataSize4;
    int32_t interfaceOffsetsOffset3;
    int32_t unresolvedIndirectCallParameterTypesOffset5;
    int32_t propertiesSize0;
    int32_t methodsSize2;
    int32_t referencedAssembliesSize;
    int32_t unresolvedIndirectCallParameterTypesOffset; // TypeIndex
    int32_t fieldRefsSize7;
    int32_t nestedTypesOffset1;
    int32_t stringLiteralDataSize7;
    int32_t imagesSize9;
    int32_t stringLiteralDataOffset0;
    int32_t fieldsSize7;
    int32_t fieldMarshaledSizesSize1;
    int32_t genericContainersSize3;
    int32_t referencedAssembliesSize3;
    int32_t genericContainersSize9;
    int32_t parameterDefaultValuesOffset2;
    int32_t nestedTypesSize9;
    int32_t vtableMethodsSize8;
    int32_t unresolvedIndirectCallParameterTypesSize5;
    int32_t parametersSize3;
    int32_t fieldMarshaledSizesOffset0;
    int32_t interfacesOffset6;
    int32_t imagesOffset2;
    int32_t attributeDataRangeOffset2;
    int32_t genericContainersOffset0;
    int32_t exportedTypeDefinitionsOffset6;
    int32_t parameterDefaultValuesOffset7;
    int32_t assembliesSize0;
    int32_t genericContainersOffset3;
    int32_t propertiesOffset9;
    int32_t stringLiteralSize9;
    int32_t windowsRuntimeTypeNamesSize1;
    int32_t interfaceOffsetsSize4;
    int32_t fieldDefaultValuesOffset; // Il2CppFieldDefaultValue
    int32_t interfacesOffset; // TypeIndex
    int32_t windowsRuntimeTypeNamesSize9;
    int32_t fieldMarshaledSizesSize5;
    int32_t unresolvedIndirectCallParameterRangesSize0;
    int32_t stringLiteralDataSize;
    int32_t stringLiteralDataSize8;
    int32_t fieldsOffset7;
    int32_t parametersSize;
    int32_t fieldDefaultValuesSize2;
    int32_t fieldsSize;
    int32_t interfaceOffsetsSize1;
    int32_t interfacesSize1;
    int32_t interfaceOffsetsOffset5;
    int32_t stringOffset0;
    int32_t vtableMethodsOffset1;
    int32_t genericParametersOffset6;
    int32_t fieldRefsSize2;
    int32_t fieldRefsSize1;
    int32_t windowsRuntimeTypeNamesOffset1;
    int32_t genericParameterConstraintsSize1;
    int32_t windowsRuntimeTypeNamesOffset4;
    int32_t unresolvedIndirectCallParameterRangesSize6;
    int32_t genericParametersSize4;
    int32_t assembliesOffset9;
    int32_t referencedAssembliesSize4;
    int32_t imagesSize3;
    int32_t attributeDataOffset1;
    int32_t interfacesSize7;
    int32_t windowsRuntimeTypeNamesOffset9;
    int32_t methodsOffset6;
    int32_t imagesSize7;
    int32_t vtableMethodsOffset2;
    int32_t unresolvedIndirectCallParameterTypesOffset4;
    int32_t fieldDefaultValuesOffset2;
    int32_t assembliesSize9;
    int32_t interfacesOffset9;
    int32_t genericContainersSize2;
    int32_t fieldAndParameterDefaultValueDataOffset6;
    int32_t exportedTypeDefinitionsOffset9;
    int32_t nestedTypesOffset; // TypeDefinitionIndex
    int32_t genericParametersSize7;
    int32_t windowsRuntimeStringsOffset1;
    int32_t windowsRuntimeTypeNamesOffset7;
    int32_t typeDefinitionsOffset2;
    int32_t attributeDataSize0;
    int32_t unresolvedIndirectCallParameterRangesSize3;
    int32_t windowsRuntimeTypeNamesSize7;
    int32_t genericParameterConstraintsOffset7;
    int32_t nestedTypesOffset0;
    int32_t fieldRefsSize9;
    int32_t unresolvedIndirectCallParameterTypesOffset1;
    int32_t fieldRefsSize0;
    int32_t genericParametersOffset0;
    int32_t parametersSize6;
    int32_t stringLiteralDataOffset5;
    int32_t attributeDataRangeSize2;
    int32_t stringLiteralSize5;
    int32_t methodsOffset4;
    int32_t parametersOffset1;
    int32_t genericParametersSize6;
    int32_t genericContainersOffset4;
    int32_t methodsSize7;
    int32_t fieldRefsOffset5;
    int32_t genericParameterConstraintsSize3;
    int32_t nestedTypesOffset2;
    int32_t attributeDataRangeOffset1;
    int32_t parameterDefaultValuesOffset4;
    int32_t referencedAssembliesOffset; // int32_t
    int32_t attributeDataOffset5;
    int32_t unresolvedIndirectCallParameterTypesSize6;
    int32_t attributeDataOffset2;
    int32_t windowsRuntimeStringsSize1;
    int32_t unresolvedIndirectCallParameterTypesOffset8;
    int32_t stringLiteralDataOffset;
    int32_t attributeDataRangeSize6;
    int32_t attributeDataSize9;
    int32_t assembliesSize8;
    int32_t imagesSize2;
    int32_t stringLiteralDataSize2;
    int32_t genericContainersOffset9;
    int32_t windowsRuntimeStringsSize2;
    int32_t stringLiteralDataSize4;
    int32_t unresolvedIndirectCallParameterRangesSize1;
    int32_t fieldsSize0;
    int32_t unresolvedIndirectCallParameterTypesOffset9;
    int32_t fieldDefaultValuesSize0;
    int32_t assembliesSize3;
    int32_t fieldRefsOffset0;
    int32_t interfaceOffsetsSize2;
    int32_t propertiesSize6;
    int32_t fieldAndParameterDefaultValueDataOffset7;
    int32_t vtableMethodsSize;
    int32_t fieldAndParameterDefaultValueDataSize8;
    int32_t referencedAssembliesSize5;
    int32_t windowsRuntimeStringsOffset6;
    int32_t imagesOffset6;
    int32_t typeDefinitionsOffset8;
    int32_t referencedAssembliesSize2;
    int32_t exportedTypeDefinitionsOffset1;
    int32_t fieldsOffset2;
    int32_t interfacesSize9;
    int32_t stringLiteralSize8;
    int32_t vtableMethodsSize6;
    int32_t typeDefinitionsOffset5;
    int32_t attributeDataSize7;
    int32_t attributeDataRangeSize;
    int32_t fieldsOffset6;
    int32_t exportedTypeDefinitionsOffset2;
    int32_t parametersSize7;
    int32_t propertiesOffset0;
    int32_t eventsOffset6;
    int32_t genericParametersOffset5;
    int32_t exportedTypeDefinitionsOffset4;
    int32_t windowsRuntimeTypeNamesOffset8;
    int32_t stringSize4;
    int32_t windowsRuntimeStringsSize8;
    int32_t genericParameterConstraintsOffset9;
    int32_t genericParametersOffset9;
    int32_t windowsRuntimeTypeNamesOffset0;
    int32_t parametersOffset0;
    int32_t parametersSize8;
    int32_t fieldDefaultValuesOffset6;
    int32_t unresolvedIndirectCallParameterRangesSize4;
    int32_t interfaceOffsetsSize3;
    int32_t exportedTypeDefinitionsOffset3;
    int32_t assembliesOffset6;
    int32_t stringLiteralOffset0;
    int32_t unresolvedIndirectCallParameterRangesSize;
    int32_t propertiesOffset8;
    int32_t windowsRuntimeStringsSize;
    int32_t interfaceOffsetsOffset0;
    int32_t fieldAndParameterDefaultValueDataSize4;
    int32_t nestedTypesSize;
    int32_t vtableMethodsSize9;
    int32_t unresolvedIndirectCallParameterTypesSize8;
    int32_t eventsOffset8;
    int32_t typeDefinitionsSize2;
    int32_t fieldsOffset9;
    int32_t interfaceOffsetsOffset1;
    int32_t parameterDefaultValuesOffset9;
    int32_t vtableMethodsOffset7;
    int32_t nestedTypesOffset9;
    int32_t fieldDefaultValuesSize9;
    int32_t referencedAssembliesSize1;
    int32_t genericParametersOffset1;
    int32_t typeDefinitionsOffset7;
    int32_t windowsRuntimeStringsOffset2;
    int32_t imagesOffset7;
    int32_t stringLiteralDataOffset6;
    int32_t windowsRuntimeStringsOffset4;
    int32_t windowsRuntimeTypeNamesSize3;
    int32_t typeDefinitionsSize0;
    int32_t parameterDefaultValuesSize5;
    int32_t parametersSize9;
    int32_t fieldMarshaledSizesOffset3;
    int32_t genericParameterConstraintsSize7;
    int32_t imagesOffset8;
    int32_t exportedTypeDefinitionsSize4;
    int32_t propertiesOffset3;
    int32_t fieldMarshaledSizesOffset; // Il2CppFieldMarshaledSize
    int32_t stringLiteralOffset1;
    int32_t windowsRuntimeStringsSize7;
    int32_t nestedTypesSize0;
    int32_t assembliesOffset0;
    int32_t genericContainersOffset7;
    int32_t imagesSize8;
    int32_t eventsSize2;
    int32_t windowsRuntimeTypeNamesSize0;
    int32_t unresolvedIndirectCallParameterRangesOffset6;
    int32_t stringLiteralOffset9;
    int32_t genericParameterConstraintsSize9;
    int32_t fieldMarshaledSizesSize7;
    int32_t unresolvedIndirectCallParameterTypesOffset7;
    int32_t unresolvedIndirectCallParameterTypesSize;
    int32_t typeDefinitionsOffset; // Il2CppTypeDefinition
    int32_t parameterDefaultValuesSize2;
    int32_t windowsRuntimeStringsSize3;
    int32_t assembliesSize7;
    int32_t interfacesSize6;
    int32_t parametersOffset; // Il2CppParameterDefinition
    int32_t methodsOffset8;
    int32_t referencedAssembliesOffset2;
    int32_t fieldAndParameterDefaultValueDataSize7;
    int32_t interfacesSize5;
    int32_t nestedTypesOffset3;
    int32_t referencedAssembliesOffset3;
    int32_t interfaceOffsetsOffset9;
    int32_t fieldRefsOffset3;
    int32_t eventsOffset9;
    int32_t stringLiteralOffset3;
    int32_t eventsSize;
    int32_t fieldDefaultValuesSize6;
    int32_t genericContainersSize7;
    int32_t windowsRuntimeTypeNamesOffset2;
    int32_t exportedTypeDefinitionsSize8;
    int32_t parameterDefaultValuesOffset1;
    int32_t fieldAndParameterDefaultValueDataOffset; // uint8_t
    int32_t attributeDataRangeOffset4;
    int32_t typeDefinitionsOffset9;
    int32_t fieldRefsSize4;
    int32_t stringOffset7;
    int32_t vtableMethodsSize1;
    int32_t fieldMarshaledSizesSize;
    int32_t propertiesOffset4;
    int32_t stringLiteralDataOffset3;
    int32_t interfaceOffsetsSize9;
    int32_t fieldRefsSize6;
    int32_t stringLiteralOffset7;
    int32_t genericParametersSize5;
    int32_t propertiesSize5;
    int32_t windowsRuntimeTypeNamesSize5;
    int32_t parameterDefaultValuesSize9;
    int32_t parameterDefaultValuesOffset6;
    int32_t fieldsSize1;
    int32_t referencedAssembliesOffset6;
    int32_t vtableMethodsSize2;
    int32_t windowsRuntimeStringsOffset9;
    int32_t windowsRuntimeStringsSize0;
    int32_t windowsRuntimeStringsSize6;
    int32_t unresolvedIndirectCallParameterRangesOffset7;
    int32_t attributeDataRangeOffset7;
    int32_t methodsSize0;
    int32_t fieldMarshaledSizesSize8;
    int32_t assembliesOffset; // Il2CppAssemblyDefinition
    int32_t fieldMarshaledSizesSize3;
    int32_t genericParameterConstraintsOffset2;
    int32_t windowsRuntimeTypeNamesSize6;
    int32_t parametersSize0;
    int32_t stringLiteralDataSize6;
    int32_t windowsRuntimeStringsOffset3;
    int32_t parametersOffset2;
    int32_t eventsSize5;
    int32_t stringSize3;
    int32_t stringLiteralSize7;
    int32_t windowsRuntimeTypeNamesSize;
    int32_t genericParameterConstraintsOffset3;
    int32_t genericContainersSize;
    int32_t parameterDefaultValuesSize;
    int32_t attributeDataSize2;
    int32_t parameterDefaultValuesSize1;
    int32_t fieldsSize5;
    int32_t nestedTypesOffset4;
    int32_t genericParameterConstraintsOffset1;
    int32_t exportedTypeDefinitionsOffset8;
    int32_t genericParameterConstraintsOffset8;
    int32_t interfaceOffsetsSize0;
    int32_t genericParametersOffset3;
    int32_t fieldAndParameterDefaultValueDataSize;
    int32_t stringLiteralDataSize1;
    int32_t attributeDataRangeOffset0;
    int32_t typeDefinitionsSize4;
    int32_t propertiesOffset1;
    int32_t assembliesOffset2;
    int32_t stringLiteralDataSize0;
    int32_t genericParametersSize8;
    int32_t genericParameterConstraintsOffset; // TypeIndex
    int32_t parametersOffset8;
    int32_t parameterDefaultValuesSize8;
    int32_t attributeDataSize5;
    int32_t attributeDataRangeSize4;
    int32_t imagesSize4;
    int32_t fieldAndParameterDefaultValueDataSize0;
    int32_t unresolvedIndirectCallParameterRangesOffset5;
    int32_t fieldDefaultValuesOffset7;
    int32_t typeDefinitionsSize;
    int32_t imagesOffset1;
    int32_t parametersOffset9;
    int32_t typeDefinitionsOffset3;
    int32_t referencedAssembliesSize6;
    int32_t interfaceOffsetsOffset8;
    int32_t interfaceOffsetsOffset4;
    int32_t assembliesSize;
    int32_t unresolvedIndirectCallParameterTypesSize4;
    int32_t imagesOffset; // Il2CppImageDefinition
    int32_t stringLiteralSize2;
    int32_t genericParameterConstraintsOffset4;
    int32_t unresolvedIndirectCallParameterRangesOffset1;
    int32_t stringLiteralDataOffset4;
    int32_t attributeDataRangeOffset3;
    int32_t interfacesSize0;
    int32_t vtableMethodsOffset6;
    int32_t propertiesSize2;
    int32_t fieldMarshaledSizesOffset1;
    int32_t unresolvedIndirectCallParameterRangesSize9;
    int32_t eventsOffset3;
    int32_t windowsRuntimeStringsOffset; // const char*
    int32_t typeDefinitionsSize1;
    int32_t genericParametersOffset2;
    int32_t methodsOffset2;
    int32_t genericParameterConstraintsSize8;
    int32_t attributeDataOffset4;
    int32_t referencedAssembliesSize0;
    int32_t interfacesOffset2;
    int32_t methodsOffset3;
    int32_t eventsSize4;
    int32_t unresolvedIndirectCallParameterTypesSize3;
    int32_t exportedTypeDefinitionsOffset0;
    int32_t interfacesSize;
    int32_t typeDefinitionsOffset6;
    int32_t typeDefinitionsSize7;
    int32_t eventsSize8;
    int32_t assembliesSize5;
    int32_t propertiesSize;
    int32_t fieldsSize8;
    int32_t stringLiteralSize;
    int32_t stringLiteralOffset8;
    int32_t stringOffset4;
    int32_t windowsRuntimeTypeNamesOffset; // Il2CppWindowsRuntimeTypeNamePair
    int32_t typeDefinitionsSize3;
    int32_t methodsSize;
    int32_t methodsSize5;
    int32_t genericParametersSize9;
    int32_t interfacesSize4;
    int32_t eventsOffset; // Il2CppEventDefinition
    int32_t unresolvedIndirectCallParameterTypesOffset3;
    int32_t attributeDataRangeSize8;
    int32_t unresolvedIndirectCallParameterRangesOffset; // Il2CppMetadataRange
    int32_t fieldsOffset0;
    int32_t methodsOffset0;
    int32_t vtableMethodsOffset9;
    int32_t interfaceOffsetsOffset6;
    int32_t interfacesOffset0;
    int32_t attributeDataRangeSize7;
    int32_t assembliesSize4;
    int32_t genericContainersOffset8;
    int32_t attributeDataRangeSize3;
    int32_t windowsRuntimeTypeNamesSize8;
    int32_t fieldRefsOffset9;
    int32_t eventsSize9;
    int32_t parameterDefaultValuesSize6;
    int32_t assembliesSize6;
    int32_t parameterDefaultValuesSize3;
    int32_t vtableMethodsSize3;
    int32_t fieldRefsSize;
    int32_t vtableMethodsSize4;
    int32_t parameterDefaultValuesOffset8;
    int32_t stringLiteralDataOffset7;
    int32_t fieldMarshaledSizesSize9;
    int32_t propertiesOffset2;
    int32_t fieldsSize2;
    int32_t stringLiteralSize3;
    int32_t fieldsSize6;
    int32_t fieldMarshaledSizesSize6;
    int32_t fieldAndParameterDefaultValueDataOffset8;
    int32_t fieldAndParameterDefaultValueDataOffset4;
    int32_t interfacesOffset4;
    int32_t attributeDataSize1;
    int32_t windowsRuntimeStringsOffset0;
    int32_t methodsSize1;
    int32_t exportedTypeDefinitionsSize;
    int32_t stringLiteralDataOffset8;
    int32_t fieldDefaultValuesSize;
    int32_t fieldsSize4;
    int32_t nestedTypesSize2;
    int32_t vtableMethodsSize5;
    int32_t referencedAssembliesSize8;
    int32_t propertiesOffset7;
    int32_t referencedAssembliesOffset8;
    int32_t vtableMethodsSize7;
    int32_t parametersOffset3;
    int32_t fieldRefsOffset6;
    int32_t fieldAndParameterDefaultValueDataSize3;
    int32_t genericParametersSize1;
    int32_t vtableMethodsSize0;
    int32_t genericContainersSize8;
    int32_t fieldAndParameterDefaultValueDataOffset1;
    int32_t fieldsOffset; // Il2CppFieldDefinition
    int32_t attributeDataOffset;
    int32_t stringLiteralOffset4;
    int32_t propertiesOffset; // Il2CppPropertyDefinition
    int32_t parameterDefaultValuesOffset0;
    int32_t genericParametersSize0;
    int32_t unresolvedIndirectCallParameterRangesSize2;
    int32_t assembliesOffset3;
    int32_t vtableMethodsOffset; // EncodedMethodIndex
    int32_t attributeDataOffset8;
    int32_t fieldMarshaledSizesOffset9;
    int32_t attributeDataRangeOffset9;
    int32_t imagesOffset4;
    int32_t nestedTypesSize1;
    int32_t fieldAndParameterDefaultValueDataOffset5;
    int32_t genericParameterConstraintsSize6;
    int32_t eventsOffset2;
    int32_t stringLiteralOffset2;
    int32_t genericParametersSize3;
    int32_t fieldRefsSize5;
    int32_t referencedAssembliesOffset7;
    int32_t typeDefinitionsSize9;
    int32_t windowsRuntimeTypeNamesOffset5;
    int32_t assembliesSize2;
    int32_t fieldRefsSize8;
    int32_t unresolvedIndirectCallParameterTypesOffset6;
    int32_t eventsSize6;
    int32_t genericParametersOffset8;
    int32_t genericContainersSize5;
    int32_t eventsSize0;
    int32_t fieldDefaultValuesOffset5;
    int32_t stringLiteralDataOffset1;
    int32_t referencedAssembliesOffset4;
    int32_t interfacesOffset1;
    int32_t fieldMarshaledSizesOffset7;
    int32_t fieldMarshaledSizesSize0;
    int32_t eventsOffset0;
    int32_t parametersOffset6;
    int32_t parametersSize1;
    int32_t fieldDefaultValuesSize8;
    int32_t stringLiteralSize0;
    int32_t methodsSize4;
    int32_t assembliesOffset7;
    int32_t fieldRefsOffset; // Il2CppFieldRef
    int32_t referencedAssembliesOffset9;
    int32_t windowsRuntimeTypeNamesSize4;
    int32_t windowsRuntimeTypeNamesSize2;
    int32_t stringSize5;
    int32_t stringLiteralOffset6;
    int32_t imagesOffset9;
    int32_t stringLiteralDataSize3;
    int32_t attributeDataRangeSize5;
    int32_t propertiesSize9;
    int32_t attributeDataOffset6;
    int32_t typeDefinitionsSize6;
    int32_t genericParameterConstraintsSize;
    int32_t parametersSize2;
    int32_t genericContainersSize0;
    int32_t attributeDataRangeOffset6;
    int32_t methodsOffset5;
    int32_t nestedTypesSize6;
    int32_t imagesOffset0;
    int32_t referencedAssembliesOffset0;
    int32_t methodsOffset9;
    int32_t exportedTypeDefinitionsSize7;
    int32_t genericContainersOffset; // Il2CppGenericContainer
    int32_t methodsOffset7;
    int32_t methodsSize6;
    int32_t imagesOffset3;
    int32_t nestedTypesOffset8;
    int32_t stringOffset2;
    int32_t stringSize9;
    int32_t unresolvedIndirectCallParameterRangesSize7;
    int32_t stringOffset3;
    int32_t genericParameterConstraintsSize4;
    int32_t propertiesSize8;
    int32_t attributeDataRangeOffset5;
    int32_t unresolvedIndirectCallParameterRangesOffset4;
    int32_t attributeDataRangeSize9;
    int32_t genericContainersOffset1;
    int32_t propertiesOffset5;
    int32_t propertiesSize4;
    int32_t exportedTypeDefinitionsSize2;
    int32_t genericParameterConstraintsSize2;
    int32_t exportedTypeDefinitionsOffset; // TypeDefinitionIndex
    int32_t fieldDefaultValuesOffset8;
    int32_t attributeDataRangeSize1;
    int32_t fieldRefsOffset7;
    int32_t eventsSize1;
    int32_t fieldAndParameterDefaultValueDataSize1;
    int32_t stringOffset; // string data for metadata
    int32_t assembliesSize1;
    int32_t stringLiteralDataOffset9;
    int32_t parameterDefaultValuesSize0;
    int32_t fieldAndParameterDefaultValueDataSize9;
    int32_t genericParametersSize;
    int32_t fieldsOffset8;
    int32_t fieldMarshaledSizesSize2;
    int32_t fieldsOffset3;
    int32_t parameterDefaultValuesOffset; // Il2CppParameterDefaultValue
    int32_t typeDefinitionsSize8;
    int32_t fieldDefaultValuesSize3;
    int32_t attributeDataSize8;
    int32_t fieldMarshaledSizesOffset2;
    int32_t fieldsOffset1;
    int32_t parametersOffset7;
    int32_t parametersOffset5;
    int32_t sanity;
    int32_t fieldDefaultValuesSize4;
    int32_t nestedTypesSize5;
    int32_t unresolvedIndirectCallParameterTypesSize2;
    int32_t version;
    int32_t attributeDataRangeSize0;
    int32_t attributeDataOffset7;
    int32_t fieldsSize3;
    int32_t stringLiteralSize6;
    int32_t fieldDefaultValuesOffset3;
    int32_t eventsOffset5;
    int32_t genericContainersSize4;
    int32_t eventsSize7;
    int32_t exportedTypeDefinitionsSize0;
    int32_t interfaceOffsetsOffset7;
    int32_t parametersOffset4;
    int32_t attributeDataOffset3;
    int32_t nestedTypesOffset7;
    int32_t eventsOffset4;
    int32_t attributeDataOffset0;
    int32_t stringSize7;
    int32_t nestedTypesSize4;
    int32_t parameterDefaultValuesOffset3;
    int32_t stringOffset1;
    int32_t propertiesOffset6;
    int32_t assembliesOffset8;
    int32_t interfaceOffsetsSize6;
    int32_t attributeDataOffset9;
    int32_t methodsSize8;
    int32_t fieldAndParameterDefaultValueDataSize2;
    int32_t parameterDefaultValuesSize7;
    int32_t stringOffset6;
    int32_t vtableMethodsOffset8;
    int32_t attributeDataRangeOffset;
    int32_t unresolvedIndirectCallParameterRangesOffset2;
    int32_t exportedTypeDefinitionsSize1;
    int32_t imagesSize5;
    int32_t parameterDefaultValuesOffset5;
    int32_t interfaceOffsetsOffset; // Il2CppInterfaceOffsetPair
    int32_t unresolvedIndirectCallParameterTypesOffset2;
    int32_t exportedTypeDefinitionsSize3;
    int32_t assembliesOffset5;
    int32_t genericContainersOffset2;
    int32_t referencedAssembliesOffset5;
    int32_t windowsRuntimeStringsOffset8;
    int32_t fieldAndParameterDefaultValueDataOffset0;
    int32_t unresolvedIndirectCallParameterTypesOffset0;
    int32_t unresolvedIndirectCallParameterRangesSize5;
    int32_t stringLiteralOffset; // string data for managed code
    int32_t unresolvedIndirectCallParameterRangesOffset3;
    int32_t interfaceOffsetsSize8;
    int32_t eventsSize3;
    int32_t genericParametersOffset7;
    int32_t fieldAndParameterDefaultValueDataOffset9;
    int32_t nestedTypesSize3;
    int32_t fieldsOffset4;
    int32_t vtableMethodsOffset4;
    int32_t fieldMarshaledSizesOffset5;
    int32_t interfacesSize2;
    int32_t exportedTypeDefinitionsOffset7;
    int32_t fieldsOffset5;
    int32_t genericParameterConstraintsSize5;
    int32_t eventsOffset1;
    int32_t imagesSize;
    int32_t windowsRuntimeStringsSize4;
} Il2CppGlobalMetadataHeader2;
#pragma pack(pop, p1)
