/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: visuals\mesh #]
*
***/

#include "build.h"
#include "sceneMeshComponent.h"
#include "sceneRenderingSystem.h"

#include "base/app/include/localService.h"
#include "base/app/include/localServiceContainer.h"
#include "rendering/runtime/include/renderingRuntimeService.h"
#include "rendering/scene/include/renderingView.h"
#include "rendering/runtime/include/renderingMaterialInstance.h"

#include "scene/common/include/sceneRuntime.h"
#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/sceneEntity.h"
#include "base/geometry/include/mesh.h"

namespace scene
{
    //--

    struct MeshGeometryLevelOfDetailSetup
    {
        uint8_t m_detailMask = 0; // mask to use
        float m_showDistance = 0.0f; // when is the LOD shown
        float m_hideDistance = 100.0f; // when is the LOD hidden
    };

    struct MeshGeometryMaterialMapping
    {
        base::StringID m_name; // name of the material in the geometry we referring to, can be left empty to map to all materials
        rendering::runtime::MaterialInstancePtr m_material;
    };

    struct MeshRenderingData : public base::NoCopy
    {
        // rendering::runtime::GeometryPtr geometry;
        base::Array<MeshGeometryLevelOfDetailSetup> m_details;
        base::Array<MeshGeometryMaterialMapping> m_materials;
    };

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshComponent);
        RTTI_PROPERTY(m_mesh).editable();
        RTTI_PROPERTY(m_castShadows).editable();
        RTTI_PROPERTY(m_receiveShadows).editable();
        RTTI_PROPERTY(m_twoSidedShadows).editable();
    RTTI_END_TYPE();

    MeshComponent::MeshComponent()
        : m_castShadows(false)
        , m_receiveShadows(false)
        , m_twoSidedShadows(false)
        , m_data(nullptr)
    {}

    MeshComponent::~MeshComponent()
    {
        ASSERT(m_data);
    }

    void MeshComponent::toggleShadowsCasting(bool castShadows)
    {
        m_castShadows = castShadows;
    }

    void MeshComponent::toggleShadowInteractions(bool receivingShadows)
    {
        m_receiveShadows = receivingShadows;
    }

    void MeshComponent::handleSceneAttach(Scene* scene)
    {
        TBaseClass::handleSceneAttach(scene);

        cacheRenderingData();
        updateRenderable();
    }

    void MeshComponent::handleSceneDetach(Scene* scene)
    {
        TBaseClass::handleSceneDetach(scene);

        if (auto rs = scene->system<RenderingSystem>())
        {
            rs->unregisterRenderable(this);
        }
    }

    void MeshComponent::updateRenderable()
    {
        if (isAttached())
        {
            if (auto rs = scene()->system<RenderingSystem>())
            {
                auto bounds = localToWorld().transformBox(m_localBounds);
                rs->registerRenderable(this, bounds, 0.0f);
            }
        }
    }

    void MeshComponent::handleTransformUpdate(const base::Matrix& parentTransform)
    {
        TBaseClass::handleTransformUpdate(parentTransform);
        updateRenderable();
    }

    void MeshComponent::mesh(const base::mesh::MeshPtr& mesh)
    {
        if (m_mesh != mesh)
        {
            m_mesh = mesh;
            cacheRenderingData();
        }
    }

    void MeshComponent::cacheRenderingData()
    {
        // TODO: try to reuse

        // release current data
        if (m_data)
        {
            MemFree(m_data);
            m_data = nullptr;
        }

        // create new data, we must have a mesh for this
        if (m_mesh)
        {
            m_localBounds = m_mesh->bounds();

            /*if (auto geometry = m_mesh->renderingObject())
            {
                auto data  = MemNew(MeshRenderingData);
                data->geometry = geometry;

                auto count = geometry->numMaterials();
                data->m_materials.resize(count);

                for (uint32_t i = 0; i < count; ++i)
                {
                    auto& materialName = geometry->materialMapping()[i];
                    data->m_materials[i].name = materialName;
                    data->m_materials[i].m_material = m_mesh->materialPtr(materialName);
                }
            }*/
        }
    }

    static rendering::scene::PassBit PASS_MAIN_SOLID("SceneMain");
    static rendering::scene::PassBit PASS_FOREGROUND_SOLID("SceneForeground");
    static rendering::scene::PassBit PASS_BACKGROUND_SOLID("SceneBackground");
    static rendering::scene::PassBit PASS_MAIN_TRANSPARENT("Transparent");
    static rendering::scene::PassBit PASS_SELECTION("SelectionOutline");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA0("DepthCamera0");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA1("DepthCamera1");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA2("DepthCamera2");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA3("DepthCamera3");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA4("DepthCamera4");
    static rendering::scene::PassBit PASS_DEPTH_CAMERA5("DepthCamera5");


    static bool IsCompatibleForSelectionOutlinePass(const rendering::scene::PassBit& incomingPassBit)
    {
        if (incomingPassBit == PASS_MAIN_SOLID) return true;
        return false;
    }

    static bool MutatePassForDepthComposition(const rendering::scene::DepthCompositionLayer layer, const rendering::scene::PassBit& incomingPassBit, rendering::scene::PassBit& compositonPassBit)
    {
        if (layer == rendering::scene::DepthCompositionLayer::Background)
        {
            if (incomingPassBit == PASS_MAIN_SOLID)
            {
                compositonPassBit = PASS_BACKGROUND_SOLID;
                return true;
            }

            return false;
        }
        else if (layer == rendering::scene::DepthCompositionLayer::Foreground)
        {
            if (incomingPassBit == PASS_MAIN_SOLID)
            {
                compositonPassBit = PASS_FOREGROUND_SOLID;
                return true;
            }

            return false;
        }

        return false;
    }

    void MeshComponent::prepareCollectionTargets(const rendering::runtime::MaterialInstance* materialRef, const rendering::scene::IRendererView& view, uint8_t visMask, base::Array<CollectionTarget>& outTargets) const
    {
#if 0
        // get the technique set
        auto techniqueSet  = materialRef ? materialRef->techniqueSet() : nullptr;
        if (!techniqueSet)
            techniqueSet = &rendering::runtime::MaterialRenderingTechnique::DEFAULT();

        // shadows
        if (view.type() == rendering::scene::RendererViewType::ShadowCascades || view.type() == rendering::scene::RendererViewType::ShadowMap)
        {
            // skip over transparent techniques
            if (techniqueSet->m_isTransparent)
                return;

            // emit to cascades
            if (visMask & 1)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA0;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
            if (visMask & 2)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA1;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
            if (visMask & 4)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA2;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
            if (visMask & 8)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA3;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
            if (visMask & 16)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA4;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
            if (visMask & 32)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_DEPTH_CAMERA5;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
        }

        // normal view
        else if (view.type() == rendering::scene::RendererViewType::Main)
        {
            // determine pass to use
            auto passBit = PASS_MAIN_SOLID;
            if (techniqueSet->m_isTransparent)
                passBit = PASS_MAIN_TRANSPARENT;

            // depth composition
            bool outputFragments = true;
            rendering::runtime::PassBit compositionPass;
            auto layer = rendering::scene::DepthCompositionLayer::Main;
            if (MutatePassForDepthComposition(layer, passBit, compositionPass))
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = compositionPass;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;

                // if we are not in the editor that's the only bit we output
                /*auto sceneType = view.viewport() scene()->type();
                if (sceneType != SceneType::EditorPreview)
                    outputFragments = false;*/
            }

            if (outputFragments)
            {
                // normal frag
                {
                    auto& info = outTargets.emplaceBack();
                    info.m_passBit = passBit;
                    info.m_material = techniqueSet;
                    info.materialID = techniqueSet->m_uniqueID;
                }

                // output selection
                if (flags().test(ElementFlagBit::Selected) && IsCompatibleForSelectionOutlinePass(passBit))
                {
                    auto& info = outTargets.emplaceBack();
                    info.m_passBit = PASS_SELECTION;
                    info.m_material = techniqueSet;
                    info.materialID = techniqueSet->m_uniqueID;
                }
            }
        }

        // data capture
        else if (view.type() == rendering::scene::RendererViewType::DataCapture)
        {
            // ignore transparent
            if (!techniqueSet->m_isTransparent)
            {
                auto& info = outTargets.emplaceBack();
                info.m_passBit = PASS_MAIN_SOLID;
                info.m_material = techniqueSet;
                info.materialID = techniqueSet->m_uniqueID;
            }
        }
#endif
    }

    float MeshComponent::calculateRequiredStreamingDistance() const
    {
        return 75.0f;
    }

    void MeshComponent::collectRenderableStuff(uint8_t cameraMask, rendering::scene::IRendererView& view)
    {
        // nothing to render
        if (!m_data)
            return;
         
        // if we are collecting shadows skip objects that don't cast shadows
        bool skipTransparent = false;
        bool forceTwoSided = false;
        if (view.type() == rendering::scene::RendererViewType::ShadowCascades || view.type() == rendering::scene::RendererViewType::ShadowMap)
        {
            if (!m_castShadows)
                return;

            //if (m_f)
              //  forceTwoSided = true;

            skipTransparent = true;
        }

        // in non-editor scenes skip over depth composition proxies
        //if (!view.useDepthComposition() && depthCompositionLayer() != DepthCompositionLayer::Main)
          //  return;

        // decode local flags
        auto isSelected = flags().test(ElementFlagBit::Selected);
        auto selectionColor = base::Vector4(0.2f, 0.5f, 1.0f, 1.0f);
        auto defaultColor = base::Vector4(1, 1, 1, 1);

        /*// push one structure with object data
        InstanceObjectData objectInstanceData;
        objectInstanceData.localToWorld = localToWorld;
        objectInstanceData.selectable = selectable.encode(); // TODO: non-final
        objectInstanceData.receiveShadows = userFlags().test(ProxyUserFlag::ReceiveShadows) ? 1.0f : 0.0f;
        objectInstanceData.customParamA = isSelected ? selectionColor : defaultColor;
        auto paramsId = view.drawList().collectObjectInstanceData(objectInstanceData);
        if (paramsId == UNMAPPED_OBJECT)
            return;*/

            // calculate distance to proxy
        auto distance = 10.0f;// localToWorld().translation().distance(view.re(0).position());

        // select the detail mask (LOD)
        auto currentDetailMask = 1;// computeDetailMask(distance);

        // temp
        base::InplaceArray<CollectionTarget, 16> collectionTargets;

        // collect draw calls
        // TODO: array view, FFS
        /*auto numChunks = data->geometry->numChunks();
        for (uint32_t chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
        {
            // filter
            auto chunk  = data->geometry->chunks() + chunkIndex;
            if (0 == (chunk->m_detailMask & currentDetailMask))
                continue;

            // validate the material
            if (chunk->m_materialIndex >= data->m_materials.size())
                continue;

            // get the material
            auto materialRef  = data->m_materials[chunk->m_materialIndex].m_material.get();

            // skip the transparent
            if (skipTransparent && materialRef && materialRef->techniqueSet() && materialRef->techniqueSet()->m_isTransparent)
                continue;

            // TODO: material overrides/additives

            // prepare a list of collection targets
            collectionTargets.reset();
            prepareCollectionTargets(materialRef, view, cameraMask, collectionTargets);
            for (auto& target : collectionTargets)
            {
                // setup sorting key
                rendering::scene::FragmentKey key;
                key.geometryID = chunk->id;
                key.distance = (uint16_t)(distance * 16);
                key.materialID = target.materialID;

                // prepare mesh fragment
                auto meshFragment  = view.drawList().allocFragment<rendering::scene::FragmentMesh>();
                meshFragment->handlerType = rendering::scene::GetHandlerIndex<rendering::scene::FragmentHandler_Mesh>();
                meshFragment->geometry = chunk;
                meshFragment->m_material = target.m_material;
                //meshFragment->selectable = selectable.encode();
                meshFragment->m_receiveShadows = 1.0f;// flags().test(ElementFlagBit::ReceiveShadows) ? 1.0f : 0.0f;
                meshFragment->m_customParamA = isSelected ? selectionColor : defaultColor;
                meshFragment->localToWorld = localToWorld();

                // collect mesh fragment
                view.drawList().collectFragment(meshFragment, key, target.m_passBit);
            }
        }*/
    }

    //--

    namespace legacy
    {
        // template for the mesh node
        class MeshNodeTemplate : public NodeTemplate
        {
            RTTI_DECLARE_VIRTUAL_CLASS(MeshNodeTemplate, NodeTemplate);

        public:
            MeshNodeTemplate()
                : m_receiveShadows(true)
                , m_castShadows(true)
                , m_twoSidedShadows(false)
            {}

            bool m_castShadows;
            bool m_receiveShadows;
            bool m_twoSidedShadows;
            base::res::Ref<base::mesh::Mesh> m_mesh;

            virtual NodeTemplatePtr convertLegacyContent() const override final
            {
                auto ret = base::CreateSharedPtr<NodeTemplate>();
                ret->placement(placement());
                ret->name(name());

                auto entTemp = base::CreateSharedPtr<EntityDataTemplate>();
                entTemp->entityClass(StaticEntity::GetStaticClass());
                ret->entityTemplate(entTemp);

                auto comp = base::CreateSharedPtr<ComponentDataTemplate>();
                comp->componentClass(MeshComponent::GetStaticClass());
                comp->name("mesh"_id);
                comp->writeParameter("mesh"_id, base::CreateVariant(m_mesh));
                comp->writeParameter("castShadows"_id, base::CreateVariant(m_castShadows));
                comp->writeParameter("receiveShadows"_id, base::CreateVariant(m_receiveShadows));
                comp->writeParameter("twoSidedShadows"_id, base::CreateVariant(m_twoSidedShadows));
                ret->addComponentTemplate(comp);

                return ret;
            }
        };

        RTTI_BEGIN_TYPE_CLASS(MeshNodeTemplate);
            RTTI_OLD_NAME("scene::MeshNodeTemplate");
            RTTI_PROPERTY(m_castShadows);
            RTTI_PROPERTY(m_receiveShadows);
            RTTI_PROPERTY(m_twoSidedShadows);
            RTTI_PROPERTY(m_mesh);
        RTTI_END_TYPE();

    } // legacy

    //--
        
} // scene