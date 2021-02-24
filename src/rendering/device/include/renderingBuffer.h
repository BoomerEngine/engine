/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingResources.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

// buffer object wrapper
class RENDERING_DEVICE_API BufferObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferObject, IDeviceObject);

public:
    struct Setup
    {
        uint32_t size = 0;
        uint32_t stride = 0;
        BufferViewFlags flags = BufferViewFlags();
        ResourceLayout layout = ResourceLayout::INVALID;
    };

    BufferObject(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup);
    virtual ~BufferObject();

    // calculate memory size used by buffer
    virtual uint32_t calcMemorySize() const override;

    //--

    /// get size of the view region (may be zero to indicate the full buffer)
    INLINE uint32_t size() const { return m_size; }

    // structure stride (only structured buffer)
    INLINE uint16_t stride() const { return m_stride; }

    /// get flags
    INLINE BufferViewFlags flags() const { return m_flags; }

    /// get initial layout of this data that we have at the start/end of the command buffer recording
    INLINE ResourceLayout initialLayout() const { return m_layout; }

    //---

    // constant buffer
    INLINE bool constants() const { return m_flags.test(BufferViewFlag::Constants); }

    // dynamic (can be updated from CPU memory)
    INLINE bool dynamic() const { return m_flags.test(BufferViewFlag::Dynamic); }

    // general index buffer
    INLINE bool index() const { return m_flags.test(BufferViewFlag::Index); }

    // general vertex buffer
    INLINE bool vertex() const { return m_flags.test(BufferViewFlag::Vertex); }

    // we can copy to/from this buffer
	// TODO: consider splitting into src/dest if that ever becomes performance issue
    INLINE bool copyCapable() const { return m_flags.test(BufferViewFlag::CopyCapable); }

    // shaders can read this buffer (can create TypedView)
    INLINE bool shadeReadable() const { return m_flags.test(BufferViewFlag::ShaderReadable); }

    // we can use this buffer for indirect arguments
    INLINE bool indirectArgs() const { return m_flags.test(BufferViewFlag::IndirectArgs); }

    // structured buffer (contains entries of constant stride)
    INLINE bool structured() const { return m_flags.test(BufferViewFlag::Structured); }
        
    // buffer can be written in atomic way by many shaders
    INLINE bool uavCapable() const { return m_flags.test(BufferViewFlag::UAVCapable); }

    //--

	// create untyped constant (uniform) view of part of the buffer
	// NOTE: offset and size must be aligned to vec4 (16 bytes)
	virtual BufferConstantViewPtr createConstantView(uint32_t offset = 0, uint32_t size = INDEX_MAX) = 0;

    // create a typed view of this buffer (read only, SRV-like)
	// NOTE: fails for structured buffers or buffers with no shaderReadable flag
	// NOTE: some format conversions are legal -> R32F -> R32U, etc but this is limited
    virtual BufferViewPtr createView(ImageFormat format, uint32_t offset = 0, uint32_t size = INDEX_MAX) = 0;

	// create structured view of this buffer (read only, SRV-like with element stride)
	// NOTE: fails for non-structured buffers or buffers with no shaderReadable flag
	// NOTE: offset and size must be multiple of the buffer stride, use defaults to map view buffer
	virtual BufferStructuredViewPtr createStructuredView(uint32_t offset = 0, uint32_t size = INDEX_MAX) = 0;

    // create writable view of this buffer (UAV-like)
	// NOTE: fails for structured buffers or buffers with no uav flag
	// NOTE: some format conversions are legal -> R32F -> R32U, etc but this is limited
    virtual BufferWritableViewPtr createWritableView(ImageFormat format, uint32_t offset = 0, uint32_t size = INDEX_MAX) = 0;

	// create structured view of this buffer (read only, SRV-like with element stride)
	// NOTE: fails for non-structured buffers or buffers with no uav flag
	// NOTE: offset and size must be multiple of the buffer stride, use defaults to map view buffer
	virtual BufferWritableStructuredViewPtr createWritableStructuredView(uint32_t offset = 0, uint32_t size = INDEX_MAX) = 0;

    //--

protected:
    uint32_t m_size = 0; // as created
    uint32_t m_stride = 0; // as created
    ResourceLayout m_layout = ResourceLayout::INVALID; // default layout at the start/end of command buffer recording
    BufferViewFlags m_flags; // internal flags

    bool validateTypedView(ImageFormat format, uint32_t offset, uint32_t& size, bool writable) const;
	bool validateStructureView(uint32_t stride, uint32_t offset, uint32_t& size, bool writable) const;
	bool validateConstantView(uint32_t offset, uint32_t& size) const;
};

///---

// view of a constant buffer - untyped, structured determined by shader
class RENDERING_DEVICE_API BufferConstantView : public IDeviceObjectView
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferConstantView, IDeviceObjectView);

public:
	BufferConstantView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t offset, uint32_t size);
	virtual ~BufferConstantView();

	// get the original buffer, guaranteed to be alive
	INLINE BufferObject* buffer() const { return static_cast<BufferObject*>(object()); }

	// offset in the original buffer resource this view is at
	INLINE uint32_t offset() const { return m_offset; }

	// size of the buffer region in the view
	INLINE uint32_t size() const { return m_size; }

private:
	uint32_t m_offset = 0;
	uint32_t m_size = 0;
};

///---

// view of a buffer via fundamental type of (uint32, float) etc, view is in general not writable (SRV)
class RENDERING_DEVICE_API BufferView : public IDeviceObjectView
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferView, IDeviceObjectView);

public:
    BufferView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, ImageFormat format, uint32_t offset, uint32_t size);
    virtual ~BufferView();

    // get the original buffer, guaranteed to be alive
    INLINE BufferObject* buffer() const { return static_cast<BufferObject*>(object()); }

    // get format of the data used to view the buffer, does not have to be the same as the buffer itself
    INLINE ImageFormat format() const { return m_format; }

    // offset in the original buffer resource this view is at
    INLINE uint32_t offset() const { return m_offset; }

    // size of the buffer region in the view
    INLINE uint32_t size() const { return m_size; }
         
private:
    ImageFormat m_format = ImageFormat::UNKNOWN;
    uint32_t m_offset = 0;
    uint32_t m_size = 0;
};

///---

// structured view of a buffer with constant element stride, view is in general not writable (SRV)
class RENDERING_DEVICE_API BufferStructuredView : public IDeviceObjectView
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferStructuredView, IDeviceObjectView);

public:
	BufferStructuredView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t stride, uint32_t offset, uint32_t size);
	virtual ~BufferStructuredView();

	// get the original buffer, guaranteed to be alive
	INLINE BufferObject* buffer() const { return static_cast<BufferObject*>(object()); }

	// get size of single buffer element (stride)
	INLINE uint32_t stride() const { return m_stride; }

	// offset in the original buffer resource this view is at
	// NOTE: always a multiply of element stride
	INLINE uint32_t offset() const { return m_offset; }

	// size of the buffer region in the view
	// NOTE: always a multiply of element stride
	INLINE uint32_t size() const { return m_size; }

private:
	uint32_t m_stride = 0;
	uint32_t m_offset = 0;
	uint32_t m_size = 0;
};

///---

// view of a buffer via fundamental type of (uint32, float) that is writable
class RENDERING_DEVICE_API BufferWritableView : public IDeviceObjectView
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferWritableView, IDeviceObjectView);

public:
    BufferWritableView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, ImageFormat format, uint32_t offset, uint32_t size);
    virtual ~BufferWritableView();

    // get the original buffer, guaranteed to be alive
    INLINE BufferObject* buffer() const { return static_cast<BufferObject*>(object()); }

    // get format of the data used to view the buffer
    INLINE ImageFormat format() const { return m_format; }

    // offset in the original buffer resource this view is at
    INLINE uint32_t offset() const { return m_offset; }

    // size of the buffer region in the view
    INLINE uint32_t size() const { return m_size; }

private:
    ImageFormat m_format = ImageFormat::UNKNOWN;
    uint32_t m_offset = 0;
    uint32_t m_size = 0;
};

///---

// writable structured view of a buffer with constant element stride (UAV)
class RENDERING_DEVICE_API BufferWritableStructuredView : public IDeviceObjectView
{
	RTTI_DECLARE_VIRTUAL_CLASS(BufferWritableStructuredView, IDeviceObjectView);

public:
	BufferWritableStructuredView(ObjectID viewId, BufferObject* buffer, IDeviceObjectHandler* impl, uint32_t stride, uint32_t offset, uint32_t size);
	virtual ~BufferWritableStructuredView();

	// get the original buffer, guaranteed to be alive
	INLINE BufferObject* buffer() const { return static_cast<BufferObject*>(object()); }

	// get size of single buffer element (stride)
	INLINE uint32_t stride() const { return m_stride; }

	// offset in the original buffer resource this view is at
	// NOTE: always a multiply of element stride
	INLINE uint32_t offset() const { return m_offset; }

	// size of the buffer region in the view
	// NOTE: always a multiply of element stride
	INLINE uint32_t size() const { return m_size; }

private:
	uint32_t m_stride = 0;
	uint32_t m_offset = 0;
	uint32_t m_size = 0;
};

//--

END_BOOMER_NAMESPACE(rendering)