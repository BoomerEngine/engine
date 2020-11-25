/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingDescriptorID.h"

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
			struct AttributeInfo
			{
				ImageFormat format = ImageFormat::UNKNOWN;
				uint16_t offset = 0;
			};

            struct BindingInfo
            {
                uint16_t bindPointIndex = 0;
                uint32_t stride = 0;
                base::StringID name;
                bool instanced = 0;

				base::Array<AttributeInfo> attributes;
            };

			//--

			IBaseVertexBindingLayout(base::Array<BindingInfo>&& data);
			virtual ~IBaseVertexBindingLayout();

			INLINE const base::Array<BindingInfo>& vertexBindPoints() const { return m_vertexBindPoints; }

			void print(base::IFormatStream& f) const;

		protected:
			base::Array<BindingInfo> m_vertexBindPoints;
        };

		//---

		struct RENDERING_API_COMMON_API DescriptorBindingElement
		{
			uint16_t bindPointIndex = 0; // where to look for "ParameterInfo"
			uint16_t descriptorElementIndex = 0; // index of element inside descriptor

			DescriptorID bindPointLayout;
			base::StringID bindPointName;
			base::StringID paramName;

			DeviceObjectViewType objectType = DeviceObjectViewType::Invalid;

			ImageFormat objectFormat = ImageFormat::UNKNOWN;
			bool writable = false;

			//--

			uint16_t apiObjectSlot = 0; // in target API

			//--
			
			DescriptorBindingElement();

			void print(base::IFormatStream& f) const;
		};

		//--

		// collection of descriptors that should be bound together
		// represents Root Signature / Pipeline Layout 
        class RENDERING_API_COMMON_API IBaseDescriptorBindingLayout : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_PIPELINES)

        public:
			IBaseDescriptorBindingLayout(const base::Array<DescriptorBindingElement>& elements);
			virtual ~IBaseDescriptorBindingLayout();

			INLINE const base::Array<DescriptorBindingElement>& elements() const { return m_elements; }

			void print(base::IFormatStream& f) const;

		private:
			base::Array<DescriptorBindingElement> m_elements;
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

            /// find a parameter index by name and type
            uint16_t resolveDescriptorBindPointIndex(const ShaderLibraryData& shaderLib, PipelineIndex descriptorLayoutIndex);

            /// find/create VBO layout
            IBaseVertexBindingLayout* resolveVertexBindingLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex vertexInputStateIndex);

			/// resolve the mapping of inputs parameters to actual slots in OpenGL
			IBaseDescriptorBindingLayout* resolveDescriptorBindingLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex parameterBindingState);

			//--
        private:
            IBaseThread* m_owner;

            base::HashMap<base::StringID, uint16_t> m_vertexBindPointMap;
            base::HashMap<UniqueParamBindPointKey, uint16_t> m_descriptorBindPointMap;
            base::Array<DescriptorID> m_descriptorBindPointLayouts;

            base::HashMap<uint64_t, IBaseVertexBindingLayout*> m_vertexLayoutMap;
            base::HashMap<uint64_t, IBaseDescriptorBindingLayout*> m_descriptorBindingMap;

			virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(base::Array<IBaseVertexBindingLayout::BindingInfo>&& elements) = 0;
			virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(base::Array<DescriptorBindingElement>&& elements) = 0;
        };

        //---

    } // api
} // rendering
