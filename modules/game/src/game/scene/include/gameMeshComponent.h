/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components #]
*
***/

#pragma once

#include "gameComponent.h"
#include "rendering/scene/include/public.h"

namespace game
{
    //--

    // a scene node capable of rendering a mesh
    class GAME_SCENE_API MeshComponent : public Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponent, Component);

    public:
        MeshComponent();
        MeshComponent(const rendering::MeshPtr& mesh);
        virtual ~MeshComponent();

        /// get the mesh
        INLINE const rendering::MeshPtr& mesh() const { return m_mesh; }

        /// are we casting static shadows ?
        INLINE bool castingShadows() const { return m_castShadows; }

        /// are we receiving shadows
        INLINE bool receivingShadows() const { return m_receiveShadows; }

        /// get forced detail level
        INLINE char forcedDetailLevel() const { return m_forceDetailLevel; }

        ///--

        /// toggle static shadow casting
        void castingShadows(bool flag);

        /// toggle receiving of shadows
        void receivingShadows(bool flag);

        /// force specific detail level
        void forceDetailLevel(char level);

        /// set new mesh
        void mesh(const rendering::MeshPtr& mesh);

        ///--

        virtual void handleAttach(World* scene) override;
        virtual void handleDetach(World* scene) override;
        virtual void handleTransformUpdate(const base::AbsoluteTransform& parentToWorld, Component* transformParentComponent) override;
        //virtual float calculateRequiredStreamingDistance() const override;

    protected:
        rendering::MeshPtr m_mesh;

        bool m_castShadows = true;
        bool m_receiveShadows = true;
        char m_forceDetailLevel = 0;

        rendering::scene::ProxyHandle m_proxy;

        void createRenderingProxy();
        void destroyRenderingProxy();
    };

    //--

} // game
