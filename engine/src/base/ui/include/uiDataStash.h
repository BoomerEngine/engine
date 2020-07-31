/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: host #]
***/

#pragma once

namespace ui
{
    //---

    /// stash for data required for UI rendering
    class BASE_UI_API DataStash : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataStash, base::IObject);

    public:
        DataStash(const StyleLibraryRef& mainStyles);
        virtual ~DataStash();

        //----

        // get the main styling sheet
        INLINE const StyleLibraryRef& styles() const { return m_styles; }

        // styles version
        INLINE uint32_t stylesVersion() const { return m_stylesVersion; }

        //----

        // add search path for icons
        void addIconSearchPath(base::StringView<char> path);

        //---

        /// load image by name, uses the search paths
        base::image::ImageRef loadImage(base::StringID key);

    protected:
        StyleLibraryRef m_styles;
        uint32_t m_stylesVersion = 1;

        base::HashMap<base::StringID, base::image::ImageRef> m_imageMap;
        base::Array<base::StringBuf> m_imageSearchPaths;

        virtual void onPropertyChanged(base::StringView<char> path) override;
    };

    //---

} // ui