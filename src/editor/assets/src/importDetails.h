/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetImportFileDetails;

// dialog for the import job
class EDITOR_ASSETS_API AssetImportDetailsDialog : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportDetailsDialog, ui::IElement);

public:
    AssetImportDetailsDialog();
    virtual ~AssetImportDetailsDialog();

    //--

    // clear the list
    void clearFiles();

    // set file status, will add file if not there
    void setFileStatus_AnyThread(StringBuf depotFileName, ImportStatus status, float time = 0.0);

    // set file progress information
    void setFileProgress_AnyThread(StringBuf depotFileName, uint64_t count, uint64_t total, StringBuf message);

    //--

public:
    ui::ListViewExPtr m_fileList;
    HashMap<StringBuf, RefPtr<AssetImportFileDetails>> m_fileItems;
    
    //--

    void setFileStatus_MainThread(StringView depotFileName, ImportStatus status, float time);
    void setFileProgress_MainThread(StringView depotFileName, uint64_t count, uint64_t total, StringView message);
};

//--

END_BOOMER_NAMESPACE_EX(ed)
