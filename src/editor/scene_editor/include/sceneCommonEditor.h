/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/resourceEditorNativeFile.h"
#include "base/world/include/worldPrefab.h"

namespace ed
{
    //--

    class ScenePreviewContainer;
    class SceneStructurePanel;
    class SceneObjectPalettePanel;
    class SceneEditMode_Default;

    /// common editor code for prefabs/scenes
    class EDITOR_SCENE_EDITOR_API SceneCommonEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneCommonEditor, ResourceEditorNativeFile);

    public:
        SceneCommonEditor(ManagedFileNativeResource* file, SceneContentNodeType rootContentType, StringView defaultEditorTag);
        virtual ~SceneCommonEditor();

        //--

        // used to determine if this is a world or prefab editor
        INLINE SceneContentNodeType contentType() const { return m_rootContentType; }

        //--

        virtual bool initialize() override;
        virtual void fillEditMenu(ui::MenuButtonContainer* menu) override;
        virtual void fillViewMenu(ui::MenuButtonContainer* menu) override;
        virtual void update() override;

        //--

        virtual void configLoad(const ui::ConfigBlock& block) override;
        virtual void configSave(const ui::ConfigBlock& block) const override;

    protected:
        base::RefPtr<ScenePreviewContainer> m_previewContainer;
        base::RefPtr<SceneStructurePanel> m_structurePanel;
        base::RefPtr<SceneObjectPalettePanel> m_palettePanel;
        ui::ScrollAreaPtr m_inspectorPanel;

        base::RefPtr<SceneEditMode_Default> m_defaultEditMode;
        
        base::RefPtr<SceneContentStructure> m_content;
        base::world::WorldPtr m_previewWorld;

        SceneContentNodeType m_rootContentType;

        //--

        void createInterface();
        void createContentStructure();

        void refreshEditMode();

        virtual void recreateContent() = 0;
        
        //--

        virtual void handleGeneralCopy() override;
        virtual void handleGeneralCut() override;
        virtual void handleGeneralPaste() override;
        virtual void handleGeneralDelete() override;
        virtual void handleGeneralDuplicate() override;

        virtual bool checkGeneralCopy() const override;
        virtual bool checkGeneralCut() const override;
        virtual bool checkGeneralPaste() const override;
        virtual bool checkGeneralDelete() const override;
        virtual bool checkGeneralDuplicate() const override;
    };

    //--

} // ed
