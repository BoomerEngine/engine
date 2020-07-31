/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: selection #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {

        //---

        /// rendering selectable
        /// this contains information required to implement selection by clicking
        class RENDERING_SCENE_API Selectable
        {
        public:
            INLINE Selectable() {}
            INLINE Selectable(const Selectable& other) = default;
            INLINE Selectable(Selectable&& other) = default;
            INLINE Selectable& operator=(const Selectable& other) = default;
            INLINE Selectable& operator=(Selectable && other) = default;
            INLINE ~Selectable() {}

            INLINE Selectable(uint32_t objectID, uint32_t subObjectID = 0)
                : m_objectID(objectID)
                , m_subObjectID(subObjectID)
            {}

            INLINE bool valid() const
            {
                return m_objectID != 0;
            }

            INLINE operator bool() const
            {
                return m_objectID != 0;
            }

            INLINE static uint32_t CalcHash(const Selectable& key)
            {
                return base::CRC32() << key.m_objectID << key.m_subObjectID;
            }

            INLINE uint32_t objectID() const
            {
                return m_objectID;
            }

            INLINE uint32_t subObjectID() const
            {
                return m_subObjectID;
            }

            //--

            INLINE bool operator==(const Selectable& other) const
            {
                return m_objectID == other.m_objectID && m_subObjectID == other.m_subObjectID;
            }

            INLINE bool operator!=(const Selectable& other) const
            {
                return !operator==(other);
            }

            INLINE bool operator<(const Selectable& other) const
            {
                if (m_objectID != other.m_objectID)
                    return m_objectID < other.m_objectID;
                return m_subObjectID < other.m_subObjectID;
            }

            //--

            // get a string representation (for debug and error printing)
            void print(base::IFormatStream& f) const;

            //--

        private:
            uint32_t m_objectID;
            uint32_t m_subObjectID;
        };

        //---

        /// encoded (for rendering) selectable ID
#pragma pack(push)
#pragma pack(2)
        struct RENDERING_SCENE_API EncodedSelectable
        {
            Selectable object;
            uint16_t x = 0;
            uint16_t y = 0;
            float depth = 0.0f; // linear distance from camera

            INLINE bool valid() const
            {
                return object.valid();
            }

            INLINE operator bool() const
            {
                return object.valid();
            }

            void print(base::IFormatStream& f) const; // debug print
        };
#pragma pack(pop)

        static_assert(sizeof(EncodedSelectable) == 16, "Invalid packing for shared CPU/GPU structured");

        //---

    } // scene
} // rendering
