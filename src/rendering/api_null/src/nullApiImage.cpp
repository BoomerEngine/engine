/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"

#include "nullApiThread.h"
#include "nullApiImage.h"

#include "base/memory/include/poolStats.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

///---

Image::Image(Thread* drv, const ImageCreationInfo& setup, const ISourceDataProvider* sourceData)
	: IBaseImage(drv, setup, sourceData)
{}

Image::~Image()
{}

//--

IBaseImageView* Image::createSampledView_ClientApi(const IBaseImageView::Setup& setup)
{
	return new ImageAnyView(owner(), this, setup, ObjectType::SampledImageView);
}

IBaseImageView* Image::createReadOnlyView_ClientApi(const IBaseImageView::Setup& setup)
{
	return new ImageAnyView(owner(), this, setup, ObjectType::ImageReadOnlyView);
}

IBaseImageView* Image::createWritableView_ClientApi(const IBaseImageView::Setup& setup)
{
	return new ImageAnyView(owner(), this, setup, ObjectType::ImageWritableView);
}

IBaseImageView* Image::createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup)
{
	return new ImageAnyView(owner(), this, setup, ObjectType::RenderTargetView);
}

//--

void Image::updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range)
{

}

void Image::copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
{

}

void Image::copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
{

}

//--

ImageAnyView::ImageAnyView(Thread* owner, Image* img, const Setup& setup, ObjectType viewType)
	: IBaseImageView(owner, viewType, img, setup)
{}

ImageAnyView::~ImageAnyView()
{}

//--

END_BOOMER_NAMESPACE(rendering::api::nul)