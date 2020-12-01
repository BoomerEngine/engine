/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			//--

			struct DescriptorBindingElement
			{
				uint16_t bindPointIndex = 0; // descriptor index in the global table
				uint16_t elementIndex = 0; // element in the descriptor

				DeviceObjectViewType objectType = DeviceObjectViewType::Invalid; // what do we expect to find there
				ImageFormat objectFormat = ImageFormat::UNKNOWN; // format for formated data
				uint8_t objectSlot = 0; // in OpenGL

				char samplerSlotIndex = 0; // element in the descriptor
				GLuint glStaticSampler = 0; // resolved static sampler

				DescriptorID bindPointLayout; // debug layout
				base::StringView bindPointName; // debug name
				base::StringView elementName; // debug name
			};

			//--

			class DescriptorBindingLayout : public IBaseDescriptorBindingLayout
			{
			public:
				DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers);
				virtual ~DescriptorBindingLayout();

				INLINE const base::Array<DescriptorBindingElement>& elements() const { return m_elements; }

				void prepare();

			private:
				base::Array<DescriptorBindingElement> m_elements;

				base::Array<GLuint> m_staticSamplers;
				base::Array<SamplerState> m_staticSamplersStates;

				bool m_prepared = false;

				Thread* m_owner = nullptr;
			};

			//--

		} // gl4
    } // api
} // rendering

