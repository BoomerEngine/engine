/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: vsc #]
***/

#pragma once

#include "versionControl.h"

namespace ed
{
    namespace vsc
    {
        namespace local
        {

            /// local changelist
            class LocalChangelist : public IChangelist
            {
            public:
                LocalChangelist();

                /// interface implementation
                virtual StringBuf description() const override final;
                virtual ChangelistID id() const override final;
                virtual Result description(const StringBuf &desc) override final;
                virtual Result files(FilePaths &outDepotPaths) const override final;
                virtual Result submit() override final;
                virtual Result revert() override final;
                virtual StringBuf shortDescription() const override final;
                virtual StringBuf longDescription() const override final;
            };

            /// local source control implementation, does not do much
            class LocalVersionControl : public IVersionControl
            {
            public:
                LocalVersionControl();

                virtual bool isReady() const override final;
                virtual Result initialize(const Settings& settings) override final;
                virtual void sync() override final;
                virtual RegistrationToken attachListener(IHostListener* listener) override final;

                virtual ChangelistPtr defaultChangelist() override final;
                virtual ChangelistPtr activeChangelist() override final;
                virtual uint32_t numChangelists() const override final;
                virtual ChangelistPtr changelist(uint32_t index) const override final;
                virtual Result refreshAllChangelists()override final;

                virtual Result createChangelist(const StringBuf &desc, ChangelistPtr &outChangelist) override final;
                virtual Result removeChangelist(const ChangelistPtr& changelist) override final;
                virtual Result activeChangelist(const ChangelistPtr& changelist) override final;
                virtual Result submitChangelist(const ChangelistPtr& changelist, const StringBuf& userComment) override final;
                virtual Result moveFilesToChangelist(const FilePaths& files, const ChangelistPtr& targetChangelist) override final;
                virtual Result fileChangelist(const FilePaths& path, ChangelistPtr& outChangelist) override final;

                virtual Result fileHistory(const FilePath& file, History& outHistory) const override final;
                virtual Result fileInfo(const FilePath& file, FileState& outFileState) const override final;
                virtual Result fileLocks(const FilePath&file, FileLocks& outFileLocks) const override final;

                virtual Result latest(const FilePaths& files, bool force) override final;
                virtual Result revision(const FilePaths& files, uint32_t revision) override final;
                virtual Result refreshFiles(const FilePaths& files) override final;

                virtual Result checkoutFiles(const FilePaths& paths) override final;
                virtual Result addFiles(const FilePaths& paths) override final;
                virtual Result deleteFiles(const FilePaths& paths) override final;
                virtual Result renameFiles(const FilePaths& oldPaths, const FilePaths& newPaths) override final;
                virtual Result revertFiles(const FilePaths& paths) override final;

            private:
                RefPtr<LocalChangelist> m_default;
            };

        } // local

        VersionControlPtr CreateLocalStub()
        {
            return CreateUniquePtr<local::LocalVersionControl>();
        }

    } // vsc
} // ed