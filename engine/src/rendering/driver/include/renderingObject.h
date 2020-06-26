/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\objects #]
***/

#pragma once

namespace rendering
{
    // rendering object
    class IObject;

    // internal ID of the object
    // used instead of pointer since ALL of the true knowledge about the object is on the rendering side
    class RENDERING_DRIVER_API ObjectID
    {
    public:
        static const uint64_t TRANSIENT_BIT = 0x1;
        static const uint64_t PREDEFINED_BIT = 0x2;
        static const uint64_t STATIC_BIT = 0x4;

        //--

        INLINE ObjectID() : m_value(0) {}; // 0
        INLINE ~ObjectID() {}

        enum EStatic { STATIC };
        enum EPredefined { PREDEFINED };
        enum ETransient { TRANSIENT };

        ObjectID(const EStatic, void* ptr);
        ObjectID(const EPredefined, uint32_t internalID);
        ObjectID(const ETransient, uint32_t internalID);

        INLINE ObjectID(const ObjectID& other) = default;
        INLINE ObjectID(ObjectID&& other) = default;
        INLINE ObjectID(std::nullptr_t) : m_value(0) {};

        INLINE ObjectID& operator=(const ObjectID& other) = default;
        INLINE ObjectID& operator=(ObjectID&& other) = default;

        INLINE bool operator==(const ObjectID& other) const { return m_value == other.m_value; }
        INLINE bool operator!=(const ObjectID& other) const { return m_value != other.m_value; }

        // reset pointer to NULL
        void reset();

        // is this a null object ID ?
        INLINE bool empty() const { return (0 == m_value); }

        // get the numerical value for the transient object
        INLINE uint32_t internalIndex() const { return (uint32_t)(m_value >> 3); }

        // get the numerical value for the static object
        INLINE void* internalPointer() const { return (void*)(m_value & ~7); }

        // is the ID for a transient object
        INLINE bool isTransient() const { return TRANSIENT_BIT == (TRANSIENT_BIT & m_value); }

        // is the ID for a predefined object
        INLINE bool isPredefined() const { return PREDEFINED_BIT == (PREDEFINED_BIT & m_value); }

        // is the ID for a static object
        INLINE bool isStatic() const { return STATIC_BIT == (STATIC_BIT & m_value); }

        // easy test
        INLINE explicit operator bool() const { return !empty(); }

        //--

        // get the empty object
        static const ObjectID& EMPTY();

        //--

        // allocate transient object ID - something that will be used later
        // transient objects are not reference counted, they are all released at the end of the command buffer they were allocated at
        // transient object IDs never repeat (until they wrap, which is considered safe and handled, also, we use 64-bit value, so yeah, it would take some time)
        static ObjectID AllocTransientID();

        //--

        // get a string representation (for debug)
        void print(base::IFormatStream& f) const;

        //--

        // predefined object
        static ObjectID DefaultPointSampler(bool clamp = true);
        static ObjectID DefaultBilinearSampler(bool clamp = true);
        static ObjectID DefaultTrilinearSampler(bool clamp = true);
        static ObjectID DefaultDepthPointSampler();
        static ObjectID DefaultDepthBilinearSampler();

    private:
        uint64_t m_value;
    };

    // debug information about object
    struct RENDERING_DRIVER_API ObjectDebugInfo
    {
        base::StringID className = "Unknown"_id;
        uint32_t deviceMemorySize = 0;
        uint32_t hostMemorySize = 0;

        ObjectDebugInfo() {};
        ObjectDebugInfo(base::StringID className, uint32_t deviceMemorySize, uint32_t hostMemorySize);
    };

} // rendering
