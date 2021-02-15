/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObject.h"
#include "apiThread.h"
#include "apiObjectRegistry.h"

namespace rendering
{
    namespace api
    {
		//--

		RTTI_BEGIN_TYPE_ENUM(ObjectType);
			RTTI_ENUM_OPTION(Unknown);

			RTTI_ENUM_OPTION(Buffer);
			RTTI_ENUM_OPTION(Image);
			RTTI_ENUM_OPTION(Sampler);
			RTTI_ENUM_OPTION(Shaders);
			RTTI_ENUM_OPTION(Output);

			RTTI_ENUM_OPTION(ImageReadOnlyView);
			RTTI_ENUM_OPTION(ImageWritableView);
			RTTI_ENUM_OPTION(SampledImageView);

			RTTI_ENUM_OPTION(RenderTargetView);
			RTTI_ENUM_OPTION(OutputRenderTargetView);

			RTTI_ENUM_OPTION(BufferTypedView);
			RTTI_ENUM_OPTION(BufferUntypedView);
			RTTI_ENUM_OPTION(StructuredBufferView);
			RTTI_ENUM_OPTION(StructuredBufferWritableView);

			RTTI_ENUM_OPTION(GraphicsPassLayout);
			RTTI_ENUM_OPTION(GraphicsRenderStates);
			RTTI_ENUM_OPTION(GraphicsPipelineObject);
			RTTI_ENUM_OPTION(ComputePipelineObject);
		RTTI_END_TYPE();
		
        //--

		IBaseObject::IBaseObject(IBaseThread* owner, ObjectType type)
            : m_owner(owner)
            , m_type(type)
        {
            m_handle = m_owner->objectRegistry()->registerObject(this);
            DEBUG_CHECK(!m_handle.empty());
        }

        IBaseObject::~IBaseObject()
        {
            m_owner->objectRegistry()->unregisterObject(m_handle, this);
        }

		void IBaseObject::print(base::IFormatStream& f) const
		{
			f.appendf("Object {} ({})", m_type, m_handle);
		}

        //--

		IBaseCopiableObject::IBaseCopiableObject(IBaseThread* drv, ObjectType type)
			: IBaseObject(drv, type)
		{}

		IBaseCopiableObject::~IBaseCopiableObject()
		{}

		//--

    } // gl4
} // rendering
