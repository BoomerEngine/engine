/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: host #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// stash for data required for UI rendering
class ENGINE_UI_API DataStash : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataStash, IObject);

public:
    DataStash(StringView stylesDepotPath = "");
    virtual ~DataStash();

    //----

    // get the main styling sheet
    INLINE style::Library* styles() const { return m_styles; }

	// get main atlas
	INLINE canvas::DynamicAtlas* mainAtlas() const { return m_mainIconAtlas; };

    // styles version
    INLINE uint32_t stylesVersion() const { return m_stylesVersion; }

    //----

    // add custom search path for icons
    void addIconSearchPath(StringView path);

	// remove custom search path
	void removeIconSearchPath(StringView path);

    //---

    /// load image by name, uses the search paths, returns image entry usable in canvas
    canvas::ImageEntry loadImage(StringID key);

	/// place already loaded image in the main canvas
	canvas::ImageEntry cacheImage(const image::Image* img, bool supportWrapping=false, uint8_t additionalPadding=0);

	//--

	/// prepare for rendering - does house keeping on the internal atlases
	void conditionalRebuildAtlases();

	//--

protected:
    StyleLibraryPtr m_styles;
    uint32_t m_stylesVersion = 1;

	RefPtr<canvas::DynamicAtlas> m_mainIconAtlas;

    HashMap<StringID, canvas::ImageEntry> m_imageMap;
	HashMap<uint32_t, canvas::ImageEntry> m_imagePtrMap;

    Array<StringBuf> m_imageSearchPaths;

    virtual void onPropertyChanged(StringView path) override;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
