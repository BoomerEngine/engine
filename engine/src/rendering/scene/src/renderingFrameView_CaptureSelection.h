/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace scene
    {

        //--

        /// command buffers to write to when recording selection fragments
        struct FrameViewCaptureSelectionRecorder : public FrameViewRecorder
        {
            command::CommandWriter viewBegin; // run at the start of the view rendering
            command::CommandWriter viewEnd; // run at the end of the view rendering

            command::CommandWriter depthPrePass; // depth pre pass, used to filter foreground fragments
            command::CommandWriter mainFragments; // selection emitting fragments

            FrameViewCaptureSelectionRecorder();
        };

        //--

        /// helper recorder class
        class RENDERING_SCENE_API FrameViewCaptureSelection : public base::NoCopy
        {
        public:
            struct Setup
            {
                Camera camera;
                base::Rect viewport;

                base::Rect captureRegion;
                DownloadDataSinkPtr captureSink;
            };

            //--

            FrameViewCaptureSelection(const FrameRenderer& frame, const Setup& setup);
            ~FrameViewCaptureSelection();

            void render(command::CommandWriter& cmd);

            //--

            INLINE const Camera& visibilityCamera() const { return m_camera; }

            //--

        private:
            const FrameRenderer& m_frame;
            const Setup& m_setup;

            base::Rect m_viewport;

            Camera m_camera;

            //--

            void initializeCommandStreams(command::CommandWriter& cmd, FrameViewCaptureSelectionRecorder& rec);

            void bindCamera(command::CommandWriter& cmd);
        };

        //--


    } // scene
} // rendering

