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
    //---

    // item in depot directory
    class BASE_EDITOR_API ManagedItem : public IObject
    {
        RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedItem, IObject);

    public:
        /// get the managed depot
        INLINE ManagedDepot* depot() const { return m_depot; }

        /// get the parent managed directory
        INLINE ManagedDirectory* parentDirectory() const { return m_directory; }

        /// get the file name with extension
        INLINE const StringBuf& name() const { return m_name; }

        /// has the file been marked as deleted ?
        INLINE bool isDeleted() const { return m_isDeleted; }

    public:
        ManagedItem(ManagedDepot* depot, ManagedDirectory* parentDir, StringView name);

        /// get depot file path of this file
        /// NOTE: file path is constructed on demand (not a cheap call)
        virtual StringBuf depotPath() const;

        /// resolve absolute file path for this file, valid only for files coming from disk
        virtual StringBuf absolutePath() const;

        /// get the per-type thumbnail, does not depend on the content of the item
        virtual const image::Image* typeThumbnail() const = 0;

        //--

        // validate directory name
        static bool ValidateDirectoryName(StringView txt);

        // validate file name (without extensions)
        static bool ValidateFileName(StringView txt);

    private:
        ManagedDepot* m_depot; // depot
        ManagedDirectory* m_directory; // parent directory

        StringBuf m_name; // file name with extension

    protected:
        bool m_isDeleted : 1; // we don't remove the file proxies, just mark them as deleted

        virtual ~ManagedItem();

        friend class ManagedFilePlaceholder;
        friend class ManagedDirectoryPlaceholder;
    };

    //---

    // get the main selected depot file
    extern BASE_EDITOR_API ManagedFile* SelectedFile();

    // get the main selected depot file
    extern BASE_EDITOR_API Array<ManagedFile*> SelectedFiles();

    // get selected directory
    extern BASE_EDITOR_API ManagedDirectory* SelectedDirectory();

    //---

} // ed

