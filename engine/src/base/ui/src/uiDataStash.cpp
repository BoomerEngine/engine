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


    static base::StringBuf SantizePath(base::StringView<char> path)
    {
        auto fixedPath = base::StringBuf(path);
        fixedPath.replaceChar('\\', '/');
        while (fixedPath.beginsWith("/"))
            fixedPath = fixedPath.subString(1);
        while (fixedPath.endsWith("/"))
            fixedPath = fixedPath.leftPart(fixedPath.length() - 1);
        return fixedPath;
    }

    void DataStash::onPropertyChanged(base::StringView<char> path)
    {
        TBaseClass::onPropertyChanged(path);

        if (path == "styles")
        {
            TRACE_INFO("UI styles were changed, all UI layouts will refresh");
            ui::IElement::InvalidateAllDataEverywhere();
            ui::Window::ForceRedrawOfEverything();
            m_stylesVersion += 1;
        }
    }
    
    void DataStash::addIconSearchPath(base::StringView<char> path)
    {
        auto fixedPath = SantizePath(path);
        if (!fixedPath.empty())
            m_imageSearchPaths.pushBackUnique(fixedPath);
    }

    void DataStash::addTemplateSearchPath(base::StringView<char> path)
    {
        auto fixedPath = SantizePath(path);
        if (!fixedPath.empty())
            m_templateSearchPath.pushBackUnique(fixedPath);
    }

    base::image::ImageRef DataStash::loadImage(base::StringID key)
    {
        base::image::ImageRef ret;

        if (key && !m_imageMap.find(key, ret))
        {
            for (const auto& searchPath : m_imageSearchPaths)
            {
                auto fullPath = base::res::ResourcePath(base::TempString("{}/{}.png", searchPath, key));
                ret = base::LoadResource<base::image::Image>(fullPath);

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

    base::storage::XMLDataRef DataStash::loadTemplate(base::StringID key)
    {
        return nullptr;
    }

    //---

}
