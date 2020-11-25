/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingImage.h"

#include "apiObject.h"

namespace rendering
{
	namespace api
	{

		//---

		/// general image view
		class RENDERING_API_COMMON_API IBaseImageView : public IBaseObject
		{
		public:
			struct Setup
			{
				ImageViewType viewType = ImageViewType::View2D;
				ImageFormat format = ImageFormat::UNKNOWN;

				bool writable = false;

				uint8_t firstMip = 0;
				uint8_t numMips = 0;
				uint16_t firstSlice = 0;
				uint16_t numSlices = 0;
			};

			IBaseImageView(IBaseThread* owner, ObjectType viewType, IBaseImage* img, IBaseSampler* sampler, const Setup& setup);
			virtual ~IBaseImageView();

			//--

			INLINE IBaseImage* image() const { return m_image; }
			INLINE IBaseSampler* sampler() const { return m_sampler; }
			INLINE const Setup& setup() const { return m_setup; }

			//--

		private:
			IBaseImage* m_image = nullptr;
			IBaseSampler* m_sampler = nullptr;

			Setup m_setup;
		};

		//---

		/// wrapper for image
		class RENDERING_API_COMMON_API IBaseImage : public IBaseCopiableObject
		{
		public:
			IBaseImage(IBaseThread* owner, const ImageCreationInfo& setup);
			virtual ~IBaseImage();

			static const auto STATIC_TYPE = ObjectType::Image;

			//--

			// get original setup
			INLINE const ImageCreationInfo& setup() const { return m_setup; }

			//---

			virtual IBaseImageView* createView_ClientApi(const IBaseImageView::Setup& setup, IBaseSampler* sampler) = 0;
			virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) = 0;
			virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) = 0;

			//---

			// generate default atoms for image (each mip and slice is copied separately)
			// NOTE: default copying sucks for the lowest mips very badly :(
			virtual bool generateCopyAtoms(const ResourceCopyRange& range, base::Array<ResourceCopyAtom>& outAtoms, uint32_t& outStagingAreaSize, uint32_t& outStagingAreaAlignment) const override;

			//---

		private:
			ImageCreationInfo m_setup;

			PoolTag m_poolTag = POOL_API_STATIC_TEXTURES;
			uint32_t m_poolMemorySize = 0;
		};

		//---

		// client side proxy for image object
		class RENDERING_API_COMMON_API ImageObjectProxy : public ImageObject
		{
		public:
			ImageObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup);

			virtual ImageViewPtr createView(SamplerObject* sampler, uint8_t firstMip, uint8_t numMips) override;
			virtual ImageViewPtr createArrayView(SamplerObject* sampler, uint8_t firstMip, uint8_t numMips, uint32_t firstSlice, uint32_t numSlices) override;
			virtual ImageWritableViewPtr createWritableView(uint8_t mip, uint32_t slice) override;
			virtual RenderTargetViewPtr createRenderTargetView(uint8_t mip, uint32_t firstSlice, uint32_t numSlices) override;
		};

		//--

	} // api
} // rendering