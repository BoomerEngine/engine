/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "dynamicImageAtlas.h"
#include "dynamicImageAtlasEntry.h"

BEGIN_BOOMER_NAMESPACE();

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DynamicImageAtlas);
RTTI_END_TYPE();

DynamicImageAtlas::DynamicImageAtlas(ImageFormat format)
    : IDynamicAtlas(format)
{
}

DynamicImageAtlas::~DynamicImageAtlas()
{
}

void DynamicImageAtlas::attach(DynamicImageAtlasEntry* image)
{
    DEBUG_CHECK_RETURN(image);
    DEBUG_CHECK_RETURN(image->image());
    DEBUG_CHECK_RETURN(!image->rect().empty());
    DEBUG_CHECK_RETURN(image->m_id == INDEX_NONE);
    
    auto lock = CreateLock(m_lock);

    auto id = createEntry_NoLock(image->image(), image->rect(), image->wrapU(), image->wrapV());
    image->m_id = id;
}

void DynamicImageAtlas::remove(DynamicImageAtlasEntry* image)
{
    DEBUG_CHECK_RETURN(image);
    DEBUG_CHECK_RETURN(image->m_id >= 0);
    
    auto lock = CreateLock(m_lock);

    removeEntry_NoLock(image->m_id);
    image->m_id = INDEX_NONE;
}

//--

END_BOOMER_NAMESPACE();
