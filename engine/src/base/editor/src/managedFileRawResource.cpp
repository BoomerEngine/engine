/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFileRawResource.h"
#include "managedFileFormat.h"
#include "managedItem.h"
#include "managedDepot.h"
#include "base/io/include/ioFileHandle.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFileRawResource);
    RTTI_END_TYPE();

    ManagedFileRawResource::ManagedFileRawResource(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName)
        : ManagedFile(depot, parentDir, fileName)
    {
    }

    ManagedFileRawResource::~ManagedFileRawResource()
    {
    }
    
    bool ManagedFileRawResource::inUse() const
    {
        return false;
    }

    Buffer ManagedFileRawResource::loadContent()
    {
        DEBUG_CHECK_EX(!isDeleted(), "Trying to load contet from deleted file");
        if (!isDeleted())
        {
            if (auto file = depot()->depot().createFileReader(depotPath()))
            {
                if (auto ret = Buffer::Create(POOL_RAW_RESOURCE, file->size(), 16))
                {
                    if (file->readSync(ret.data(), ret.size()) == ret.size())
                        return ret;

                    TRACE_ERROR("ManagedFile: IO error loading content of '{}'", depotPath());
                }
                else
                {
                    TRACE_ERROR("ManagedFile: Not enough memory to load content of '{}'", depotPath());
                }
            }
            else
            {
                TRACE_WARNING("ManagedFile: Failed to load content for '{}'", depotPath());
            }
        }
        else
        {
            TRACE_WARNING("ManagedFile: Can't load from deleted file '{}'", depotPath());
        }

        return nullptr;
    }

    bool ManagedFileRawResource::storeContent(const Buffer& data)
    {
        bool saved = false;

        DEBUG_CHECK_EX(!isDeleted(), "Trying to save contet to deleted file");
        if (!isDeleted())
        {
            if (data)
            {
                if (auto writer = depot()->depot().createFileWriter(depotPath()))
                {
                    if (writer->writeSync(data.data(), data.size()) == data.size())
                    {
                        TRACE_INFO("ManagedFile: Save new data into '{}' ({})", depotPath(), MemSize(data.size()));
                        saved = true;
                    }
                    else
                    {
                        TRACE_WARNING("ManagedFile: Unable to save all data into '{}'", depotPath());
                        writer->discardContent();
                    }
                }
                else
                {
                    TRACE_WARNING("ManagedFile: Unable to open writer for saving into '{}'", depotPath());
                }
            }
            else
            {
                TRACE_WARNING("ManagedFile: Nothing to save for '{}'", depotPath());
            }
        }
        else
        {
            TRACE_WARNING("ManagedFile: Can't save into deleted file '{}'", depotPath());
        }

        // if file was saved change it's status
        if (saved)
        {
            modify(false);

            DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_SAVED, ManagedFilePtr(AddRef(static_cast<ManagedFile*>(this))));
            DispatchGlobalEvent(eventKey(), EVENT_MANAGED_FILE_SAVED);
        }

        return saved;
    }

} // ed
