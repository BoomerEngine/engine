/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2MaterialFile.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

namespace hl2
{
    ///---

#if 0
    namespace prv
    {

        static void GenerateMaterialCode(rendering::pipeline::DynamicCompiler& dc, const vmt::MaterialDocument& doc)
        {
            // general stuff
            auto uv = dc.runInputFunction("PrimaryUV", rendering::pipeline::DynamicCodeChunkDataType::Float2);

            // defaults
            auto diffuseColor = dc.constant(base::Vector4(1.0f, 1.0f, 0.0f, 1.0));//::ONE());

            // consts
            auto one = dc.constant(1.0f);
            auto half = dc.constant(0.5f);
            auto two = dc.constant(2.0f);

            // get the diffuse color
            if (doc.hasParam("basetexture"))
            {
                auto diffuseMap = dc.requestParam("basetexture", rendering::pipeline::DynamicCodeChunkDataType::Texture2D);
                diffuseColor = dc.sampleTexture(diffuseMap, uv);
                //diffuseColor = dc.swizzle(uv, "xy01");
            }

            // detail texture
            if (doc.hasParam("detail"))
            {
                auto detailScale = dc.requestParam("detailscale", rendering::pipeline::DynamicCodeChunkDataType::Float);
                auto detailBlendFactor = dc.requestParam("detailblendfactor", rendering::pipeline::DynamicCodeChunkDataType::Float);
                auto detailMap = dc.requestParam("detail", rendering::pipeline::DynamicCodeChunkDataType::Texture2D);

                auto detailBlendMode = doc.paramInt("detailblendmode");
                if (0 == detailBlendMode)
                {
                    auto detailMapColor = dc.sampleTexture(detailMap, dc.mul(uv, detailScale));
                    auto biasedDetailMap = dc.mul(dc.mul(detailMapColor, two), detailBlendFactor);
                    diffuseColor = dc.emitLocal(dc.mul(diffuseColor, biasedDetailMap));
                }
            }

            // alpha test
            /*if (doc.hasParam("AlphaTest") && doc.paramInt("AlphaTest", 0))
            {
                auto diffuseMap = dc.requestParam("basetexture", rendering::pipeline::DynamicCodeChunkDataType::Texture2D);
                auto alpha = dc.swizzle(diffuseMap, "w");
                //dc.
                //auto
            }*/

            // export results
            dc.emitOutput("outDiffuseColor", diffuseColor);
        }

    } // prv

#endif

    ///---

    /// cooker for creating material shader files from a VMT file
    class MaterialShaderCookerVMT : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialShaderCookerVMT, base::res::IResourceCooker);

    public:
        MaterialShaderCookerVMT()
        {}

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override
        {
            // load raw content
            const auto& materialScriptPath = cooker.queryResourcePath();
            auto rawContent = cooker.loadToBuffer(materialScriptPath);
            if (!rawContent)
                return false;

            // get the output shader data
            /*auto shaderFile = base::rtti_cast<rendering::PipelineLibrary>(createdResource);
            if (!shaderFile)
                return false;

            // load content
            auto doc = vmt::MaterialDocument::ParseFromData(cooker, rawContent);
            if (!doc)
                return false;

            // load primary template code based on the material
            base::StringBuf templateFullPath = base::TempString("/engine/materials/hl2interop/{}.csl", doc->name.toLower());
            base::StringBuf templateCode;
            if (!cooker.loadToString(templateFullPath, templateCode))
            {
                TRACE_ERROR("Missing interop material template '{}'", doc->name);
                return false;
            }

            // prepare dynamic compiler
            rendering::pipeline::DynamicCompiler dc;
            prv::GenerateMaterialCode(dc, *doc);

            // generate code to compile
            base::StringBuilder finalCode;
            dc.generateCode(templateCode, finalCode);
            TRACE_SPAM("Dynamic code: {}", finalCode.c_str());

            // compile
            base::StringID generatorName = "glsl"_id; // TODO
            base::StringBuf contextName = cooker.queryResourcePath().path();
            return rendering::pipeline::CompileShader(cooker, generatorName, templateFullPath, finalCode.toString(), shaderFile);*/
            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialShaderCookerVMT);
        //RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::runtime::PipelineLibraryFile>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("vmt");
    RTTI_END_TYPE();

} // hl2