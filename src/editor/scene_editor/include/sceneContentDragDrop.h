/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

#include "engine/ui/include/uiDragDrop.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// drag&drop data with scene node
class EDITOR_SCENE_EDITOR_API SceneNodeDragDropData : public ui::IDragDropData
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneNodeDragDropData, ui::IDragDropData);

public:
    SceneNodeDragDropData(const SceneContentNode* node);

    INLINE const SceneContentNodePtr& data() const { return m_node; }

private:
    virtual ui::ElementPtr createPreview() const;

    SceneContentNodePtr m_node;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
