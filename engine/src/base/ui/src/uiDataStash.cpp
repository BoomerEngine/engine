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
    }

    DataStash::~DataStash()
    {
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

    base::image::ImageRef DataStash::loadImage(base::StringID key)
    {
        base::image::ImageRef ret;

        if (key && !m_imageMap.find(key, ret))
        {
            for (const auto& searchPath : m_imageSearchPaths)
            {
                ret = base::LoadResource<base::image::Image>(base::TempString("{}{}.png", searchPath, key));

                if (ret)
                {
                    //base::image::PremultiplyAlpha(ret->view());
                    m_imageMap[key] = ret;
                    break;
                }
            }
        }

        return ret;
    }

    //---

}
