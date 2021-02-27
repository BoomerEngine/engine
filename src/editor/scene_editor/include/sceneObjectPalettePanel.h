/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/world/include/entity.h"
#include "engine/ui/include/uiSimpleListModel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct SceneResourceBasedObjectFactoryInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(SceneResourceBasedObjectFactoryInfo);

public:
    ClassType resourceClass;
    SpecificClassType<Entity> entityClass;
    Array<SpecificClassType<Entity>> allowedEntityClasses;
};

class SceneResourceBasedObjectFactoryListModel;

//--

/// general object palette panel for scene editor
class EDITOR_SCENE_EDITOR_API SceneObjectPalettePanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneObjectPalettePanel, ui::IElement);

public:
    SceneObjectPalettePanel(ScenePreviewContainer* preview);
    virtual ~SceneObjectPalettePanel();

    virtual void configSave(const ui::ConfigBlock& block) const;
    virtual void configLoad(const ui::ConfigBlock& block);

    SpecificClassType<Entity> selectedEntityClass(ClassType resClass) const;

private:
    Array<SceneResourceBasedObjectFactoryInfo> m_resourceBindings;

    ui::ListViewPtr m_classMappingList;
    RefPtr<SceneResourceBasedObjectFactoryListModel> m_classMappingModel;

    ui::ListViewPtr m_favouritePrefabsList;

    void enumerateResourceClasses();
    void createInterface();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
