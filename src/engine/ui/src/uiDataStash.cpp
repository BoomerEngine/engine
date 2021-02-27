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
#include "engine/canvas/include/atlas.h"

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

	m_mainIconAtlas = RefNew<canvas::DynamicAtlas>(1024, 1);
}

DataStash::~DataStash()
{
	m_mainIconAtlas.reset();
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
    DEBUG_CHECK_RETURN(ValidateDepotPath(path, DepotPathClass::AbsoluteDirectoryPath));

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

canvas::ImageEntry DataStash::cacheImage(const image::Image* img, bool supportWrapping /*= false*/, uint8_t additionalPadding /*= 0*/)
{
	if (const auto* ret = m_imagePtrMap.find(img))
		return *ret;

	if (img)
	{
		if (auto entry = m_mainIconAtlas->registerImage(img, supportWrapping, additionalPadding))
		{
			m_imagePtrMap[img] = entry;
			return entry;
		}
	}

	return canvas::ImageEntry();
}

canvas::ImageEntry DataStash::loadImage(StringID key)
{
	if (const auto* ret = m_imageMap.find(key))
		return *ret;

	if (key)
	{
		for (const auto& searchPath : m_imageSearchPaths)
		{	
			if (auto imagePtr = LoadImageFromDepotPath(TempString("{}{}.png", searchPath, key)))
			{
				auto entry = cacheImage(imagePtr);
				m_imageMap[key] = entry;
				return entry;
			}
        }
    }

    return canvas::ImageEntry();
}

//---

END_BOOMER_NAMESPACE_EX(ui)
