/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_atlas_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

typedef int AtlasImageID; // in general 16-bit only

class IAtlas;
typedef RefPtr<IAtlas> AtlasPtr;

/*class IDynamicAtlas;
typedef RefPtr<IDynamicAtlas> DynamicAtlasPtr;*/

class DynamicImageAtlas;
typedef RefPtr<DynamicImageAtlas> DynamicImageAtlasPtr;

class DynamicGlyphAtlas;
typedef RefPtr<DynamicGlyphAtlas> DynamicGlyphAtlasPtr;

class DynamicImageAtlasEntry;
typedef RefPtr<DynamicImageAtlasEntry> DynamicImageAtlasEntryPtr;

//--

END_BOOMER_NAMESPACE()
