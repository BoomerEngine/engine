/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptCompiledProject.h"
#include "scriptPortableData.h"

namespace base
{
    namespace script
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(CompiledProject);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4scripts");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Compiled Script Project");
            RTTI_PROPERTY(m_data);
        RTTI_END_TYPE();

        CompiledProject::CompiledProject()
        {}

        CompiledProject::CompiledProject(const RefPtr<PortableData>& data)
            : m_data(data)
        {
            if (m_data)
                m_data->parent(this);
        }

        //---

    } // script
} // base