/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiImage.h"

#include "dx11Sampler.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			///---

			/// wrapper for image
			class Image : public IBaseImage
			{
			public:
				Image(Thread* drv, const ImageCreationInfo& setup);
				virtual ~Image();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				//--

				virtual IBaseImageView* createView_ClientApi(const IBaseImageView::Setup& setup) override final;
				virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) override final;
				virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) override final;

				//--

				virtual void applyCopyAtoms(const base::Array<ResourceCopyAtom>& atoms, Frame* frame, const StagingArea& area) override final;

				//--

			private:
				// internals
			};

			//--

			class ImageAnyView : public IBaseImageView
			{
			public:
				ImageAnyView(Thread* owner, Image* img, const Setup& setup);
				virtual ~ImageAnyView();

				INLINE Image* image() const { return static_cast<Image*>(IBaseImageView::image()); }
				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

			private:
				// internals
			};

			//--

		} // dx11
    } // api
} // rendering