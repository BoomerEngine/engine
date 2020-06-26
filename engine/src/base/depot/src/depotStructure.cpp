/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#include "build.h"
#include "depotFileSystem.h"
#include "depotFileSystemNative.h"
#include "depotStructure.h"
#include "depotPackageManifest.h"

#include "base/resources/include/resourceUncached.h"
#include "base/resources/include/resourceMetadata.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resources/include/resourceSerializationMetadata.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/nativeFileReader.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/commandline.h"
#include "base/system/include/thread.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlUtils.h"

namespace base
{
    namespace depot
    {

        //--

        namespace helper
        {

            static RefPtr<PackageManifest> LoadManifest(const IFileSystem& fs, StringView<char> mountPath)
            {
                // load the manifest from the just mounted file system
                auto manifestReader = fs.createReader("package.manifest");
                if (!manifestReader)
                    return nullptr;

                // load the manifest from the file
                stream::NativeFileReader fileReader(*manifestReader);
                auto manifest = rtti_cast<PackageManifest>(res::LoadUncached(manifestReader->originInfo(), PackageManifest::GetStaticClass(), fileReader));
                if (manifest)
                {
                    auto depotMountPoint = base::res::ResourceMountPoint(mountPath);
                    manifest->translatePaths(depotMountPoint);
                }

                return manifest;
            }

            static StringBuf CombineMountPaths(StringView<char> basePath, StringView<char> relativeMountPath)
            {
                ASSERT_EX(basePath.empty() || basePath.endsWith("/"), "Invalid base path");

                StringBuf mergedPath = TempString("{}{}", basePath, relativeMountPath);
                mergedPath.replaceChar('\\', '/');

                if (!mergedPath.endsWith("/"))
                    mergedPath = TempString("{}/", mergedPath);

                return mergedPath;
            }

        } // helper

           //--

        DepotFileSystem::DepotFileSystem(const res::ResourceMountPoint& mountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type)
            : m_mountPoint(mountPoint)
            , m_fileSystem(std::move(fileSystem))
            , m_type(type)
        {
        }

        DepotFileSystem::~DepotFileSystem()
        {
        }

        //--

        IDepotObserver::~IDepotObserver()
        {}

        //--

        DepotStructure::DepotStructure()
        {
            m_crcCache.create();
        }

        bool DepotStructure::populateFromManifest(const io::AbsolutePath& depotManifestPath)
        {
            // load manifest
            auto settings = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), depotManifestPath);
            if (!settings)
            {
                TRACE_WARNING("Unable to load depot mapping from '{}'", depotManifestPath);
                return false;
            }

            // manifest base path
            const auto manifestBasePath = depotManifestPath.basePath();

            // look for the depot entries
            auto node = settings->nodeFirstChild(settings->root(), "PathMapping");
            while (node != 0)
            {
                auto depotPath = StringBuf(settings->nodeAttributeOfDefault(node, "path", ""));
                if (depotPath.empty())
                {
                    TRACE_WARNING("{}: error: local depot path not specified (missing 'path') attribute", settings->nodeLocationInfo(node));
                    continue;
                }

                depotPath.replaceChar('\\', '/');

                if (!depotPath.empty() && !depotPath.endsWith("/"))
                    depotPath = TempString("{}/", depotPath);

                auto relativeSourcePath = StringBuf(settings->nodeAttributeOfDefault(node, "source", ""));
                if (relativeSourcePath.empty())
                {
                    TRACE_WARNING("{}: error: source path not specified (missing 'source') attribute", settings->nodeLocationInfo(node));
                    continue;
                }

                bool readonly = false;
                settings->nodeAttributeOfDefault(node, "readonly", "false").match(readonly);

                auto sourcePath = manifestBasePath.addDir(relativeSourcePath.c_str());
                if (sourcePath.empty())
                {
                    TRACE_WARNING("{}: error: unable to resolve source path '{}'", settings->nodeLocationInfo(node), relativeSourcePath);
                    continue;
                }

                auto fileSystemType = DepotFileSystemType::Engine;

                auto projectFileSystem = CreateUniquePtr<FileSystemNative>(sourcePath, !readonly, this);
                if (!processFileSystem(depotPath, std::move(projectFileSystem), fileSystemType))
                {
                    TRACE_ERROR("Failed to mount engine file system from '{}' to '{}'", sourcePath, depotPath);
                    //return false;
                }
                else
                {
                    TRACE_INFO("Attached '{}' from '{}'", depotPath, sourcePath);
                }

                node = settings->nodeSibling(node, "PathMapping");
            }

            // initialized
            TRACE_INFO("Depot file loader initialized");
            return true;
        }

        DepotStructure::~DepotStructure()
        {
            m_crcCache.reset();
            m_fileSystems.clear();
            m_fileSystemsPtrs.clear();
        }

        bool DepotStructure::enableWriteOperations()
        {
            bool valid = true;

            for (const auto& fs : m_fileSystems)
            {
                if (!fs->fileSystem().enableWriteOperations())
                {
                    TRACE_ERROR("File system mounted at '{}' does not permit write operations", fs->mountPoint());
                    valid = false;
                }
            }

            return valid;
        }

        void DepotStructure::enableDepotObservers()
        {
            for (const auto& fs : m_fileSystems)
                fs->fileSystem().enableFileSystemObservers();
        }

        void DepotStructure::rebuildFileSystemMap()
        {
            // clear map
            m_fileSystemsAtDirectory.clear();

            // attach system
            for (auto fs  : m_fileSystemsPtrs)
                registerFileSystemBinding(fs);
        }

        void DepotStructure::registerFileSystemBinding(const DepotFileSystem* fs)
        {
            auto& mountPoint = fs->mountPoint();
            if (!mountPoint.root())
            {
                auto& prefixPath = mountPoint.path();
                ASSERT(prefixPath.endsWith("/"));

                auto shorterPath = prefixPath.leftPart(prefixPath.length()-1);

                auto basePrefixPath = shorterPath.stringBeforeLast("/");
                if (!basePrefixPath.empty())
                    basePrefixPath = TempString("{}/", basePrefixPath);

                auto mountDirectoryName = shorterPath.stringAfterLast("/", true);
                ASSERT(!mountDirectoryName.empty());

                auto& entry = m_fileSystemsAtDirectory[basePrefixPath].emplaceBack();
                entry.name = mountDirectoryName;
                entry.fileSystem = fs;
            }
        }

        void DepotStructure::attachFileSystem(StringView<char> prefixPath, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type)
        {
            // validate path
            ASSERT_EX(prefixPath.empty() || prefixPath.endsWith("/"), "Invalid prefix path for mounting file system")

            // create wrapper
            auto depotMountPoint = base::res::ResourceMountPoint(prefixPath);
            auto depotWrapper = base::CreateUniquePtr<DepotFileSystem>(depotMountPoint, std::move(fileSystem), type);
            m_fileSystems.pushBack(std::move(depotWrapper));
            m_fileSystemsPtrs.pushBack(m_fileSystems.back().get());

            // get the base path
            registerFileSystemBinding(m_fileSystems.back().get());
        }

        void DepotStructure::detachProjectFileSystems()
        {
            // unmount project dependent file system
            m_fileSystemsPtrs.clear();
            auto fileSystems = std::move(m_fileSystems);
            for (auto& entry : fileSystems)
            {
                if (entry->type() == DepotFileSystemType::Engine)
                {
                    // keep alive
                    m_fileSystems.pushBack(std::move(entry));
                }
                else
                {
                    TRACE_INFO("Unmounting '{}'", entry->mountPoint());
                }
            }
        }

        void DepotStructure::attachObserver(IDepotObserver* observer)
        {
            if (observer)
            {
                auto lock = CreateLock(m_observersLock);
                m_observers.pushBack(observer);
            }
        }

        void DepotStructure::detttachObserver(IDepotObserver* observer)
        {
            auto lock = CreateLock(m_observersLock);
            m_observers.remove(observer);
        }

        io::FileHandlePtr DepotStructure::createFileReader(StringView<char> fileSystemPath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath))
                return nullptr;

            // get the timestamp
            if (auto ret = localFileSystem->createReader(localFileSystemPath))
                return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath, true))
                    return nullptr;

                // try with the target file system
                if (auto ret = localFileSystem->createReader(localFileSystemPath))
                    return ret;
            }

            return nullptr;
        }

        bool DepotStructure::storeFileContent(StringView<char> fileSystemPath, const Buffer& newContent) const
        {
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // try with the target file system
            if (localFileSystem->isWritable())
                if (const_cast<IFileSystem*>(localFileSystem)->writeFile(localFileSystemPath, newContent))
                    return true;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath, true))
                    return false;

                // try with the target file system
                if (localFileSystem->isWritable())
                    if (const_cast<IFileSystem*>(localFileSystem)->writeFile(localFileSystemPath, newContent))
                        return true;
            }

            // not written
            return false;
        }

        ///---

        void DepotStructure::notifyFileChanged(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);

                    TRACE_INFO("File {} was reported as changed", fullPath);

                    {
                        auto lock = CreateLock(m_observersLock);
                        for (auto it : m_observers)
                            it->notifyFileChanged(fullPath);
                    }

                    break;
                }
            }
        }

        void DepotStructure::notifyFileAdded(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);
                    TRACE_INFO("File {} was reported as added", fullPath);

                    {
                        auto lock = CreateLock(m_observersLock);
                        for (auto it : m_observers)
                            it->notifyFileAdded(fullPath);
                    }

                    break;
                }
            }
        }

        void DepotStructure::notifyFileRemoved(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);
                    TRACE_INFO("File {} was reported as removed", fullPath);
                        
                    {
                        auto lock = CreateLock(m_observersLock);
                        for (auto it : m_observers)
                            it->notifyFileRemoved(fullPath);
                    }

                    break;
                }
            }
        }

        void DepotStructure::notifyDirAdded(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);
                    TRACE_INFO("Directory {} was reported as added", fullPath);

                    {
                        auto lock = CreateLock(m_observersLock);
                        for (auto it : m_observers)
                            it->notifyDirAdded(fullPath);
                    }

                    break;
                }
            }
        }

        void DepotStructure::notifyDirRemoved(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);
                    TRACE_INFO("Directory {} was reported as removed", fullPath);

                    {
                        auto lock = CreateLock(m_observersLock);
                        for (auto it : m_observers)
                            it->notifyDirRemoved(fullPath);
                    }

                    break;
                }
            }
        }

        //---

        bool DepotStructure::processFileSystem(StringView<char> mountPath, UniquePtr<IFileSystem> fs, DepotFileSystemType type)
        {
            // no file system
            if (!fs)
                return false;

            // load the manifest from the file system
            auto manifest = helper::LoadManifest(*fs, mountPath);
            if (manifest)
            {
                StringBuilder buf;

                if (manifest->globalID())
                    buf.appendf("ID: {} ", manifest->globalID());
                if (manifest->version())
                    buf.appendf("Version: {} ", manifest->version());
                if (manifest->description())
                    buf.append(manifest->description());

                // keep the manifest around
                TRACE_INFO("Found manifest for '{}' {}", mountPath, buf.c_str());
                m_manifests.pushBack(manifest);

                // process dependencies
                for (auto& dep : manifest->dependencies())
                {
                    if (!dep.prefixPath.empty())
                    {
                        // create the combined path
                        auto mergedMountPath = helper::CombineMountPaths(mountPath, dep.prefixPath);

                        // request a file system
                        if (dep.provider)
                        {
                            auto fileSystem = dep.provider->createFileSystem(this);
                            if (fileSystem)
                            {
                                // attach
                                attachFileSystem(mergedMountPath, std::move(fileSystem), type);
                            }
                            else
                            {
                                TRACE_ERROR("Unable to create file system for bind point '{}'", mergedMountPath);
                            }
                        }
                        else
                        {
                            TRACE_ERROR("Missing file system provider for bind point '{}'", mergedMountPath);
                        }
                    }
                }
            }
            else
            {
                TRACE_INFO("No manifest at '{}'", mountPath);
            }

            // finally, attach file system to the loader
            // NOTE: we are attached after our dependencies to make sure the path filtering can be done in a simple way
            attachFileSystem(mountPath, std::move(fs), type);

            // mounted
            return true;
        }

    } // depot
} // base
