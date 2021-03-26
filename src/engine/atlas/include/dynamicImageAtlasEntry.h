/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// entry in dynamic image atlas
class ENGINE_ATLAS_API DynamicImageAtlasEntry : public IObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(DynamicImageAtlasEntry, IObject);

public:
	DynamicImageAtlasEntry();
	DynamicImageAtlasEntry(Image* image, bool wrapU = false, bool wrapV = false);
	DynamicImageAtlasEntry(Image* image, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool wrapU = false, bool wrapV = false);
	virtual ~DynamicImageAtlasEntry();

	//--

    INLINE const ImagePtr& image() const { return m_image; }

	INLINE const Rect& rect() const { return m_rect; }

    INLINE uint32_t width() const { return m_rect.width(); }
    INLINE uint32_t height() const { return m_rect.height(); }

	INLINE bool wrapU() const { return m_wrapU; }
	INLINE bool wrapV() const { return m_wrapV; }

    INLINE AtlasImageID id() const { return m_id; }

	//--

private:
	AtlasImageID m_id = INDEX_NONE;

    ImagePtr m_image; // payload, should always be valid
	Rect m_rect; // source are in the image

	bool m_wrapU = false;
	bool m_wrapV = false;

	friend class DynamicImageAtlas;
};

//--

END_BOOMER_NAMESPACE()
