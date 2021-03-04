/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_font_glue.inl"

extern "C"
{
    struct FT_FaceRec_;
    typedef struct FT_FaceRec_*  FT_Face;
}

BEGIN_BOOMER_NAMESPACE_EX(font)

class Font;
class FontInputText;

struct FontStyleParams;
struct FontAssemblyParams;

class Glyph;
class GlyphCache;
class GlyphBuffer;

typedef uint8_t FontID; // id of the font itself, we don't have that many fonts so this is just a number
typedef uint32_t FontGlyphID; // glyph ID, usually this is just the wchar_t
typedef uint32_t FontStyleHash; // render hash of the parameters the glyph was rendered with

END_BOOMER_NAMESPACE_EX(font)

BEGIN_BOOMER_NAMESPACE()

typedef RefPtr<font::Font> FontPtr;
typedef res::Ref<font::Font> FontRef;

//--

// load font from file memory
extern ENGINE_FONT_API FontPtr LoadFontFromMemory(Buffer ptr);

// load font from file
extern ENGINE_FONT_API FontPtr LoadFontFromFile(IReadFileHandle* file);

// load font from absolute file
extern ENGINE_FONT_API FontPtr LoadFontFromAbsolutePath(StringView absolutePath);

// load font from absolute file
extern ENGINE_FONT_API FontPtr LoadFontFromDepotPath(StringView depotPath);

//--

END_BOOMER_NAMESPACE()

