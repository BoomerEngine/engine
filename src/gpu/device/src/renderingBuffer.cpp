/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#include "build.h"
#include "renderingBuffer.h"
#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferObject);
RTTI_END_TYPE();

BufferObject::BufferObject(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup)
    : IDeviceObject(id, impl)
    , m_size(setup.size)
    , m_stride(setup.stride)
    , m_flags(setup.flags)
    , m_layout(setup.layout)
{
	DEBUG_CHECK_EX(m_size > 0, "Invalid buffer size");
    DEBUG_CHECK_EX(!id.empty(), "Invalid objects can't be used to create pseudo-valid looking views");
    DEBUG_CHECK_EX(!m_stride || (0 == (m_size % m_stride)), "Buffer size must be multiple of stride");

    if (m_stride)
        m_flags |= BufferViewFlag::Structured;

    DEBUG_CHECK_EX(!m_flags.test(BufferViewFlag::Structured) || m_stride, "Non-structured buffers can't have the Structured flag set");
}

BufferObject::~BufferObject()
{}

uint32_t BufferObject::calcMemorySize() const
{
    return m_size;
}

bool BufferObject::validateConstantView(uint32_t offset, uint32_t& size) const
{
	DEBUG_CHECK_RETURN_V(constants(), nullptr);

	DEBUG_CHECK_RETURN_V(m_stride == 0, nullptr);

	DEBUG_CHECK_RETURN_V((offset & 255) == 0, nullptr);
	DEBUG_CHECK_RETURN_V(offset < m_size, nullptr);

	if (size == INDEX_MAX)
		size = m_size - offset;

	DEBUG_CHECK_RETURN_V(size > 0, false);
	//DEBUG_CHECK_RETURN_V((size & 255) == 0, nullptr);
	DEBUG_CHECK_RETURN_V(size <= m_size, nullptr);

	DEBUG_CHECK_RETURN_V(offset + size <= m_size, nullptr);

	return true;
}

bool BufferObject::validateTypedView(ImageFormat format, uint32_t offset, uint32_t &size, bool writable) const
{
	if (writable)
	{
		DEBUG_CHECK_RETURN_V(uavCapable(), false);
	}
	else
	{
		DEBUG_CHECK_RETURN_V(shadeReadable(), false);
	}
		
	DEBUG_CHECK_RETURN_V(m_stride == 0, false);// "Offset must be multiple of structure stride");
	DEBUG_CHECK_RETURN_V(offset < m_size, false);// "Trying to create a view past the buffer size");
		
	// TODO: check format compatibility

	if (size == INDEX_MAX)
		size = m_size - offset;

	DEBUG_CHECK_RETURN_V(format != ImageFormat::UNKNOWN, false);
	const auto alignment = GetImageFormatInfo(format).bitsPerPixel / 8;
	DEBUG_CHECK_RETURN_V(size > 0, false);

	DEBUG_CHECK_RETURN_V((size % alignment) == 0, false);
	DEBUG_CHECK_RETURN_V((offset % alignment) == 0, false);

    return true;
}

bool BufferObject::validateStructureView(uint32_t stride, uint32_t offset, uint32_t& size, bool writable) const
{
	if (writable)
	{
		DEBUG_CHECK_RETURN_V(uavCapable(), false);
	}
	else
	{
		DEBUG_CHECK_RETURN_V(shadeReadable(), false);
	}

	DEBUG_CHECK_RETURN_V(m_stride != 0, false);
	DEBUG_CHECK_RETURN_V(m_stride == stride, false);
	DEBUG_CHECK_RETURN_V(offset < m_size, false);// "Trying to create a view past the buffer size");

	// TODO: check format compatibility

	if (size == INDEX_MAX)
		size = m_size - offset;

	DEBUG_CHECK_RETURN_V((size % stride) == 0, false);
	DEBUG_CHECK_RETURN_V((offset % stride) == 0, false);
	return true;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferView);
RTTI_END_TYPE();

BufferView::BufferView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, ImageFormat format, uint32_t offset, uint32_t size)
    : IDeviceObjectView(viewId, DeviceObjectViewType::Buffer, buffer, impl)
    , m_format(format)
    , m_offset(offset)
    , m_size(size)
{
	ASSERT(buffer != nullptr);
	ASSERT(!buffer->structured());
	ASSERT(buffer->shadeReadable());
	ASSERT(buffer->stride() == 0);
	ASSERT(offset + size <= buffer->size());
}

BufferView::~BufferView()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferConstantView);
RTTI_END_TYPE();

BufferConstantView::BufferConstantView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t offset, uint32_t size)
	: IDeviceObjectView(viewId, DeviceObjectViewType::ConstantBuffer, buffer, impl)
	, m_offset(offset)
	, m_size(size)
{
	ASSERT(buffer != nullptr);
	ASSERT(!buffer->structured());
	ASSERT(buffer->constants());
	ASSERT(buffer->stride() == 0);
	ASSERT((offset & 15) == 0); // vec4 alignment
	ASSERT((size & 15) == 0); // vec4 alignment
	ASSERT(offset + size <= buffer->size());
}

BufferConstantView::~BufferConstantView()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferStructuredView);
RTTI_END_TYPE();

BufferStructuredView::BufferStructuredView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t stride, uint32_t offset, uint32_t size)
	: IDeviceObjectView(viewId, DeviceObjectViewType::BufferStructured, buffer, impl)
	, m_stride(stride)
	, m_offset(offset)
	, m_size(size)
{
	ASSERT(buffer != nullptr);
	ASSERT(buffer->structured());
	ASSERT(buffer->shadeReadable());
	ASSERT(buffer->stride() == stride);
	ASSERT(m_stride != 0);
	ASSERT(m_offset % m_stride == 0);
	ASSERT(m_size % m_stride == 0);
	ASSERT(offset + size <= buffer->size());
}

BufferStructuredView::~BufferStructuredView()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferWritableView);
RTTI_END_TYPE();

BufferWritableView::BufferWritableView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, ImageFormat format, uint32_t offset, uint32_t size)
    : IDeviceObjectView(viewId, DeviceObjectViewType::BufferWritable, buffer, impl)
    , m_format(format)
    , m_offset(offset)
    , m_size(size)
{
	ASSERT(buffer != nullptr);
	ASSERT(!buffer->structured());
	ASSERT(buffer->uavCapable());
	ASSERT(buffer->stride() == 0);
	ASSERT(offset + size <= buffer->size());
}

BufferWritableView::~BufferWritableView()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BufferWritableStructuredView);
RTTI_END_TYPE();

BufferWritableStructuredView::BufferWritableStructuredView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t stride, uint32_t offset, uint32_t size)
	: IDeviceObjectView(viewId, DeviceObjectViewType::BufferStructuredWritable, buffer, impl)
	, m_stride(stride)
	, m_offset(offset)
	, m_size(size)
{
	ASSERT(buffer != nullptr);
	ASSERT(buffer->structured());
	ASSERT(buffer->uavCapable());
	ASSERT(buffer->stride() == stride);
	ASSERT(m_stride != 0);
	ASSERT(m_offset % m_stride == 0);
	ASSERT(m_size % m_stride == 0);
	ASSERT(offset + size <= buffer->size());
}

BufferWritableStructuredView::~BufferWritableStructuredView()
{}

//---

END_BOOMER_NAMESPACE_EX(gpu)
