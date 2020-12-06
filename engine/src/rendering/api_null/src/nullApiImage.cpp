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

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			///---

			Image::Image(Thread* drv, const ImageCreationInfo& setup)
				: IBaseImage(drv, setup)
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

			void Image::initializeFromStaging(IBaseCopyQueueStagingArea* data)
			{

			}

			void Image::updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range)
			{

			}

			//--

			ImageAnyView::ImageAnyView(Thread* owner, Image* img, const Setup& setup, ObjectType viewType)
				: IBaseImageView(owner, viewType, img, setup)
			{}

			ImageAnyView::~ImageAnyView()
			{}

			//--

		} // nul
    } // api
} // rendering
