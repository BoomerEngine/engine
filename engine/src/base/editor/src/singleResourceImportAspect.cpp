/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "singleResourceEditor.h"
#include "singleResourceImportAspect.h"
#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiTextLabel.h"
#include "managedFileFormat.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_CLASS(AssetImportCheckWidget);
    RTTI_END_TYPE();

    AssetImportCheckWidget::AssetImportCheckWidget()
    {
        this->styleType("SimpleGroupBox"_id);

        layoutVertical();

        m_fileNameText = createChild<ui::TextLabel>("-- no file --");
        m_statusText = createChild<ui::TextLabel>("[img:question_red] Unknown");

        {
            auto buttonBox = createChild();
            buttonBox->layoutHorizontal();

            {
                m_buttonRecheck = buttonBox->createChild<ui::Button>("[img:arrow_refresh] Recheck");
                m_buttonRecheck->OnClick = [this]() { cmdRecheckeck(); };
                m_buttonRecheck->enable(false);
            }

            {
                m_buttonReimport = buttonBox->createChild<ui::Button>("[img:cog] Reimport");
                m_buttonReimport->OnClick = [this]() { cmdReimport(); };
                m_buttonReimport->enable(false);
            }

            {
                m_buttonShowFileList = buttonBox->createChild<ui::Button>("[img:table_cog] Assets");
                m_buttonShowFileList->OnClick = [this]() { cmdShowSourceAssets(); };
                m_buttonShowFileList->enable(false);
            }
        }
    }

    void AssetImportCheckWidget::bindFile(const ManagedFile* file)
    {
        if (m_file != file)
        {
            m_file = file;

            m_buttonReimport->enable(false);
            m_buttonShowFileList->enable(false);
            m_buttonRecheck->enable(false);

            if (m_file)
            {
                cmdRecheckeck();
            }
            else
            {
                m_fileNameText = createChild<ui::TextLabel>("-- no file --");
                m_statusText = createChild<ui::TextLabel>("[img:question_red] Unknown");
            }
        }
    }

    AssetImportCheckWidget::~AssetImportCheckWidget()
    {}

    void AssetImportCheckWidget::cmdShowSourceAssets()
    {

    }

    void AssetImportCheckWidget::cmdRecheckeck()
    {
        if (m_file)
        {
            m_fileNameText->text(m_file->name());
            m_statusText->text("[img:hourglass] Checking...");
        }
    }

    void AssetImportCheckWidget::cmdReimport()
    {

    }

    void AssetImportCheckWidget::cmdSaveConfiguration()
    {

    }

    void AssetImportCheckWidget::cmdLoadConfiguration()
    {

    }
    
    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(SingleResourceEditorImportAspect);
    RTTI_END_TYPE();

    SingleResourceEditorImportAspect::SingleResourceEditorImportAspect()
    {}

    bool SingleResourceEditorImportAspect::initialize(SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto cookedEditor = base::rtti_cast<SingleLoadedResourceEditor>(editor))
        {
            if (cookedEditor->file()->fileFormat().canUserImport())
            {
                return true;
            }
        }

        return false;
    }

    void SingleResourceEditorImportAspect::shutdown()
    {
        // nothing
    }

    //--

} // editor

