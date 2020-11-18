/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "glDevice.h"
#include "glBuffer.h"
#include "glDeviceThread.h"
#include "glObjectCache.h"
#include "glObjectRegistry.h"
#include "glOutput.h"

namespace rendering
{
    namespace gl4
    {

        ///---


        OutputObjectPtr Device::createOutput(const OutputInitInfo& info)
        {
            if (auto id = m_thread->createOutput(info))
                if (auto* object = m_objectRegistry->resolveStatic<Output>(id))
                    return base::RefNew<OutputObjectProxy>(id, m_objectRegistry->proxy(), object->flipped(), object->windowInterface());

            return nullptr;
        }

        ///---

        void Device::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
        {
            m_windows->enumMonitorAreas(outMonitorAreas);
        }

        void Device::enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const
        {
            m_windows->enumDisplays(outDisplayInfos);
        }

        void Device::enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const
        {
            m_windows->enumResolutions(displayIndex, outResolutions);
        }

        void Device::enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const
        {
            m_windows->enumVSyncModes(displayIndex, outVSyncModes);
        }

        void Device::enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const
        {
            m_windows->enumRefreshRates(displayIndex, info, outRefreshRates);
        }


        ///---

    } // gl4
} // rendering
