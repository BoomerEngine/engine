/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#pragma once

#include "renderingFrameView.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace scene
    {

        //--

        /// command buffers to write to when recording shadow cascades
        struct RENDERING_SCENE_API FrameViewCascadesRecorder : public FrameViewRecorder
        {
            struct CascadeSlice
            {
                command::CommandWriter solid;
                command::CommandWriter masked;

                CascadeSlice();
            };

            CascadeSlice slices[MAX_CASCADES];

            FrameViewCascadesRecorder(FrameViewRecorder* parentView);
        };

        //--

        /// view for rendering shadow cascades
        class RENDERING_SCENE_API FrameViewCascades : public base::NoCopy
        {
        public:
            FrameViewCascades(const FrameRenderer& frame, const CascadeData& cascades);
            ~FrameViewCascades();

            void render(command::CommandWriter& cmd, FrameViewRecorder* parentView);

            //--

            INLINE const CascadeInfo& cascade(int index) const { return m_cascades.cascades[index]; }

            //--

        private:
            const FrameRenderer& m_frame;
            const CascadeData& m_cascades;

            //--

            void initializeCommandStreams(command::CommandWriter& cmd, FrameViewCascadesRecorder& rec);

            //--
        };

        //--

    } // scene
} // rendering

