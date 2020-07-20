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
#include "base/resource/include/resourceLoader.h"
#include "base/resource_compiler/include/depotStructure.h"
#include "editorConfig.h"

namespace ed
{
    class ManagedDepotVersionControlListener;
    class ManagedThumbnailHelper;
    class DirectoryWatcher;
    class IVersionControl;

    /// editor side depot
    class BASE_EDITOR_API ManagedDepot : public NoCopy
    {
    public:
        ManagedDepot(depot::DepotStructure& loader, ConfigGroup config);
        ~ManagedDepot();

        /// get the low-level depot structure
        INLINE depot::DepotStructure& depot() const { return m_depot; }

        /// get the event key for event listening
        INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

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

        /// update/sync any changed
        void update();

        ///---

        /// create path (all directories along the way)
        ManagedDirectory* createPath(StringView<char> depotPath, const char** leftOver = nullptr);

        /// find directory in depot, optionally returns the part of the path that is a file name
        ManagedDirectory* findPath(StringView<char> depotPath, const char** leftOver = nullptr) const;

        /// find managed file for given depot path
        ManagedFile* findManagedFile(StringView<char> depotPath) const;

        ///---

        /// restore depot configuration
        void restoreConfiguration();

        ///----

    private:
        // root structure
        ManagedDirectoryPtr m_root;

        // configuration entry
        ConfigGroup m_config;

        // booked marked directories
        TBookmarkedDirectories m_bookmarkedDirectories;

        // list of modified filed
        TModifiedFiles m_modifiedFiles;

        // thumbnail service
        UniquePtr<ManagedThumbnailHelper> m_thumbnailHelper;

        // depot file loader
        depot::DepotStructure& m_depot;

        //--

        // managed depot event key
        GlobalEventKey m_eventKey;

        // event listener for the low-level depot events
        GlobalEventTable m_depotEvents;

        //--

        // directories has changed
        HashMap<StringBuf, NativeTimePoint> m_modifiedDirectories;

        void handleDepotFileNotificataion(StringView<char> path);
        void processDepotFileNotificataion();

        //--

        // external events
        void toogleDirectoryBookmark(ManagedDirectory* dir, bool state);
        void toogleFileModified(ManagedFile* dir, bool state);

        friend class ManagedFile;
        friend class ManagedDirectory;
    };

} // ed
