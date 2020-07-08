/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"

namespace fbx
{
    //--

    /// editor aspect for displaying the wavefront import properties manifest
    class ASSETS_FBX_EDITOR_API FBXProcessingAspect : public ed::SingleResourceEditorManifestAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FBXProcessingAspect, ed::SingleResourceEditorManifestAspect);

    public:
        FBXProcessingAspect();

        virtual bool initialize(ed::SingleResourceEditor* editor) override;

    private:
        ui::DataInspectorPtr m_properties;
    };

    //--

} // fbx
