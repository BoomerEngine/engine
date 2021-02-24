/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "scriptPortableData.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

class PortableData;

// compiled script project, contains metadata
class BASE_SCRIPT_API CompiledProject : public base::res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledProject, base::res::IResource);

public:
    CompiledProject();
    CompiledProject(const RefPtr<PortableData>& data);

    //--

    // get compiled script data
    INLINE const RefPtr<PortableData>& data() const { return m_data; }

    //--

private:
    RefPtr<PortableData> m_data;
};

//---

END_BOOMER_NAMESPACE(base::script)
