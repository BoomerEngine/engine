/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\image #]
***/

#include "build.h"

#include "dx11Thread.h"
#include "dx11Image.h"

#include "base/memory/include/poolStats.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			///---

			Image::Image(Thread* drv, const ImageCreationInfo& setup)
				: IBaseImage(drv, setup)
			{}

			Image::~Image()
			{}

			//--

			IBaseImageView* Image::createView_ClientApi(const IBaseImageView::Setup& setup, IBaseSampler* sampler)
			{
				auto* localSampler = static_cast<Sampler*>(sampler);
				return new ImageAnyView(owner(), this, localSampler, setup);
			}

			IBaseImageView* Image::createWritableView_ClientApi(const IBaseImageView::Setup& setup)
			{
				return new ImageAnyView(owner(), this, nullptr, setup);
			}

			IBaseImageView* Image::createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup)
			{
				return new ImageAnyView(owner(), this, nullptr, setup);
			}

			//--

			void Image::applyCopyAtoms(const base::Array<ResourceCopyAtom>& atoms, Frame* frame, const StagingArea& area)
			{

			}

			//--

			ImageAnyView::ImageAnyView(Thread* owner, Image* img, Sampler* sampler, const Setup& setup)
				: IBaseImageView(owner, setup.writable ? ObjectType::ImageWritableView : ObjectType::ImageView, img, sampler, setup)
			{}

			ImageAnyView::~ImageAnyView()
			{}

			//--

		} // dx11
    } // api
} // rendering
