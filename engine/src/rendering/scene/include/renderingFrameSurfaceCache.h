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
        class RENDERING_SCENE_API FrameSurfaceCache : public IDeviceObject, public base::mem::GlobalPoolObject<POOL_RENDERING_FRAME>
        {
        public:
            FrameSurfaceCache(); // initialized to the max resolution of the device
            virtual ~FrameSurfaceCache();

            //--

            // adjust to support given resolution, may fail
            bool adjust(uint32_t requiredWidth, uint32_t requiredHeight);

            //--

            ImageView m_sceneFullColorRT; // main scene render target
            ImageView m_sceneFullDepthRT; // main scene depth buffer
            ImageView m_sceneResolvedColor; // resolved copy of scene color buffer
            ImageView m_sceneResolvedDepth; // resolved copy of scene depth buffer
            ImageView m_sceneSelectionDepthRT; // selection outline rendering depth RT

            ImageView m_cascadesShadowDepthRT; // array of shadow maps for global cascade shadows
            ImageView m_globalAOShadowMaskRT; // screen size AO/shadow mask buffer

            ImageView m_velocityBufferRT; // full scale screen space XY velocity buffer
            ImageView m_linarizedDepthRT; // full scale linearized depth (AO)
            ImageView m_viewNormalRT; // full scale linearized reconstructed normal (AO)

            BufferView m_selectables; // capture buffer for selection
            uint32_t m_maxSelectables = 0; // maximum number of selectables we can capture

            //--

        private:
            virtual base::StringBuf describe() const override final;
            virtual void handleDeviceReset() override final;
            virtual void handleDeviceRelease() override final;

            uint32_t m_maxSupportedWidth = 0;
            uint32_t m_maxSupportedHeight = 0;

            static const auto HdrFormat = ImageFormat::RGBA16F;
            static const auto DepthFormat = ImageFormat::D24S8;
            static const auto ShadowDepthFormat = ImageFormat::D32;

            bool createViewportSurfaces(uint32_t width, uint32_t height);
            void destroyViewportSurfaces();
        };

        ///---

    } // scene
} // rendering