/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: visuals\mesh #]
*
***/

#pragma once

#include "scene/common/include/sceneComponent.h"
#include "sceneRenderingSystem.h"

namespace scene
{
    //--

    // cached rendering data for a mesh
    struct MeshRenderingData;

    //--

    // a scene node capable of rendering a mesh
    class SCENE_OBJECTS_API MeshComponent : public Component, public IRenderable
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponent, Component);

    public:
        MeshComponent();
        virtual ~MeshComponent();

        /// get the mesh
        INLINE const base::mesh::MeshPtr& mesh() const { return m_mesh; }

        /// are we casting static shadows ?
        INLINE bool isCastingShadows() const { return m_castShadows; }

        /// are we receiving shadows
        INLINE bool isReceivingShadows() const { return m_receiveShadows; }

        ///--

        /// toggle static shadow casting
        void toggleShadowsCasting(bool castStaticShadows);

        /// toggle receiving of shadows
        void toggleShadowInteractions(bool receivingShadows);

        /// set new mesh
        void mesh(const base::mesh::MeshPtr& mesh);

        ///--

        virtual void handleSceneAttach(Scene* scene) override;
        virtual void handleSceneDetach(Scene* scene) override;
        virtual void handleTransformUpdate(const base::Matrix& parentTransform) override;
        virtual float calculateRequiredStreamingDistance() const override;

    protected:
        base::mesh::MeshPtr m_mesh;
        base::Box m_localBounds;

        bool m_castShadows;
        bool m_receiveShadows;
        bool m_twoSidedShadows;

        MeshRenderingData* m_data;

        //---

        void updateRenderable();
        void cacheRenderingData();

        virtual void collectRenderableStuff(uint8_t cameraMask, rendering::scene::IRendererView& view) override;

        struct CollectionTarget
        {
            const rendering::runtime::MaterialRenderingTechnique* m_material = nullptr;
            rendering::scene::PassBit m_passBit;
            uint16_t m_materialID = 0;
        };

        void prepareCollectionTargets(const rendering::runtime::MaterialInstance* materialRef, const rendering::scene::IRendererView& view, uint8_t visMask, base::Array<CollectionTarget>& outTargets) const;
    };

} // scene
