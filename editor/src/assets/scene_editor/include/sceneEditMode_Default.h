/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#pragma once

#include "sceneEditMode.h"

namespace ed
{
    //--

    class SceneDefaultPropertyInspectorPanel;

    /// default scene editor edit mode - entity operations
    class ASSETS_SCENE_EDITOR_API SceneEditMode_Default : public ISceneEditMode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneEditMode_Default, ISceneEditMode)

    public:
        SceneEditMode_Default(ActionHistory* actionHistory);
        virtual ~SceneEditMode_Default();

        //--

        // ISceneEditMode
        virtual ui::ElementPtr queryUserInterface() const override;
        virtual Array<SceneContentNodePtr> querySelection() const override;
        virtual void configurePanelToolbar(ScenePreviewContainer* container, const ScenePreviewPanel* panel, ui::ToolBar* toolbar) override;

        virtual void handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame) override;
        virtual ui::InputActionPtr handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt) override;
        virtual bool handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt) override;
        virtual void handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleUpdate(float dt) override;

        virtual void handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection);
        virtual void handleTreeCutNodes(const Array<SceneContentNodePtr>& selection);
        virtual void handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection);
        virtual void handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode);

        //--

        virtual void handleGeneralCopy() override;
        virtual void handleGeneralCut() override;
        virtual void handleGeneralPaste() override;
        virtual void handleGeneralDelete() override;

        virtual bool checkGeneralCopy() const override;
        virtual bool checkGeneralCut() const override;
        virtual bool checkGeneralPaste() const override;
        virtual bool checkGeneralDelete() const override;

        //--

        INLINE const HashSet<SceneContentNodePtr>& selection() const { return m_selection; }

        void actionChangeSelection(const Array<SceneContentNodePtr>& selection);

        void changeSelection(const Array<SceneContentNodePtr>& selection);

        void handleSelectionChanged();

        void activeNode(SceneContentNode* node);

        void reset();

    protected:
        void processObjectDeletion(const Array<SceneContentNodePtr>& selection);

        RefPtr<SceneDefaultPropertyInspectorPanel> m_panel;

        //--

        HashSet<SceneContentNodePtr> m_selection;

        bool m_canCopySelection = false;
        bool m_canCutSelection = false;
        bool m_canDeleteSelection = false;

        void updateSelectionFlags();

        //--

        RefPtr<ui::ClassPickerBox> m_entityClassSelector;
        RefPtr<ui::ClassPickerBox> m_componentClassSelector;

        SceneContentNodeWeakPtr m_activeNode;

        void processVisualSelection(bool ctrl, bool shift, const base::Array<rendering::scene::Selectable>& selectables);

        void createEntityAtNodes(const Array<SceneContentNodePtr>& selection, ClassType entityClass);
        void createComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass);
    };

    //--

    //--

} // ed
