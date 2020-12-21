/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneManager_StandaloneMesh.h"

namespace rendering
{
    namespace scene
    {

        //---

		RTTI_BEGIN_TYPE_CLASS(ObjectManagerMesh);
		RTTI_END_TYPE();

		ObjectManagerMesh::ObjectManagerMesh()
		{}

		ObjectManagerMesh::~ObjectManagerMesh()
		{}

		void ObjectManagerMesh::initialize(Scene* scene, IDevice* dev)
		{
			
		}

		void ObjectManagerMesh::shutdown()
		{
		}

		void ObjectManagerMesh::prepare(command::CommandWriter& cmd, IDevice* dev, const FrameRenderer& frame)
		{

		}

		void ObjectManagerMesh::render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame)
		{

		}

		void ObjectManagerMesh::handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies)
		{

		}

		//--

		void ObjectManagerMesh::run(const CommandAttachObject& op)
		{

		}

		void ObjectManagerMesh::run(const CommandDetachObject& op)
		{

		}

		void ObjectManagerMesh::run(const CommandSetLocalToWorld& op)
		{

		}

		void ObjectManagerMesh::run(const CommandSetFlag& op)
		{

		}

		void ObjectManagerMesh::run(const CommandClearFlag& op)
		{

		}

        //---

    } // scene
} // rendering