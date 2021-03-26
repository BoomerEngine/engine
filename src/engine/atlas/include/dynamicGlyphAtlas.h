/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "dynamicAtlas.h"
#include "engine/font/include/fontGlyph.h"
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// texture array based image cache for font glyphs
class ENGINE_ATLAS_API DynamicGlyphAtlas : public IDynamicAtlas
{
	RTTI_DECLARE_POOL(POOL_CANVAS);

public:
	DynamicGlyphAtlas();
	virtual ~DynamicGlyphAtlas();

	//--

	// map a single font glyph
	AtlasImageID mapGlyph(const FontGlyph& glyph);

	// map multiple glyphs (saves on lock)
	void mapGlyphs(const FontGlyphBuffer& glyphs, Array<AtlasImageID>& outImages);

	//--

private:
	SpinLock m_glyphToImageMapLock;
	HashMap<FontGlyphKey, AtlasImageID> m_glyphToImageMap;
};

//--

END_BOOMER_NAMESPACE()


