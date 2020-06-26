/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#include "build.h"
#include "modelRenderer.h"

namespace viewer
{

    //--

    RTTI_BEGIN_TYPE_CLASS(ModelRenderer);
    RTTI_END_TYPE();

    ModelRenderer::ModelRenderer()
    {}

    ModelRenderer::~ModelRenderer()
    {}

    bool ModelRenderer::initialize(IDriver* driver)
    {
        return true;
    }

    void ModelRenderer::compose(const ImageView& color, const ImageView& depth, const Rect& area, const IFramePayload* paload, const FrameCollectorNode& node)
    {

    }

    //--

} // viewer
