/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedThumbnails.h"
#include "managedDepot.h"

#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/io/include/ioFileHandle.h"
#include "base/resource/include/resourceThumbnail.h"
#include "base/image/include/imageView.h"
#include "base/image/include/image.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resourceFileSaver.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/ui/include/uiMessageBox.h"
#include "base/ui/include/uiElementConfig.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFile);
RTTI_END_TYPE();

ManagedFile::ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName)
    : ManagedItem(depot, parentDir, fileName)
    , m_isModified(false)
    , m_fileFormat(nullptr)
{
    // lookup the file class
    auto fileExt = fileName.afterFirst(".");
    m_fileFormat = ManagedFileFormatRegistry::GetInstance().format(fileExt);

    // event key, do not build from path, it sucks
    m_eventKey = MakeUniqueEventKey(fileName);
}

ManagedFile::~ManagedFile()
{
}

const image::Image* ManagedFile::typeThumbnail() const
{
    return m_fileFormat->thumbnail();
}

void ManagedFile::refreshVersionControlStateRefresh(bool sync /*= false*/)
{
    // TODO
}

void ManagedFile::modify(bool flag)
{
    if (m_isModified != flag)
    {
        depot()->toogleFileModified(this, flag);
        m_isModified = flag;

        TRACE_INFO("File '{}' was reported as {}", depotPath(), flag ? "modified" : "not modified");

        if (auto dir = parentDirectory())
            dir->updateModifiedContentCount();
    }
}

void ManagedFile::deleted(bool flag)
{
    if (flag != isDeleted())
    {
        m_isDeleted = flag;

        if (flag)
        {
            DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_DELETED, ManagedFilePtr(AddRef(this)));
            DispatchGlobalEvent(eventKey(), EVENT_MANAGED_FILE_DELETED);
        }
        else
        {
            DispatchGlobalEvent(eventKey(), EVENT_MANAGED_FILE_CREATED);
            DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_CREATED, ManagedFilePtr(AddRef(this)));
        }
    }
}

//--

END_BOOMER_NAMESPACE(ed)

