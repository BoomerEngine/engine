/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

namespace ed
{
    //--

    DECLARE_UI_EVENT(EVENT_EDIT_MODE_CHANGED);
    DECLARE_UI_EVENT(EVENT_EDIT_MODE_SELECTION_CHANGED);
    
    // a container for preview panels in various layouts (single, 4 views, etc)
    class ASSETS_SCENE_EDITOR_API ScenePreviewContainer : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePreviewContainer, ui::IElement);

    public:
        ScenePreviewContainer();
        virtual ~ScenePreviewContainer();

        //--

        // scene content
        INLINE SceneContentStructure* content() { return m_content; }

        // active edit mode
        INLINE const SceneEditModePtr& mode() const { return m_editMode; }

        // all created preview panels
        INLINE const Array<ScenePreviewPanelPtr>& panels() const { return m_panels; }

        //--

        // bind content
        void bindContent(SceneContentStructure* content, ISceneEditMode* initialEditMode=nullptr);

        // switch to edit mode 
        void actionSwitchMode(ISceneEditMode* newMode);

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        Array<ScenePreviewPanelPtr> m_panels;

        SceneEditModePtr m_editMode;

        SceneContentStructure* m_content = nullptr;

        void createPanels();
        void destroyPanels();

        void deactivateEditMode();
        void activateEditMode();
    };

    //--

} // ed