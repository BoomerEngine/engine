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
    //--

    // internal ID of the object
    // used instead of pointer since ALL of the true knowledge about the object is on the rendering side
    class RENDERING_DEVICE_API ObjectID
    {
    public:
        static const uint64_t TYPE_NULL = 0;
        static const uint64_t TYPE_TRANSIENT = 1;
        static const uint64_t TYPE_PREDEFINED = 2;
        static const uint64_t TYPE_STATIC = 3;

        //--

        INLINE ObjectID() { data.raw = 0;  }; // 0
        INLINE ~ObjectID() = default;

        INLINE ObjectID(const ObjectID& other) = default;
        INLINE ObjectID(ObjectID&& other) = default;
        INLINE ObjectID(std::nullptr_t) { data.raw = 0; };

        INLINE ObjectID& operator=(const ObjectID& other) = default;
        INLINE ObjectID& operator=(ObjectID&& other) = default;

        INLINE bool operator==(const ObjectID& other) const { return data.raw == other.data.raw; }
        INLINE bool operator!=(const ObjectID& other) const { return data.raw != other.data.raw; }

        // is this a null object ID ?
        INLINE bool empty() const { return (TYPE_NULL == data.fields.type); }

        // get the internal object index
        INLINE uint32_t index() const { return data.fields.index; }

        // get the internal object generation
        INLINE uint32_t generation() const { return data.fields.generation; }

        // is the ID for a transient object
        INLINE bool isTransient() const { return data.fields.type == TYPE_TRANSIENT; }

        // is the ID for a predefined object
        INLINE bool isPredefined() const { return data.fields.type == TYPE_PREDEFINED; }

        // is the ID for a static object
        INLINE bool isStatic() const { return data.fields.type == TYPE_STATIC; }

        // easy test
        INLINE explicit operator bool() const { return !empty(); }

        //--

        // NULL object ID
        static const ObjectID& EMPTY();

        //--

        // allocate transient object ID - something that will be used later
        // transient objects are not reference counted, they are all released at the end of the command buffer they were allocated at
        // transient object IDs never repeat (until they wrap, which is considered safe and handled, also, we use 64-bit value, so yeah, it would take some time)
        static ObjectID AllocTransientID();

        // create static object
        static ObjectID CreateStaticID(uint32_t internalIndex, uint32_t internalGeneration);

        // create predefined object ID
        static ObjectID CreatePredefinedID(uint8_t predefinedIndex);

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

        //--

    private:
        union
        {
            struct
            {
                uint64_t type : 2;
                uint64_t index : 30;
                uint64_t generation : 32;
            } fields;

            uint64_t raw;

        } data;
    };

    //--

} // rendering
