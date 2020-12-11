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

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/resource/include/resourcePath.h"
#include "base/canvas/include/canvasStorage.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataStash);
        RTTI_PROPERTY(m_styles);
    RTTI_END_TYPE();

    //---

    DataStash::DataStash(const StyleLibraryRef& mainStyles, base::canvas::IStorage* canvasStorage)
        : m_styles(mainStyles)
		, m_canvasStorage(AddRef(canvasStorage))
    {
		// create main atlas for UI icons
		m_mainIconAtlasIndex = m_canvasStorage->createAtlas(1024, 1, "UIIcons");
    }

    DataStash::~DataStash()
    {
		if (m_mainIconAtlasIndex)
		{
			m_canvasStorage->destroyAtlas(m_mainIconAtlasIndex);
			m_mainIconAtlasIndex = 0;
		}
    }

    void DataStash::onPropertyChanged(base::StringView path)
    {
        TBaseClass::onPropertyChanged(path);

        if (path == "styles")
        {
            TRACE_INFO("UI styles were changed, all UI layouts will refresh");
            ui::IElement::InvalidateAllDataEverywhere();
            m_stylesVersion += 1;
        }
    }
    
    void DataStash::addIconSearchPath(base::StringView path)
    {
        DEBUG_CHECK_RETURN(base::ValidateDepotPath(path, base::DepotPathClass::AbsoluteDirectoryPath));

        if (!path.empty())
            m_imageSearchPaths.pushBackUnique(base::StringBuf(path));
    }

	void DataStash::removeIconSearchPath(base::StringView path)
	{
		m_imageSearchPaths.remove(path);
	}

	void DataStash::conditionalRebuildAtlases()
	{
		m_canvasStorage->conditionalRebuild();
	}

	base::canvas::ImageEntry DataStash::cacheImage(const base::image::Image* img, bool supportWrapping /*= false*/, uint8_t additionalPadding /*= 0*/)
	{
		if (const auto* ret = m_imagePtrMap.find(img))
			return *ret;

		if (img && m_mainIconAtlasIndex)
		{
			if (auto entry = m_canvasStorage->registerImage(m_mainIconAtlasIndex, img, supportWrapping, additionalPadding))
			{
				m_imagePtrMap[img] = entry;
				return entry;
			}
		}

		return base::canvas::ImageEntry();
	}

    base::canvas::ImageEntry DataStash::loadImage(base::StringID key)
    {
		if (const auto* ret = m_imageMap.find(key))
			return *ret;

		if (key && m_mainIconAtlasIndex)
		{
			for (const auto& searchPath : m_imageSearchPaths)
			{	
				if (auto imagePtr = base::LoadResource<base::image::Image>(base::TempString("{}{}.png", searchPath, key)).acquire())
				{
					auto entry = cacheImage(imagePtr);
					m_imageMap[key] = entry;
					return entry;
				}
            }
        }

        return base::canvas::ImageEntry();
    }

    //---

}
