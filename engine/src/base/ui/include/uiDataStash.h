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
        DataStash(const StyleLibraryRef& mainStyles, base::canvas::IStorage* canvasStorage);
        virtual ~DataStash();

        //----

        // get the main styling sheet
        INLINE const StyleLibraryRef& styles() const { return m_styles; }

		// get canvas data storage (images)
		INLINE base::canvas::IStorage* canvasStorage() const { return m_canvasStorage; }

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
        StyleLibraryRef m_styles;
        uint32_t m_stylesVersion = 1;

		base::canvas::ImageAtlasIndex m_mainIconAtlasIndex = 0;
		base::RefPtr<base::canvas::IStorage> m_canvasStorage;

        base::HashMap<base::StringID, base::canvas::ImageEntry> m_imageMap;
		base::HashMap<const base::image::Image*, base::canvas::ImageEntry> m_imagePtrMap;

        base::Array<base::StringBuf> m_imageSearchPaths;

        virtual void onPropertyChanged(base::StringView path) override;
    };

    //---

} // ui