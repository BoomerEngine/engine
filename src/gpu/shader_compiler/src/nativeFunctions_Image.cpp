/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\functions #]
***/

#include "build.h"
#include "function.h"
#include "nativeFunction.h"
#include "typeUtils.h"
#include "typeLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

static bool IsLoadableImage(const DataType& dataType, bool multisampled = false)
{
	if (!dataType.isResource())
		return false;

	const auto& res = dataType.resource();
	if (res.type != DeviceObjectViewType::ImageWritable && res.type != DeviceObjectViewType::Image)
		return false;

	if ((res.multisampled != multisampled) || res.depth)
		return false;

	return true;
}

static DataType GetImageSizeType(TypeLibrary& typeLibrary, const DataType& imageType)
{
    auto numComponents = imageType.resource().sizeComponentCount();
    return typeLibrary.unsignedType(numComponents);
}

static DataType GetRequiredCoordinateType(TypeLibrary& typeLibrary, const DataType& imageType, bool normalized)
{
    auto numComponents = imageType.resource().addressComponentCount();

    if (normalized)
        return typeLibrary.floatType(numComponents);
    else
        return typeLibrary.integerType(numComponents);
}

//--

class FunctionTextureLoad : public INativeFunction
{
	RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoad, INativeFunction);

public:
	virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
	{
		if (numArgs < 3)
		{
			err.reportError(loc, "Function expects arguments: (non-samplable Texture, Pos)");
			return DataType();
		}

		auto& srcImageFormat = argTypes[0];
		if (!IsLoadableImage(srcImageFormat, false))
		{
			err.reportError(loc, "Only non-multisampled non-samplable UAV textures can be used here");
			return DataType();
		}

		// determine required input types
		argTypes[0] = argTypes[0].unmakePointer(); // forces a load the variable
		argTypes[1] = GetRequiredCoordinateType(typeLibrary, srcImageFormat, false);

		// determine the output type based on the image format
		return typeLibrary.packedFormatElementType(argTypes[0].resource().resolvedFormat);
	}

	virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
	{
		for (uint32_t i = 0; i < retData.size(); ++i)
			retData.component(i, DataValueComponent());
	}
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoad);
	RTTI_METADATA(FunctionNameMetadata).name("imageLoad");
RTTI_END_TYPE();

//--

class FunctionTextureLoadSample : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadSample, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        if (numArgs < 3)
        {
            err.reportError(loc, "Function expects 3 arguments: (uav non-samplable texture, Pos, SampleIndex)");
            return DataType();
        }

        if (!IsLoadableImage(argTypes[0], true))
        {
            err.reportError(loc, "Only multisampled non-samplable textures can be used here");
            return DataType();
        }

        // determine required input types
        auto& srcImageFormat = argTypes[0];
        argTypes[0] = argTypes[0].unmakePointer(); // forces a load the variable
        argTypes[1] = GetRequiredCoordinateType(typeLibrary,srcImageFormat, false);
        argTypes[2] = DataType::IntScalarType();

        if (!srcImageFormat.resource().multisampled)
        {
            err.reportError(loc, "Function requires a multi sampled input");
            return DataType();
        }

        // determine the output type based on the image format
        return typeLibrary.packedFormatElementType(argTypes[0].resource().resolvedFormat);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        for (uint32_t i = 0; i < retData.size(); ++i)
            retData.component(i, DataValueComponent());
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoadSample);
    RTTI_METADATA(FunctionNameMetadata).name("imageLoadSample");
RTTI_END_TYPE();

//---

class FunctionTextureStore : public INativeFunction
{
	RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureStore, INativeFunction);

public:
	virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
	{
		if (numArgs < 4)
		{
			err.reportError(loc, "Function expects 3 arguments: (uav non-samplable texture, Pos, Value)");
			return DataType();
		}

		if (!IsLoadableImage(argTypes[0], true))
		{
			err.reportError(loc, "Only non-multisampled non-samplable UAV textures can be used here");
			return DataType();
		}

		auto& srcImageFormat = argTypes[0];

		// determine required input types
		argTypes[0] = argTypes[0].unmakePointer(); // forces a load
		argTypes[1] = GetRequiredCoordinateType(typeLibrary, srcImageFormat, false);
		argTypes[2] = typeLibrary.packedFormatElementType(argTypes[0].resource().resolvedFormat);
		return DataType(BaseType::Void);
	}

	virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
	{
		// nothing
	}
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureStore);
	RTTI_METADATA(FunctionNameMetadata).name("imageStore");
RTTI_END_TYPE();

//---

class FunctionTextureStoreSample : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureStoreSample, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        if (numArgs < 4)
        {
            err.reportError(loc, "Function expects 4 arguments: (uav-non samplable texture, Pos, SampleIndex, Value)");
            return DataType();
        }

		if (!IsLoadableImage(argTypes[0], true))
        {
            err.reportError(loc, "Only multisampled non-samplable UAV textures can be used here");
            return DataType();
        }

        auto& srcImageFormat = argTypes[0];

        // determine required input types
        argTypes[0] = argTypes[0].unmakePointer(); // forces a load
        argTypes[1] = GetRequiredCoordinateType(typeLibrary, srcImageFormat, false);
        argTypes[2] = DataType::IntScalarType();
        argTypes[3] = typeLibrary.packedFormatElementType(argTypes[0].resource().resolvedFormat);
        return DataType(BaseType::Void);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        // nothing
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureStoreSample);
    RTTI_METADATA(FunctionNameMetadata).name("imageStoreSample");
RTTI_END_TYPE();

//---

static DataType GetTextureSizeType(TypeLibrary& typeLibrary, const DataType& imageType)
{
    auto numComponents = imageType.resource().sizeComponentCount();
    return typeLibrary.unsignedType(numComponents);
}

class FunctionTextureSize : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSize, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        if (numArgs != 1)
        {
            err.reportError(loc, "Function expects 1 argument: (image)");
            return DataType();
        }

		if (!IsLoadableImage(argTypes[0]))
        {
            err.reportError(loc, "Only uav textures can be used here");
            return DataType();
        }

        // determine required input types
        auto& srcImageFormat = argTypes[0];
        argTypes[0] = argTypes[0].unmakePointer(); // forces a load
        return GetTextureSizeType(typeLibrary, srcImageFormat);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        for (uint32_t i = 0; i < retData.size(); ++i)
            retData.component(i, DataValueComponent());
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSize);
    RTTI_METADATA(FunctionNameMetadata).name("imageSize");
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
