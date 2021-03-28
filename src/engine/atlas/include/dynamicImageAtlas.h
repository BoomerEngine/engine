/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "dynamicAtlas.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// abstract canvas atlas
class ENGINE_ATLAS_API DynamicImageAtlas : public IDynamicAtlas
{
    RTTI_DECLARE_VIRTUAL_CLASS(DynamicImageAtlas, IDynamicAtlas);

public:
	DynamicImageAtlas(ImageFormat format);
	virtual ~DynamicImageAtlas();

	//--

	// add dynamic image to atlas
    void attach(DynamicImageAtlasEntry* image);

    // remove image from atlas
    void remove(DynamicImageAtlasEntry* image);

	//--
};

//--

END_BOOMER_NAMESPACE()
