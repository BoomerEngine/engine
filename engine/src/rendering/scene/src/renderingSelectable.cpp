/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: selection #]
***/

#include "build.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace scene
    {

        //---

        void Selectable::print(base::IFormatStream& f) const
        {
            if (empty())
            {
                f << "None";
            }
            else
            {
                f.appendf("ObjectID={}", m_objectID);

                if (m_subObjectID != 0)
                    f.appendf(",SubObjectID={}", m_subObjectID);

                if (m_extraBits != 0)
                    f.appendf(",Extra={}", Hex(m_extraBits));
            }
        }

        EncodedSelectable Selectable::encode() const
        {
            EncodedSelectable ret;
            uint64_t MASK = 0xFFFFFFFF;
            ret.SelectableId = m_objectID;
            //ret.SelectableSubIdA = (uint32_t)(m_subObjectID & MASK);
            //ret.SelectableSubIdB = (uint32_t)((m_subObjectID >> 32) & MASK);
            //ret.SelectableExtra = m_extraBits;
            return ret;
        }

        void Selectable::updateHash()
        {
            if (empty())
            {
                m_hash = 0;
            }
            else
            {
                base::CRC64 crc;
                crc << m_objectID;
                crc << m_subObjectID;
                crc << m_extraBits;
                m_hash = crc.crc();
            }
        }

    } // scene
} // rendering