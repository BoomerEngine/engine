/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "engine/ui/include/uiSimpleTreeModel.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// scene structure panel (tree)
class EDITOR_SCENE_EDITOR_API SceneStructurePanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneStructurePanel, ui::IElement);

public:
    SceneStructurePanel(SceneContentStructure* scene, ScenePreviewContainer* preview);
    virtual ~SceneStructurePanel();

    void syncExternalSelection(const Array<SceneContentNodePtr>& nodes);
        
    virtual void configSave(const ui::ConfigBlock& block) const;
    virtual void configLoad(const ui::ConfigBlock& block);

private:
    RefPtr<SceneContentStructure> m_scene;
    RefPtr<ScenePreviewContainer> m_preview;

    //--

    void presetAddNew();
    void presetRemove();
    void presetSave();
    void presetSelect();

    struct ScenePreset
    {
        RTTI_DECLARE_POOL(POOL_UI_OBJECTS);

    public:
        StringBuf name;
        bool dirty = false;
        Array<StringBuf> hiddenNodes;
        Array<StringBuf> shownNodes;
    };

    ui::ComboBoxPtr m_presetList;

    Array<ScenePreset*> m_presets;
    ScenePreset* m_presetActive = nullptr;

    void collectPresetState(ScenePreset& outState) const;
    void applyPresetState(const ScenePreset& state);

    void updatePresetList(StringView presetToSelect);
    void applySelectedPreset();

    void presetSave(const ui::ConfigBlock& block) const;
    void presetLoad(const ui::ConfigBlock& block);

    //--

    ui::SearchBarPtr m_searchBar;

    RefPtr<SceneContentTreeView> m_tree;

    void treeSelectionChanged();

    void handleTreeObjectDeletion();
    void handleTreeObjectCopy();
    void handleTreeObjectCut();
    void handleTreeObjectPaste(bool relative);

    //--
};

//--

END_BOOMER_NAMESPACE_EX(ed)
