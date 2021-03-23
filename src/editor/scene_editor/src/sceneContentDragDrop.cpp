/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneContentNodes.h"
#include "sceneContentDragDrop.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

SceneNodeDragDropData::SceneNodeDragDropData(const SceneContentNode* node)
    : m_node(AddRef(node))
{}

ui::ElementPtr SceneNodeDragDropData::createPreview() const
{
    StringBuilder caption;
    caption << "[img:node] ";
    caption << m_node->name();

    return RefNew<ui::TextLabel>(caption.view());
}

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneNodeDragDropData);
RTTI_END_TYPE();

//--
    
END_BOOMER_NAMESPACE_EX(ed)
