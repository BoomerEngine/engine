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

struct TextureParamSetup
{
    int textureIndex = -1;
    //int m_samplerIndex; // eh.....
    int coordsIndex= -1;
    int gradXIndex= -1;
    int gradYIndex= -1;
    int lodBiasIndex= -1;
    int lodIndex= -1;
    int offsetIndex= -1;
    int sampleIndex= -1; // multisampled textures only
    int componentIndex= -1; // for gather
    int zComparandIndex= -1; // only shadow textures

    uint32_t numCoords = 0;
    uint32_t numOffsetCoords = 0;
    uint32_t numRequiredArgs = 0;
    uint32_t numMaxArgs = 0;

    DataType coordsType;
    DataType offsetType;
};

enum class TextureModeFlag
{
    ExplicitLOD = FLAG(0), // explicit LOD is provided
    Offset = FLAG(1), // additional offset is provided
    Grad = FLAG(2), // texture gradients are explicitly provided
    Gather = FLAG(3), // it's a gather operation
    Fetch = FLAG(4), // it's a fetch operation
    Sample = FLAG(5), // we expect MSAA texture
    Depth = FLAG(6), // we expect depth texture
    Bias = FLAG(7), // additional lod bias
};

typedef DirectFlags<TextureModeFlag> TextureModeFlags;

static DataType GetRequiredCoordinateType(TypeLibrary& typeLibrary, const DataType& imageType, bool normalized)
{
    auto numComponents = imageType.resource().addressComponentCount();

    if (normalized)
        return typeLibrary.floatType(numComponents);
    else
        return typeLibrary.integerType(numComponents);
}

static bool BuildParamSetup(TypeLibrary& typeLibrary, const char* functionName, uint32_t numArgs, const ResourceType& imageType, TextureModeFlags flags, TextureParamSetup& outSetup, const TextTokenLocation& loc, ITextErrorReporter& err)
{
    outSetup.textureIndex = 0;
    //outSetup.m_samplerIndex = 1;
    outSetup.coordsIndex = 1;

    StringBuilder argText;

    if (flags.test(TextureModeFlag::Sample))
        argText << "TextureMS";
    else
        argText << "Texture";
    argText << ", Coords";

    auto coordsInteger = flags.test(TextureModeFlag::Fetch) || flags.test(TextureModeFlag::Gather);
    auto numComponents = imageType.addressComponentCount();
    outSetup.coordsType = coordsInteger ? typeLibrary.integerType(numComponents) : typeLibrary.floatType(numComponents);
    outSetup.offsetType = typeLibrary.integerType(1);

    if (flags.test(TextureModeFlag::Offset))
    {
        auto numOffsetComponents = imageType.offsetComponentCount();
        if (0 == numOffsetComponents)
        {
            err.reportError(loc, TempString("Image format {} does not support offets", imageType));
            return false;
        }

        outSetup.offsetType = typeLibrary.integerType(numOffsetComponents);
    }

    uint32_t numRequiredArgs = 2; // texture, coord
    bool valid = true;

    // depth formats always require the z-comparand
    if (imageType.depth)
    {
        argText << ", DepthCompareValue";
        outSetup.zComparandIndex = numRequiredArgs;
        numRequiredArgs += 1;
    }

    // if we have the Sample index we must have a multisampled texture
    if (!imageType.multisampled && flags.test(TextureModeFlag::Sample))
    {
        err.reportError(loc, TempString("Function {} expects multi-sampled input but 'Texture{}' was provided", functionName, imageType));
        valid = false;
    }
    else if (imageType.multisampled && !flags.test(TextureModeFlag::Sample))
    {
        err.reportError(loc, TempString("Function {} expects non multi-sampled input but 'Texture{}' was provided", functionName, imageType));
        valid = false;
    }

    // we expect depth texture
    if (imageType.depth && !flags.test(TextureModeFlag::Depth))
    {
        err.reportError(loc, TempString("Function {} expects a non-depth texture but 'Texture{}' was provided", functionName, imageType));
        valid = false;
    }
    else if (!imageType.depth && flags.test(TextureModeFlag::Depth))
    {
        err.reportError(loc, TempString("Function {} expects a depth texture but 'Texture{}' was provided", functionName, imageType));
        valid = false;
    }

    // determine rest of the format
    if (flags.test(TextureModeFlag::Fetch))
    {
        ASSERT(!flags.test(TextureModeFlag::Gather));
        ASSERT(!flags.test(TextureModeFlag::Grad));
        ASSERT(!flags.test(TextureModeFlag::ExplicitLOD));

        if (flags.test(TextureModeFlag::Sample))
        {
            if (flags.test(TextureModeFlag::Offset))
            {
                err.reportError(loc, "Offset mode is not supported on textures with sub-samples");
                valid = false;
            }

            argText << ", SampleIndex";
            outSetup.sampleIndex = numRequiredArgs;
            numRequiredArgs += 1;
        }
        else
        {
            outSetup.lodIndex = numRequiredArgs;
            numRequiredArgs += 1;
            argText << ", LOD";

            if (flags.test(TextureModeFlag::Offset))
            {
                argText << ", Offset";
                outSetup.offsetIndex = numRequiredArgs;
                numRequiredArgs += 1;
            }
        }
    }
    else if (flags.test(TextureModeFlag::Gather))
    {
        ASSERT(!flags.test(TextureModeFlag::Fetch));
        ASSERT(!flags.test(TextureModeFlag::Grad));
        ASSERT(!flags.test(TextureModeFlag::ExplicitLOD));

        // when offset is specified it's specified now
        if (flags.test(TextureModeFlag::Offset))
        {
            outSetup.offsetIndex = numRequiredArgs;
            argText << ", Offset";
            numRequiredArgs += 1;
        }
    }
    // no fetch and no gather, maybe an explicit lod ?
    else if (flags.test(TextureModeFlag::ExplicitLOD))
    {
        ASSERT(!flags.test(TextureModeFlag::Gather));
        ASSERT(!flags.test(TextureModeFlag::Fetch));
        ASSERT(!flags.test(TextureModeFlag::Grad));

        // explicit LOD value
        outSetup.lodIndex = numRequiredArgs;
        argText << ", LOD";
        numRequiredArgs += 1;

        // when offset is specified it's specified now
        if (flags.test(TextureModeFlag::Offset))
        {
            argText << ", Offset";
            outSetup.offsetIndex = numRequiredArgs;
            numRequiredArgs += 1;
        }
    }
    // no fetch and no gather no explicit lod, maybe a gradient ?
    else if (flags.test(TextureModeFlag::Grad))
    {
        // explicit gradients
        outSetup.gradXIndex = numRequiredArgs;
        numRequiredArgs += 1;
        argText << ", GradX";
        outSetup.gradYIndex = numRequiredArgs;
        numRequiredArgs += 1;
        argText << ", GradY";

        // when offset is specified it's specified now
        if (flags.test(TextureModeFlag::Offset))
        {
            argText << ", Offset";
            outSetup.offsetIndex = numRequiredArgs;
            numRequiredArgs += 1;
        }
    }
    // no fetch, no gather, no explicit LOD, no gradients, it's an ordinary texture instruction
    else 
    { 
        // when offset is specified it's specified now
        if (flags.test(TextureModeFlag::Bias))
        {
            argText << ", Bias";
            outSetup.lodBiasIndex = numRequiredArgs;
            numRequiredArgs += 1;
        }

        // when offset is specified it's specified now
        if (flags.test(TextureModeFlag::Offset))
        {
            argText << ", Offset";
            outSetup.offsetIndex = numRequiredArgs;
            numRequiredArgs += 1;
        }
    }

    // check argument count
    if (numArgs != numRequiredArgs)
    {
        err.reportError(loc, TempString("Function {} requires {} arguments but {} arguments were provided. Expected arguments: ({})", functionName, numRequiredArgs, numArgs, argText.c_str()));
        valid = false;
    }

    // setup is valid
    return valid;
}

static DataType GetTextureOutputType(TypeLibrary& typeLibrary, ImageFormat packedFormat)
{
    return typeLibrary.packedFormatElementType(packedFormat);
}

static DataType GetTextureGatherOutputType(TypeLibrary& typeLibrary, ImageFormat imageFormat)
{
    if (GetImageFormatInfo(imageFormat).formatClass == ImageFormatClass::INT)
        return typeLibrary.integerType(4);
    if (GetImageFormatInfo(imageFormat).formatClass == ImageFormatClass::UINT)
        return typeLibrary.unsignedType(4);
    else
        return typeLibrary.floatType(4);
}

static DataType GetTextureSizeType(TypeLibrary& typeLibrary, const DataType& imageType)
{
    auto numSizeComponents = imageType.resource().sizeComponentCount();
    return typeLibrary.unsignedType(numSizeComponents);
}

static bool IsSamplableTexture(const DataType& dataType)
{
	if (!dataType.isResource())
		return false;

	const auto& res = dataType.resource();
	if (res.type != DeviceObjectViewType::SampledImage)
		return false;

	return true;
}

//---

class FunctionTextureSample_Base : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSample_Base, INativeFunction);

public:
    TextureModeFlags m_flags;

    FunctionTextureSample_Base(TextureModeFlags flags = TextureModeFlags())
        : m_flags(flags)
    {
    }

    const char* functionName() const
    {
        return cls()->findMetadataRef<FunctionNameMetadata>().name();
    }

    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const TextTokenLocation& loc, ITextErrorReporter& err) const override final
    {
        // get the image setup
        if (numArgs < 1)
        {
            err.reportError(loc, TempString("Function '{}' for sure requires at least argument", functionName()));
            return DataType();
        }

        // check if the resource type is supported
        // NOTE: some texture functions do not support some operations
        if (!IsSamplableTexture(argTypes[0]))
        {
            err.reportError(loc, TempString("Function '{}' supports only textures with samplers", functionName()));
            return DataType();
        }

        // generate setup
        TextureParamSetup paramSetup;
        if (!BuildParamSetup(typeLibrary, functionName(), numArgs, argTypes[0].resource(), m_flags, paramSetup, loc, err))
            return DataType();

        // image type
        argTypes[paramSetup.textureIndex] = argTypes[paramSetup.textureIndex]; // forces a named reference
        //argTypes[paramSetup.m_samplerIndex] = argTypes[paramSetup.m_samplerIndex].unmakePointer(); // forces a load
        argTypes[paramSetup.coordsIndex] = paramSetup.coordsType.unmakePointer();

        bool isFetch = m_flags.test(TextureModeFlag::Fetch);
        if (paramSetup.gradXIndex != -1)
            argTypes[paramSetup.gradXIndex] = paramSetup.coordsType;
        if (paramSetup.gradYIndex != -1)
            argTypes[paramSetup.gradYIndex] = paramSetup.coordsType;
        if (paramSetup.lodBiasIndex != -1)
            argTypes[paramSetup.lodBiasIndex] = DataType::FloatScalarType();
        if (paramSetup.lodIndex != -1)
            argTypes[paramSetup.lodIndex] = isFetch ? DataType::IntScalarType() : DataType::FloatScalarType();
        if (paramSetup.offsetIndex != -1)
            argTypes[paramSetup.offsetIndex] = paramSetup.offsetType;
        if (paramSetup.componentIndex != -1)
            argTypes[paramSetup.componentIndex] = DataType::IntScalarType();
        if (paramSetup.sampleIndex != -1)
            argTypes[paramSetup.sampleIndex] = DataType::IntScalarType();
        if (paramSetup.zComparandIndex != -1)
            argTypes[paramSetup.zComparandIndex] = DataType::FloatScalarType();

        // determine the output type based on the image format
        if (m_flags.test(TextureModeFlag::Depth))
            return DataType::FloatScalarType();
        else if (m_flags.test(TextureModeFlag::Gather))
            return GetTextureGatherOutputType(typeLibrary, argTypes[0].resource().resolvedFormat);
        else
            return GetTextureOutputType(typeLibrary, argTypes[0].resource().resolvedFormat);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        for (uint32_t i = 0; i < retData.size(); ++i)
            retData.component(i, DataValueComponent());
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSample_Base);
RTTI_END_TYPE();

//---

class FunctionTextureSample : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSample, FunctionTextureSample_Base);

public:
    FunctionTextureSample() : FunctionTextureSample_Base(TextureModeFlags()) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSample);
    RTTI_METADATA(FunctionNameMetadata).name("texture");
RTTI_END_TYPE();

//---

class FunctionTextureSampleOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSampleOffset, FunctionTextureSample_Base);

public:
    FunctionTextureSampleOffset() : FunctionTextureSample_Base(TextureModeFlag::Offset) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSampleOffset);
RTTI_METADATA(FunctionNameMetadata).name("textureOffset");
RTTI_END_TYPE();

//---

class FunctionTextureBiasSample : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureBiasSample, FunctionTextureSample_Base);

public:
    FunctionTextureBiasSample() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Bias)) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureBiasSample);
    RTTI_METADATA(FunctionNameMetadata).name("textureBias");
RTTI_END_TYPE();

//---

class FunctionTextureSampleBiasOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSampleBiasOffset, FunctionTextureSample_Base);

public:
    FunctionTextureSampleBiasOffset() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Bias) | TextureModeFlag::Offset) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSampleBiasOffset);
    RTTI_METADATA(FunctionNameMetadata).name("textureBiasOffset");
RTTI_END_TYPE();

//---

class FunctionTextureSampleExplicitLod : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSampleExplicitLod, FunctionTextureSample_Base);

public:
    FunctionTextureSampleExplicitLod() : FunctionTextureSample_Base(TextureModeFlag::ExplicitLOD) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSampleExplicitLod);
RTTI_METADATA(FunctionNameMetadata).name("textureLod");
RTTI_END_TYPE();

//---

class FunctionTextureSampleExplicitLodOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSampleExplicitLodOffset, FunctionTextureSample_Base);

public:
    FunctionTextureSampleExplicitLodOffset() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::ExplicitLOD) | TextureModeFlags(TextureModeFlag::Offset)) {};
    virtual bool expandCombinedSampler() const override final { return true; }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSampleExplicitLodOffset);
RTTI_METADATA(FunctionNameMetadata).name("textureLodOffset");
RTTI_END_TYPE();

//---

class FunctionTextureLoadLod : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadLod, FunctionTextureSample_Base);

public:
    FunctionTextureLoadLod() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Fetch)) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoadLod);
RTTI_METADATA(FunctionNameMetadata).name("textureLoadLod");
RTTI_END_TYPE();

//---

class FunctionTextureLoadLodOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadLodOffset, FunctionTextureSample_Base);

public:
    FunctionTextureLoadLodOffset() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Fetch) | TextureModeFlag::Offset) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoadLodOffset);
RTTI_METADATA(FunctionNameMetadata).name("textureLoadLodOffset");
RTTI_END_TYPE();

//---

class FunctionTextureLoadLodSample : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadLodSample, FunctionTextureSample_Base);

public:
    FunctionTextureLoadLodSample() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Fetch) | TextureModeFlag::Sample) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoadLodSample);
RTTI_METADATA(FunctionNameMetadata).name("textureLoadSample");
RTTI_END_TYPE();

//---

class FunctionTextureLoadLodOffsetSample : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadLodOffsetSample, FunctionTextureSample_Base);

public:
    FunctionTextureLoadLodOffsetSample() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Fetch) | TextureModeFlag::Offset | TextureModeFlag::Sample) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureLoadLodOffsetSample);
    RTTI_METADATA(FunctionNameMetadata).name("textureLoadSampleOffset");
RTTI_END_TYPE();

//---

class FunctionTextureGather : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureGather, FunctionTextureSample_Base);

public:
    FunctionTextureGather() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Gather)) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureGather);
RTTI_METADATA(FunctionNameMetadata).name("textureGather");
RTTI_END_TYPE();

//---

class FunctionTextureGatherOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureGatherOffset, FunctionTextureSample_Base);

public:
    FunctionTextureGatherOffset() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Gather) | TextureModeFlag::Offset) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureGatherOffset);
RTTI_METADATA(FunctionNameMetadata).name("textureGatherOffset");
RTTI_END_TYPE();

//---

class FunctionTextureDepthCompare : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureDepthCompare, FunctionTextureSample_Base);

public:
    FunctionTextureDepthCompare() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Depth)) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureDepthCompare);
    RTTI_METADATA(FunctionNameMetadata).name("textureDepthCompare");
RTTI_END_TYPE();

//---

class FunctionTextureDepthCompareOffset : public FunctionTextureSample_Base
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureDepthCompareOffset, FunctionTextureSample_Base);

public:
    FunctionTextureDepthCompareOffset() : FunctionTextureSample_Base(TextureModeFlags(TextureModeFlag::Depth) | TextureModeFlag::Offset) {};
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureDepthCompareOffset);
    RTTI_METADATA(FunctionNameMetadata).name("textureDepthCompareOffset");
RTTI_END_TYPE();

//---

class FunctionTextureSizeLod : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureSizeLod, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const TextTokenLocation& loc, ITextErrorReporter& err) const override final
    {
        if (numArgs != 2)
        {
            err.reportError(loc, "Function expects 2 arguments");
            return DataType();
        }

		if (!IsSamplableTexture(argTypes[0]))
        {
            err.reportError(loc, TempString("Texture function support only samplable texture (ones with sampler=XXX defined)"));
            return DataType();
        }

        auto& srcImageFormat = argTypes[0];

        // determine required input types
        argTypes[0] = argTypes[0].unmakePointer(); // forces a load
        argTypes[1] = DataType::UnsignedScalarType(); // LOD is required
        return GetTextureSizeType(typeLibrary, srcImageFormat);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        // nothing
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionTextureSizeLod);
    RTTI_METADATA(FunctionNameMetadata).name("textureSizeLod");
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
