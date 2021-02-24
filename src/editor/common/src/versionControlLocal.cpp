/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: vsc #]
***/

#include "build.h"
#include "versionControl.h"
#include "versionControlLocal.h"

BEGIN_BOOMER_NAMESPACE(ed::vsc)

namespace local
{

    LocalChangelist::LocalChangelist() {}

    StringBuf LocalChangelist::description() const
    {
        return "Local Changes";
    }

    ChangelistID LocalChangelist::id() const
    {
        return 0;
    }

    Result LocalChangelist::description(const StringBuf &desc)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalChangelist::files(FilePaths &outDepotPaths) const
    {
        return Result();
    }

    Result LocalChangelist::submit()
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalChangelist::revert()
    {
        return Result("Local source control does not allow this operation");
    }

    StringBuf LocalChangelist::shortDescription() const
    {
        return "Local";
    }

    StringBuf LocalChangelist::longDescription() const
    {
        return "Local Changes";
    }

    //--

    LocalVersionControl::LocalVersionControl()
    {
        m_default = RefNew<LocalChangelist>();
    }

    bool LocalVersionControl::isReady() const
    {
        return true;
    }

    Result LocalVersionControl::initialize(const Settings &settings)
    {
        return Result();
    }

    void LocalVersionControl::sync()
    {
    }

    RegistrationToken LocalVersionControl::attachListener(IHostListener *listener)
    {
        return RegistrationToken();
    }

    ChangelistPtr LocalVersionControl::defaultChangelist()
    {
        return m_default;
    }

    ChangelistPtr LocalVersionControl::activeChangelist()
    {
        return m_default;
    }

    uint32_t LocalVersionControl::numChangelists() const
    {
        return 1;
    }

    ChangelistPtr LocalVersionControl::changelist(uint32_t index) const
    {
        if (index == 1)
            return m_default;
        return nullptr;
    }

    Result LocalVersionControl::refreshAllChangelists()
    {
        return Result();
    }

    Result LocalVersionControl::createChangelist(const StringBuf &desc, ChangelistPtr &outChangelist)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::removeChangelist(const ChangelistPtr &changelist)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::activeChangelist(const ChangelistPtr &changelist)
    {
        if (changelist == m_default)
            return Result();
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::submitChangelist(const ChangelistPtr &changelist, const StringBuf &userComment)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::moveFilesToChangelist(const FilePaths &files, const ChangelistPtr &targetChangelist)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::fileChangelist(const FilePaths &path, ChangelistPtr &outChangelist)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::fileHistory(const FilePath &file, History &outHistory) const
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::fileInfo(const FilePath &file, FileState &outFileState) const
    {
        outFileState = FileState();
        outFileState.m_localAction = FileLocalAction::None;
        outFileState.m_isManaged = false;
        return Result();
    }

    Result LocalVersionControl::fileLocks(const FilePath &file, FileLocks &outFileLocks) const
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::latest(const FilePaths &files, bool force)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::revision(const FilePaths &files, uint32_t revision)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::refreshFiles(const FilePaths &files)
    {
        return Result();
    }

    Result LocalVersionControl::checkoutFiles(const FilePaths &paths)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::addFiles(const FilePaths &paths)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::deleteFiles(const FilePaths &paths)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::renameFiles(const FilePaths &oldPaths, const FilePaths &newPaths)
    {
        return Result("Local source control does not allow this operation");
    }

    Result LocalVersionControl::revertFiles(const FilePaths &paths)
    {
        return Result("Local source control does not allow this operation");
    }

} // local

END_BOOMER_NAMESPACE(ed::vsc)
