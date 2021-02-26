/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiImage.h"

#include "gl4Sampler.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

///---

class ImageAnyView;

///---

/// wrapper for image
class Image : public IBaseImage
{
public:
	Image(Thread* drv, const ImageCreationInfo& setup, const ISourceDataProvider* sourceData);
	virtual ~Image();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

	INLINE GLuint format() const { return m_glFormat; }
	INLINE GLuint viewType() const { return m_glViewType; } // GL_TEXTURE_2D

	INLINE GLuint object() { ensureCreated(); return m_glImage; }

	//--

	virtual IBaseImageView* createSampledView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createReadOnlyView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createWritableView_ClientApi(const IBaseImageView::Setup& setup) override final;
	virtual IBaseImageView* createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup) override final;

	//--

	virtual void updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range) override final;
	virtual void copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;
	virtual void copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;

	void copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range);
	void copyFromImage(const ResolvedImageView& view, const ResourceCopyRange& src, const ResourceCopyRange& dest);
    void download(const DownloadArea* area, const ResourceCopyRange& range);

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
	ImageAnyView(Thread* owner, Image* img, const Setup& setup, ObjectType viewType);
	virtual ~ImageAnyView();

	INLINE Image* image() const { return static_cast<Image*>(IBaseImageView::image()); }
	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

	ResolvedImageView resolve();

private:
	GLuint m_glViewType = 0;
	GLuint m_glViewObject = 0;

	void ensureCreated();
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
