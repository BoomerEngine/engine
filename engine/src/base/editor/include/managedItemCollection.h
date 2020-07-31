/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "versionControl.h"

#include "base/io/include/absolutePath.h"
#include "base/resource/include/resourceMountPoint.h"

namespace ed
{

    //---

    // collection of actual, physical managed files
    class BASE_EDITOR_API ManagedFileCollection
    {
    public:
        ManagedFileCollection();
        ManagedFileCollection(const ManagedFileCollection& other);
        ManagedFileCollection(ManagedFileCollection&& other);
        ManagedFileCollection& operator=(const ManagedFileCollection& other);
        ManagedFileCollection& operator=(ManagedFileCollection&& other);
        ~ManagedFileCollection();

        ManagedFileCollection(const Array<ManagedFile*>& files);

        //--

        INLINE const Array<ManagedFile*>& files() const { return m_files.keys(); }
        INLINE const Array<ManagedItem*>& items() const { return *(const Array<ManagedItem*>*) &m_files.keys(); }

        INLINE bool empty() const { return m_files.empty(); }

        //--

        /// remove all files from collection
        void clear();

        /// add file to collection, returns true if added, false if already was in the collection
        bool collectFile(ManagedFile* file);

        /// remove file from collection, returns true if remove, false if was not in collection
        bool removeFile(ManagedFile* file);

        /// check if file is in collection
        bool containsFile(ManagedFile* file) const;

        //--

    public:
        HashSet<ManagedFile*> m_files;
    };

    //---

} // ed

