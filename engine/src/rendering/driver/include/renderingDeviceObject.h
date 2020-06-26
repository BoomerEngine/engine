/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

namespace rendering
{
    ///---
    
    // a rendering device dependent object that need to create a state on the renderer side
    class RENDERING_DRIVER_API IDeviceObject : public base::NoCopy
    {
    public:
        IDeviceObject();
        virtual ~IDeviceObject();

        /// device we are attached to
        inline IDriver* device() const { return m_device; }

        /// get the owning object
        virtual base::ObjectPtr owner() const { return nullptr; }

        /// describe the object
        virtual base::StringBuf describe() const = 0;

        //--

        // device data has to be reset, called from main thread with no other rendering happening
        virtual void handleDeviceReset() = 0;

        // device data has to be released, called from main thread with no other rendering happening
        virtual void handleDeviceRelease() = 0;

        //--

        // switch ownership to give device
        void switchDeviceOwnership(IDriver* device);

    private:
        IDriver* m_device;
        std::atomic<uint32_t> m_registered = 0;
    };

    ///---

} // rendering

