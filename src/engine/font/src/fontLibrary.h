/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "core/system/include/mutex.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

/// internal font library, contains code and data shared between ALL fonts
/// mostly needed because of free type 
class FontLibrary : public ISingleton
{
    DECLARE_SINGLETON(FontLibrary);

public:
    /// load font from memory
    FT_Face loadFace(const void* data, uint32_t dataSize);

    /// free loaded font
    void freeFace(FT_Face face);

private:
    FontLibrary();

    FT_Library m_library;
    uint32_t m_numLoadedFaces;

    Mutex m_lock; // resources may be loaded from multiple threads, we need to sync the access to the library

    void conditionalInitialize_NoLock();
    void conditionalShutdown_NoLock();
};

END_BOOMER_NAMESPACE_EX(font)
