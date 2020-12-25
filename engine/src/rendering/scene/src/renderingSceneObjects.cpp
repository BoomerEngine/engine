/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneObjects.h"
#include "renderingSceneObjectManager.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/mesh/include/renderingMeshChunkProxy.h"
#include "rendering/material/include/renderingMaterial.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialRuntimeTemplate.h"

namespace rendering
{
    namespace scene
    {
		//--

		template< typename T >
		struct ProxyMemoryAllocator
		{
			uint32_t size = 0;

			uint8_t* memPtr = nullptr;
			uint8_t* memEndPtr = nullptr;

			inline ProxyMemoryAllocator()
			{
				size = sizeof(T);
			}

			template< typename W >
			void add(uint32_t count = 1)
			{
				size += sizeof(W) * count;
			}

			T* allocate()
			{
				memPtr = (uint8_t*)base::mem::GlobalPool<POOL_RENDERING_RUNTIME>::Alloc(size, alignof(T));
				memzero(memPtr, size);
				memEndPtr = memPtr + size;

				auto* ret = new (memPtr) T();
				memPtr += sizeof(T);

				return ret;
			}

			template< typename W >
			W* retive(uint32_t count = 1)
			{
				auto* ret = (W*)memPtr;
				memPtr = ret + (sizeof(T) * count);
				ASSERT(memPtr <= memEndPtr);
				return ret;
			}
		};

		//---

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IObjectManager);
		RTTI_END_TYPE();

		IObjectManager::~IObjectManager()
		{}

		//---

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IObjectProxy);
		RTTI_END_TYPE();

		IObjectProxy::~IObjectProxy()
		{}

        //---

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IObjectProxyBase);
		RTTI_END_TYPE();

		IObjectProxyBase::IObjectProxyBase(ObjectType type)
			: m_type(type)
		{}

		IObjectProxyBase::~IObjectProxyBase()
		{}

		ObjectType IObjectProxyBase::type() const
		{
			return m_type;
		}

		bool IObjectProxyBase::attached() const
		{
			return m_flags.test(scene::ObjectProxyFlagBit::Attached);
		}

		//---

		void ObjectProxyMeshChunk::updatePassTypes()
		{
			if (shader)
			{
				const auto& shaderMetadata = shader->metadata();
				if (!shaderMetadata.hasTransparency)
				{
					if (!shaderMetadata.hasVertexAnimation && staticGeometry)
						depthPassType = 0;
					else
						depthPassType = 1;
				}
				else
				{
					depthPassType = -1;
				}

				// forward pass flags
				if (shaderMetadata.hasTransparency)
					forwardPassType = 2;
				else if (shaderMetadata.hasPixelDiscard)
					forwardPassType = 1;
				else
					forwardPassType = 0;
			}
			else
			{
				depthPassType = -1;
				forwardPassType = -1;
			}
		}

        //---

		RTTI_BEGIN_TYPE_NATIVE_CLASS(ObjectProxyMesh);
		RTTI_END_TYPE();

		ObjectProxyMesh::ObjectProxyMesh()
			: IObjectProxyBase(ObjectType::Mesh)
		{}

		ObjectProxyMesh::~ObjectProxyMesh()
		{}

		ObjectProxyMeshPtr ObjectProxyMesh::Compile(const Setup& setup)
		{
			DEBUG_CHECK_RETURN_EX_V(setup.mesh, "No source mesh", nullptr);
			
			const auto maxLods = setup.mesh->detailLevels().size();
			DEBUG_CHECK_RETURN_EX_V(setup.forcedLodLevel == -1 || (setup.forcedLodLevel >= 0 && setup.forcedLodLevel < (int)maxLods), "Invalid LOD index", nullptr);

			base::InplaceArray<uint16_t, 256> chunksToCreate;
			for (auto i : setup.mesh->chunks().indexRange())
			{
				const auto& chunk = setup.mesh->chunks().typedData()[i];
				const auto& materialName = setup.mesh->materials().typedData()[chunk.materialIndex].name;

				if (!setup.selectiveMaterialMask.empty() && !setup.selectiveMaterialMask.contains(materialName))
					continue;

				if (setup.excludedMaterialMask.contains(materialName))
					continue;

				if (!chunk.proxy) // not renderable
					continue;
				
				chunksToCreate.pushBack(i);
			}

			if (chunksToCreate.empty())
				return nullptr;

			const auto numLods = (setup.forcedLodLevel == -1) ? maxLods : 1;

			ProxyMemoryAllocator<ObjectProxyMesh> allocator;
			allocator.add<ObjectProxyMeshLOD>(numLods);
			allocator.add<ObjectProxyMeshChunk>(chunksToCreate.size());

			auto* ret = allocator.allocate();
			ret->m_numChunks = 0;// chunksToCreate.size();
			ret->m_numLods = numLods;
			ret->m_localBox = setup.mesh->bounds();

			// setup lod table
			auto* lodTable = (ObjectProxyMeshLOD*) ret->lods();
			if (setup.forcedLodLevel != -1)
			{
				const auto& sourceLod = setup.mesh->detailLevels()[setup.forcedLodLevel];
				lodTable[0].minDistance = sourceLod.rangeMin;
				lodTable[0].maxDistance = sourceLod.rangeMax;
			}
			else
			{
				for (uint32_t i = 0; i < maxLods; ++i)
				{
					const auto& sourceLod = setup.mesh->detailLevels().typedData()[i];
					lodTable[i].minDistance = sourceLod.rangeMin;
					lodTable[i].maxDistance = sourceLod.rangeMax;
				}
			}

			// setup chunks
			auto* chunkTable = (ObjectProxyMeshChunk*)ret->chunks();
			for (auto index : chunksToCreate)
			{
				const auto& chunk = setup.mesh->chunks().typedData()[index];

				// determine material to use at chunk
				const IMaterial* sourceMaterial = setup.forceMaterial;
				if (!sourceMaterial)
				{
					// apply override
					const auto& materialName = setup.mesh->materials().typedData()[chunk.materialIndex].name;
					if (const auto* overrideMaterial = setup.materialOverrides.find(materialName))
						if (*overrideMaterial)
							sourceMaterial = *overrideMaterial;

					// if no override was applied use the mesh's material
					if (!sourceMaterial)
						sourceMaterial = setup.mesh->materials()[chunk.materialIndex].material.get();

					// use the fall back

					// if we still have no material don't render this chunk
					if (!sourceMaterial)
						continue;
				}					

				// setup chunk
				chunkTable->data = chunk.proxy;				
				chunkTable->lodMask = chunk.detailMask;
				chunkTable->renderMask = chunk.renderMask;
				chunkTable->material = sourceMaterial->dataProxy();
				chunkTable->shader = sourceMaterial->templateProxy();
				chunkTable->staticGeometry = setup.staticGeometry;
				ASSERT_EX(chunkTable->material != nullptr, "Material without data proxy");
                ASSERT_EX(chunkTable->shader != nullptr, "Material without shader");
				chunkTable->updatePassTypes();

				// write chunk
				chunkTable += 1;
			}

			// count actually written chunks
			ret->m_numChunks = (uint16_t)(chunkTable - ret->chunks());

			return AddRef(ret);
		}

		//---

    } // scene
} // rendering