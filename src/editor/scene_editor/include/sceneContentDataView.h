/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once
#include "base/object/include/dataView.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

struct SceneContentEditableObject
{
    StringBuf name;
    SceneContentEntityNodePtr owningNode;
    ObjectIndirectTemplatePtr editableData;
    Array<ObjectIndirectTemplatePtr> baseData;

    DataViewPtr createDataView() const;
};

//--

END_BOOMER_NAMESPACE(ed)
