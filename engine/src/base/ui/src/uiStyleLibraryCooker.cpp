/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#include "build.h"
#include "uiStyleLibraryTree.h"
#include "uiStyleLibraryPacker.h"

#include "uiStyleLibrary.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/io/include/utils.h"
#include "base/containers/include/stringBuilder.h"
#include "base/image/include/imageUtils.h"
#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/font/include/font.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace ui
{
    namespace style
    {

        //--

        // a simple cooker for UI styles
        class StylesLibraryCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(StylesLibraryCooker, base::res::IResourceCooker);

        public:
            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override;
        };

        RTTI_BEGIN_TYPE_CLASS(StylesLibraryCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<ui::style::Library>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("scss");
        RTTI_END_TYPE();

        //--

        class CookedContentLoader : public IStyleLibraryContentLoader
        {
        public:
            CookedContentLoader(base::res::IResourceCookerInterface& cooker)
                : m_cooker(cooker)
            {}

            virtual base::image::ImagePtr loadImage(base::StringView<char> context, base::StringView<char> imageFileName) override final
            {
                base::StringBuf fullResourcePath;
                if (!m_cooker.findFile(context, imageFileName, fullResourcePath, 4))
                    return nullptr;

                auto image = m_cooker.loadDependencyResource<base::image::Image>(base::res::ResourcePath(fullResourcePath));
                if (!image)
                    return nullptr;

                base::image::ImagePtr retImage;
                if (!m_localImages.find(image, retImage))
                {
                    retImage = base::CreateSharedPtr<base::image::Image>(image->view());
                    if (retImage->view().channels() == 4)
                        base::image::PremultiplyAlpha(retImage->view());
                    m_localImages[image] = retImage;
                }

                return retImage;
            }

            virtual base::FontPtr loadFont(base::StringView<char> context, base::StringView<char> fontFileName) override final
            {
                base::StringBuf fullResourcePath;
                if (!m_cooker.findFile(context, fontFileName, fullResourcePath, 4))
                    return nullptr;

                auto font = m_cooker.loadDependencyResource<base::font::Font>(base::res::ResourcePath(fullResourcePath));
                if (!font)
                    return nullptr;

                base::FontPtr retFont;
                if (!m_localFonts.find(font, retFont))
                {
                    retFont = base::CreateSharedPtr<base::font::Font>(font->data());
                    m_localFonts[font] = retFont;
                }

                return retFont;
            }

            void reparentObject(base::IObject* target)
            {
                for (const auto& obj : m_localImages.values())
                    obj->parent(target);

                for (const auto& obj : m_localFonts.values())
                    obj->parent(target);
            }

        private:    
            base::res::IResourceCookerInterface& m_cooker;

            base::HashMap<base::image::ImagePtr, base::image::ImagePtr> m_localImages;
            base::HashMap<base::FontPtr, base::FontPtr> m_localFonts;
        };

        base::res::ResourceHandle StylesLibraryCooker::cook(base::res::IResourceCookerInterface& cooker) const
        {
            // load content
            base::StringBuf contextName = base::StringBuf(cooker.queryResourcePath().path());
            auto rawContent = cooker.loadToBuffer(contextName);

            // load the CCS library
            CookedContentLoader loader(cooker);
            base::res::CookerIncludeHandler includeHandler(cooker);
            base::res::CookerErrorReporter errorHandler(cooker);
            auto ret = ParseStyleLibrary(contextName, rawContent, loader, includeHandler, errorHandler);
            loader.reparentObject(ret.get());
            return ret;
        }

        //--

    } // style
} // ui
