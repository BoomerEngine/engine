/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#pragma once

#include "base/object/include/object.h"
#include "rendering/scene/include/renderingFrame.h"
#include "scene/common/include/sceneNodePlacement.h"

namespace ed
{
    namespace world
    {

        ///--

        class SceneTempObjectSystem;

        /// temporary object - displayed in scene as normal object but not saved
        class ISceneTempObject : public base::IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ISceneTempObject, base::IObject);
            RTTI_SHARED_FROM_THIS_TYPE(ISceneTempObject);

        public:
            ISceneTempObject();
            virtual ~ISceneTempObject();

            //---

            /// get current placement
            virtual const scene::NodeTemplatePlacement& placement() const { return m_placement; }

            /// get current selection state
            virtual bool isSelected() const { return m_selected; }

            //---

            /// change position of the object
            virtual void placement(const scene::NodeTemplatePlacement& placement);

            /// change selection state
            virtual void selection(bool selected);

            //---

            /// update (per frame)
            virtual void handleTick(float dt);

            /// render debug fragments 
            virtual void handleRendering(rendering::scene::FrameInfo& frame);

            //---

            // attach temp object to scene
            virtual void attachToScene(SceneTempObjectSystem& owner);

            // depatch temp object from scene
            virtual void deattachFromScene();

        protected:
            SceneTempObjectSystem* m_owner;
            scene::NodeTemplatePlacement m_placement;
            bool m_selected;
        };

        ///--

        /// a node template based temp object - will stream-in the content properly
        class SceneTempObjectNode : public ISceneTempObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTempObjectNode, ISceneTempObject);

        public:
            SceneTempObjectNode(const scene::NodeTemplatePtr& nodeTemplate);
            virtual ~SceneTempObjectNode();

            //---

            /// change position of the object
            virtual void placement(const scene::NodeTemplatePlacement& placement) override;

            /// change selection state
            virtual void selection(bool selected) override;

            //---

            /// update (per frame)
            virtual void handleTick(float dt) override;

            /// render debug fragments 
            virtual void handleRendering(rendering::scene::FrameInfo& frame) override;

            //---

            // attach temp object to scene
            virtual void attachToScene(SceneTempObjectSystem& owner) override;

            // depatch temp object from scene
            virtual void deattachFromScene() override;

        private:
            scene::NodeTemplatePtr m_nodeTemplate;
        };

        ///--

        /// temporary object "system", note: we are independent on the SceneContent crap
        class SceneTempObjectSystem : public base::NoCopy
        {
        public:
            SceneTempObjectSystem(const scene::ScenePtr& scene);
            ~SceneTempObjectSystem();

            /// update internals
            void tick(float dt);

            /// render debug stuff
            void render(rendering::scene::FrameInfo& frame);

        private:
            scene::ScenePtr m_scene;
            base::Array<base::RefPtr< ISceneTempObject >> m_objects;
            bool m_updating;
            bool m_objectsRemovedWhileUpdating;

            void handleObjectAttached(const base::RefPtr< ISceneTempObject >& obj);
            void handleObjectDetached(const base::RefPtr< ISceneTempObject >& obj);

            friend class ISceneTempObject;
        };

        ///--

    } // world
} // ed