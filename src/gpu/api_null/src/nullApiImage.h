/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiImage.h"

#include "nullApiSampler.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

///---

/// wrapper for image
class Image : public IBaseImage
{
public:
	Image(Thread* drv, const ImageCreationInfo& setup, const ISourceDataProvider* sourceData);
	virtual ~Image();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

	//--

	virtual IBaseImageView* createSampledView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createReadOnlyView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) override final;

	//--

	virtual void updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range) override final;
	virtual void copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;
	virtual void copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;

	//--

private:
	// internals
};

//--

class ImageAnyView : public IBaseImageView
{
public:
	ImageAnyView(Thread* owner, Image* img, const Setup& setup, ObjectType viewType);
	virtual ~ImageAnyView();

	INLINE Image* image() const { return static_cast<Image*>(IBaseImageView::image()); }
	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

private:
	// internals
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
