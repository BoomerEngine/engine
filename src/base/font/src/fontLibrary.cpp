/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "fontLibrary.h"
#include "base/system/include/scopeLock.h"

BEGIN_BOOMER_NAMESPACE(base::font)

//---

FontLibrary::FontLibrary()
    : m_numLoadedFaces(0)
    , m_library(nullptr)
{}

void FontLibrary::conditionalInitialize_NoLock()
{
    if (0 == m_numLoadedFaces++)
    {
        DEBUG_CHECK(m_library == nullptr);

        FT_Error err = FT_Init_FreeType(&m_library);
        if (err != 0)
        {
            TRACE_ERROR("Failed to initalize FreeType font library. Error code = 0x%X", err);
        }
        else
        {
            TRACE_INFO("Initialized font library");
        }
    }
}

void FontLibrary::conditionalShutdown_NoLock()
{
    DEBUG_CHECK(m_numLoadedFaces > 0);

    if (0 == --m_numLoadedFaces)
    {
        if (m_library != nullptr)
        {
            FT_Done_FreeType(m_library);
            m_library = nullptr;

            TRACE_INFO("Shutdown font library");
        }
    }
}

FT_Face FontLibrary::loadFace(const void* data, uint32_t dataSize)
{
    ScopeLock<> lock(m_lock);

    // Make sure the library is initialized
    conditionalInitialize_NoLock();

    // load font
    if (m_library != nullptr)
    {
        // Load file
        FT_Face face = NULL;
        FT_Error err = FT_New_Memory_Face(m_library, (const FT_Byte*)data, dataSize, 0, &face);
        if (err == 0 && face)
        {
            TRACE_INFO("Loaded font from memory buffer, family {}, {} glyphs", face->family_name, face->num_glyphs);
            return face;
        }
        else
        {
            TRACE_ERROR("failed to load font from memory buffer. Error code = {}\n", Hex(err));
        }
    }
    else
    {
        TRACE_ERROR("failed to load font from memory buffer because there's no font library");
    }

    conditionalShutdown_NoLock();
    return nullptr;
}

void FontLibrary::freeFace(FT_Face face)
{
    if (face)
    {
        ScopeLock<> lock(m_lock);

        // release the font data (under lock..)
        FT_Done_Face(face);

        // deinitalize the library if there are no more fonts
        conditionalShutdown_NoLock();
    }
}

END_BOOMER_NAMESPACE(base::font)
