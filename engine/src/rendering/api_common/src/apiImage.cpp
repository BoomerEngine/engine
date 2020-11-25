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

namespace rendering
{
    namespace api
    {
		//--

		IBaseImageView::IBaseImageView(IBaseThread* owner, ObjectType viewType, IBaseImage* img, IBaseSampler* sampler, const Setup& setup)
			: IBaseObject(owner, viewType)
			, m_image(img)
			, m_sampler(sampler)
			, m_setup(setup)
		{}

		IBaseImageView::~IBaseImageView()
		{}

		//--

		IBaseImage::IBaseImage(IBaseThread* owner, const ImageCreationInfo& setup)
			: IBaseCopiableObject(owner, ObjectType::Image)
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

		bool IBaseImage::generateCopyAtoms(const ResourceCopyRange& range, base::Array<ResourceCopyAtom>& outAtoms, uint32_t& outStagingAreaSize, uint32_t& outStagingAreaAlignment) const
		{
			DEBUG_CHECK_RETURN_V(range.image.firstMip < m_setup.numMips, false);
			DEBUG_CHECK_RETURN_V(range.image.firstMip + range.image.numMips <= m_setup.numMips, false);
			DEBUG_CHECK_RETURN_V(range.image.firstSlice < m_setup.numSlices, false);
			DEBUG_CHECK_RETURN_V(range.image.firstSlice + range.image.numSlices <= m_setup.numSlices, false);

			outStagingAreaAlignment = 256;
			outStagingAreaSize = 0;

			// create standard update layout
			for (uint32_t i = 0; i < range.image.numSlices; ++i)
			{
				for (uint32_t j = 0; j < range.image.numMips; ++j)
				{
					auto& atom = outAtoms.emplaceBack();
					atom.copyElement.image.mip = range.image.firstMip + j;
					atom.copyElement.image.slice = range.image.firstSlice + i;

					atom.stagingAreaOffset = base::Align<uint32_t>(outStagingAreaSize, 16);
					outStagingAreaSize += m_setup.calcMipDataSize(j);
				}
			}

			return true;
		}

		//--

		ImageObjectProxy::ImageObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup)
			: ImageObject(id, impl, setup)
		{
		}

		ImageViewPtr ImageObjectProxy::createView(SamplerObject* sampler, uint8_t firstMip, uint8_t numMips)
		{
			rendering::ImageView::Setup setup;
			if (!validateView(sampler, firstMip, numMips, 0, slices(), setup))
				return nullptr;

			auto* samplerObj = sampler->resolveInternalApiObject<IBaseSampler>();
			DEBUG_CHECK_RETURN_V(samplerObj, nullptr);

			if (auto* obj = resolveInternalApiObject<IBaseImage>())
			{
				IBaseImageView::Setup localSetup;
				localSetup.firstMip = setup.firstMip;
				localSetup.firstSlice = setup.firstSlice;
				localSetup.numMips = setup.numMips;
				localSetup.numSlices = setup.numSlices;
				localSetup.format = obj->setup().format;
				localSetup.writable = false;

				if (auto* view = obj->createView_ClientApi(localSetup, samplerObj))
					return base::RefNew<rendering::ImageView>(view->handle(), this, owner(), setup);
			}

			return nullptr;
		}

		ImageViewPtr ImageObjectProxy::createArrayView(SamplerObject* sampler, uint8_t firstMip, uint8_t numMips, uint32_t firstSlice, uint32_t numSlices)
		{
			rendering::ImageView::Setup setup;
			if (!validateView(sampler, firstMip, numMips, firstSlice, numMips, setup))
				return nullptr;

			auto* samplerObj = sampler->resolveInternalApiObject<IBaseSampler>();
			DEBUG_CHECK_RETURN_V(samplerObj, nullptr);

			if (auto* obj = resolveInternalApiObject<IBaseImage>())
			{
				IBaseImageView::Setup localSetup;
				localSetup.firstMip = setup.firstMip;
				localSetup.firstSlice = setup.firstSlice;
				localSetup.numMips = setup.numMips;
				localSetup.numSlices = setup.numSlices;
				localSetup.format = obj->setup().format;
				localSetup.writable = false;

				if (auto* view = obj->createView_ClientApi(localSetup, samplerObj))
					return base::RefNew<rendering::ImageView>(view->handle(), this, owner(), setup);
			}

			return nullptr;
		}

		ImageWritableViewPtr ImageObjectProxy::createWritableView(uint8_t mip, uint32_t slice)
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
				localSetup.writable = false;

				if (auto* view = obj->createWritableView_ClientApi(localSetup))
					return base::RefNew<rendering::ImageWritableView>(view->handle(), this, owner(), setup);
			}

			return nullptr;
		}

		RenderTargetViewPtr ImageObjectProxy::createRenderTargetView(uint8_t mip, uint32_t firstSlice, uint32_t numSlices)
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
				localSetup.writable = false;

				if (auto* view = obj->createRenderTargetView_ClientApi(localSetup))
					return base::RefNew<rendering::RenderTargetView>(view->handle(), this, owner(), setup);
			}

			return nullptr;
		}

		//--

    } // api
} // rendering
