/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingDescriptorID.h"
#include "rendering/device/include/renderingShaderMetadata.h"

namespace rendering
{
    namespace api
    {

        //---

		// vertex layout/binding states, owned by cache since they repeat a lot
        class RENDERING_API_COMMON_API IBaseVertexBindingLayout : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_PIPELINES)

        public:
			IBaseVertexBindingLayout(IBaseObjectCache* cache, const base::Array<ShaderVertexStreamMetadata>& streams);
			virtual ~IBaseVertexBindingLayout();

			INLINE const base::Array<ShaderVertexStreamMetadata>& vertexStreams() const { return m_elements; }

			void print(base::IFormatStream& f) const;

		protected:
			base::Array<ShaderVertexStreamMetadata> m_elements;
        };

		//--

		// collection of descriptors that should be bound together
		// represents Root Signature / Pipeline Layout 
        class RENDERING_API_COMMON_API IBaseDescriptorBindingLayout : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_PIPELINES)

        public:
			IBaseDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors);
			virtual ~IBaseDescriptorBindingLayout();

			INLINE const base::Array<ShaderDescriptorMetadata>& descriptors() const { return m_descriptors; }

			void print(base::IFormatStream& f) const;

		private:
			base::Array<ShaderDescriptorMetadata> m_descriptors;
        };

		//--

        struct UniqueParamBindPointKey
        {
            base::StringID name;
            DescriptorID layout;

			INLINE UniqueParamBindPointKey() {};

            INLINE static uint32_t CalcHash(const UniqueParamBindPointKey& key) { return base::CRC32() << key.name << key.layout.value(); }

            INLINE bool operator==(const UniqueParamBindPointKey& other) const { return (name == other.name) && (layout == other.layout); }
            INLINE bool operator!=(const UniqueParamBindPointKey& other) const { return !operator==(other); }
        };

		//---

        class RENDERING_API_COMMON_API IBaseObjectCache : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_PIPELINES)

        public:
			IBaseObjectCache(IBaseThread* owner);
            virtual ~IBaseObjectCache();

			//--

			// cache owner
			INLINE IBaseThread* owner() const { return m_owner; }

			//--

			// clear all objects from the cache
			virtual void clear();

			//--

            /// find a bindpoint index by name
            uint16_t resolveVertexBindPointIndex(base::StringID name);

            /// find a parameter index by name and type
            uint16_t resolveDescriptorBindPointIndex(base::StringID name, DescriptorID layout);

            /// find/create VBO layout
            IBaseVertexBindingLayout* resolveVertexBindingLayout(const ShaderMetadata* metadata);

			/// resolve the mapping of inputs parameters to actual slots in OpenGL
			IBaseDescriptorBindingLayout* resolveDescriptorBindingLayout(const ShaderMetadata* metadata);

			//--
        private:
            IBaseThread* m_owner;

            base::HashMap<base::StringID, uint16_t> m_vertexBindPointMap;
            base::HashMap<UniqueParamBindPointKey, uint16_t> m_descriptorBindPointMap;
            base::Array<DescriptorID> m_descriptorBindPointLayouts;

            base::HashMap<uint64_t, IBaseVertexBindingLayout*> m_vertexLayoutMap;
            base::HashMap<uint64_t, IBaseDescriptorBindingLayout*> m_descriptorBindingMap;

			virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams) = 0;
			virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers) = 0;
        };

        //---

    } // api
} // rendering
