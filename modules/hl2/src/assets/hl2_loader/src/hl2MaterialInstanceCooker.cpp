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
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/parser/include/textParser.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

namespace hl2
{
    ///---

    /// cooker for creating material instances a VMT file
    class MaterialInstanceCookerVMT : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceCookerVMT, base::res::IResourceCooker);

    public:
        MaterialInstanceCookerVMT()
            //: m_defaultLitTemplate(base::res::ResourcePath("engine/materials/templates/pbr_simple.v4mat"), rendering::content::MaterialTemplate::GetStaticClass())
            //, m_defaultUnlitTemplate(base::res::ResourcePath("engine/materials/templates/unlit.v4mat"), rendering::content::MaterialTemplate::GetStaticClass())
        {}

        /*bool textureParam(const vmt::MaterialDocument& doc, const char* paramName, const char* materialParamName, base::res::IResourceCookerInterface& cooker, rendering::runtime::MaterialInstance& outParams) const
        {
            // get texture path
            auto path = doc.param(paramName);
            if (path.empty())
                return false;

            // get full path
            base::StringBuf hl2FullPath = base::TempString("materials/{}.vtf", path.toLower());

            // resolve path
            base::StringBuf fullPath;
            if (cooker.queryResolvedPath(hl2FullPath, cooker.queryResourcePath().path(), false, fullPath))
            {
                //base::res::Ref<rendering::runtime::ITexture> textureRef;
                //textureRef.set(base::res::ResourcePath(fullPath));
                //outParams.writeParameter(materialParamName, texturePath);
                return true;
            }

            return false;
        }*/

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override
        {
            // load raw content
            const auto& materialScriptPath = cooker.queryResourcePath();
            auto rawContent = cooker.loadToBuffer(materialScriptPath);
            if (!rawContent)
                return false;

            // load content
            auto doc = vmt::MaterialDocument::ParseFromData(cooker, rawContent);
            if (!doc)
                return false;

            // return resource
            auto ret = nullptr;// base::RefNew<rendering::runtime::MaterialInstance>();

            // get the name of the material template
            /*rendering::content::MaterialInstanceParamInfoSet materialParams;
            auto materialName = doc->name;
            if (materialName == "unlitgeneric")
            {
                materialParams.materialTemplate(m_defaultUnlitTemplate.loadAndConvertToSyncRef());
                textureParam(*doc, "basetexture", "DiffuseMap", cooker, materialParams);
            }
            else
            {
                materialParams.materialTemplate(m_defaultLitTemplate.loadAndConvertToSyncRef());
                textureParam(*doc, "basetexture", "DiffuseMap", cooker, materialParams);
            }*/

            /*
            // collect the parameters that are understood by the shader
            base::VariantTable params;
            doc->createTextureParam("basetexture", ctx, params);
            doc->createTextureParam("detail", ctx, params);
            doc->createFloatParam("detailscale", ctx, params, 1.0f);
            doc->createFloatParam("detailblendfactor", ctx, params, 1.0f);
            doc->createFloatParam("detailblendmode", ctx, params, 0);
            doc->createFloatParam("AlphaTest", ctx, params, 0);
            doc->createFloatParam("AlphaTestReference", ctx, params, 0.5f);
            doc->createFloatParam("AllowAlphaToCoverage", ctx, params, 1.0f);*/

            return ret;
        }

    private:
        // material templates
        //rendering::MaterialTemplateAsyncRef m_defaultUnlitTemplate;
        //rendering::MaterialTemplateAsyncRef m_defaultLitTemplate;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialInstanceCookerVMT);
        //RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::runtime::MaterialInstance>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("vmt");
    RTTI_END_TYPE();

} // hl2