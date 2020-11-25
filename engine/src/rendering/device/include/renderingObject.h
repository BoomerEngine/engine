/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#pragma once

namespace rendering
{

    ///---
    
    // an object created by device API and passed to engine side
    // NOTE: destruction of this object will destroy the API-internal object once the current frame finishes
    class RENDERING_DEVICE_API IDeviceObject : public base::IReferencable
	{
		RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDeviceObject);

    public:
        IDeviceObject(ObjectID id, IDeviceObjectHandler* impl);
        virtual ~IDeviceObject();

        // get the assigned object ID (it should static ID)
        // NOTE: this ID can be used until the object is deleted, after that it's use is no longer valid (although it will not crash)
        INLINE ObjectID id() const { return m_id; }

        //--

        // calculate GPU memory size used by the object
        virtual uint32_t calcMemorySize() const { return 0; }

        //--

		// resolve pointer to internal API object, always valid if this object is alive
		// NOTE: obviously should not be cached :)
		api::IBaseObject* resolveInternalApiObjectRaw(uint8_t objectType) const;

		// resolve pointer to internal API object, always valid if this object is alive
		// NOTE: obviously should not be cached :)
		template< typename T >
		INLINE T* resolveInternalApiObject() const
		{
			return (T*)resolveInternalApiObjectRaw((uint8_t) T::STATIC_TYPE);
		}

		//--

	protected:
		// TODO: fix
		INLINE IDeviceObjectHandler* owner() const { return m_impl.unsafe(); }

    private:
        ObjectID m_id;

        base::RefWeakPtr<IDeviceObjectHandler> m_impl;
    };

    ///---

    // a view of device resource
    // NOTE: destruction of this object will destroy some of the API-internal object but NOT the original resource
    // NOTE: this object will keep the original resource alive
    class RENDERING_DEVICE_API IDeviceObjectView : public base::IReferencable
    {
		RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDeviceObjectView);

    public:
        IDeviceObjectView(ObjectID viewId, DeviceObjectViewType viewType, IDeviceObject* object, IDeviceObjectHandler* impl);
        virtual ~IDeviceObjectView();

        //--

        // type of the view (descriptor type)
        INLINE DeviceObjectViewType viewType() const { return m_viewType; }

        // get the assigned view ID 
        INLINE ObjectID viewId() const { return m_viewId; }

        // get source resource, guaranteed alive hence no shared ptr
        INLINE IDeviceObject* object() const { return m_object; }

        //--

    private:
        ObjectID m_viewId;
        DeviceObjectViewType m_viewType;
        DeviceObjectPtr m_object;
        base::RefWeakPtr<IDeviceObjectHandler> m_impl;
    };

    ///---

    /// internal API-owned pointer
    class RENDERING_DEVICE_API IDeviceObjectHandler : public base::IReferencable
    {
    public:
        virtual ~IDeviceObjectHandler();

        virtual void releaseToDevice(ObjectID id) = 0;

		virtual api::IBaseObject* resolveInternalObjectPtrRaw(ObjectID id, uint8_t objectType) = 0;

		template< typename T >
		INLINE T* resolveInternalObject(ObjectID id) // rendering::api::IBaseObject
		{
			return (T*)resolveInternalObjectPtrRaw(id, (uint8_t)T::STATIC_TYPE);
		}
    };

    ///---

    // sampler object wrapper
    class RENDERING_DEVICE_API SamplerObject : public IDeviceObject
    {
		RTTI_DECLARE_VIRTUAL_CLASS(SamplerObject, IDeviceObject);

    public:
        SamplerObject(ObjectID id, IDeviceObjectHandler* impl);
        virtual ~SamplerObject();
    };

    ///---

} // rendering

