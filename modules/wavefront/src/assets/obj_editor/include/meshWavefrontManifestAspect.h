/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"

namespace wavefront
{
    //--

    /// editor aspect for displaying the wavefront import properties manifest
    class ASSETS_OBJ_EDITOR_API WavefrontProcessingAspect : public ed::SingleResourceEditorManifestAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WavefrontProcessingAspect, ed::SingleResourceEditorManifestAspect);

    public:
        WavefrontProcessingAspect();

        virtual bool initialize(ed::SingleResourceEditor* editor) override;

    private:
        ui::DataInspectorPtr m_properties;
    };

    //--

} // wavefront
