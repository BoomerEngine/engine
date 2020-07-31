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

#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/commandline.h"
#include "base/system/include/thread.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlUtils.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/containers/include/utf8StringFunctions.h"

namespace base
{
    namespace depot
    {

        //--

        namespace helper
        {

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

        DepotStructure::DepotStructure()
        {
            m_eventKey = MakeUniqueEventKey("DepotStructure");
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
            auto node = settings->nodeFirstChild(settings->root(), "module");
            while (node != 0)
            {
                // is this module in the depot ?
                if (auto depotPath = StringBuf(settings->nodeAttributeOfDefault(node, "depot", "")))
                {
                    // read only ?
                    bool readonly = false;
                    settings->nodeAttributeOfDefault(node, "readonly", "false").match(readonly);

                    // convert to depot path separators
                    depotPath.replaceChar('\\', '/');

                    // fixup the path to be a propper mound point path
                    if (!depotPath.endsWith("/"))
                        depotPath = TempString("{}/", depotPath);
                    if (!depotPath.beginsWith("/"))
                        depotPath = TempString("/{}", depotPath);

                    // make sure mount path is valid
                    if (ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath))
                    {
                        // get the module path (it's relative to the project.xml)
                        if (auto modulePath = StringBuf(settings->nodeValue(node)))
                        {
                            // fixup path separator at the end
                            modulePath.replaceChar('\\', '/');
                            if (!modulePath.endsWith("/"))
                                modulePath = TempString("{}/", modulePath);

                            // format the path to the depot data folder
                            io::AbsolutePathBuilder moduleDepothPathBuilder(depotManifestPath.basePath());
                            moduleDepothPathBuilder.pushDirectories(UTF16StringBuf(modulePath));
                            moduleDepothPathBuilder.pushDirectory(L"data");

                            // create the absolute path to the project's depot
                            const auto moduleDepotPath = moduleDepothPathBuilder.toAbsolutePath(false);
                            TRACE_INFO("Depot path for module '{}' resolved to '{}'", modulePath, moduleDepotPath);

                            // parse the mount point path
                            const auto depotMountPoint = res::ResourceMountPoint(depotPath);

                            // add the file system
                            auto fileSystemType = DepotFileSystemType::Engine;
                            auto projectFileSystem = CreateUniquePtr<FileSystemNative>(moduleDepotPath, !readonly, this);
                            if (!processFileSystem(depotMountPoint, std::move(projectFileSystem), fileSystemType))
                            {
                                TRACE_ERROR("Failed to mount engine file system from '{}' to '{}'", moduleDepotPath, depotPath);
                            }
                            else
                            {
                                TRACE_INFO("Attached '{}' from '{}'", depotPath, moduleDepotPath);
                            }
                        }
                    }
                    else
                    {
                        TRACE_ERROR("Mount path '{}' is not a valid absolute depot directory path, unable to mount this file system", depotPath);
                        continue;
                    }
                }

                // got to next node in XML
                node = settings->nodeSibling(node, "module");
            }

            // initialized
            TRACE_INFO("Depot file loader initialized");
            return true;
        }

        DepotStructure::~DepotStructure()
        {
            m_fileSystems.clear();
            m_fileSystemsPtrs.clear();
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

        void DepotStructure::attachFileSystem(const res::ResourceMountPoint& depotMountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type)
        {
            // create wrapper
            auto depotWrapper = CreateUniquePtr<DepotFileSystem>(depotMountPoint, std::move(fileSystem), type);
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

        io::ReadFileHandlePtr DepotStructure::createFileReader(StringView<char> filePath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return nullptr;

            // get the timestamp
            if (auto ret = localFileSystem->createReader(localFileSystemPath))
                return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return nullptr;

                // try with the target file system
                if (auto ret = localFileSystem->createReader(localFileSystemPath))
                    return ret;
            }

            return nullptr;
        }

        io::AsyncFileHandlePtr DepotStructure::createFileAsyncReader(StringView<char> filePath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return nullptr;

            // get the timestamp
            if (auto ret = localFileSystem->createAsyncReader(localFileSystemPath))
                return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return nullptr;

                // try with the target file system
                if (auto ret = localFileSystem->createAsyncReader(localFileSystemPath))
                    return ret;
            }

            return nullptr;
        }

        io::WriteFileHandlePtr DepotStructure::createFileWriter(StringView<char> filePath) const
        {
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return false;

            // try with the target file system
            if (localFileSystem->isWritable())
                if (auto ret = localFileSystem->createWriter(localFileSystemPath))
                    return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return false;

                // try with the target file system
                if (localFileSystem->isWritable())
                    if (auto ret = localFileSystem->createWriter(localFileSystemPath))
                        return ret;
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
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_CHANGED, fullPath);

                    break;
                }
            }
        }

        void DepotStructure::notifyFileAdded(IFileSystem* fs, StringView<char> rawFilePath)
        {
            for (auto ptr : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath;
                    ptr->mountPoint().expandPathFromRelative(rawFilePath, fullPath);

                    TRACE_INFO("File {} was reported as added", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_ADDED, fullPath);

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
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_REMOVED, fullPath);

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
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_ADDED, fullPath);

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
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_REMOVED, fullPath);

                    break;
                }
            }
        }

        //---

        bool DepotStructure::processFileSystem(const res::ResourceMountPoint& depotMountPoint, UniquePtr<IFileSystem> fs, DepotFileSystemType type)
        {
            // no file system
            if (!fs)
                return false;

            /*// load the manifest from the file system
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
            }*/

            // finally, attach file system to the loader
            // NOTE: we are attached after our dependencies to make sure the path filtering can be done in a simple way
            attachFileSystem(depotMountPoint, std::move(fs), type);

            // mounted
            return true;
        }

        //--

    } // depot
} // base
