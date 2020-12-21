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
#include "base/canvas/include/canvasAtlas.h"
#include "base/resource/include/resourcePath.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataStash);
        RTTI_PROPERTY(m_styles);
    RTTI_END_TYPE();

    //---

    DataStash::DataStash(const StyleLibraryRef& mainStyles)
        : m_styles(mainStyles)
    {
		m_mainIconAtlas = base::RefNew<base::canvas::DynamicAtlas>(1024, 1);
    }

    DataStash::~DataStash()
    {
		m_mainIconAtlas.reset();
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
		
	}

	base::canvas::ImageEntry DataStash::cacheImage(const base::image::Image* img, bool supportWrapping /*= false*/, uint8_t additionalPadding /*= 0*/)
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

		return base::canvas::ImageEntry();
	}

    base::canvas::ImageEntry DataStash::loadImage(base::StringID key)
    {
		if (const auto* ret = m_imageMap.find(key))
			return *ret;

		if (key)
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
