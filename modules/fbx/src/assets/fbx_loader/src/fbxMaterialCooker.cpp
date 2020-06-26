/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"
#include "fbxFileData.h"
#include "fbxFileLoaderService.h"
#include "fbxMaterialCooker.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"
#include "base/containers/include/inplaceArray.h"
#include "rendering/material/include/renderingMaterialInstance.h"

//#include "rendering/assets/include/staticTexture.h"
//#include "rendering/runtime/include/renderingMaterialTemplate.h"
//#include "rendering/runtime/include/renderingMaterialInstance.h"

namespace fbx
{

    //--

    base::ConfigProperty<base::StringBuf> cvLitMaterialTemplate("Assets.FBX", "DefaultLitMaterialTemplate", "modules/fbx/materials/default_lit.v4mi");
    base::ConfigProperty<base::StringBuf> cvUnlitMaterialTemplate("Assets.FBX", "DefaultUnlitMaterialTemplate", "modules/fbx/materials/default_unlit.v4mi");

    //--

    // Property 'ShadingModel', type KString
    // Property 'MultiLayer', type bool
    // Property 'EmissiveColor', type Color
    // Property 'EmissiveFactor', type Number
    // Property 'AmbientColor', type Color
    // Property 'AmbientFactor', type Number
    // Property 'DiffuseColor', type Color
    // Property 'DiffuseFactor', type Number
    // Property 'Bump', type Vector
    // Property 'NormalMap', type Vector
    // Property 'BumpFactor', type Number
    // Property 'TransparentColor', type Color
    // Property 'TransparencyFactor', type Number
    // Property 'DisplacementColor', type Color
    // Property 'DisplacementFactor', type Number
    // Property 'VectorDisplacementColor', type Color
    // Property 'VectorDisplacementFactor', type Number
    // Property 'SpecularColor', type Color
    // Property 'SpecularFactor', type Number
    // Property 'ShininessExponent', type Number
    // Property 'ReflectionColor', type Color
    // Property 'ReflectionFactor', type Number
    // Property 'Emissive', type Vector
    // Property 'Ambient', type Vector
    // Property 'Diffuse', type Vector
    // Property 'Specular', type Vector
    // Property 'Shininess', type Number
    // Property 'Opacity', type Number
    // Property 'Reflectivity', type Number

    //--

    RTTI_BEGIN_TYPE_CLASS(MaterialCooker);
        //RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::runtime::MaterialInstance>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(0);
    RTTI_END_TYPE();

    //--

    static bool TryReadTexturePath(FbxSurfaceMaterial& material, const char* propertyName, base::StringBuf& outPath)
    {
        auto prop = material.FindProperty(propertyName, false);
        if (!prop.IsValid())
            return false;

        auto numTextures = prop.GetSrcObjectCount<FbxTexture>();
        if (0 == numTextures)
            return false;

        auto lTex  = prop.GetSrcObject<FbxFileTexture>(0);
        if (!lTex)
            return false;

        outPath = lTex->GetFileName();
        return true;
    }

    /*static bool TryBindTexture(base::res::IResourceCookerInterface& cooker, FbxSurfaceMaterial& material, const char* propertyName, base::StringView<char> engineNames, rendering::runtime::MaterialInstance& outMaterial, base::StringBuf* outPath = nullptr)
    {
        base::StringBuf path;
        if (!TryReadTexturePath(material, propertyName, path))
            return false;

        base::StringBuf fileSystemPath;
        if (!cooker.findFile(path, fileSystemPath, 3))
        {
            path.replaceChar('\\', '/');

            auto fileName = path.stringAfterLast("/");
            base::StringBuf textureFileName = base::TempString("textures/{}", fileName);

            if (!cooker.findFile(textureFileName , fileSystemPath, 2))
            {
                return false;
            }
        }

        // build the resource path
        auto resourcePath = base::res::ResourcePath(fileSystemPath);

        // write path
        if (outPath)
            *outPath = fileSystemPath;

        // bind to all specified names
        base::InplaceArray<base::StringView<char>, 20> enginePropertyNames;
        engineNames.slice(";", false, enginePropertyNames);

        //for (auto& name : enginePropertyNames)
          //  outMaterial.materialTexture(name, resourcePath);

        return true;
    }*/

    //--

    MaterialCooker::MaterialCooker()
    {}

    base::res::ResourceHandle MaterialCooker::cook(base::res::IResourceCookerInterface& cooker) const
    {
        // load the FBX scene
        auto importedScene = base::GetService<FileLoadingService>()->loadScene(cooker);
        if (!importedScene)
        {
            TRACE_ERROR("Failed to load scene from import file");
            return false;
        }

        // get all materials
        FbxArray<FbxSurfaceMaterial*> allMaterials;
        importedScene->sceneData()->FillMaterialArray(allMaterials);

        // oh well, no materials (may happen if we try to load an animation...)
        if (allMaterials.Size() == 0)
        {
            TRACE_WARNING("No materials found in FBX file");
            return false;
        }

        // find material by name
        base::StringBuf materialName = allMaterials[0]->GetName();
        //materialName = base::StringBuf(cooker.queryResourcePath().param("Material", materialName));
        TRACE_INFO("Loaded {} materials, importing {}", allMaterials.Size(), materialName);

        // find the material in the list
        FbxSurfaceMaterial* materialToImport = nullptr;
        for (int i=0; i<allMaterials.Size(); ++i)
        {
            if (materialName == allMaterials[i]->GetName())
            {
                materialToImport = allMaterials[i];
                break;
            }
        }

        // not found
        if (!materialToImport)
        {
            TRACE_ERROR("Material '{}' not found in FBX file", materialName);
            return false;
        }

        // setup params
        auto materialParams = base::CreateSharedPtr<rendering::MaterialInstance>();

        // get shading model
        auto shadingModel = materialToImport->ShadingModel.Get();
        TRACE_INFO("Material '{}' uses shading model '{}'", materialName, shadingModel.Buffer());
        if (shadingModel != "Unlit" || shadingModel != "unlit" || shadingModel != "flat")
        {
            // use the lit material template
            //materialParams.materialTemplate(manifest->m_defaultLitTemplate.loadAndConvertToSyncRef());
            //materialParams->baseMaterial(manifest->m_defaultLitTemplate);
        }
        else
        {
            // fallback to unlit
            //materialParams->baseMaterial(manifest->m_defaultUnlitTemplate);
            //materialParams.materialTemplate(manifest->m_defaultUnlitTemplate.loadAndConvertToSyncRef());
        }

        // load diffuse map
        base::StringBuf diffusePath;
        //if (!TryBindTexture(cooker, *materialToImport, "DiffuseColor", "BaseColorMap;DiffuseMap", *materialParams, &diffusePath))
          //  TryBindTexture(cooker, *materialToImport, "Diffuse", "BaseColorMap;DiffuseMap", *materialParams, &diffusePath);

        // load normal map
        //if (!TryBindTexture(cooker, *materialToImport, "NormalMap", "NormalMap", *materialParams))
          //  TryBindTexture(cooker, *materialToImport, "Bump", "NormalMap", *materialParams);

        // if we have a valid diffuse defined try to auto find other material maps (ie. find normalmap, etc)
        if (!diffusePath.empty())
        {
            // split the path
            auto path = diffusePath.view().beforeLast("/");
            auto file = diffusePath.view().afterLast("/").beforeLast(".");
            auto coreFile = file.beforeLastOrFull("_"); // strip the _D
            auto ext = diffusePath.view().afterLast(".");

            // get the material template we chose to use
            //if (auto templatePtr = materialParams->resolveBaseMaterial().get())
            {
                // look at the params
                /*for (auto& paramInfo : templatePtr->parametersInfo())
                {
                    // oh well
                    if (paramInfo.m_automaticTextureSuffix.empty())
                        continue;

                    // already defined
                    if (materialParams.hasParam(paramInfo.name))
                        continue;

                    // get list of potential suffixes for this param
                    base::InplaceArray<base::StringView<char>, 10> suffixes;
                    paramInfo.m_automaticTextureSuffix.view().slice(";", false, suffixes);

                    // try each suffix
                    for (auto& suffix : suffixes)
                    {
                        // try path with core name
                        {
                            // assemble a full path
                            base::StringBuilder pathBuilder;
                            pathBuilder << path << "/" << coreFile << suffix << "." << ext;

                            // do we have this file ?
                            uint64_t crc;
                            auto fullPath = pathBuilder.toString();
                            TRACE_INFO("Testing '{}'", fullPath.c_str());
                            if (cooker.queryFileCRC(fullPath, crc))
                            {
                                TRACE_INFO("Found automatic texture for {} at '{}'", paramInfo.name, fullPath);
                                materialParams.materialTexture(paramInfo.name.view(), base::res::ResourcePath(fullPath.c_str()));
                                break;
                            }
                        }

                        // try path with full name (in case diffuse was without suffix)
                        {
                            // assemble a full path
                            base::StringBuilder pathBuilder;
                            pathBuilder << path << "/" << file << suffix << "." << ext;

                            // do we have this file ?
                            uint64_t crc;
                            auto fullPath = pathBuilder.toString();
                            TRACE_INFO("Testing '{}'", fullPath.c_str());
                            if (cooker.queryFileCRC(fullPath, crc))
                            {
                                TRACE_INFO("Found automatic texture for {} at '{}'", paramInfo.name, fullPath);
                                materialParams.materialTexture(paramInfo.name.view(), base::res::ResourcePath(fullPath.c_str()));
                                break;
                            }
                        }
                    }
                }*/
            }
        }

        // process all properties
        for (auto prop = materialToImport->GetFirstProperty(); prop.IsValid(); prop = materialToImport->GetNextProperty(prop))
        {
            base::StringBuf path;
            if (TryReadTexturePath(*materialToImport, prop.GetName(), path))
            {
                TRACE_INFO("Property '{}', type {} = '{}'", prop.GetNameAsCStr(), prop.GetPropertyDataType().GetName(), path);
            }
            else
            {
                TRACE_INFO("Property '{}', type {}", prop.GetNameAsCStr(), prop.GetPropertyDataType().GetName());
            }
        }

        return materialParams;
    }

} // fbx

