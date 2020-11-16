/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "editorService.h"
#include "assetFileImportBackgroundCommand.h"
#include "assetFileImportProcessTab.h"

#include "base/io/include/ioSystem.h"
#include "base/xml/include/xmlUtils.h"
#include "base/app/include/commandline.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileList.h"
#include "base/net/include/messageConnection.h"

namespace ed
{

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportCommand);
        RTTI_FUNCTION_SIMPLE(handleImportQueueFileStatusChangeMessage);
    RTTI_END_TYPE();

    AssetImportCommand::AssetImportCommand(const res::ImportListPtr& fileList, AssetProcessingListModel* listModel)
        : IBackgroundCommand("import")
        , m_fileList(fileList)
        , m_listModel(AddRef(listModel))
    {
    }

    AssetImportCommand::~AssetImportCommand()
    {
        deleteTempFile();
    }

    void AssetImportCommand::cancelSingleFile(const StringBuf& depotPath)
    {
        res::ImportQueueFileCancel msg;
        msg.depotPath = depotPath;
        connection()->send(msg);
    }

    //--

    void AssetImportCommand::deleteTempFile()
    {
        if (m_tempListPath)
        {
            base::io::DeleteFile(m_tempListPath);
            m_tempListPath = StringBuf();
        }
    }

    void AssetImportCommand::handleImportQueueFileStatusChangeMessage(const res::ImportQueueFileStatusChangeMessage& msg)
    {
        m_listModel->setFileStatus(msg.depotPath, msg.status, msg.time);
    }

    void AssetImportCommand::update()
    {
        TBaseClass::update();
    }

    bool AssetImportCommand::configure(app::CommandLine& outCommandline)
    {
        // no files to import
        if (!m_fileList || m_fileList->files().empty())
            return false;

        // save list to a XML
        auto xml = SaveObjectToXML(m_fileList);
        if (!xml)
            return false;

        static std::atomic<uint32_t> GImportIndex = 0;
        const auto& tempDir = base::io::SystemPath(io::PathCategory::TempDir);
        StringBuf tempListPath = TempString("{}importList{}.xml", tempDir, GImportIndex++);

        // save the XML to a temporary file
        if (!xml::SaveDocument(*xml, tempListPath))
            return false;

        outCommandline.param("assetListPath", tempListPath);
        
        m_tempListPath = tempListPath;
        return true;
    }

    //--

} // ed