/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#include "build.h"

#include "sceneContentDataView.h"
#include "base/resource/include/objectIndirectTemplateDataView.h"

namespace ed
{
    //--

    DataViewPtr SceneContentEditableObject::createDataView() const
    {
        InplaceArray<const ObjectIndirectTemplate*, 10> ptrs;
        ptrs.reserve(baseData.size());

        for (const auto& ptr : baseData)
            ptrs.pushBack(ptr);

        return RefNew<ObjectIndirectTemplateDataView>(editableData, ptrs.typedData(), ptrs.size());
    }

    //--
    
} // ed
