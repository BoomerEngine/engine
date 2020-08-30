/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

namespace ed
{
    //--
    
    // a container for preview panels in various layouts (single, 4 views, etc)
    class ASSETS_SCENE_COMMON_API EditorScenePreviewContainer : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorScenePreviewContainer, ui::IElement);

    public:
        EditorScenePreviewContainer(bool fullScene);
        virtual ~EditorScenePreviewContainer();

        //--

        // active edit mode
        INLINE const EditorSceneEditModePtr& mode() const { return m_editMode; }

        // all created preview panels
        INLINE const Array<EditorScenePreviewPanelPtr>& panels() const { return m_panels; }

        // get the scene being edited
        INLINE const EditorScenePtr& scene() const { return m_scene; }

        //--

        // switch to edit mode 
        void actionSwitchMode(IEditorSceneEditMode* newMode);

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        Array<EditorScenePreviewPanelPtr> m_panels;

        EditorSceneEditModePtr m_editMode;

        EditorScenePtr m_scene;

        void createPanels();
        void destroyPanels();
    };

    //--

} // ed