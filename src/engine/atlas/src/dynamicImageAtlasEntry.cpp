/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "dynamicImageAtlasEntry.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE();

//--

RTTI_BEGIN_TYPE_CLASS(DynamicImageAtlasEntry);
RTTI_END_TYPE();

DynamicImageAtlasEntry::DynamicImageAtlasEntry()
{}

DynamicImageAtlasEntry::DynamicImageAtlasEntry(const Image* image, bool wrapU /*= false*/, bool wrapV /*= false*/)
    : m_image(AddRef(image))
    , m_rect(0, 0, image->width(), image->height())
    , m_wrapU(wrapU)
    , m_wrapV(wrapV)
{
}

DynamicImageAtlasEntry::DynamicImageAtlasEntry(const Image* image, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool wrapU /*= false*/, bool wrapV /*= false*/)
    : m_image(AddRef(image))
    , m_rect(x, y, x + w, y + h)
    , m_wrapU(wrapU)
    , m_wrapV(wrapV)
{
    DEBUG_CHECK_EX(x + w <= image->width(), "Invalid range");
    DEBUG_CHECK_EX(y + h <= image->height(), "Invalid range");
}

DynamicImageAtlasEntry::~DynamicImageAtlasEntry()
{

}

//--

END_BOOMER_NAMESPACE();
