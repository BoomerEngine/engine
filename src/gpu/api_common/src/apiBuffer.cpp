/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiBuffer.h"
#include "apiObjectRegistry.h"

#include "core/memory/include/poolStats.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

IBaseBufferView::IBaseBufferView(IBaseThread* owner, IBaseBuffer* buffer, ObjectType viewType, const Setup& setup)
	: IBaseObject(owner, viewType)
	, m_setup(setup)
	, m_buffer(buffer)
{}

IBaseBufferView::~IBaseBufferView()
{}
		
//--

static PoolTag DetermineBestMemoryPool(const BufferCreationInfo& setup)
{
	if (setup.allowVertex)
		return POOL_API_VERTEX_BUFFER;
	if (setup.allowIndex)
		return POOL_API_INDEX_BUFFER;
	if (setup.allowCostantReads)
		return POOL_API_CONSTANT_BUFFER;
	if (setup.allowIndirect)
		return POOL_API_INDIRECT_BUFFER;
	return POOL_API_STORAGE_BUFFER;
}

IBaseBuffer::IBaseBuffer(IBaseThread* owner, const BufferCreationInfo& setup, const ISourceDataProvider* initData)
	: IBaseCopiableObject(owner, ObjectType::Buffer)
	, m_initData(AddRef(initData))
	, m_setup(setup)
{
	m_poolTag = DetermineBestMemoryPool(setup);
	PoolNotifyAllocation(m_poolTag, m_setup.size);
}

IBaseBuffer::~IBaseBuffer()
{
	PoolNotifyFree(m_poolTag, m_setup.size);
}

//--

BufferObjectProxy::BufferObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup)
	: BufferObject(id, impl, setup)
{}

BufferConstantViewPtr BufferObjectProxy::createConstantView(uint32_t offset/* = 0*/, uint32_t size /*= INDEX_MAX*/)
{
	if (!validateConstantView(offset, size))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseBuffer>())
		if (auto* view = obj->createConstantView_ClientApi(offset, size))
			return RefNew<gpu::BufferConstantView>(view->handle(), this, owner(), offset, size);

	return nullptr;
}

BufferViewPtr BufferObjectProxy::createView(ImageFormat format, uint32_t offset, uint32_t size)
{
	if (!validateTypedView(format, offset, size, false))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseBuffer>())
		if (auto* view = obj->createView_ClientApi(format, offset, size))
			return RefNew<gpu::BufferView>(view->handle(), this, owner(), format, offset, size);

	return nullptr;
}

BufferStructuredViewPtr BufferObjectProxy::createStructuredView(uint32_t offset, uint32_t size)
{
	if (!validateStructureView(stride(), offset, size, false))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseBuffer>())
		if (auto* view = obj->createStructuredView_ClientApi(offset, size))
			return RefNew<gpu::BufferStructuredView>(view->handle(), this, owner(), stride(), offset, size);

	return nullptr;
}

BufferWritableViewPtr BufferObjectProxy::createWritableView(ImageFormat format, uint32_t offset, uint32_t size)
{
	if (!validateTypedView(format, offset, size, true))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseBuffer>())
		if (auto* view = obj->createWritableView_ClientApi(format, offset, size))
			return RefNew<gpu::BufferWritableView>(view->handle(), this, owner(), format, offset, size);

	return nullptr;
}

BufferWritableStructuredViewPtr BufferObjectProxy::createWritableStructuredView(uint32_t offset, uint32_t size)
{
	if (!validateStructureView(stride(), offset, size, true))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseBuffer>())
		if (auto* view = obj->createWritableStructuredView_ClientApi(offset, size))
			return RefNew<gpu::BufferWritableStructuredView>(view->handle(), this, owner(), stride(), offset, size);

	return nullptr;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
