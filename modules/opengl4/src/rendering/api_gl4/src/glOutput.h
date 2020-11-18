/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\output #]
***/

#pragma once

#include "rendering/device/include/renderingOutput.h"
#include "rendering/api_common/include/renderingWindow.h"

#include "glObject.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {

        //---

        // an output object - OpenGL "swapchain" kind of object
        class Output : public Object
        {
        public:
            Output(Device* drv, DeviceThread* th, WindowManager* windows, DriverOutputClass cls, bool flipped);
            virtual ~Output();

            static const auto STATIC_TYPE = ObjectType::Output;

            INLINE DriverOutputClass outputClass() const { return m_outputClass; }
            INLINE bool flipped() const { return m_flipped; }

            INLINE uint64_t windowHandle() const { return m_windowHandle; }
            INLINE uint64_t deviceHandle() const { return m_deviceHandle; }

            INLINE INativeWindowInterface* windowInterface() const { return m_windowInterface; }

            INLINE ObjectID colorID() const { return m_colorID; }
            INLINE ObjectID depthID() const { return m_depthID; }

            //---

            void bind(uint64_t windowHandle, uint64_t deviceHandle, ImageFormat colorFormat, ImageFormat depthFormat, INativeWindowInterface* window);

            bool prepare(ImageView* outColorRT, ImageView* outDepthRT, base::Point& outViewport) const;

            //--

        protected:
            DriverOutputClass m_outputClass;
            bool m_flipped = false;

            ObjectID m_colorID;
            ObjectID m_depthID;

            WindowManager* m_windows = nullptr;
            DeviceThread* m_thread = nullptr;

            uint64_t m_windowHandle = 0; // HWND
            uint64_t m_deviceHandle = 0; // HDC
            ImageFormat m_colorFormat = ImageFormat::UNKNOWN;
            ImageFormat m_depthFormat = ImageFormat::UNKNOWN;

            INativeWindowInterface* m_windowInterface = nullptr;
        };

        //---

        // engine side proxy for device output
        class OutputObjectProxy : public rendering::IOutputObject
        {
        public:
            OutputObjectProxy(ObjectID id, ObjectRegistryProxy* impl, bool flipped, INativeWindowInterface* window);

            virtual bool prepare(ImageView* outColorRT, ImageView* outDepthRT, base::Point& outViewport) override final;

        private:
            base::RefWeakPtr<ObjectRegistryProxy> m_proxy;
        };

        //---

    } // gl4
} // rendering