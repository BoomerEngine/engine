/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::io)

/// simple class that watches over the directory
class BASE_IO_API DirectoryMonitor : public base::NoCopy
{
public:
    DirectoryMonitor(const StringBuf& path, bool recursive);
    ~DirectoryMonitor();

    /// get the path we are watching over
    INLINE const StringBuf& path() const { return m_path; }

    /// is the check recursive (are we supposed to react to changes in the sub folders as well?)
    INLINE bool isRecursive() const { return m_isRecursive; }

    //--

    /// check if the directory was modified and if it was return true
    /// NOTE: doing the check resets the internal flag
    bool testAndReset();

private:
    StringBuf m_path;
    bool m_isRecursive;

    std::atomic<uint32_t> m_modified;
    IDirectoryWatcherListener* m_listener;
    DirectoryWatcherPtr m_watcher;
};

END_BOOMER_NAMESPACE(base::io)
