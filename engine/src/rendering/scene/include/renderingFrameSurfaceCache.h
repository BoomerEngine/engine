/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "rendering/driver/include/renderingDeviceObject.h"

namespace rendering
{
    namespace scene
    {

        ///---

        /// collection of surfaces to be used with frame rendering
        class RENDERING_SCENE_API FrameSurfaceCache : public IDeviceObject
        {
        public:
            FrameSurfaceCache(const FrameParams_Resolution& res);
            virtual ~FrameSurfaceCache();

            // check if this cache can be used with scene with given parameters
            bool supports(const FrameParams_Resolution& res) const;

            // get buffer resource
            const BufferView* fetchBuffer(FrameResource resourceType) const;

            // get image resource
            const ImageView* fetchImage(FrameResource resourceType) const;

            //--

        private:
            virtual base::StringBuf describe() const override final;
            virtual void handleDeviceReset() override final;
            virtual void handleDeviceRelease() override final;

            uint32_t m_maxSupportedWidth = 0;
            uint32_t m_maxSupportedHeight = 0;
            uint8_t m_supportedMSAALevel = 0;

            ImageView m_sceneFullColorRT; // MSAA scene color render target, full size
            ImageView m_sceneFullDepthRT; // MSAA scene depth render target, full size 
            ImageView m_sceneResolvedColor; // non-MSAA resolved color
            ImageView m_sceneResolvedDepth; // non-MSAA resolved color
        };

        ///---

    } // scene
} // rendering