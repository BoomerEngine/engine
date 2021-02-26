/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\types #]
***/

#pragma once

#include "renderingShaderCodeNode.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

class TypeLibrary;

// get the underlying scalar type that is the fundamental of given type
// for scalars this is simple and the scalar type is returned
// for other types it does not work except for those few special composite types like vector4, int3, etc...
extern GPU_SHADER_COMPILER_API BaseType ExtractBaseType(const DataType& type);

// get the number of components in the type
// for scalars it returns 1
// for rest of the types it returns 0 except for those few special composite types like vector3, int3, et...
extern GPU_SHADER_COMPILER_API uint32_t ExtractComponentCount(const DataType& type);

// get the number of rows in the matrix type
// for scalars and vectors it returns 1
// for rest of the types it returns 0 except for those few special composite types like float42, float43, etc
extern GPU_SHADER_COMPILER_API uint32_t ExtractRowCount(const DataType& type);

/// can we swizzle this type at all
extern GPU_SHADER_COMPILER_API bool CanSwizzle(const DataType& type);

/// can we use a component mask on given type
extern GPU_SHADER_COMPILER_API bool CanUseComponentMask(const DataType& type, const ComponentMask& swizzle);

/// get a base scalar type of special composite types (vector & matrices)
extern GPU_SHADER_COMPILER_API DataType GetBaseElementType(const DataType& compositeType);

/// get a similar type with more components based on the expand mode
extern GPU_SHADER_COMPILER_API DataType GetExpandedType(TypeLibrary& typeLibrary, const DataType& scalarType, TypeMatchTypeExpand expandType);

/// get a similar type with less components
extern GPU_SHADER_COMPILER_API DataType GetContractedType(TypeLibrary& typeLibrary, const DataType& inputType, uint32_t componentCount);

/// get a similar type but with changed base type, allows to change int->float, float3->int3, etc
extern GPU_SHADER_COMPILER_API DataType GetCastedType(TypeLibrary& typeLibrary, const DataType& sourceType, BaseType baseType);

/// get the inner type of array int[5] -> int, float4[10] -> float4, bool[15][5] -> bool[5] -> bool
/// fails if passed type is not an array
extern GPU_SHADER_COMPILER_API DataType GetArrayInnerType(const DataType& dataType);

END_BOOMER_NAMESPACE_EX(gpu::compiler)
