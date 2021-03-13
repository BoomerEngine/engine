/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: vsc #]
***/

#pragma once

#include "core/system/include/registration.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

/// file local user action
enum class VersionControlFileLocalAction : uint8_t
{
    None, // we have no local action on the file
    Added, // file added by me
    Deleted, // file deleted by me
    MoveAdded, // file has been moved to this by me
    MoveDeleted, // file has been moved from this by me
    CheckedOut, // file is checked out by me
};

/// Action performed on file
enum class VersionControlFileActionType : uint8_t
{
    // mark local file as part of a local change set
    Checkout,

    // revert file to the last version on the server
    Revert,

    // delete versioned file both locally and at the remote depot
    Delete,

    // add local file to remote depot
    Add,

    // get latest version of file, safe, will not override local work
    GetLatest,

    // get latest version of file by force, will override any local files
    ForceGetLatest,

    // get particular revision of file
    GetRevision,

    // submit changes to file
    Submit,
};

/// file state
struct EDITOR_ASSETS_API VersionControlFileState
{
    VersionControlFileLocalAction m_localAction; // intended local action for the file

    bool m_isManaged:1; // file is under source control
    bool m_isOutdated:1; // there is never version of the file on the server
    bool m_isLocked:1; // we have exclusive lock on the file
    bool m_isExternalCheckout:1; // somebody has exclusive checkout of the file
    bool m_isExternalLock:1; // somebody has exclusive lock of the file
    bool m_isExclusiveCheckout:1; // true if this file will have to be checkout exclusively

    int m_haveRevision; // the revision number of the file we have
    int m_headRevision; // the latest revision of the file in the depot

    INLINE VersionControlFileState()
        : m_localAction(VersionControlFileLocalAction::None)
        , m_isManaged(false)
        , m_isExclusiveCheckout(false)
        , m_isExternalCheckout(false)
        , m_isExternalLock(false)
        , m_isOutdated(false)
        , m_isLocked(false)
    {}

    INLINE bool operator==(const VersionControlFileState& other) const
    {
        return (m_localAction == other.m_localAction)
                && (m_isManaged == other.m_isManaged)
                && (m_isOutdated == other.m_isOutdated)
                && (m_isLocked == other.m_isLocked)
                && (m_isExclusiveCheckout == other.m_isExclusiveCheckout)
                && (m_isExternalCheckout == other.m_isExternalCheckout)
                && (m_isExternalLock == other.m_isExternalLock);
    }

    INLINE bool operator!=(const VersionControlFileState& other) const
    {
        return !operator==(other);
    }

    // get debug text
    void print(IFormatStream& builder) const;

    //--

    // can we checkout a file with this state ?
    bool canCheckout() const;

    // can we checkin a file with this state ?
    bool canCheckin() const;

    // can we revert file with this state ?
    bool canRevert() const;

    // can we get latest version o file with this state ?
    bool canGetLatest() const;
};

/// ID of the version control change
typedef int ChangelistID;

/// ID of the file revision
typedef uint32_t RevisionID;

//------

/// state of file locks
struct EDITOR_ASSETS_API FileLocks
{
    FileLocks();

    Array<StringBuf> m_lockUser; // users that have lock on the file
    Array<StringBuf> m_checkoutUsers; // users that have checked out the file
};

//------

/// version control settings
struct EDITOR_ASSETS_API Settings
{
    Settings();

    StringBuf m_root; // root of the physical depot
    StringBuf m_userName;
    StringBuf m_password;
    StringBuf m_host;
    StringBuf m_workspace;
};

//------

/// result structure for version control operations
struct EDITOR_ASSETS_API Result
{
public:
    // error, in case error is returned
    // empty if there were no issues
    StringBuf m_errorText;

    // files that we have problems with
    Array<StringBuf> m_errorFiles;

    Result();
    Result(const StringBuf& errorText);

    // have the operation finished without errors ?
    INLINE bool valid() const
    {
        return m_errorText.empty();
    }
};

//------

/// information about file's revision history
struct EDITOR_ASSETS_API HistoryEntry
{
    // when?
    TimeStamp m_time;

    // who?
    StringBuf m_user;

    // where?
    StringBuf m_workspace;

    // what?
    StringBuf m_desc;

    // #revision
    uint32_t m_revision;

    // changelist
    ChangelistID m_changelist;

    //--

    HistoryEntry();
};

//------

/// history information about a file
class EDITOR_ASSETS_API History
{
public:
    StringBuf m_file;
    StringBuf m_entries;

    History();
};

//------

/// wrapper for a collection of changes that go together
class EDITOR_ASSETS_API IChangelist : public IReferencable
{
public:
    IChangelist();
    virtual ~IChangelist();

    /// get changelist description (last evaluated)
    virtual StringBuf description() const = 0;

    /// get changelist ID (internal ID)
    virtual ChangelistID id() const = 0;

    /// set changelist description
    virtual Result description(const StringBuf& desc) = 0;

    /// get files belonging to this changelist
    virtual Result files(Array<StringBuf>& outDepotPaths) const = 0;

    /// submit all files in the changelist, deactivates the changelist after this is done
    virtual Result submit() = 0;

    /// revert all files in this changelist to the base state
    virtual Result revert() = 0;

    // get short (compact) description text
    virtual StringBuf shortDescription() const = 0;

    // get longer (verbose) description text
    virtual StringBuf longDescription() const = 0;
};

/// host side listener for version control updated
class EDITOR_ASSETS_API IHostListener
{
public:
    virtual ~IHostListener();

    /// file under source control has changed its state
    virtual void VersionControl_FileStatusChanged(const StringBuf& file, VersionControlFileState fileState) {};

    /// source control action on a file has finished
    virtual void VersionControl_FileActionPerformed(const StringBuf& file, VersionControlFileActionType fileAction, bool wasSuccessful, const StringBuf& extraInfo) {};

    /// state of changelist changed
    virtual void VersionControl_ChangelistListChanged(ChangelistID id) {};

    /// content of changelist changed
    virtual void VersionControl_ChangelistContentChanged(ChangelistID id) {};

    /// active changelist changed
    virtual void VersionControl_ChangelistActiveChanged(ChangelistID id) {};
};

/// interface for version control system
/// NOTE: WIP, support for GIT must be added
class EDITOR_ASSETS_API IVersionControl
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IVersionControl);

public:
    virtual ~IVersionControl();

    /// have we finished initialization and are we ready to use ?
    /// NOTE: may never finish
    virtual bool isReady() const = 0;

    /// initalize source control with given settings
    virtual Result initialize(const Settings& settings) = 0;

    /// sync any pending operations
    virtual void sync() = 0;

    /// attach host-side listener
    virtual RegistrationToken attachListener(IHostListener* listener) = 0;

    ///---

    /// get the default changelist, one always exists
    virtual ChangelistPtr defaultChangelist() = 0;

    /// get active changelist where all file operations are added
    virtual ChangelistPtr activeChangelist() = 0;

    /// get number of openend changelist
    virtual uint32_t numChangelists() const = 0;

    /// get changelist by index
    virtual ChangelistPtr changelist(uint32_t index) const = 0;

    /// refresh list of changelists
    virtual Result refreshAllChangelists() = 0;

    ///---

    /// create new changelist
    virtual Result createChangelist(const StringBuf &desc, ChangelistPtr &outChangelist) = 0;

    /// remove changelist
    virtual Result removeChangelist(const ChangelistPtr& changelist) = 0;

    /// set given changelist to be the active one
    virtual Result activeChangelist(const ChangelistPtr& changelist) = 0;

    /// submit content of given changelist
    virtual Result submitChangelist(const ChangelistPtr& changelist, const StringBuf& userComment) = 0;

    /// move files from to given changelist
    virtual Result moveFilesToChangelist(const Array<StringBuf>& files, const ChangelistPtr& targetChangelist) = 0;

    /// get changelist that contains given file
    virtual Result fileChangelist(const Array<StringBuf>& path, ChangelistPtr& outChangelist) = 0;

    ///---

    /// get history entry for given file
    virtual Result fileHistory(const StringBuf& file, History& outHistory) const = 0;

    /// get source control state of a single file
    virtual Result fileInfo(const StringBuf& file, VersionControlFileState& outVersionControlFileState) const = 0;

    /// get information about locks on the file
    virtual Result fileLocks(const StringBuf&file, FileLocks& outFileLocks) const = 0;

    ///---

    /// get latest version of given files
    virtual Result latest(const Array<StringBuf>& files, bool force) = 0;

    /// get specific version of give files
    virtual Result revision(const Array<StringBuf>& files, uint32_t revision) = 0;

    /// refresh tracked state of files, state will be reported via host notifiers
    virtual Result refreshFiles(const Array<StringBuf>& files) = 0;

    ///---

    /// add existing files to changelist
    virtual Result checkoutFiles(const Array<StringBuf>& paths) = 0;

    /// add new files to changelist (they will be added to remote depot when changelist is submited)
    virtual Result addFiles(const Array<StringBuf>& paths) = 0;

    /// add existing files to be deleted in the remote depot
    virtual Result deleteFiles(const Array<StringBuf>& paths) = 0;

    /// move files from one location to other
    virtual Result renameFiles(const Array<StringBuf>& oldPaths, const Array<StringBuf>& newPaths) = 0;

    /// revert all local changes in files
    virtual Result revertFiles(const Array<StringBuf>& paths) = 0;
};

//---

/// create stub for version control
extern VersionControlPtr CreateLocalStub();

//---

END_BOOMER_NAMESPACE_EX(ed::vsc)
