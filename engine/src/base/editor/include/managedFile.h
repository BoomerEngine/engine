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
    class BASE_EDITOR_API ManagedFile : public ManagedItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedFile, ManagedItem);

    public:
        /// get LAST version control state of the file, use the refresh methods to make this up to date
        INLINE const vsc::FileState& lastVersionControlState() const { return m_state; }

        /// get file format, it's known for all files in the managed depot
        /// NOTE: if a file has no recognized format than it's not displayed in the depot
        INLINE const ManagedFileFormat& fileFormat() const { return *m_fileFormat; }

        /// get the event key for this item, allows us to observer changes
        INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

        /// has this file been REPORTED as modified ?
        /// NOTE: modified files are also stored in a set in the managedDepot for easier access
        INLINE bool isModified() const { return m_isModified; }

    public:
        ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> fileName);
        virtual ~ManagedFile();

        //--

        /// get the opened editor for this file (valid only if file has been opened)
        INLINE const ResourceEditorPtr& editor() const { return m_editor; }

        /// is the file open? (editor must exist)
        INLINE bool opened() const { return !m_editor.empty(); }

        //--

        /// request version control status update
        void refreshVersionControlStateRefresh(bool sync = false);

        //--

        /// change file's "modified" flag
        /// NOTE: modified files are stored in a list in managed depot and also can't be "deleted" without additional message box
        virtual void modify(bool flag);

        /// toggle the "deleted" flag
        virtual void deleted(bool flag);

        //--

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::ImageRef& typeThumbnail() const override;

        //---

        /// can this file be opened ?
        virtual bool canOpen() const;

        /// is file in use (loaded?)
        virtual bool inUse() const;

        /// open this file edition, optionally the window can be activate
        virtual bool open(bool activate=true);

        /// save edited file (valid only if editor opened)
        virtual bool save();

        /// close this editor, if force closed we won't ask for saving
        virtual bool close(bool force=false);

        //---

    protected:
        const ManagedFileFormat* m_fileFormat; // file format description

        GlobalEventKey m_eventKey;
        vsc::FileState m_state;
        bool m_isModified;

        ResourceEditorPtr m_editor;

        ///---

        void changeFileState(const vsc::FileState& state);
    };

    //---

} // editor

