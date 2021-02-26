/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "scriptPortableData.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

class PortableData;

// compiled script project, contains metadata
class CORE_SCRIPT_API CompiledProject : public res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledProject, res::IResource);

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

END_BOOMER_NAMESPACE_EX(script)
