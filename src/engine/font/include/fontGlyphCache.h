/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once


BEGIN_BOOMER_NAMESPACE_EX(font)

class Glyph;

/// container for cached glyphs
/// NOTE: the glyphs in the cache live untill the cache is flushed
/// NOTE: when the cache is flushed the glyphs cannot be in use anywhere (dangling pointers..)
/// NOTE: glyph cache is NOT THREADSAFE and requires external synchronization
class ENGINE_FONT_API GlyphCache : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_FONTS)

public:
    GlyphCache();
    ~GlyphCache();

    /// get total memory consumed by glyphs
    INLINE uint32_t totalMemoryBytes() const { return m_totalGlyphMemory; }

    /// remove all entries from the glyph cache
    void clear();

    /// purge cache from glyphs older than given generations
    /// this is a simple form of "garbage collecting" - if a glyph is not requested for few generations we can assume it's no longer needed
    /// NOTE: each purge starts a new generation
    /// NOTE: purging with relative generation 0 removes all glyphs from the cache, recommended values is 2 or 3
    void purge(uint32_t relativeGeneration);

    /// get or generate glyph 
    /// NOTE: may fail
    const Glyph* fetchGlyph(const FontStyleParams& styleParams, uint32_t styleHash, FT_Face faceData, FontID fontId, uint32_t glyph);

private:
    typedef uint64_t GlyphKey;

    /// compute a glyph cache key from the glyph properties
    static GlyphKey ComputeKey(FontID fontId, FontGlyphID glyphId, FontStyleHash styleHash);

    // build a glyph data
    Glyph* buildGlyph(const FontStyleParams& styleParams, FT_Face faceData, FontID fontId, uint32_t glyph);

    //---

    struct GlyphEntry
    {
        Glyph* glyph = nullptr;
        uint32_t lastGeneration = 0; // LRU
    };

    /// cached glyphs
    typedef HashMap< GlyphKey, GlyphEntry > TGlyphMap;
    TGlyphMap m_glyphs;

    /// estimated memory consumed by the glyphs
    uint32_t m_totalGlyphMemory;

    /// current generation index
    uint32_t m_generationIndex;

    /// internal locking
    SpinLock m_lock;
};

END_BOOMER_NAMESPACE_EX(font)
