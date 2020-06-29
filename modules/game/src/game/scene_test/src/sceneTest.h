/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "base/resources/include/resourceLoader.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"

#include "base/font/include/font.h"
#include "base/font/include/fontInputText.h"
#include "base/font/include/fontGlyphCache.h"
#include "base/font/include/fontGlyphBuffer.h"

#include "rendering/scene/include/renderingFrameParams.h"

namespace game
{
    namespace test
    {
        //---

        class FlyCameraEntity;

        //---

        // order of test
        class SceneTestOrderMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTestOrderMetadata, base::rtti::IMetadata);

        public:
            INLINE SceneTestOrderMetadata()
                : m_order(-1)
            {}

            SceneTestOrderMetadata& order(int val)
            {
                m_order = val;
                return *this;
            }

            int m_order;
        };

        //---

        /// a basic rendering test for the scene
        class ISceneTest : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneTest);

        public:
            ISceneTest();

            bool processInitialization();

            virtual void initialize();
            virtual void configure();
            virtual void render(rendering::scene::FrameParams& info);
            virtual void update(float dt);
            virtual bool processInput(const base::input::BaseEvent& evt);

            void reportError(base::StringView<char> msg);

            rendering::MeshPtr loadMesh(base::StringView<char> meshName, bool fullPath=false);

        protected:
            WorldPtr m_world;
            bool m_failed = false;
        };

        ///---

        /// a basic test with empty initial world
        class ISceneTestEmptyWorld : public ISceneTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ISceneTestEmptyWorld, ISceneTest);

        public:
            ISceneTestEmptyWorld();

            void recreateWorld();
            virtual void createWorldContent();

        protected:
            virtual void initialize() override;

            base::RefPtr<FlyCameraEntity> m_camera;

            base::Vector3 m_initialCameraPosition;
            base::Angles m_initialCameraRotation;
        };

        ///---

    } // test
} // game