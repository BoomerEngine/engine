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
#include "editorWindow.h"

#include "resourceEditor.h"

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

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFile);
    RTTI_END_TYPE();

    ManagedFile::ManagedFile(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> fileName)
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

    const image::ImageRef& ManagedFile::typeThumbnail() const
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

    namespace prv
    {
        class ManagedFileOpenerHelper : public base::ISingleton
        {
            DECLARE_SINGLETON(ManagedFileOpenerHelper);

        public:
            ManagedFileOpenerHelper()
            {
                Array<SpecificClassType<IResourceEditorOpener>> openers;
                RTTI::GetInstance().enumClasses(openers);

                for (auto openerClass : openers)
                    if (auto opener = openerClass->createPointer<IResourceEditorOpener>())
                        m_openers.pushBack(opener);
            }

            bool canOpenFile(const ManagedFile* file) const
            {
                for (auto* opener : m_openers)
                    if (opener->canOpen(file->fileFormat()))
                        return true;

                return false;
            }

            ResourceEditorPtr createEditor(ManagedFile* file) const
            {
                for (auto* opener : m_openers)
                    if (opener->canOpen(file->fileFormat()))
                        return opener->createEditor(file);

                return nullptr;
            }

        private:
            Array<IResourceEditorOpener*> m_openers;

            virtual void deinit() override
            {
                m_openers.clearPtr();
            }
        };

    } // prv


    bool ManagedFile::inUse() const
    {
        return false;
    }

    bool ManagedFile::canOpen() const
    {
        return prv::ManagedFileOpenerHelper::GetInstance().canOpenFile(this);
    }

    bool ManagedFile::open(bool activate /*= true*/)
    {
        // fall back in case the editor pointer is not set
        auto& mainWindow = GetService<Editor>()->mainWindow();
        mainWindow.iterateMainTabs([this](ui::DockPanel* panel)
            {
                if (auto editor = rtti_cast<ResourceEditor>(panel))
                {
                    if (editor->file() == this)
                    {
                        m_editor = AddRef(editor);
                        return true;
                    }
                }

                return false;
            });

        // if we have the editor activate it
        if (m_editor)
        {
            if (activate)
                mainWindow.activateMainTab(m_editor);
            return true;
        }

        // create best editor for this file
        auto editor = prv::ManagedFileOpenerHelper::GetInstance().createEditor(this);
        if (!editor)
            return false;

        // initialize the editor, this will load the editor
        // TODO: it's possible to move it to thread if needed
        if (!editor->initialize())
            return false;

        // load the general editor config
        //auto config = GetService<Editor>()->

        // attach editor to main window and select it
        // TODO: great place to load config for where should the editor window be placed
        m_editor = editor;
        mainWindow.attachMainTab(editor, activate);
        return true;
    }

    bool ManagedFile::save()
    {
        if (m_editor)
            return m_editor->save();
        return false;
    }

    bool ManagedFile::close(bool force /*= false*/)
    {        
        if (m_editor)
        {
            if (m_isModified && !force)
            {
                base::StringBuilder txt;
                txt.appendf("File '{}' is [b][color:#F00]modified[/color][/b].\n \nDo you want to save it or discard the changes?", depotPath());

                ui::MessageBoxSetup setup;
                setup.title("Confirm closing editor");
                setup.message(txt.toString());
                setup.yes().no().cancel().defaultYes().warn();
                setup.caption(ui::MessageButton::Yes, "[img:save] Save");
                setup.caption(ui::MessageButton::No, "[img:delete_black] Discard");
                setup.m_constructiveButton = ui::MessageButton::Yes;
                setup.m_destructiveButton = ui::MessageButton::No;

                const auto ret = ui::ShowMessageBox(m_editor, setup);
                if (ret == ui::MessageButton::Yes)
                {
                    if (!m_editor->save())
                    {
                        ui::PostWindowMessage(m_editor, ui::MessageType::Error, "FileSave"_id, TempString("Error saving file '{}'", depotPath()));
                    }
                }
                else if (ret == ui::MessageButton::Cancel)
                {
                    return false;
                }
            }

            auto& mainWindow = GetService<Editor>()->mainWindow();
            mainWindow.detachMainTab(m_editor);

            m_editor->cleanup();
            m_editor.reset();
        }
        
        return true;
    }

    void ManagedFile::changeFileState(const vsc::FileState& state)
    {
        m_state = state;
    }

    //--

} // depot

