/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
* [# platform: winapi #]
***/

#pragma once

#include "glDriverThread.h"

#include <Windows.h>

namespace rendering
{
    namespace gl4
    {
        //--

        // WinAPI specific OpenGL 4 driver thread
        class DriverThreadWinApi : public DriverThread
        {
        public:
            DriverThreadWinApi(Driver* drv, WindowManager* windows);
            virtual ~DriverThreadWinApi();

            // initialize
            bool initialize(const base::app::CommandLine& cmdLine);

            //--

            virtual ObjectID createOutput(const DriverOutputInitInfo& info) override final;
            virtual void releaseOutput(ObjectID output) override final;
            virtual void bindOutput(ObjectID output) override final;
            virtual void swapOutput(ObjectID output) override final;

        private:
            HGLRC m_hRC = NULL;
            HDC m_hFakeDC = NULL;
            HWND m_hFakeHWND = NULL;
            HDC m_hActiveDC = NULL;

            bool initializeContext_Thread(const base::app::CommandLine& cmdLine);
            void shutdownContext_Thread();
        };

        //--

    } // gl4
} // driver