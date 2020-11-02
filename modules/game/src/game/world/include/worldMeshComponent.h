/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components\mesh #]
*
***/

#pragma once

#include "worldComponent.h"
#include "rendering/scene/include/public.h"

namespace game
{
    //--

    class MeshComponentTemplate;

    //--

    // a scene node capable of rendering a mesh
    class GAME_WORLD_API MeshComponent : public Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponent, Component);

    public:
        MeshComponent();
        MeshComponent(const rendering::MeshPtr& mesh);
        virtual ~MeshComponent();

        /// get the mesh
        INLINE const rendering::MeshRef& mesh() const { return m_mesh; }

        /// are we casting static shadows ?
        INLINE bool castingShadows() const { return m_castShadows; }

        /// are we receiving shadows
        INLINE bool receivingShadows() const { return m_receiveShadows; }

        /// get forced detail level
        INLINE char forcedDetailLevel() const { return m_forceDetailLevel; }

        /// get forced visibility distance
        INLINE uint16_t forcedDistance() const { return m_forcedDistance; }

        /// get object color
        INLINE base::Color color() const { return m_color; }

        /// get object extra color
        INLINE base::Color colorEx() const { return m_colorEx; }

        ///--

        /// toggle static shadow casting
        void castingShadows(bool flag);

        /// toggle receiving of shadows
        void receivingShadows(bool flag);

        /// force specific detail level
        void forceDetailLevel(char level);

        /// force particular visibility distance
        void forceDistance(float distance);

        /// change object color
        void color(base::Color color);
        
        /// change object extra color
        void colorEx(base::Color color);

        /// set new mesh
        void mesh(const rendering::MeshPtr& mesh);

        ///--

        virtual void handleAttach(World* scene) override;
        virtual void handleDetach(World* scene) override;
        virtual void handleTransformUpdate(const Component* source, const base::AbsoluteTransform& parentTransform, const base::Matrix& parentToWorld) override;
        //virtual float calculateRequiredStreamingDistance() const override;

    protected:
        rendering::MeshRef m_mesh;
        
        base::Color m_color = base::Color::WHITE;
        base::Color m_colorEx = base::Color::BLACK;

        bool m_castShadows = true;
        bool m_receiveShadows = true;
        char m_forceDetailLevel = 0;
        uint16_t m_forcedDistance = 0;

        rendering::scene::ProxyHandle m_proxy;

        void createRenderingProxy();
        void destroyRenderingProxy();

        void recreateRenderingProxy();

        //--

        friend class MeshComponentTemplate;
    };

    //--

} // game