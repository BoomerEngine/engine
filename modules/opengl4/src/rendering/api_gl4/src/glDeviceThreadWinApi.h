/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
* [# platform: winapi #]
***/

#pragma once

#include "glDeviceThread.h"

#include <Windows.h>

namespace rendering
{
    namespace gl4
    {
        //--

        // WinAPI specific OpenGL 4 driver thread
        class DeviceThreadWinApi : public DeviceThread
        {
        public:
            DeviceThreadWinApi(Device* drv, WindowManager* windows);
            virtual ~DeviceThreadWinApi();

            // initialize
            bool initialize(const base::app::CommandLine& cmdLine);

            //--

            virtual ObjectID createOutput(const OutputInitInfo& info) override final;
            virtual void bindOutput(ObjectID output) override final;
            virtual void swapOutput(ObjectID output) override final;

            virtual void prepareWindowForDeletion_Thread(uint64_t windowHandle, uint64_t deviceHandle) override final;
            virtual void postWindowForDeletion_Thread(uint64_t windowHandle) override final;

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
} // rendering