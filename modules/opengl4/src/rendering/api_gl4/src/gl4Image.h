/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiImage.h"

#include "gl4Sampler.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			///---

			class ImageAnyView;

			///---

			/// wrapper for image
			class Image : public IBaseImage
			{
			public:
				Image(Thread* drv, const ImageCreationInfo& setup);
				virtual ~Image();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				INLINE GLuint format() const { return m_glFormat; }
				INLINE GLuint viewType() const { return m_glViewType; } // GL_TEXTURE_2D

				INLINE GLuint object() { ensureCreated(); return m_glImage; }

				//--

				virtual IBaseImageView* createView_ClientApi(const IBaseImageView::Setup& setup, IBaseSampler* sampler) override final;
				virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) override final;
				virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) override final;

				//--

				virtual void applyCopyAtoms(const base::Array<ResourceCopyAtom>& atoms, Frame* frame, const StagingArea& area) override final;

				void copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range);

				//--

				ResolvedImageView resolve();

				//--

			private:
				GLuint m_glImage = 0;
				GLuint m_glFormat = 0;
				GLuint m_glViewType = 0;

				void ensureCreated();

				friend class ImageAnyView;
			};

			//--

			class ImageAnyView : public IBaseImageView
			{
			public:
				ImageAnyView(Thread* owner, Image* img, Sampler* sampler, const Setup& setup);
				virtual ~ImageAnyView();

				static const ObjectType STATIC_TYPE = ObjectType::ImageView;

				INLINE Image* image() const { return static_cast<Image*>(IBaseImageView::image()); }
				INLINE Sampler* sampler() const { return static_cast<Sampler*>(IBaseImageView::sampler());; }
				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				ResolvedImageView resolve();

			private:
				GLuint m_glViewObject = 0;

				void ensureCreated();
			};

			//--

		} // gl4
    } // api
} // rendering