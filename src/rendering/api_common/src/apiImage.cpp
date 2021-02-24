/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiImage.h"
#include "apiSampler.h"

#include "base/memory/include/poolStats.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//--

IBaseImageView::IBaseImageView(IBaseThread* owner, ObjectType viewType, IBaseImage* img, const Setup& setup)
	: IBaseObject(owner, viewType)
	, m_image(img)
	, m_setup(setup)
{}

IBaseImageView::~IBaseImageView()
{}

//--

IBaseImage::IBaseImage(IBaseThread* owner, const ImageCreationInfo& setup, const ISourceDataProvider* initData)
	: IBaseCopiableObject(owner, ObjectType::Image)
	, m_initData(AddRef(initData))
	, m_setup(setup)
{
	if (m_setup.initialLayout == ResourceLayout::INVALID)
		m_setup.initialLayout = m_setup.computeDefaultLayout();

	if (setup.allowRenderTarget)
		m_poolTag = POOL_API_RENDER_TARGETS;
	else if (setup.allowUAV)
		m_poolTag = POOL_API_STORAGE_BUFFER;
	else if (setup.allowDynamicUpdate)
		m_poolTag = POOL_API_DYNAMIC_TEXTURES;
	else
		m_poolTag = POOL_API_STATIC_TEXTURES;

	m_poolMemorySize = setup.calcMemoryUsage();
	base::mem::PoolStats::GetInstance().notifyAllocation(m_poolTag, m_poolMemorySize);
}

IBaseImage::~IBaseImage()
{
	base::mem::PoolStats::GetInstance().notifyFree(m_poolTag, m_poolMemorySize);
}

//--

ImageObjectProxy::ImageObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup)
	: ImageObject(id, impl, setup)
{
}

ImageSampledViewPtr ImageObjectProxy::createSampledView(uint32_t firstMip /*= 0*/, uint32_t firstSlice /*= 0*/)
{
	rendering::ImageSampledView::Setup setup;
	if (!validateSampledView(firstMip, INDEX_MAX, firstSlice, INDEX_MAX, setup))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseImage>())
	{
		IBaseImageView::Setup localSetup;
		localSetup.firstMip = setup.firstMip;
		localSetup.firstSlice = setup.firstSlice;
		localSetup.numMips = setup.numMips;
		localSetup.numSlices = setup.numSlices;
		localSetup.format = obj->setup().format;

		if (auto* view = obj->createSampledView_ClientApi(localSetup))
			return base::RefNew<rendering::ImageSampledView>(view->handle(), this, owner(), setup);
	}

	return nullptr;
}

ImageSampledViewPtr ImageObjectProxy::createSampledViewEx(uint32_t firstMip, uint32_t firstSlice, uint32_t numMips, uint32_t numSlices)
{
	rendering::ImageSampledView::Setup setup;
	if (!validateSampledView(firstMip, numMips, firstSlice, numSlices, setup))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseImage>())
	{
		IBaseImageView::Setup localSetup;
		localSetup.firstMip = setup.firstMip;
		localSetup.firstSlice = setup.firstSlice;
		localSetup.numMips = setup.numMips;
		localSetup.numSlices = setup.numSlices;
		localSetup.format = obj->setup().format;

		if (auto* view = obj->createSampledView_ClientApi(localSetup))
			return base::RefNew<rendering::ImageSampledView>(view->handle(), this, owner(), setup);
	}

	return nullptr;
}

ImageReadOnlyViewPtr ImageObjectProxy::createReadOnlyView(uint32_t mip, uint32_t slice)
{
	rendering::ImageReadOnlyView::Setup setup;
	if (!validateReadOnlyView(mip, slice, setup))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseImage>())
	{
		IBaseImageView::Setup localSetup;
		localSetup.firstMip = setup.mip;
		localSetup.firstSlice = setup.slice;
		localSetup.numMips = 1;
		localSetup.numSlices = 1;
		localSetup.format = obj->setup().format;

		if (auto* view = obj->createReadOnlyView_ClientApi(localSetup))
			return base::RefNew<rendering::ImageReadOnlyView>(view->handle(), this, owner(), setup);
	}

	return nullptr;
}

ImageWritableViewPtr ImageObjectProxy::createWritableView(uint32_t mip, uint32_t slice)
{
	rendering::ImageWritableView::Setup setup;
	if (!validateWritableView(mip, slice, setup))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseImage>())
	{
		IBaseImageView::Setup localSetup;
		localSetup.firstMip = setup.mip;
		localSetup.firstSlice = setup.slice;
		localSetup.numMips = 1;
		localSetup.numSlices = 1;
		localSetup.format = obj->setup().format;

		if (auto* view = obj->createWritableView_ClientApi(localSetup))
			return base::RefNew<rendering::ImageWritableView>(view->handle(), this, owner(), setup);
	}

	return nullptr;
}

RenderTargetViewPtr ImageObjectProxy::createRenderTargetView(uint32_t mip, uint32_t firstSlice, uint32_t numSlices)
{
	rendering::RenderTargetView::Setup setup;
	if (!validateRenderTargetView(mip, firstSlice, numSlices, setup))
		return nullptr;

	if (auto* obj = resolveInternalApiObject<IBaseImage>())
	{
		IBaseImageView::Setup localSetup;
		localSetup.firstMip = setup.mip;
		localSetup.firstSlice = setup.firstSlice;
		localSetup.numMips = 1;
		localSetup.numSlices = setup.numSlices;
		localSetup.format = obj->setup().format;

		if (auto* view = obj->createRenderTargetView_ClientApi(localSetup))
			return base::RefNew<rendering::RenderTargetView>(view->handle(), this, owner(), setup);
	}

	return nullptr;
}

//--

END_BOOMER_NAMESPACE(rendering::api)