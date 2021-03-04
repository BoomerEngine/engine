/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

enum class DirectoryWatcherEventType : uint8_t
{
    FileAdded,
    DirectoryAdded,
    FileRemoved,
    DirectoryRemoved,
    FileContentChanged,
    FileMetadataChanged,
};

// directory watcher event, sent to the listener interface
struct DirectoryWatcherEvent
{
    DirectoryWatcherEventType type;
    StringBuf path;
};

// directory watcher callback
// NOTE: this may be called at any time and from any thread (sometimes a deep OS thread)
class CORE_IO_API IDirectoryWatcherListener : public NoCopy
{
public:
    virtual ~IDirectoryWatcherListener();

    /// handle file event
    virtual void handleEvent(const DirectoryWatcherEvent& evt) = 0;
};

// an abstract directory watcher interface
// NOTE: the directory watcher watches the specified directory and all sub directories
class CORE_IO_API IDirectoryWatcher : public IReferencable
{
public:
    virtual ~IDirectoryWatcher();

    //! attach listener
    virtual void attachListener(IDirectoryWatcherListener* listener) = 0;

    //! detach event listener
    virtual void dettachListener(IDirectoryWatcherListener* listener) = 0;

protected:
    IDirectoryWatcher();
};

END_BOOMER_NAMESPACE()
