/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\functions #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderTypeLibrary.h"

namespace rendering
{
    namespace compiler
    {
        //---

		static bool IsLoadableImage(const DataType& dataType, bool multisampled = false)
		{
			if (!dataType.isResource())
				return false;

			const auto& res = dataType.resource();
			if (res.type != DeviceObjectViewType::ImageWritable)
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

        class FunctionTextureLoadSample : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureLoadSample, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs < 3)
                {
                    err.reportError(loc, "Function expects 3 arguments: (RWTexture, Pos, SampleIndex)");
                    return DataType();
                }

                if (!IsLoadableImage(argTypes[0], true))
                {
                    err.reportError(loc, "Only multisampled uav textures can be used here");
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
            RTTI_METADATA(FunctionNameMetadata).name("texelLoadSample");
        RTTI_END_TYPE();

        //---

        class FunctionTextureStoreSample : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionTextureStoreSample, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs < 4)
                {
                    err.reportError(loc, "Function expects 4 arguments: (RWTexture, Pos, SampleIndex, Value)");
                    return DataType();
                }

				if (!IsLoadableImage(argTypes[0], true))
                {
                    err.reportError(loc, "Only multisampled uav textures can be used here");
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
            RTTI_METADATA(FunctionNameMetadata).name("texelStoreSample");
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
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function expects 1 argument: (RWTexture)");
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
            RTTI_METADATA(FunctionNameMetadata).name("texelSize");
        RTTI_END_TYPE();

        //---

    } // shader
} // rendering