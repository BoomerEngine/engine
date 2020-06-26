/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"

namespace ed
{
    //--

    /// editor aspect for displaying the texture material bindings in the mesh
    class MeshPackingAspect : public SingleResourceEditorManifestAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshPackingAspect, SingleResourceEditorManifestAspect);

    public:
        MeshPackingAspect();

        virtual bool initialize(SingleResourceEditor* editor) override;

    private:
        ui::DataInspectorPtr m_properties;
    };

    //--

} // ed
