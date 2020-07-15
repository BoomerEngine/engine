/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"

namespace ed
{

    //---

    // file in the depot, represents actual file in a file system
    // NOTE: many resource can be created from a single file thus
    class BASE_EDITOR_API ManagedFile : public ManagedItem, public IObjectObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedFile, ManagedItem);

    public:
        /// get LAST version control state of the file, use the refresh methods to make this up to date
        INLINE const vsc::FileState& lastVersionControlState() const { return m_state; }

        /// get file format, it's known for all files in the managed depot
        /// NOTE: if a file has no recognized format than it's not displayed in the depot
        INLINE const ManagedFileFormat& fileFormat() const { return *m_fileFormat; }

    public:
        ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> fileName);
        virtual ~ManagedFile();

        /// request version control status update
        void refreshVersionControlStateRefresh(bool sync = false);

        /// is this file read only ?
        bool isReadOnly() const;

        /// Set read only flag on a file
        void readOnlyFlag(bool readonly);

        /// Can this file be checked out ?
        bool canCheckout() const;

        /// Get short file description
        StringBuf shortDescription() const;

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::ImageRef& ManagedFile::typeThumbnail() const;

        //---

        /// has this file been REPORTED as modified ?
        /// NOTE: this is just a flag, does nothing special
        INLINE bool isModified() const { return m_isModified; }

        /// mark file as modified
        void markAsModifed();

        /// unmark file as modified
        void unmarkAsModified();

        //---

        // load content of this file as raw bytes
        Buffer loadRawContent() const;

        // load content for this file for edition
        res::ResourcePtr loadContent() const;

        // load resource metadata, valid only for serialized resources
        res::MetadataPtr loadMetadata() const;

        //---

        // store new raw content for this file
        bool storeRawContent(Buffer data);

        // store new text for this file
        bool storeRawContent(base::StringView<char> data);

        // store new content for this file
        bool storeContent(const res::ResourceHandle& content);

        //---

        /// request a thumbnail to be loaded from backend
        void loadThubmbnail(bool force = false);

        /// new thumbnail data was loaded
        void newThumbnailDataAvaiable(const res::ResourceThumbnail& thumbnailData);

        /// query thumbnail data for file
        virtual bool fetchThumbnailData(uint32_t& versionToken, image::ImageRef& outThumbnailImage, Array<StringBuf>& outComments) const override;

    protected:
        const ManagedFileFormat* m_fileFormat; // file format description

        bool m_isInitialStateRefreshed:1; // version control state was initially refreshed
        bool m_isSaved:1; // file is being saved right now, prevents from some crap to be propagated
        bool m_isModified : 1; // file was reported as modified

        struct ThumbState
        {
            image::ImageRef image;
            Array<StringBuf> comments;
            uint32_t version = 0;
            bool firstLoadRequested = false;
            bool generatedRequested = false;
            int loadRequestCount = 0;
        };

        ThumbState m_thumbnailState;
        SpinLock m_thumbnailStateLock;

        vsc::FileState m_state; // last source control state

        ///----

        Buffer loadRawContent(StringView<char> depotPath) const;
        bool storeRawContent(StringView<char> depotPath, Buffer data);

        res::BaseReference loadContentInternal(SpecificClassType<res::IResource> resoureClass);

        ///---

        virtual void onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData) override final;

        void changeFileState(const vsc::FileState& state);
    };

    //---

} // editor

