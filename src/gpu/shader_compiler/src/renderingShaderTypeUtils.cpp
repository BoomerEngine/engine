/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\types #]
***/

#include "build.h"

#include "renderingShaderTypeUtils.h"
#include "renderingShaderTypeLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

// get the underlying scalar type that is the foundament of given type
// for scalars this is simple and the scalar type is returned
// for other types it does not work except for those few special composite types like vector4, int3, etc...
BaseType ExtractBaseType(const DataType& type)
{
    if (type.isScalar())
    {
        return type.baseType();
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::VectorType)
    {
        return type.composite().members()[0].type.baseType(); // we assume that the special struct like float4 have all members of the same type
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::MatrixType)
    {
        return type.composite().members()[0].type.composite().members()[0].type.baseType(); // we assume that the special struct like float4 have all members of the same type
    }

    return BaseType::Invalid;
}

// get the number of components in the type
// for scalars it returns 1
// for rest of the types it returns 0 except for those few special composite types like vector3, int3, et...
uint32_t ExtractComponentCount(const DataType& type)
{
    if (type.isScalar())
    {
        return 1;
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::VectorType)
    {
        return type.composite().members().size(); // we assume that the special types have only needed members
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::MatrixType)
    {
        auto& member0 = type.composite().members()[0];
        return ExtractComponentCount(member0.type);
    }

    return 0;
}

// get the number of components in the type
// for scalars it returns 1
// for rest of the types it returns 0 except for those few special composite types like vector3, int3, et...
uint32_t ExtractRowCount(const DataType& type)
{
    if (type.isScalar())
    {
        return 1;
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::VectorType)
    {
        return 1;
    }
    else if (type.isComposite() && type.composite().hint() == CompositeTypeHint::MatrixType)
    {
        return type.composite().members().size(); // we assume that the special types have only needed members
    }

    return 0;
}

/// can we swizzle this type at all
bool CanSwizzle(const DataType& type)
{
    // we can only swizzle the special composite types
    if (type.isComposite() && type.composite().hint() != CompositeTypeHint::User)
        return true;

    // we can swizzle the numerical scalars
    if (type.isNumericalScalar())
        return true;

    // nope, type cannot be swizzled
    return false;
}

/// can we use a read swizzle on given type
bool CanUseComponentMask(const DataType& type, const ComponentMask& swizzle)
{
    ASSERT(CanSwizzle(type)); // this check should have been done first

    // get number of components in the type
    // the swizzle cannot address more components than the type has
    auto numComponents = ExtractComponentCount(type);
    return swizzle.numberOfComponentsNeeded() <= numComponents;
}

/// get a base scalar type of special composite types (vector & matrices)
DataType GetBaseElementType(const DataType& compositeType)
{
    // scalars fall through
    if (compositeType.isScalar())
        return compositeType;

    // create a simple scalar type 
    auto baseType = ExtractBaseType(compositeType);
    return DataType(baseType);
}

/// get a similar type with more components based on the expand mode
DataType GetExpandedType(TypeLibrary& typeLibrary, const DataType& scalarType, TypeMatchTypeExpand expandType)
{
    ASSERT(scalarType.isScalar());

    switch (expandType)
    {
    case TypeMatchTypeExpand::DontExpand:
        return scalarType;

    case TypeMatchTypeExpand::ExpandXX:
        return typeLibrary.simpleCompositeType(scalarType.baseType(), 2);

    case TypeMatchTypeExpand::ExpandXXX:
        return typeLibrary.simpleCompositeType(scalarType.baseType(), 3);

    case TypeMatchTypeExpand::ExpandXXXX:
        return typeLibrary.simpleCompositeType(scalarType.baseType(), 4);
    }

    FATAL_ERROR("Invalid type expand mode");
    return DataType();
}

/// get a similar type with more components
DataType GetContractedType(TypeLibrary& typeLibrary, const DataType& inputType, uint32_t componentCount)
{
    auto baseType = ExtractBaseType(inputType);
    auto retType = typeLibrary.simpleCompositeType(baseType, componentCount);
    retType.applyFlags(inputType.flags());
    return retType;
}

/// get a similar type but with changed base type
DataType GetCastedType(TypeLibrary& typeLibrary, const DataType& sourceType, BaseType baseType)
{
    auto componentCount = ExtractComponentCount(sourceType);
    return typeLibrary.simpleCompositeType(baseType, componentCount);
}

/// get the inner type of array int[5] -> int, float4[10] -> float4, bool[15][5] -> bool[5] -> bool
DataType GetArrayInnerType(const DataType& dataType)
{
    ASSERT(dataType.isArray());
            
    auto subArray = dataType.arrayCounts().innerCounts();
    return dataType.applyArrayCounts(subArray);
}

END_BOOMER_NAMESPACE_EX(gpu::compiler)
