/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingObjectView.h"
#include "renderingDeviceService.h"
#include "renderingDriver.h"

namespace rendering
{
    //---

    ObjectView::ObjectView(ObjectViewType type)
        : m_type(type)
    {}

    void ObjectView::print(base::IFormatStream& f) const
    {
        if (empty())
        {
            f << "empty";
        }
        else
        {
            switch (m_type)
            {
                case ObjectViewType::Constants: f << "Constants "; break;
                case ObjectViewType::Buffer: f << "Buffer "; break;
                case ObjectViewType::Image: f << "Image "; break;
                default: f << "Unknown "; break;
            }

            f << "(" << m_id << ")";
        }
    }

    void ObjectView::destroy()
    {
        if (m_id)
        {
            base::GetService<DeviceService>()->device()->releaseObject(m_id);
            m_id = ObjectID();
            m_type = ObjectViewType::Invalid;
        }
    }

    //---

} // rendering
