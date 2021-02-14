/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#include "build.h"
#include "depotFileSystem.h"
#include "depotStructure.h"
#include "depotStructureUtils.h"
#include "depotStructureRename.h"

#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceFileTables.h"
#include "base/resource/include/resourceFileTablesBuilder.h"
#include "base/io/include/ioFileHandle.h"

namespace base
{
    namespace depot
    {
        //--

        RenameConfiguration::RenameConfiguration()
        {}

        //--

        IRenameProgressTracker::~IRenameProgressTracker()
        {}

        //--

        bool CopyFileContent(io::IReadFileHandle& sourceFile, io::IWriteFileHandle& targetFile, StringView sourcePath, StringView targetPath, uint64_t size)
        {
            uint64_t left = size;
            while (left)
            {
                uint8_t buffer[4096];

                // read source data
                auto copySize = std::min<uint64_t>(sizeof(buffer), left);
                auto readSize = sourceFile.readSync(buffer, copySize);
                if (readSize != copySize)
                {
                    TRACE_ERROR("IO error reading {} bytes at {} from '{}', only {} read", copySize, sourceFile.pos(), sourcePath, readSize);
                    return false;
                }

                // write data to target
                auto writeSize = targetFile.writeSync(buffer, copySize);
                if (writeSize != copySize)
                {
                    TRACE_ERROR("IO error writing {} bytes at {} to '{}', only {} written", copySize, targetFile.pos(), targetPath, writeSize);
                    return false;
                }

                left -= copySize;
            }

            return true;
        }

        FileRenameJob* CreateFileJob(DepotStructure& depot, StringView sourceDepotPath, StringView targetDepotPath, bool applyLinks, const HashMap<res::ResourcePath, res::ResourcePath>& additionalRenamedPaths)
        {
            TRACE_INFO("Checking resource table for '{}'", sourceDepotPath);

            // open source file
            auto file = depot.createFileAsyncReader(sourceDepotPath);
            if (!file)
            {
                TRACE_ERROR("Unable to open file '{}'", sourceDepotPath);
                return nullptr;
            }

            // load file tables
            Buffer tablesBuffer;
            if (!res::LoadFileTables(file, tablesBuffer))
            {
                TRACE_ERROR("Unable to load file tables for '{}'", sourceDepotPath);
                return nullptr;
            }

            // prepare new tables as a copy of the source with modified links
            auto* job = new FileRenameJob();
            job->sourceDepotPath = StringBuf(sourceDepotPath);
            job->targetDepotPath = targetDepotPath.empty() ? job->sourceDepotPath : StringBuf(targetDepotPath); // can be the same (path refresh only, no move)

            // copy source tables but modify the resource links
            const auto& sourceTables = *(res::FileTables*)tablesBuffer.data();
            job->writeHeader = *sourceTables.header();
            job->writeTables.initFromTables(sourceTables, [&additionalRenamedPaths, &depot, applyLinks, job](StringBuf& inOutPath)
                {
                    auto originalPath = inOutPath;
                    auto sourcePath = res::ResourcePath(inOutPath);

                    // always resolve the links to get up to date file
                    res::ResourcePath renamedFilePath;
                    if (depot.queryFileLoadPath(inOutPath, renamedFilePath))
                    {
                        if (sourcePath != renamedFilePath)
                        {
                            TRACE_INFO("Resolved link '{}' -> '{}'", sourcePath, renamedFilePath);
                            sourcePath = renamedFilePath;
                        }
                    }

                    res::ResourcePath resolvedPath;
                    if (additionalRenamedPaths.find(sourcePath, resolvedPath))
                    {
                        TRACE_INFO("Changed reference '{}' -> '{}'", sourcePath, resolvedPath);
                        inOutPath = StringBuf(resolvedPath.view());
                    }
                    else if (applyLinks) // no real resource change due to move/copy but we can update the reference using discovered links
                    {
                        inOutPath = StringBuf(sourcePath.view());
                    }

                    if (originalPath != inOutPath)
                    {
                        auto& entry = job->pathChanges.emplaceBack();
                        entry.oldPath = originalPath;
                        entry.newPath = inOutPath;
                    }
                });

            // log
            if (job->pathChanges.empty())
            {
                TRACE_INFO("No resource refs changes in '{}'", sourceDepotPath);
            }
            else
            {
                TRACE_INFO("Discovered {} resource refs changes in '{}'", job->pathChanges.size(), sourceDepotPath);
            }

            return job;
        }

        static void CollectFileToRename(DepotStructure& depot, StringView sourcePath, StringView destPath, HashMap<StringBuf, StringBuf>& outFileRenames, IProgressTracker& log)
        {
            DEBUG_CHECK_RETURN(ValidateDepotPath(sourcePath, DepotPathClass::AbsoluteFilePath));
            DEBUG_CHECK_RETURN(ValidateDepotPath(destPath, DepotPathClass::AbsoluteFilePath));

            TRACE_INFO("Added file rename entry '{}' -> '{}'", sourcePath, destPath);
            outFileRenames[StringBuf(sourcePath)] = StringBuf(destPath);
        }

        static void CollectDirectoryToRename(DepotStructure& depot, StringView sourcePath, StringView destPath, HashMap<StringBuf, StringBuf>& outFileRenames, IProgressTracker& log)
        {
            log.reportProgress(TempString("Scanning '{}'...", sourcePath));

            if (log.checkCancelation())
                return;

            DEBUG_CHECK_RETURN(ValidateDepotPath(sourcePath, DepotPathClass::AbsoluteDirectoryPath));
            DEBUG_CHECK_RETURN(ValidateDepotPath(destPath, DepotPathClass::AbsoluteDirectoryPath));
                
            depot.enumDirectoriesAtPath(sourcePath, [&depot, sourcePath, destPath, &outFileRenames, &log](const DepotStructure::DirectoryInfo& dir) -> bool {
                const StringBuf childSourcePath = TempString("{}{}/", sourcePath, dir.name);
                const StringBuf childDestPath = TempString("{}{}/", destPath, dir.name);
                TRACE_INFO("Discovered child directory '{}' in '{}'", dir.name, sourcePath);
                CollectDirectoryToRename(depot, childSourcePath, childDestPath, outFileRenames, log);
                return false;
                });

            depot.enumFilesAtPath(sourcePath, [&depot, sourcePath, destPath, &outFileRenames, &log](const DepotStructure::FileInfo& file) -> bool {
                const StringBuf childSourcePath = TempString("{}{}", sourcePath, file.name);
                const StringBuf childDestPath = TempString("{}{}", destPath, file.name);
                CollectFileToRename(depot, childSourcePath, childDestPath, outFileRenames, log);
                return false;
                });
        }

        static void CollectFilesToUpdate(DepotStructure& depot, StringView depotPath, const HashSet<StringBuf>& alreadyVisitedFiles, Array<StringBuf>& outFiles, IProgressTracker& log)
        {
            DEBUG_CHECK_RETURN(ValidateDepotPath(depotPath, DepotPathClass::AbsoluteFilePath));

            if (alreadyVisitedFiles.contains(depotPath))
                outFiles.emplaceBack(depotPath);
        }

        static void CollectDirectoryToUpdate(DepotStructure& depot, StringView depotPath, const HashSet<StringBuf>& alreadyVisitedFiles, Array<StringBuf>& outFiles, IProgressTracker& log)
        {
            log.reportProgress(TempString("Scanning '{}'...", depotPath));

            if (log.checkCancelation())
                return;

            DEBUG_CHECK_RETURN(ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath));

            depot.enumDirectoriesAtPath(depotPath, [&depot, depotPath, &alreadyVisitedFiles, &outFiles, &log](const DepotStructure::DirectoryInfo& dir) -> bool {
                const StringBuf childDepotPath = TempString("{}{}/", depotPath, dir.name);
                TRACE_INFO("Discovered child directory '{}' in '{}'", dir.name, depotPath);
                CollectFilesToUpdate(depot, childDepotPath, alreadyVisitedFiles, outFiles, log);
                return false;
                });

            depot.enumFilesAtPath(depotPath, [&depot, depotPath, &alreadyVisitedFiles, &outFiles, &log](const DepotStructure::FileInfo& file) -> bool {
                const StringBuf childDepotPath = TempString("{}{}", depotPath, file.name);
                CollectFilesToUpdate(depot, childDepotPath, alreadyVisitedFiles, outFiles, log);
                return false;
                });
        }

        bool CollectRenameJobs(DepotStructure& depot, const RenameConfiguration& config, IProgressTracker& log, Array<FileRenameJob*>& outJobs, Array<StringBuf>& outFailedFiles)
        {
            log.reportProgress("Scanning content...");

            // collect stuff to rename
            HashMap<StringBuf, StringBuf> filesToRename;
            for (const auto& elem : config.elements)
            {
                if (elem.file)
                    CollectFileToRename(depot, elem.sourceDepotPath, elem.renamedDepotPath, filesToRename, log);
                else
                    CollectDirectoryToRename(depot, elem.sourceDepotPath, elem.renamedDepotPath, filesToRename, log);

                if (log.checkCancelation())
                    return false;
            }

            TRACE_INFO("Found {} files to rename", filesToRename.size());

            // create path mapping
            // NOTE: resource paths are not case sensitive but a file rename may change case only
            HashMap<res::ResourcePath, res::ResourcePath> pathsToRename;
            pathsToRename.reserve(filesToRename.size());
            for (const auto pair : filesToRename.pairs())
            {
                const auto sourcePath = res::ResourcePath(pair.key);
                const auto destPath = res::ResourcePath(pair.value);
                if (sourcePath && destPath && (sourcePath != destPath)) // NOTE: the res path can be the same if the only thing we are changing is the file name casing
                    pathsToRename[sourcePath] = destPath;
            }

            // create a rename job for each file
            HashSet<StringBuf> visitedFiles;
            for (const auto pair : filesToRename.pairs())
            {
                visitedFiles.insert(pair.key);
                visitedFiles.insert(pair.value);

                if (log.checkCancelation())
                {
                    outJobs.clearPtr();
                    return false;
                }

                if (auto* job = CreateFileJob(depot, pair.key, pair.value, config.applyLinks, pathsToRename))
                {
                    job->deleteOriginalFile = config.moveFilesNotCopy;
                    outJobs.pushBack(job);
                }
                else
                {
                    outFailedFiles.pushBack(pair.key);
                }
            }

            // collect stuff to update
            if (config.fixupExternalReferences && ValidateDepotPath(config.fixupReferencesRootDir, DepotPathClass::AbsoluteDirectoryPath))
            {
                Array<StringBuf> additionalFilesToUpdate;
                CollectDirectoryToUpdate(depot, config.fixupReferencesRootDir, visitedFiles, additionalFilesToUpdate, log);

                TRACE_INFO("Found {} files to update", additionalFilesToUpdate.size());

                for (const auto& path : additionalFilesToUpdate)
                {
                    if (log.checkCancelation())
                    {
                        outJobs.clearPtr();
                        return false;
                    }

                    if (auto* job = CreateFileJob(depot, path, path, config.applyLinks, pathsToRename))
                    {
                        if (!job->pathChanges.empty())
                            outJobs.pushBack(job);
                        else
                            delete job;
                    }
                }
            }

            return true;
        }

        //--

        bool CreateTombstone(DepotStructure& depot, StringView sourcePath, const res::ResourcePath& destPath)
        {
            StringBuilder txt;
            txt << "LINK:";
            txt << destPath.view();

            StringBuilder linkPath;
            linkPath << sourcePath;
            linkPath << ".renamed";

            auto file = depot.createFileWriter(linkPath.view());
            if (!file)
            {
                TRACE_ERROR("Cannot open link file '{}' for writing tombstone", linkPath);
                return false;
            }

            auto data = txt.view().toBuffer();
            if (file->writeSync(data.data(), data.size()) != data.size())
            {
                TRACE_ERROR("Unable to write tombstone '{}'", linkPath);
                return false;
            }

            TRACE_INFO("Created tombstone for '{}' redirecting to '{}'", sourcePath, destPath);
            return true;
        }

        bool ApplyJobPathsUpdate(DepotStructure& depot, FileRenameJob& job)
        {
            // begin update process
            TRACE_INFO("Fixing resource table for '{}'", job.sourceDepotPath);

            // open source file again
            auto sourceFile = depot.createFileReader(job.sourceDepotPath);
            if (!sourceFile)
            {
                TRACE_ERROR("Unable to open '{}' for reading", job.sourceDepotPath);
                return false;
            }

            // open target file for writing
            auto targetFile = depot.createFileWriter(job.targetDepotPath);
            if (!targetFile)
            {
                TRACE_ERROR("Unable to open '{}' for writing", job.targetDepotPath);
                return false;
            }

            // store the new tables in the target file
            if (!job.writeTables.write(targetFile, 0, 0, 0, &job.writeHeader))
            {
                TRACE_ERROR("Unable to write file tables to '{}'", job.targetDepotPath);
                targetFile->discardContent();
                return false;
            }

            // copy object content
            uint32_t objectIndex = 0;
            for (auto& info : job.writeTables.exportTable)
            {
                sourceFile->pos(info.dataOffset);
                info.dataOffset = targetFile->pos();

                if (!CopyFileContent(*sourceFile, *targetFile, job.sourceDepotPath, job.targetDepotPath, info.dataSize))
                {
                    TRACE_ERROR("Unable to copy object {} at {}, size {}", objectIndex, info.dataOffset, info.dataSize);
                    targetFile->discardContent();
                    return false;
                }

                objectIndex += 1;
            }

            // how much object data we have ?
            uint64_t objectEndPos = targetFile->pos();

            // copy buffer content
            /*for (auto& info : writeTables.bufferTable)
            {
                sourceFile->pos(info.dataOffset);
                info.dataOffset = targetFile->pos();

                if (!CopyFileContent(*sourceFile, *targetFile, info.dataSize, log))
                {
                    log.appendf("*ERROR*: Unable to copy object at {}, size {}" info.dataOffset, info.dataSize);
                    targetFile->discardContent();
                    return false;
                }
            }*/

            // how much buffer data we have ?
            uint64_t bufferEndPos = targetFile->pos();

            // write tables once again with updated data
            targetFile->pos(0);
            if (!job.writeTables.write(targetFile, 0, objectEndPos, bufferEndPos, &job.writeHeader))
            {
                TRACE_ERROR("Unable to write final file tables to '{}'", job.targetDepotPath);
                targetFile->discardContent();
                return false;
            }

            // close the reader first before the writer to allow for update
            sourceFile.reset();
            targetFile.reset();

            // content updated
            TRACE_INFO("Written updated version of '{}' to '{}'", job.sourceDepotPath, job.targetDepotPath);
            return true;
        }

        bool ApplyJobNoUpdate(DepotStructure& depot, FileRenameJob& job)
        {
            // try fast path
            StringBuf sourceAbsolutePath;
            StringBuf destAbsolutePath;
            if (depot.queryFileAbsolutePath(job.sourceDepotPath, sourceAbsolutePath))
            {
                TRACE_INFO("File '{}' resolved to physical location '{}'", job.sourceDepotPath, sourceAbsolutePath);
                if (depot.queryFileAbsolutePath(job.targetDepotPath, destAbsolutePath))
                {
                    TRACE_INFO("File '{}' resolved to physical location '{}'", job.targetDepotPath, destAbsolutePath);

                    if (job.deleteOriginalFile)
                    {
                        if (io::MoveFile(sourceAbsolutePath, destAbsolutePath))
                        {
                            TRACE_INFO("File '{}' moved to '{}' using system IO", job.sourceDepotPath, job.targetDepotPath);
                            return true;
                        }
                        else
                        {
                            TRACE_WARNING("Unable to move '{}' to '{}' using system IO", job.sourceDepotPath, job.targetDepotPath);
                        }
                    }
                    else
                    {
                        if (io::CopyFile(sourceAbsolutePath, destAbsolutePath))
                        {
                            TRACE_INFO("File '{}' copied to '{}' using system IO", job.sourceDepotPath, job.targetDepotPath);
                            return true;
                        }
                        else
                        {
                            TRACE_WARNING("Unable to copy '{}' to '{}' using system IO", job.sourceDepotPath, job.targetDepotPath);
                        }
                    }
                }
                else
                {
                    TRACE_WARNING("File '{}' has no physical location", job.targetDepotPath);
                }
            }
            else
            {
                TRACE_WARNING("File '{}' has no physical location", job.sourceDepotPath);
            }

            // open the source data
            auto sourceFile = depot.createFileReader(job.sourceDepotPath);
            if (!sourceFile)
            {
                TRACE_ERROR("Unable to open source '{}'", job.sourceDepotPath);
                return false;
            }

            // open target for writing
            auto targetFile = depot.createFileWriter(job.targetDepotPath);
            if (!targetFile)
            {
                TRACE_ERROR("Unable to open target '{}'", job.targetDepotPath);
                return false;
            }

            // copy data
            if (!CopyFileContent(*sourceFile, *targetFile, job.sourceDepotPath, job.targetDepotPath, sourceFile->size()))
            {
                TRACE_ERROR("Unable to copy content from '{}' to '{}'", job.sourceDepotPath, job.targetDepotPath);
                targetFile->discardContent();
                return false;
            }

            // copied/moved
            return true;
        }

        bool ApplyJob(DepotStructure& depot, FileRenameJob& job)
        {
            if (job.pathChanges.empty())
                return ApplyJobNoUpdate(depot, job);
            else
                return ApplyJobPathsUpdate(depot, job);
        }

        void ApplyRenameJobs(DepotStructure& depot, const RenameConfiguration& config, IRenameProgressTracker& progress, const Array<FileRenameJob*>& jobs)
        {
            for (auto index : jobs.indexRange())
            {
                if (progress.checkCancelation())
                    break;

                auto* job = jobs[index];
                const auto sameName = (job->sourceDepotPath == job->targetDepotPath);
                const auto deleteOriginal = job->deleteOriginalFile && !sameName;

                if (sameName)
                    progress.reportProgress(index, jobs.size(), TempString("Updating '{}'", job->sourceDepotPath));
                else if (deleteOriginal)
                    progress.reportProgress(index, jobs.size(), TempString("Moving '{}'", job->sourceDepotPath));
                else
                    progress.reportProgress(index, jobs.size(), TempString("Copying '{}'", job->sourceDepotPath));

                const auto status = ApplyJob(depot, *job);
                progress.reportFileStatus(job->sourceDepotPath, status);

                if (status && deleteOriginal)
                {
                    if (config.createThumbstones)
                        CreateTombstone(depot, job->sourceDepotPath, res::ResourcePath(job->targetDepotPath));

                    StringBuf sourceAbsolutePath;
                    if (depot.queryFileAbsolutePath(job->sourceDepotPath, sourceAbsolutePath))
                    {
                        if (!io::DeleteFile(sourceAbsolutePath))
                        {
                            TRACE_WARNING("Unable to delete file '{}'", job->sourceDepotPath);
                        }
                    }
                }
            }
        }

        //--

    } // depot
} // base
