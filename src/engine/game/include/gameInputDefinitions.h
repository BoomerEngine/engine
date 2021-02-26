/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// file with input mapping definitions
class ENGINE_GAME_API InputDefinitions : public res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputDefinitions, res::IResource);

public:
    InputDefinitions();

    // get root context
    INLINE const InputActionTablePtr& root() const { return m_root; }

    //--

    // find input table by name
    InputActionTablePtr findTable(StringView name) const;

public:
    InputActionTablePtr m_root; // root context
};

//---

END_BOOMER_NAMESPACE()
