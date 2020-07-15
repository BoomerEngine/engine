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
#include "editorService.h"

#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/object/include/objectObserver.h"
#include "base/io/include/ioFileHandle.h"
#include "base/resource/include/resourceThumbnail.h"
#include "base/image/include/imageView.h"
#include "base/image/include/image.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resourceFileSaver.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFile);
    RTTI_END_TYPE();

    ManagedFile::ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> fileName)
        : ManagedItem(depot, parentDir, fileName)
        , m_isInitialStateRefreshed(false)
        , m_isSaved(false)
        , m_isModified(false)
        , m_fileFormat(nullptr)
    {
        // lookup the file class
        auto fileExt = fileName.afterFirst(".");
        m_fileFormat = ManagedFileFormatRegistry::GetInstance().format(fileExt);

        // use class thumbnail as the thumbnail for now
        m_thumbnailState.image = nullptr;
        m_thumbnailState.version = 1;
    }

    ManagedFile::~ManagedFile()
    {
    }

    const image::ImageRef& ManagedFile::typeThumbnail() const
    {
        return m_fileFormat->thumbnail();
    }

    void ManagedFile::refreshVersionControlStateRefresh(bool sync /*= false*/)
    {
        // TODO
    }

    void ManagedFile::loadThubmbnail(bool force /*= false*/)
    {
        bool requestLoad = false;

        {
            auto lock = CreateLock(m_thumbnailStateLock);
            if (force || !m_thumbnailState.firstLoadRequested)
            {
                m_thumbnailState.firstLoadRequested = true; // only try to load thumbnail once for every file
                m_thumbnailState.loadRequestCount += 1;
                requestLoad = true;
            }
        }

        if (requestLoad)
            m_depot->thumbnailHelper().loadThumbnail(this);
    }
        
    void ManagedFile::newThumbnailDataAvaiable(const res::ResourceThumbnail& thumbnailData)
    {
        if (thumbnailData.imagePixels)
        {
            image::ImageView imageView(image::NATIVE_LAYOUT, image::PixelFormat::Uint8_Norm, 4,
                thumbnailData.imagePixels.data(), thumbnailData.imageWidth, thumbnailData.imageHeight);

            auto newImage = CreateSharedPtr<image::Image>(imageView);

            {
                auto lock = CreateLock(m_thumbnailStateLock);
                m_thumbnailState.image = newImage;
                m_thumbnailState.comments = thumbnailData.comments;
                m_thumbnailState.loadRequestCount -= 1;
                m_thumbnailState.version += 1;
            }
        }
    }

    bool ManagedFile::fetchThumbnailData(uint32_t& versionToken, image::ImageRef& outThumbnailImage, Array<StringBuf>& outComments) const
    {
        auto lock = CreateLock(m_thumbnailStateLock);

        if (m_thumbnailState.version != versionToken)
        {
            versionToken = m_thumbnailState.version;
            outThumbnailImage = m_thumbnailState.image;
            outComments = m_thumbnailState.comments;
            return true;
        }

        return false;
    }

    void ManagedFile::markAsModifed()
    {
        if (!m_isModified)
        {
            m_depot->toogleFileModified(this, true);
            m_isModified = true;

            TRACE_INFO("File '{}' was reported as modified", depotPath());

            if (auto dir = parentDirectory())
                dir->updateModifiedContentCount();
        }
    }

    void ManagedFile::unmarkAsModified()
    {
        if (m_isModified)
        {
            m_depot->toogleFileModified(this, true);
            m_isModified = false;

            TRACE_INFO("File '{}' was reported as clear", depotPath());

            if (auto dir = parentDirectory())
                dir->updateModifiedContentCount();
        }
    }

    bool ManagedFile::isReadOnly() const
    {
        return false;//IO::GetInstance().isFileReadOnly(absolutePath());
    }

    void ManagedFile::readOnlyFlag(bool readonly)
    {
        //IO::GetInstance().readOnlyFlag(absolutePath(), readonly);
    }

    bool ManagedFile::canCheckout() const
    {
        return true;
    }

    StringBuf ManagedFile::shortDescription() const
    {
        StringBuilder ret;

        ret.append("File ");
        ret.append(m_name);

        return ret.toString();
    }

    void ManagedFile::changeFileState(const vsc::FileState& state)
    {
        m_state = state;
    }

    Buffer ManagedFile::loadRawContent(StringView<char> depotPath) const
    {
        if (auto file = m_depot->loader().createFileReader(depotPath))
        {
            if (auto ret = Buffer::Create(POOL_TEMP, file->size(), 16))
            {
                if (file->readSync(ret.data(), ret.size()) == ret.size())
                    return ret;

                TRACE_ERROR("IO error loading content of '{}'", depotPath);
            }
            else
            {
                TRACE_ERROR("Not enough memory to load content of '{}'", depotPath);
            }
        }
        else
        {
            TRACE_WARNING("Failed to load content for '{}'", depotPath);
        }

        return nullptr;
    }

    Buffer ManagedFile::loadRawContent() const
    {
        if (!isDeleted())
        {
            return loadRawContent(depotPath());
        }
        else
        {
            TRACE_WARNING("Failed to load content from deleted file '{}'", depotPath());
        }

        return nullptr;
    }

    res::ResourcePtr ManagedFile::loadContent() const
    {
        if (m_fileFormat->nativeResourceClass())
        {
            const auto resourceKey = res::ResourceKey(res::ResourcePath(depotPath()), m_fileFormat->nativeResourceClass());
            return base::LoadResource(resourceKey).acquire();
        }

        return nullptr;
    }

    res::MetadataPtr ManagedFile::loadMetadata() const
    {
        if (auto file = m_depot->loader().createFileAsyncReader(depotPath()))
        {
            base::res::FileLoadingContext loadingContext;
            loadingContext.loadSpecificClass = base::res::Metadata::GetStaticClass();

            if (base::res::LoadFile(file, loadingContext))
            {
                if (loadingContext.loadedObjects.size() == 1)
                {
                    return base::rtti_cast<res::Metadata>(loadingContext.loadedObjects[0]);
                }
            }
        }

        return nullptr;
    }

    bool ManagedFile::storeRawContent(base::StringView<char> data)
    {
        auto buf = base::Buffer::Create(POOL_TEMP, data.length(), 1, data.data());
        return storeRawContent(buf);
    }

    bool ManagedFile::storeRawContent(StringView<char> depotPath, Buffer data)
    {
        if (data)
        {
            if (auto writer = m_depot->loader().createFileWriter(depotPath))
            {
                if (writer->writeSync(data.data(), data.size()) == data.size())
                {
                    unmarkAsModified();
                    return true;
                }
                else
                {
                    writer->discardContent();
                }
            }
        }

        TRACE_WARNING("Failed to store content for '{}'", depotPath);
        return false;
    }

    bool ManagedFile::storeRawContent(Buffer data)
    {
        if (!isDeleted())
        {
            return storeRawContent(depotPath(), data);
        }
        else
        {
            TRACE_WARNING("Failed to store content for delete file '{}'", depotPath());
        }

        return false;
    }

    bool ManagedFile::storeContent(const res::ResourceHandle& content)
    {
        if (content)
        {
            res::ResourceMountPoint mountPoint;
            if (m_depot->loader().queryFileMountPoint(depotPath(), mountPoint))
            {
                if (auto writer = m_depot->loader().createFileWriter(depotPath()))
                {
                    res::FileSavingContext context;
                    context.basePath = mountPoint.path();
                    context.rootObject.pushBack(content);

                    if (res::SaveFile(writer, context))
                    {
                        unmarkAsModified();
                        return true;
                    }
                    else
                    {
                        writer->discardContent();
                    }
                }
            }
        }

        TRACE_WARNING("Unable to save new content for file '{}'", depotPath());
        return false;
    }

    //--

    void ManagedFile::onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData)
    {
        
    }

#if 0
    static void CollectCompatibleResourceOpeners(const ManagedFilePtr& filePtr, Array<RefPtr<ManagedResourceOpener>>& outOpeners)
    {
        if (!filePtr || filePtr->isDeleted())
            return;

        // enumerate openers
        Array<ClassType> openerClasses;
        RTTI::GetInstance().enumClasses(ManagedResourceOpener::GetStaticClass(), openerClasses);

        struct Entry
        {
            RefPtr<ManagedResourceOpener> m_opener;
            uint32_t priority;
        };

        Array<Entry> entries;

        for (auto openerClass  : openerClasses)
        {
            auto opener = openerClass->createSharedPtr<ManagedResourceOpener>();
            if (opener)
            {
                auto prio = opener->canHandleFileFormat(filePtr->fileFormat());
                if (prio > 0)
                {
                    auto& entry = entries.emplaceBack();
                    entry.m_opener = opener;
                    entry.priority = prio;
                }
            }
        }

        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.priority < b.priority; });

        for (auto& entry : entries)
            outOpeners.pushBack(entry.m_opener);
    }

    bool ManagedFile::openEditor(const ui::ElementPtr& owner)
    {
        // create compatible openers
        Array<RefPtr<ManagedResourceOpener>> openers;
        CollectCompatibleResourceOpeners(), openers);
        if (openers.empty())
        {
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().error().message(TempString("No editor found for file '{}'", name())));
            return false;
        }

        // open the resource editor
        bool opened = false;
        for (auto& opener : openers)
        {
            opened |= opener->showResourceEditor(owner, ));
        }

        // not opened
        if (!opened)
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().error().message(TempString("Unable to open editor for file '{}'", name())));
        return opened;
    }
#endif

    //--

} // depot

