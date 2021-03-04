/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"
#include "core/resource/include/fileLoader.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

// file dependencies
struct ManagedFileDependencies
{
    Array<FileLoadingDependency> dependencies;
};

//---

// file in the depot, represents actual file in a file system
// NOTE: many resource can be created from a single file thus
class EDITOR_COMMON_API ManagedFile : public ManagedItem
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
    ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName);
    virtual ~ManagedFile();

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
    virtual const image::Image* typeThumbnail() const override;

    //---

protected:
    const ManagedFileFormat* m_fileFormat; // file format description

    GlobalEventKey m_eventKey;
    vsc::FileState m_state;
    bool m_isModified;

    ///---

    void changeFileState(const vsc::FileState& state);
};

//---

END_BOOMER_NAMESPACE_EX(ed)
