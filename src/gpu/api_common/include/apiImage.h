/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/image.h"

#include "apiObject.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

/// general image view
class GPU_API_COMMON_API IBaseImageView : public IBaseObject
{
public:
	struct Setup
	{
		ImageViewType viewType = ImageViewType::View2D;
		ImageFormat format = ImageFormat::UNKNOWN;

		uint8_t firstMip = 0;
		uint8_t numMips = 0;
		uint16_t firstSlice = 0;
		uint16_t numSlices = 0;				
	};

	IBaseImageView(IBaseThread* owner, ObjectType viewType, IBaseImage* img, const Setup& setup);
	virtual ~IBaseImageView();

	//--

	INLINE IBaseImage* image() const { return m_image; }
	INLINE const Setup& setup() const { return m_setup; }

	//--

private:
	IBaseImage* m_image = nullptr;

	Setup m_setup;
};

//---

/// wrapper for image
class GPU_API_COMMON_API IBaseImage : public IBaseCopiableObject
{
public:
	IBaseImage(IBaseThread* owner, const ImageCreationInfo& setup, const ISourceDataProvider* initData);
	virtual ~IBaseImage();

	static const auto STATIC_TYPE = ObjectType::Image;

	//--

	// get original setup
	INLINE const ImageCreationInfo& setup() const { return m_setup; }

	//---

	virtual IBaseImageView* createSampledView_ClientApi(const IBaseImageView::Setup& setup) = 0;
	virtual IBaseImageView* createReadOnlyView_ClientApi(const IBaseImageView::Setup& setup) = 0;
	virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) = 0;
	virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) = 0;

	//--

protected:
	ImageCreationInfo m_setup;

	SourceDataProviderPtr m_initData;

	PoolTag m_poolTag = POOL_API_STATIC_TEXTURES;
	uint32_t m_poolMemorySize = 0;
};

//---

// client side proxy for image object
class GPU_API_COMMON_API ImageObjectProxy : public ImageObject
{
public:
	ImageObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup);

	virtual ImageSampledViewPtr createSampledView(uint32_t firstMip, uint32_t firstSlice) override;
	virtual ImageSampledViewPtr createSampledViewEx(ImageViewType viewType, uint32_t firstMip, uint32_t firstSlice, uint32_t numMips, uint32_t numSlices) override;
	virtual ImageReadOnlyViewPtr createReadOnlyView(uint32_t mip = 0, uint32_t slice = 0) override;
	virtual ImageWritableViewPtr createWritableView(uint32_t mip, uint32_t slice) override;
	virtual RenderTargetViewPtr createRenderTargetView(uint32_t mip, uint32_t firstSlice, uint32_t numSlices) override;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
