/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#pragma once

namespace viewer
{
    class LoadedModel;

    // A payload for rendering by the model renderer
    class ModelRendererPayload : public IFramePayload
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ModelRendererPayload, IFramePayload);

    public:
        struct ModelInstance
        {
            RefPtr<LoadedModel>  model;
            Matrix localToWorld;
        };

        Array<ModelInstance> modelInstances;
    };

    // A renderer "composer" class for rendering model into the frame
    // This class is given a payload (which will be our model to render) and some basic info 
    // like render targets, viewport, etc and will be asked to record all the necessary 
    // command buffers to facilitate the rendering of the model
    class ModelRenderer : public IFramePayloadComposer
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ModelRenderer, IFramePayloadComposer);

    public:
        ModelRenderer();
        virtual ~ModelRenderer();

    protected:
        virtual bool initialize(IDriver* driver) override;
        virtual void compose(const ImageView& color, const ImageView& depth, const Rect& area, const IFramePayload* paload, const FrameCollectorNode& node);
    };

} // viewer

