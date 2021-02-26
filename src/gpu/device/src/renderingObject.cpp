/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#include "build.h"
#include "renderingObject.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDeviceObject);
RTTI_END_TYPE();

IDeviceObject::IDeviceObject(ObjectID id, IDeviceObjectHandler* impl)
    : m_id(id)
    , m_impl(impl)
{
}

IDeviceObject::~IDeviceObject()
{
    if (auto impl = m_impl.lock())
    {
        TRACE_SPAM("Rendering: Releasing device object {}", m_id);
        impl->releaseToDevice(m_id);
    }
}

api::IBaseObject* IDeviceObject::resolveInternalApiObjectRaw(uint8_t objectType) const
{
	if (auto impl = m_impl.lock())
		return impl->resolveInternalObjectPtrRaw(m_id, objectType);

	return nullptr;
}

//---

RTTI_BEGIN_TYPE_ENUM(DeviceObjectViewType);
	RTTI_ENUM_OPTION(Invalid);
	RTTI_ENUM_OPTION(ConstantBuffer);
	RTTI_ENUM_OPTION(Buffer);
	RTTI_ENUM_OPTION(BufferWritable);
	RTTI_ENUM_OPTION(BufferStructured);
	RTTI_ENUM_OPTION(BufferStructuredWritable);
	RTTI_ENUM_OPTION(Image);
	RTTI_ENUM_OPTION(ImageWritable);
	RTTI_ENUM_OPTION(SampledImage);
	RTTI_ENUM_OPTION(RenderTarget);
	RTTI_ENUM_OPTION(Sampler);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDeviceObjectView);
RTTI_END_TYPE();

IDeviceObjectView::IDeviceObjectView(ObjectID viewId, DeviceObjectViewType viewType, IDeviceObject* object, IDeviceObjectHandler* impl)
    : m_viewId(viewId)
    , m_viewType(viewType)
    , m_object(AddRef(object))
    , m_impl(impl)
{}

IDeviceObjectView::~IDeviceObjectView()
{
    if (auto impl = m_impl.lock())
    {
        TRACE_SPAM("Rendering: Releasing device object view {} of object {}", m_viewId, m_object->id());
        impl->releaseToDevice(m_viewId);
    }
}

//---

IDeviceObjectHandler::~IDeviceObjectHandler()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SamplerObject);
RTTI_END_TYPE();

SamplerObject::SamplerObject(ObjectID id, IDeviceObjectHandler* impl)
    : IDeviceObject(id, impl)
{}

SamplerObject::~SamplerObject()
{}

//---

END_BOOMER_NAMESPACE_EX(gpu)
