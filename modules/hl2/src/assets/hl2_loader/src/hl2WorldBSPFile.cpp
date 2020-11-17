/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2WorldBSPFile.h"

namespace hl2
{
    namespace bsp
    {

        RTTI_BEGIN_TYPE_CLASS(File);
            RTTI_PROPERTY(m_buffer);
        RTTI_END_TYPE();

        File::File()
            : m_header(nullptr)
        {}

        File::~File()
        {}

        base::Buffer File::gameLumpData(uint32_t fourCC) const
        {
            return nullptr;
        }

        uint32_t File::gameLumpVersion(uint32_t fourCC) const
        {
            return 0;
        }

        const base::RefPtr<File> File::LoadFromFile(base::res::IResourceCookerInterface& cooker)
        {
            // load content to path
            const auto& sourcePath = cooker.queryResourcePath();
            auto data = cooker.loadToBuffer(sourcePath);
            if (!data)
            {
                TRACE_ERROR("Unable to load data from '{}'", sourcePath);
                return nullptr;
            }

            // check if we are a BSP file
            auto header  = (const dheader_t*) data.data();
            if (data.size() < sizeof(dheader_t) || header->ident != 0x50534256)
            {
                TRACE_ERROR("File '{}' is not a valid BSP file", sourcePath);
                return nullptr;
            }

            // print basic stats
            TRACE_INFO("BSP file version: {}", header->version);

            // setup object
            auto ret = base::RefNew<File>();
            ret->m_buffer = data;
            ret->m_header = (const dheader_t*)ret->m_buffer.data();
            return ret;
        }

        void File::onPostLoad()
        {
            TBaseClass::onPostLoad();
            m_header = (const dheader_t *) m_buffer.data();
        }

        //--

    } // bsp
} // hl2