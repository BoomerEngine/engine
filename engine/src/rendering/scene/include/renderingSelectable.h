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

        /// encoded (for rendering) selectable ID
        struct RENDERING_SCENE_API EncodedSelectable
        {
            uint32_t SelectableId;
            uint32_t SelectablePixel;
            float SelectableDepth;
            uint32_t Dummy;

            INLINE EncodedSelectable()
                : SelectableId(0)
                , SelectablePixel(0)
                , SelectableDepth(0.0f)
            {}

            INLINE base::Point position() const
            {
                auto x  = SelectablePixel & 0xFFFF;
                auto y  = (SelectablePixel >> 16) & 0xFFFF;
                return base::Point((int)x, (int)y);
            }
        };

        //---

        /// rendering selectable
        /// this contains information required to implement selection by clicking
        class RENDERING_SCENE_API Selectable
        {
        public:
            INLINE Selectable()
                : m_objectID()
                , m_subObjectID(0)
                , m_extraBits(0)
                , m_hash(0)
            {}

            INLINE Selectable(const EncodedSelectable& encodedSelectable)
                : m_objectID(encodedSelectable.SelectableId)
                , m_subObjectID(0)
                , m_extraBits(0)
            {
                updateHash();
            }

            INLINE Selectable(uint32_t objectID, uint64_t subObjectID=0, uint32_t extra=0)
                : m_objectID(objectID)
                , m_subObjectID(subObjectID)
                , m_extraBits(extra)
            {
                updateHash();
            }

            INLINE Selectable(const Selectable& other)
                : m_objectID(other.m_objectID)
                , m_subObjectID(other.m_subObjectID)
                , m_extraBits(other.m_extraBits)
                , m_hash(other.m_hash)
            {}

            INLINE Selectable& operator=(const Selectable& other)
            {
                if (this != &other)
                {
                    m_objectID = other.m_objectID;
                    m_subObjectID = other.m_subObjectID;
                    m_extraBits = other.m_extraBits;
                    m_hash = other.m_hash;
                }

                return *this;
            }

            INLINE bool empty() const
            {
                return m_objectID == 0;
            }

            INLINE static uint32_t CalcHash(const Selectable& key)
            {
                return key.m_hash;
            }

            INLINE uint32_t objectID() const
            {
                return m_objectID;
            }

            //--

            INLINE bool operator==(const Selectable& other) const
            {
                return m_hash == other.m_hash;
            }

            INLINE bool operator<(const Selectable& other) const
            {
                return m_hash < other.m_hash;
            }

            INLINE bool operator!=(const Selectable& other) const
            {
                return m_hash != other.m_hash;
            }

            //--

            // get a string representation (for debug and error printing)
            void print(base::IFormatStream& f) const;

            //--

            // encode for rendering
            EncodedSelectable encode() const;

        private:
            uint32_t m_objectID;
            uint64_t m_subObjectID;
            uint32_t m_extraBits;
            uint64_t m_hash; // computed from data

            void updateHash();
        };

    } // scene
} // rendering
