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

    class IDeviceObjectHandler;

    ///---
    
    // an object created by device API and passed to engine side
    // NOTE: destruction of this object will destroy the API-internal object once the current frame finishes
    class RENDERING_DEVICE_API IDeviceObject : public base::IReferencable
    {
    public:
        IDeviceObject(ObjectID id, IDeviceObjectHandler* impl);
        virtual ~IDeviceObject();

        // get the assigned object ID (it should static ID)
        // NOTE: this ID can be used until the object is deleted, after that it's use is no longer valid (although it will not crash)
        INLINE ObjectID id() const { return m_id; }

    private:
        ObjectID m_id;
        base::RefWeakPtr<IDeviceObjectHandler> m_impl;
    };

    ///---

    /// internal API-owned pointer
    class RENDERING_DEVICE_API IDeviceObjectHandler : public base::IReferencable
    {
    public:
        virtual ~IDeviceObjectHandler();
        virtual void releaseToDevice(ObjectID id) = 0;
    };

    ///---

} // rendering

