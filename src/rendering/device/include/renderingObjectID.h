/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering)

//--

namespace gl4 { class Object; }
namespace vulkan { class Object; }
namespace dx12 { class Object; }

//--

// internal ID of the object
// used instead of pointer since ALL of the true knowledge about the object is on the rendering side
class RENDERING_DEVICE_API ObjectID
{
public:
	INLINE ObjectID() = default;
    INLINE ~ObjectID() = default;

	INLINE ObjectID(uint32_t idx, uint32_t gen, const void* debugPtr)
		: m_index(idx)
		, m_generation(gen)
	{
#ifndef BUILD_RELEASE
		m_ptr.rawPtr = debugPtr;
#endif
	}

    INLINE ObjectID(const ObjectID& other) = default;
    INLINE ObjectID(ObjectID&& other) = default;
	INLINE ObjectID(std::nullptr_t) {}

    INLINE ObjectID& operator=(const ObjectID& other) = default;
    INLINE ObjectID& operator=(ObjectID&& other) = default;

	INLINE bool operator==(const ObjectID& other) const { return m_index == other.m_index && m_generation == other.m_generation; }
    INLINE bool operator!=(const ObjectID& other) const { return m_index != other.m_index || m_generation != other.m_generation; }
	INLINE bool operator<(const ObjectID& other) const { return m_generation < other.m_generation; } // guaranteed ordering, generation is never reused

    // is this a null object ID ?
    INLINE bool empty() const { return (0 == m_index) && (0 == m_generation); }

    // get the internal object index
	INLINE uint32_t index() const { return m_index; }

    // get the internal object generation
	INLINE uint32_t generation() const { return m_generation; }

    // easy test
    INLINE explicit operator bool() const { return !empty(); }

    //--

    // NULL object ID
    static const ObjectID& EMPTY();

    //--

    // get a string representation (for debug)
    void print(base::IFormatStream& f) const;

    //--

	// hash for maps
	INLINE static uint32_t CalcHash(const ObjectID& id) { return id.m_generation; }

	//--

private:
    uint32_t m_index = 0;
    uint32_t m_generation = 0;

#ifndef BUILD_RELEASE
	union
	{
		const void* rawPtr = nullptr;
		const gl4::Object* gl4Ptr;
		const vulkan::Object* vulkanPtr;
		const dx12::Object* dx12Ptr;
	} m_ptr;
#endif
};

//--

END_BOOMER_NAMESPACE(rendering)
