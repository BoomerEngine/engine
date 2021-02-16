/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "versionControl.h"

namespace ed
{
    class ManagedFileFormatRegistry;

    ///----

    // resource tag for a managed file
    struct BASE_EDITOR_API ManagedFileTag
    {
        StringBuf name;
        Color color;
        bool baked = false;
    };

    // format information
    class BASE_EDITOR_API ManagedFileFormat : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)

    public:
        ManagedFileFormat(StringView extension);
        ~ManagedFileFormat();

        /// should we show this format in asset browser
        INLINE bool showInBrowser() const { return m_showInBrowser; }

        /// get the file extension
        INLINE const StringBuf& extension() const { return m_extension; }

        /// get the user readable description
        INLINE const StringBuf& description() const { return m_description; }

        /// get the name of the resource class linked to this format (if known)
        /// NOTE: this is set only for engine-serialized resources as non-engine files may be transformed into many different resource types
        INLINE SpecificClassType<res::IResource> nativeResourceClass() const { return m_nativeResourceClass; }

        /// do we have custom type thumbnail ?
        INLINE bool hasTypeThumbnail() const { return m_hasTypeThumbnail; }

        /// can this format be created by user ?
        INLINE bool canUserCreate() const { return !m_factory.empty(); }

        /// can this format be imported by user ?
        INLINE bool canUserImport() const { return !m_importExtensions.empty(); }

        /// get list of extensions this format can be imported from
        INLINE const Array<StringBuf>& importExtensions() const { return m_importExtensions; }

        /// get file format tags
        INLINE const Array<ManagedFileTag>& tags() const { return m_tags; }

        //--

        /// create empty resource
        res::ResourcePtr createEmpty() const;

        //--

        // is this file lodable/cookable as given type ?
        bool loadableAsType(ClassType resourceClass) const;

        /// get the default thumbnail image for this file type
        const image::Image* thumbnail() const;

        /// print the file type tags
        void printTags(IFormatStream& f, StringView separator="") const;

    private:
        StringBuf m_extension;
        StringBuf m_description;
        mutable image::ImagePtr m_thumbnail;

        SpecificClassType<res::IResource> m_nativeResourceClass;

        Array<ManagedFileTag> m_tags;

        RefPtr<res::IFactory> m_factory;

        Array<StringBuf> m_importExtensions;

        bool m_showInBrowser = false;
        mutable bool m_hasTypeThumbnail = false;
        mutable bool m_thumbnailLoadAttempted = false;

        friend class ManagedFileFormatRegistry;
    };

    ///----

    // registry of file formats
    class BASE_EDITOR_API ManagedFileFormatRegistry : public ISingleton
    {
        DECLARE_SINGLETON(ManagedFileFormatRegistry);

    public:
        ManagedFileFormatRegistry();

        //--

        /// get list of formats that can be CREATED by the user
        INLINE const Array<ManagedFileFormat*>& creatableFormats() const { return m_userCreatableFormats; }

        /// get list of formats that can be IMPORTED by the user
        INLINE const Array<ManagedFileFormat*>& importableFormats() const { return m_userImportableFormats; }

        /// get list of all formats
        INLINE const Array<ManagedFileFormat*>& allFormats() const { return m_allFormats; }

        //--

        /// cache formats
        void cacheFormats();

        /// get file format description for given extension
        /// NOTE: we always return a valid object here (except for empty extension)
        const ManagedFileFormat* format(StringView extension);

    private:
        SpinLock m_lock;

        Array<ManagedFileFormat*> m_allFormats;
        Array<ManagedFileFormat*> m_userCreatableFormats;
        Array<ManagedFileFormat*> m_userImportableFormats;

        HashMap<uint64_t, ManagedFileFormat*> m_formatMap;

        virtual void deinit() override;
    };

    ///----

} // depot

