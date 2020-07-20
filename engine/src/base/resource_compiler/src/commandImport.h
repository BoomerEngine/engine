/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

#include "base/app/include/command.h"

namespace base
{
    namespace res
    {
        //--

        struct ImportQueueFileCancel;

        //--

        class CommandImport : public app::ICommand
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandImport, app::ICommand);

        public:
            CommandImport();
            virtual ~CommandImport();

            virtual bool run(base::net::MessageConnectionPtr connection, const app::CommandLine& commandline) override final;

            //--

            SpinLock m_canceledFilesLock;
            HashSet<StringBuf> m_canceledFiles;

            bool checkFileCanceled(const StringBuf& depotPath) const;
            void handleImportQueueFileCancel(const base::res::ImportQueueFileCancel& message); // NOTE: called from different thread

            //--
        };

        //--

    } // res
} // base
