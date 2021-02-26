/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/renderingObject.h"
#include "gpu/device/include/renderingObjectID.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

// lower class for the API-side object
// NOTE: API objects are NOT reference counted, they are destroyed once host-side object dies and the frame that happens finishes
class GPU_API_COMMON_API IBaseObject : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_API_OBJECTS)

public:
	IBaseObject(IBaseThread* drv, ObjectType type);
    virtual ~IBaseObject(); // called on rendering thread once all frames this object was used are done rendering

	// object owner - thread that created it
	INLINE IBaseThread* owner() const { return m_owner; }

	// type of the object
    INLINE ObjectType objectType() const { return m_type; }

	// handle to this object
    INLINE ObjectID handle() const { return m_handle; }

	//--

	// calculate or estimate GPU memory usage of this object
	virtual uint64_t calcMemoryUsage() const { return 0; }

	// describe this object in more details
	virtual void additionalPrint(ImageFormat& f) const { }

	// can this object be deleted ?
	// TODO: add internal flag if used more often
	INLINE bool canDelete() const { return m_type != ObjectType::OutputRenderTargetView; }

	// called THE MOMENT client releases last reference to the client-side object
	// NOTE: this may be called from any thread!
	virtual void disconnectFromClient() {};

	//--

	virtual const IBaseCopiableObject* toCopiable() const { return nullptr; }
	virtual IBaseCopiableObject* toCopiable() { return nullptr; }

	//--

	void print(IFormatStream& f) const;

private:
	IBaseThread* m_owner = nullptr;
    ObjectType m_type = ObjectType::Unknown;
    ObjectID m_handle;
};

//--

// object + async copy interface
class GPU_API_COMMON_API IBaseCopiableObject : public IBaseObject
{
	RTTI_DECLARE_POOL(POOL_API_OBJECTS)

public:
	IBaseCopiableObject(IBaseThread* drv, ObjectType type);
	virtual ~IBaseCopiableObject(); // called on rendering thread once all frames this object was used are done rendering

	// update from dynamic data
	virtual void updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range) = 0;

	// copy content from a buffer
	virtual void copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) = 0;

	// copy content from an image
	virtual void copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) = 0;

	///--

	virtual const IBaseCopiableObject* toCopiable() const override final { return this; }
	virtual IBaseCopiableObject* toCopiable()  override final { return this; }

	///--
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
