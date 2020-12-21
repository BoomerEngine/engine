/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingSceneObjectManager.h"
#include "rendering/material/include/renderingMaterialRuntimeService.h"

namespace rendering
{
    namespace scene
    {
		
		///--

        // object handler for standalone meshes
        class ObjectManagerMesh : public IObjectManager
        {
			RTTI_DECLARE_VIRTUAL_CLASS(ObjectManagerMesh, IObjectManager);

		public:
			ObjectManagerMesh();
			virtual ~ObjectManagerMesh();

			//--

			virtual ObjectType objectType() const { return ObjectType::Mesh; }

			virtual void initialize(Scene* scene, IDevice* dev) override final;
			virtual void shutdown() override final;

			virtual void prepare(command::CommandWriter& cmd, IDevice* dev, const FrameRenderer& frame) override final;
			virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) override final;

			//--

			virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) override final;

			//--

			virtual void run(const CommandAttachObject& op) override final;
			virtual void run(const CommandDetachObject& op) override final;
			virtual void run(const CommandSetLocalToWorld& op) override final;
			virtual void run(const CommandSetFlag& op) override final;
			virtual void run(const CommandClearFlag& op) override final;

			//--

        private:
            MaterialCache* m_materialCache = nullptr;
        };

        ///---
        
    } // scene
} // rendering

