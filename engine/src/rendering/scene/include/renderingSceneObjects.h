/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "renderingSelectable.h"
#include "renderingScene.h"

namespace rendering
{
    namespace scene
    {

		///---

		/// rendering object proxy
		class RENDERING_SCENE_API IObjectProxyBase : public IObjectProxy
		{
			RTTI_DECLARE_VIRTUAL_CLASS(IObjectProxyBase, IObjectProxy);

		public:
			IObjectProxyBase(ObjectType type);
			virtual ~IObjectProxyBase();

			//--

			base::Matrix m_localToWorld;

			//--

			const ObjectType m_type;

			ObjectProxyFlags m_flags;

			//float hideDistance = 0.0f;

			base::Color m_color = base::Color::WHITE;

			base::Color m_colorEx = base::Color::BLACK;

			Selectable m_selectable;

			//--



			//--

			virtual ObjectType type() const override final;
			virtual bool attached() const override final;
		};

		///---

		struct ObjectProxyMeshChunk
		{
			MeshChunkProxyPtr data;
			MaterialDataProxyPtr material;
			uint8_t lodMask = 0;
			uint32_t renderMask = 0;
		};

		struct ObjectProxyMeshLOD
		{
			float minDistance = 0.0f;
			float maxDistance = 0.0f;
		};

		class RENDERING_SCENE_API ObjectProxyMesh : public IObjectProxyBase
		{
			RTTI_DECLARE_VIRTUAL_CLASS(ObjectProxyMesh, IObjectProxyBase);

		public:
			ObjectProxyMesh();
			virtual ~ObjectProxyMesh();

			uint8_t numLods = 0;
			uint16_t numChunks = 0;

			inline const ObjectProxyMeshLOD* lods() const { return (const ObjectProxyMeshLOD*)(this + 1); }
			inline const ObjectProxyMeshChunk* chunks() const { return (const ObjectProxyMeshChunk*)(lods() + numLods); }

			// WARNING: packed data follows!
			//

			//--

			struct Setup
			{
				char forcedLodLevel = -1;

				const Mesh* mesh = nullptr;

				const IMaterial* forceMaterial = nullptr;

				base::HashMap<base::StringID, const IMaterial*> materialOverrides;
				base::HashSet<base::StringID> selectiveMaterialMask;
				base::HashSet<base::StringID> excludedMaterialMask;
			};

			// compile a mesh proxy from given setup
			static ObjectProxyMeshPtr Compile(const Setup& setup);
		};

		//--

    } // scene
} // rendering

