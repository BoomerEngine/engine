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
        DataStash(base::StringView stylesDepotPath = "");
        virtual ~DataStash();

        //----

        // get the main styling sheet
        INLINE style::Library* styles() const { return m_styles; }

		// get main atlas
		INLINE base::canvas::DynamicAtlas* mainAtlas() const { return m_mainIconAtlas; };

        // styles version
        INLINE uint32_t stylesVersion() const { return m_stylesVersion; }

        //----

        // add custom search path for icons
        void addIconSearchPath(base::StringView path);

		// remove custom search path
		void removeIconSearchPath(base::StringView path);

        //---

        /// load image by name, uses the search paths, returns image entry usable in canvas
        base::canvas::ImageEntry loadImage(base::StringID key);

		/// place already loaded image in the main canvas
		base::canvas::ImageEntry cacheImage(const base::image::Image* img, bool supportWrapping=false, uint8_t additionalPadding=0);

		//--

		/// prepare for rendering - does house keeping on the internal atlases
		void conditionalRebuildAtlases();

		//--

    protected:
        StyleLibraryPtr m_styles;
        uint32_t m_stylesVersion = 1;

		base::RefPtr<base::canvas::DynamicAtlas> m_mainIconAtlas;

        base::HashMap<base::StringID, base::canvas::ImageEntry> m_imageMap;
		base::HashMap<const base::image::Image*, base::canvas::ImageEntry> m_imagePtrMap;

        base::Array<base::StringBuf> m_imageSearchPaths;

        virtual void onPropertyChanged(base::StringView path) override;
    };

    //---

} // ui