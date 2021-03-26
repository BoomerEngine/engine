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

/// general atlas - supports both images and font glyphs
class ENGINE_ATLAS_API IAtlas : public IObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(IAtlas, IObject);

public:
	IAtlas();
	virtual ~IAtlas();

	//--

	struct AtlasImageInfo
	{
		uint32_t width = 0;
		uint32_t height = 0;
		bool wrapU = false;
		bool wrapV = false;
	};

	//--

	// describe the entry
	virtual bool describe(AtlasImageID id, AtlasImageInfo& outInfo) const = 0;

    /// shader readable view of the atlas's texture
    virtual const gpu::ImageSampledViewPtr& imageSRV() const = 0;

    /// shader readable buffer with information about entries in the atlas
    virtual const gpu::BufferStructuredViewPtr& imageEntriesSRV() const = 0;

	//--
};

//--

END_BOOMER_NAMESPACE()
