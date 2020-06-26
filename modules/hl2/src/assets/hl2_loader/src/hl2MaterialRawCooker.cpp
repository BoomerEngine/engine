/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2MaterialFile.h"

#include "base/containers/include/stringBuilder.h"
#include "base/io/include/ioFileHandle.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/parser/include/textParser.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace hl2
{
#if 0
    // a rendering material compiler for materials based on lua cooker.m_servicesript
    class MaterialCompilerVMT : public rendering::runtime::IMaterialCompilationCallback
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialCompilerVMT, rendering::runtime::IMaterialCompilationCallback);

    public:
        MaterialCompilerVMT(const base::VariantTable& params, const base::RefPtr<rendering::runtime::ShaderLibrary>& shaders)
            : params(params)
            , m_shaders(shaders)
        {}

        virtual CAN_YIELD base::RefPtr<rendering::runtime::MaterialRenderingTechnique> compileTechniqueSet(const base::VariantTable& params) const override
        {
            auto techniqueSet = base::CreateSharedPtr<rendering::runtime::MaterialRenderingTechnique>();

            // create a default rendering technique
            techniqueSet->m_isTwoSided = params.get<bool>("TwoSidedRendering"_id, false);
            techniqueSet->m_isTransparent = params.get<bool>("Transparent"_id, false);
            techniqueSet->m_isMasked = params.get<bool>("Masked"_id, false);

            // use shaders
            if (m_shaders)
            {
                // use the shaders during rendering
                techniqueSet->m_shaders = m_shaders->runtimeObject();

                // compile param table
                for (auto &descriptorName : m_shaders->materialLayoutList())
                {
                    // compile parameter table
                    auto compiledParams = rendering::runtime::CompiledParamTable::Compile(params, params, m_shaders->sourceData(), descriptorName);
                    if (compiledParams)
                        techniqueSet->m_parameters.pushBack(compiledParams);
                }
            }

            // add selectors
            if (techniqueSet->m_isTwoSided)
            {
                static rendering::pipeline::PermutationKey twoSidedFlag("FLAG_TWOSIDED");
                techniqueSet->m_additionalSelector.key(twoSidedFlag, techniqueSet->m_isTwoSided ? 1 : 0);
            }

            // render the default technique in the forward pass
            return techniqueSet;
        }

    private:
        base::VariantTable params;
        base::RefPtr<rendering::runtime::ShaderLibrary> m_shaders;
    };

#endif

    ///---

    /// cooker for creating materials from a VMT file
    class MaterialRawCookerVMT : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialRawCookerVMT, base::res::IResourceCooker);

    public:
        MaterialRawCookerVMT()
        {}

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override
        {
            // load raw content
            auto materialScriptPath = cooker.queryResourcePath().path();
            auto rawContent = cooker.loadToBuffer(materialScriptPath);
            if (!rawContent)
                return false;

            /*// get the output material template
            auto materialTemplate = base::rtti_cast<rendering::content::MaterialTemplate>(createdResource);
            if (!materialTemplate)
                return false;

            // load content
            auto doc = vmt::MaterialDocument::ParseFromData(cooker, rawContent);
            if (!doc)
                return false;

            // get the path of the resource we are loading so we can use it as a shader
            auto selfPath = cooker.queryResourcePath();
            auto selfShaders = base::LoadResource<rendering::runtime::ShaderLibrary>(selfPath);

            // collect the parameters that are understood by the shader
            base::VariantTable params;
            doc->createTextureParam("basetexture", cooker, params);
            doc->createTextureParam("detail", cooker, params);
            doc->createFloatParam("detailscale", cooker, params, 1.0f);
            doc->createFloatParam("detailblendfactor", cooker, params, 1.0f);
            doc->createFloatParam("detailblendmode", cooker, params, 0);
            doc->createFloatParam("AlphaTest", cooker, params, 0);
            doc->createFloatParam("AlphaTestReference", cooker, params, 0.5f);
            doc->createFloatParam("AllowAlphaToCoverage", cooker, params, 1.0f);

            // create a compiler around the function
            auto runtimeCompiler = base::CreateSharedPtr<MaterialCompilerVMT>(params, selfShaders);

            // setup material template content
            materialTemplate->content(base::Array<rendering::content::MaterialTemplateParamInfo>(), runtimeCompiler);
            return true;*/

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialRawCookerVMT);
        //RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::runtime::IMaterialTemplate>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("vmt");
    RTTI_END_TYPE();

} // hl2