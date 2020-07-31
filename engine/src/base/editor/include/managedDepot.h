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

namespace ed
{
    //--

    /// editor side depot
    class BASE_EDITOR_API ManagedDepot : public NoCopy
    {
    public:
        ManagedDepot(depot::DepotStructure& loader);
        ~ManagedDepot();

        /// get the low-level depot structure
        INLINE depot::DepotStructure& depot() const { return m_depot; }

        /// get the event key for event listening
        INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

        /// get modified files
        INLINE const HashSet<ManagedFile*>& modifiedFiles() { return m_modifiedFiles; }
        INLINE const Array<ManagedFile*>& modifiedFilesList() { return m_modifiedFiles.keys(); }
        
        /// get bookmarked directories
        INLINE const HashSet<ManagedDirectory*>& bookmarkedDirectories() const { return m_bookmarkedDirectories; }
        INLINE const Array<ManagedDirectory*>& bookmarkedDirectoriesList() const { return m_bookmarkedDirectories.keys(); }

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

        void configLoad(const ui::ConfigBlock& block);
        void configSave(const ui::ConfigBlock& block);

        ///----

    private:
        // root structure
        ManagedDirectoryPtr m_root;

        // booked marked directories
        HashSet<ManagedDirectory*> m_bookmarkedDirectories;

        // list of modified filed
        HashSet<ManagedFile*> m_modifiedFiles;

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

    //--

} // ed
