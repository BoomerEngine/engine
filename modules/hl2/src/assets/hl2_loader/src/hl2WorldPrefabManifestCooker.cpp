/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"

#include "base/io/include/ioFileHandle.h"
#include "base/io/include/timestamp.h"
#include "base/io/include/utils.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resourceStaticResource.h"
#include "base/parser/include/textParser.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/memoryReader.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/reflection/include/variantTable.h"

#include "hl2WorldBSPFile.h"

namespace hl2
{

    extern const base::Matrix HL2Transform;

    /// manifest for cooking a prefab from a BSP file
    class WorldPrefabCookerManifestBSP : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldPrefabCookerManifestBSP, base::res::IResourceCooker);

    public:
        WorldPrefabCookerManifestBSP()
        {
        }

        struct Entity : public base::IReferencable
        {
            base::VariantTable params;
        };

        typedef base::Array<base::RefPtr<Entity>> Entities;

        bool parseType(base::StringView<char> txt, base::Vector3& outVector) const
        {
            base::StringParser parser(txt);
            if (!parser.parseFloat(outVector.x)) return false;
            if (!parser.parseFloat(outVector.y)) return false;
            if (!parser.parseFloat(outVector.z)) return false;
            return true;
        }

        static void SinCos(float x, float* outSin, float* outCos)
        {
            *outSin = sin(DEG2RAD * x);
            *outCos = cos(DEG2RAD * x);
        }

        static void HL2AngleToQuaternion(float x, float y, float z, base::Quat &outQuat)
        {
            float sr, sp, sy, cr, cp, cy;

            SinCos( z * 0.5f, &sy, &cy );
            SinCos( y * 0.5f, &sp, &cp );
            SinCos( x * 0.5f, &sr, &cr );

            float srXcp = sr * cp, crXsp = cr * sp;
            outQuat.x = srXcp*cy-crXsp*sy;
            outQuat.y = crXsp*cy+srXcp*sy;

            float crXcp = cr * cp, srXsp = sr * sp;
            outQuat.z = crXcp*sy-srXsp*cy;
            outQuat.w = crXcp*cy+srXsp*sy;

            outQuat.x = -outQuat.x;
            outQuat.z = -outQuat.z;
        }

        bool parseType(base::StringView<char> txt, base::Quat& outQuaternion) const
        {
            base::StringParser parser(txt);
            base::Vector3 rot;
            if (!parser.parseFloat(rot.x)) return false;
            if (!parser.parseFloat(rot.y)) return false;
            if (!parser.parseFloat(rot.z)) return false;

            HL2AngleToQuaternion(rot.z, rot.x, rot.y, outQuaternion);
            return true;
        }

        bool parseType(base::StringView<char> txt, base::Color& outColor) const
        {
            base::StringParser parser(txt);
            if (!parser.parseUint8(outColor.r)) return false;
            if (!parser.parseUint8(outColor.g)) return false;
            if (!parser.parseUint8(outColor.b)) return false;
            return true;
        }

        bool parseType(base::StringView<char> txt, float& outFloat) const
        {
            base::StringParser parser(txt);
            if (!parser.parseFloat(outFloat)) return false;
            return true;
        }

        bool parseType(base::StringView<char> txt, int& outInt) const
        {
            base::StringParser parser(txt);
            if (!parser.parseInt32(outInt)) return false;
            return true;
        }

        bool parseEntities(base::StringView<char> txt, Entities& outEntities) const
        {
            // setup parser
            base::parser::TextParser parser("Entities", base::parser::IErrorReporter::GetDefault(), base::parser::ICommentEater::StandardComments());
            parser.reset(base::StringView<char>(txt));
            base::io::SaveFileFromString(base::io::AbsolutePath::Build(L"/home/rexdex/entities.txt"), txt);

            // typed parameter lists
            base::HashSet<base::StringID> floatParams, intParams, colorParams, vectorParams, angleParams;
            vectorParams.insert("origin"_id);
            angleParams.insert("angles"_id);
            colorParams.insert("rendercolor"_id);
            intParams.insert("startdisabled"_id);
            intParams.insert("disableshadows"_id);
            intParams.insert("disablereceiveshadows"_id);

            // process the entities
            while (parser.parseKeyword("{", false))
            {
                auto entity = base::CreateSharedPtr<Entity>();
                for (;;)
                {
                    if (parser.parseKeyword("}", false))
                    {
                        outEntities.pushBack(entity);
                        break;
                    }

                    base::StringView<char> name;
                    if (!parser.parseString(name, false, true))
                    {
                        parser.error(base::TempString("Missing entity parameter name"));
                        return false;
                    }

                    base::StringView<char> valueStr;
                    if (!parser.parseString(valueStr, false, true))
                    {
                        parser.error(base::TempString("Missing entity parameter value"));
                        return false;
                    }

                    auto paramName = base::StringID(base::StringBuf(name).toLower().c_str());
                    if (vectorParams.contains(paramName))
                    {
                        base::Vector3 value;
                        if (parseType(valueStr, value))
                            entity->params.setValue(paramName, value);
                    }
                    else if (angleParams.contains(paramName))
                    {
                        base::Quat value;
                        if (parseType(valueStr, value))
                            entity->params.setValue(paramName, value);
                    }
                    else if (colorParams.contains(paramName))
                    {
                        base::Color value;
                        if (parseType(valueStr, value))
                            entity->params.setValue(paramName, value);
                    }
                    else if (floatParams.contains(paramName))
                    {
                        float value;
                        if (parseType(valueStr, value))
                            entity->params.setValue(paramName, value);
                    }
                    else if (intParams.contains(paramName))
                    {
                        int value;
                        if (parseType(valueStr, value))
                            entity->params.setValue(paramName, value);
                    }
                    else
                    {
                        entity->params.setValue(paramName, valueStr);
                    }
                }
            }

            TRACE_INFO("Parsed {} entities", outEntities.size());
            return true;
        }

        base::RefPtr<bsp::File> loadData(base::res::IResourceCookerInterface& cooker) const
        {
            base::Task task(2, "Loading from cache");

            // get the file name part
            auto sourceFilePath = cooker.queryResourcePath().path();

            // load stuff from cache
            /*return cooker.cacheData<bsp::File>(sourceFilePath, "BSPData", [sourceFilePath, &cooker]() -> base::RefPtr<bsp::File>
            {
                return bsp::File::LoadFromFile(cooker);
            });*/
            return nullptr;
        }

        /*scene::NodeTemplatePtr setupGenericNode(const scene::NodeTemplatePtr &node, uint32_t index, const base::VariantTable& ent) const
        {
            if (node)
            {
                // get class name
                auto className = ent.get("classname"_id, base::StringBuf::EMPTY());

                // get name
                auto name = ent.get("name"_id, base::StringBuf::EMPTY());
                if (name.empty())
                    name = base::TempString("{}{}", className, index);
                node->name(base::StringID(name.c_str()));

                // get transform params
                auto position = ent.get<base::Vector3>("origin"_id, base::Vector3::ZERO());
                auto rotation = ent.get<base::Quat>("angles"_id, base::Quat::IDENTITY());

                // assemble transform
                scene::NodeTemplatePlacement nodeTransform;
                nodeTransform.T = HL2Transform.transformPoint(position);
                nodeTransform.R = rotation.toRotator();
                node->placement(nodeTransform);

                // hide triggers
                //if (className == "trigger_multiple" || className == "trigger_once" || className.beginsWith("trigger_"))
                  //  node->visibilityEditorFlag(false);

                //if (ent.get<int>("startdisabled"_id, 0))
                  //  node->visibilityEditorFlag(false);

                if (auto meshNode = base::rtti_cast<scene::MeshNodeTemplate>(node))
                {
                    if (ent.get<int>("disableshadows"_id, 0))
                        meshNode->toggleShadowCasting(false);

                    if (ent.get<int>("disablereceiveshadows"_id, 0))
                        meshNode->toggleShadowInteractions(false);
                }
            }

            return node;
        }

        scene::NodeTemplatePtr createEntityNode(base::res::IResourceCookerInterface& cooker, const bsp::File& bspData, uint32_t index, const base::VariantTable& ent) const
        {
            // get class name
            auto className = ent.get("classname"_id, base::StringBuf::EMPTY());

            // HACK: get the root path (mounting of the VFS)
            auto sourceFilePath = cooker.m_cooker.queryResourcePath().path();
            auto rootPathToDepot = sourceFilePath.stringBeforeFirst("/maps/");

            // logic
            if (className == "worldspawn")
            {
                auto node = base::CreateSharedPtr<scene::MeshNodeTemplate>();

                auto meshPath = base::res::ResourcePath(base::TempString("{}:Model=0", sourceFilePath).c_str());
                auto meshRef = rendering::MeshAsyncRef(meshPath, rendering::content::Mesh::GetStaticClass());
                node->meshRef(meshRef);
                node->visibilityDistanceOverride(10000.0f);
                node->toggleTwoSidedShadows(true);

                return setupGenericNode(node, index, ent);
            }
            else if (className == "prop_dynamic" || className == "prop_physics" || className == "prop_door_rotating")
            {
                auto modelPath = ent.get("model"_id, base::StringBuf::EMPTY());
                if (modelPath.empty())
                    return nullptr;

                auto node = base::CreateSharedPtr<scene::MeshNodeTemplate>();

                auto meshPath = base::res::ResourcePath(base::TempString("{}/{}", rootPathToDepot, modelPath).c_str());
                auto meshRef = rendering::MeshAsyncRef(meshPath, rendering::content::Mesh::GetStaticClass());
                node->meshRef(meshRef);

                return setupGenericNode(node, index, ent);
            }

            // try to create auto node for a model shit
            auto model = ent.get("model"_id, base::StringBuf::EMPTY());
            if (model.beginsWith("*"))
            {
                auto modelIndexStr = model.subString(1);
                uint32_t modelIndex = 0;
                if (base::MatchResult::OK == modelIndexStr.view().match(modelIndex))
                {
                    auto node = base::CreateSharedPtr<scene::MeshNodeTemplate>();

                    auto meshPath = base::res::ResourcePath(base::TempString("{}:Model={}", sourceFilePath, modelIndex).c_str());
                    auto meshRef = rendering::MeshAsyncRef(meshPath);
                    node->meshRef(meshRef);

                    if (!setupGenericNode(node, modelIndex, ent))
                        return nullptr;

                    //auto brushOffset = HL2Transform.transformPoint(bspData.models()[modelIndex].origin);
                    //node->relativePosition(node->relativePosition() + brushOffset);

                    return node;
                }
            }
            else if (model.endsWith(".mdl"))
            {
                auto modelPath = ent.get("model"_id, base::StringBuf::EMPTY());
                if (modelPath.empty())
                    return nullptr;

                auto node = base::CreateSharedPtr<scene::MeshNodeTemplate>();

                auto meshPath = base::res::ResourcePath(base::TempString("{}/{}", rootPathToDepot, modelPath).c_str());
                auto meshRef = rendering::MeshAsyncRef(meshPath, rendering::content::Mesh::GetStaticClass());
                node->meshRef(meshRef);

                return setupGenericNode(node, index, ent);
            }

            // no entity
            return nullptr;
        }

        bool createStaticPros(scene::Prefab& prefab, base::res::IResourceCookerInterface& cooker, const bsp::File& bsp) const
        {
            // HACK: get the root path (mounting of the VFS)
            auto sourceFilePath = cooker.m_cooker.queryResourcePath().path();
            auto rootPathToDepot = sourceFilePath.stringBeforeFirst("/maps/");

            // get data
            auto spropData = bsp.gameLumpData(0x73707270);
            if (!spropData)
                return true;

            // get version of the data
            auto spropVersion = bsp.gameLumpVersion(0x73707270);
            uint32_t dataSizeForVersion = 0;

            // process the data
            base::stream::MemoryReader reader(spropData);

            // load the dictionary count
            uint32_t numMeshes = 0;
            reader >> numMeshes;
            TRACE_INFO("Found {} meshes in static prop dictionary", numMeshes);

            // prepare the mesh references
            base::Array<rendering::MeshAsyncRef> meshes;
            for (uint32_t i=0; i<numMeshes; ++i)
            {
                bsp::StaticPropDictLump_t data;
                reader.read(&data, sizeof(data));

                auto meshResourcePath = base::res::ResourcePath(base::TempString("{}/{}", rootPathToDepot, data.m_Name).c_str());
                TRACE_INFO("Mesh[{}]: '{}'", i, meshResourcePath);

                meshes.pushBack(rendering::MeshAsyncRef(meshResourcePath));
            }

            // skip leafs
            uint32_t numLeafs = 0;
            reader >> numLeafs;
            TRACE_INFO("Found {} leafs in static prop data", numLeafs);
            reader.seek(reader.pos() + sizeof(bsp::StaticPropLeafLump_t) * numLeafs);

            // load objets
            uint32_t numObjects = 0;
            reader >> numObjects;
            TRACE_INFO("Found {} objects in static prop data", numLeafs);
            for (uint32_t i=0; i<numObjects; ++i)
            {
                bsp::StaticPropLump_t data;
                memset(&data, 0, sizeof(bsp::StaticPropLump_t));

                if (spropVersion >= 4)
                {
                    reader >> data.Origin.x;
                    reader >> data.Origin.y;
                    reader >> data.Origin.z;
                    reader >> data.Angles.x;
                    reader >> data.Angles.y;
                    reader >> data.Angles.z;
                    reader >> data.PropType;
                    reader >> data.FirstLeaf;
                    reader >> data.LeafCount;
                    reader >> data.Solid;
                    reader >> data.Flags;
                    reader >> data.Skin;
                    reader >> data.FadeMinDist;
                    reader >> data.FadeMaxDist;
                    reader >> data.LightingOrigin.x;
                    reader >> data.LightingOrigin.y;
                    reader >> data.LightingOrigin.z;
                }

                if (spropVersion >= 5)
                {
                    reader >> data.ForcedFadeScale;
                }

                if (spropVersion == 6 || spropVersion == 7)
                {
                    reader >> data.MinDXLevel;
                    reader >> data.MaxDXLevel;
                }

                if (spropVersion >= 8)
                {
                    reader >> data.MinCPULevel;
                    reader >> data.MaxCPULevel;
                    reader >> data.MinGPULevel;
                    reader >> data.MaxGPULevel;
                }

                if (spropVersion >= 7)
                {
                    reader >> data.DiffuseModulation;
                }

                if (spropVersion >= 9 && spropVersion <= 10)
                {
                    reader >> data.DisableX360;
                }

                if (spropVersion >= 10)
                {
                    reader >> data.FlagsEx;
                }

                if (spropVersion >= 11)
                {
                    reader >> data.UniformScale;
                }
                else
                {
                    data.UniformScale = 1.0f;
                }

                if (data.PropType < meshes.size())
                {
                    auto node = base::CreateSharedPtr<scene::MeshNodeTemplate>();
                    node->meshRef(meshes[data.PropType]);

                    // convert rotation
                    base::Quat rot;
                    HL2AngleToQuaternion(data.Angles.z, data.Angles.x, data.Angles.y, rot);

                    // assemble transform
                    scene::NodeTemplatePlacement nodeTransform;
                    nodeTransform.T = HL2Transform.transformPoint(data.Origin);
                    nodeTransform.R = rot.toRotator();
                    nodeTransform.S = base::Vector3(data.UniformScale, data.UniformScale, data.UniformScale);
                    node->placement(nodeTransform);

                    auto meshName = node->meshRef().path().path().stringAfterLast("/").stringBeforeFirst(".");
                    node->name(base::StringID(base::BaseTempString("{}{}", meshName, i).c_str()));

                    //*if (data.Flags & bsp::STATIC_PROP_NO_SHADOW)
                      //  node->toggleShadowCasting(false);

                    prefab.nodeContainer()->attachNode(node);
                }
            }

            return true;
        }*/

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override final
        {
            // load the BSP
            auto bspData = loadData(cooker);
            if (!bspData)
                return false;

            // parse entities
            Entities entities;
            if (!parseEntities(bspData->entities(), entities))
                return false;

            // list shit
            for (uint32_t i=0; i<entities.size(); ++i)
            {
                auto ent = entities[i];
                auto className = ent->params.getValueOrDefault("classname"_id, base::StringBuf::EMPTY());
                auto name = ent->params.getValueOrDefault("name"_id, base::StringBuf::EMPTY());
                TRACE_INFO("Entity[{}]: class '{}' ({})", i, className, name);

                // add node to prefab
                /*if (auto node = createEntityNode(cooker, *bspData, i, *ent))
                    createdPrefab->nodeContainer()->attachNode(node);*/
            }

            // create static props
            /*if (!createStaticPros(*createdPrefab, cooker, *bspData))
                return false;*/

            // create dynamic props
            /*if (!createDynamicProps(*createdPrefab, cooker, *bspData))
                return false;*/

            // create the world model
            /*const
            {

            }*/

            // done
            //TRACE_INFO("Created {} nodes in the prefab", createdPrefab->nodeContainer()->nodes().size());
            return nullptr;
        }
    };

    /*RTTI_BEGIN_TYPE_CLASS(WorldPrefabCookerManifestBSP);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<scene::Prefab>();
        RTTI_METADATA(base::res::ResourceSourceFormat).addSourceExtension("bsp");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(1);
    RTTI_END_TYPE();*/

} // hl2