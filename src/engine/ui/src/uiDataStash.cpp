/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: host #]
***/

#include "build.h"
#include "uiDataStash.h"
#include "uiStyleLibrary.h"
#include "uiElement.h"
#include "uiWindow.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataStash);
    RTTI_PROPERTY(m_styles);
RTTI_END_TYPE();

//---

DataStash::DataStash(StringView stylesDepotPath)
{
	m_imageSearchPaths.pushBack("/engine/interface/icons/");
	m_imageSearchPaths.pushBack("/engine/interface/images/");

	if (!stylesDepotPath)
		stylesDepotPath = "/engine/interface/styles/flat.scss";

	m_styles = style::LoadStyleLibrary(stylesDepotPath);
	if (!m_styles)
		m_styles = RefNew<style::Library>();
}

DataStash::~DataStash()
{
}

void DataStash::onPropertyChanged(StringView path)
{
    TBaseClass::onPropertyChanged(path);

    if (path == "styles")
    {
        TRACE_INFO("UI styles were changed, all UI layouts will refresh");
        IElement::InvalidateAllDataEverywhere();
        m_stylesVersion += 1;
    }
}
    
void DataStash::addIconSearchPath(StringView path)
{
    if (!path.empty())
        m_imageSearchPaths.pushBackUnique(StringBuf(path));
}

void DataStash::removeIconSearchPath(StringView path)
{
	m_imageSearchPaths.remove(path);
}

void DataStash::conditionalRebuildAtlases()
{
		
}

CanvasImagePtr DataStash::cacheImage(const Image* img, bool supportWrapping /*= false*/, uint8_t additionalPadding /*= 0*/)
{
	if (!img)
		return nullptr;

	if (const auto* ret = m_imagePtrMap.find(img->runtimeUniqueId()))
		return *ret;

	auto entry = RefNew<CanvasImage>(img, supportWrapping, supportWrapping);

	m_imagePtrMap[img->runtimeUniqueId()] = entry;
	return entry;
}

CanvasImagePtr DataStash::loadImage(StringID key)
{
	if (const auto* ret = m_imageMap.find(key))
		return *ret;

	for (const auto& searchPath : m_imageSearchPaths)
	{	
		if (auto imagePtr = LoadImageFromDepotPath(TempString("{}{}.png", searchPath, key)))
		{
			auto entry = cacheImage(imagePtr);
			m_imageMap[key] = entry;
			return entry;
		}
    }

	return nullptr;
}

//---

END_BOOMER_NAMESPACE_EX(ui)
