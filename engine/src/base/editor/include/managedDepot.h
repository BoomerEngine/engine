/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/io/include/ioDirectoryWatcher.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/mutex.h"
#include "base/resources/include/resourceLoader.h"
#include "base/depot/include/depotStructure.h"
#include "editorConfig.h"

namespace ed
{
    class ManagedDepotVersionControlListener;
    class ManagedThumbnailHelper;
    class DirectoryWatcher;
    class IVersionControl;

    /// Managed file event
    enum class ManagedDepotEvent : uint8_t
    {
        FileCreated, // file was created
        FileDeleted,  // file was deleted
        FileOpened, // file was opened for edition
        FileClosed, // editor for this file was closed
        FileContentChanged, // content of file on disk was changed (does not mean resource was reloaded)
        FileVSCChanged, // source control state for the file has changed
        FileModifiedChanged, // source control state for the file has changed

        DirCreated, // directory was created
        DirDeleted, // directory was deleted
        DirBookmarkChanged, // directory bookmark status changed
    };

    /// Managed depot listener
    class BASE_EDITOR_API IManagedDepotListener : public NoCopy
    {
    public:
        IManagedDepotListener();
        virtual ~IManagedDepotListener();

        void detach();

        virtual void managedDepotEvent(ManagedItem* item, ManagedDepotEvent eventType) {};

    private:
        ManagedDepot* m_depot;
    };

    /// editor side depot
    class BASE_EDITOR_API ManagedDepot : public depot::IDepotObserver
    {
    public:
        ManagedDepot(depot::DepotStructure& loader, ConfigGroup config);
        ~ManagedDepot();

        /// get the related depot loader
        INLINE depot::DepotStructure& loader() const { return m_loader; }

        /// depot configuration entry
        INLINE const ConfigGroup& config() const { return m_config; }
        INLINE ConfigGroup& config() { return m_config; }

        /// get modified files
        typedef HashSet<ManagedFile*> TModifiedFiles;
        INLINE const TModifiedFiles& modifiedFiles() { return m_modifiedFiles; }

        /// get bookmarked directories
        typedef HashSet<ManagedDirectory*> TBookmarkedDirectories;
        INLINE const TBookmarkedDirectories& bookmarkedDirectories() const { return m_bookmarkedDirectories; }

        /// get the thumbnail service helper
        INLINE ManagedThumbnailHelper& thumbnailHelper() const { return *m_thumbnailHelper; }

        /// get root directory for the depot
        INLINE ManagedDirectory* root() const { return m_root; }

        ///----

        /// populate depot with files
        void populate();

        ///---

        /// create path (all directories along the way)
        ManagedDirectory* createPath(StringView<char> depotPath, const char** leftOver = nullptr);

        /// find directory in depot, optionally returns the part of the path that is a file name
        ManagedDirectory* findPath(StringView<char> depotPath, const char** leftOver = nullptr) const;

        /// find managed file for given depot path
        ManagedFile* findManagedFile(StringView<char> depotPath) const;

        //----

        /// dispatch file events to listeners, flushes the event list
        void dispatchEvents();

        /// register depot listener
        void registerListener(IManagedDepotListener* listener);

        /// Unregister depot listener
        void unregisterListener(IManagedDepotListener* listener);

        ///---

        /// restore depot configuration
        void restoreConfiguration();

        ///----

    private:
        enum class InternalFileEventType
        {
            FileAdded,
            FileRemoved,
            DirAdded,
            DirRemoved,
        };

        // Internal file event
        struct InternalFileEvent
        {
            InternalFileEventType type;
            StringBuf depotPath;
        };

        // root structure
        ManagedDirectoryPtr m_root;

        // configuration entry
        ConfigGroup m_config;

        // depot listeners
        typedef Array<IManagedDepotListener*> TListeners;
        Mutex m_listenersLock;
        TListeners m_listeners;

        // booked marked directories
        TBookmarkedDirectories m_bookmarkedDirectories;

        // list of modified filed
        TModifiedFiles m_modifiedFiles;

        // thumbnail service
        UniquePtr<ManagedThumbnailHelper> m_thumbnailHelper;

        // depot file loader
        depot::DepotStructure& m_loader;

        //---

        // internal events
        SpinLock m_internalEventsLock;
        Array<InternalFileEvent> m_internalEvents;

        // IDepotObserver
        virtual void notifyFileChanged(StringView<char> depotFilePath) override final;
        virtual void notifyFileAdded(StringView<char> depotFilePath) override final;
        virtual void notifyFileRemoved(StringView<char> depotFilePath) override final;
        virtual void notifyDirAdded(StringView<char> depotFilePath) override final;
        virtual void notifyDirRemoved(StringView<char> depotFilePath) override final;

        // dispatch internal (managed depot) events 
        void dispatchEvent(ManagedItem* item, ManagedDepotEvent eventType);

        // external events
        void toogleDirectoryBookmark(ManagedDirectory* dir, bool state);
        void toogleFileModified(ManagedFile* dir, bool state);

        friend class ManagedFile;
        friend class ManagedDirectory;
    };

} // ed
