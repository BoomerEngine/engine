/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "base/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE(game)

//---

/// file with input mapping definitions
class GAME_HOST_API InputDefinitions : public base::res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputDefinitions, base::res::IResource);

public:
    InputDefinitions();

    // get root context
    INLINE const InputActionTablePtr& root() const { return m_root; }

    //--

    // find input table by name
    InputActionTablePtr findTable(base::StringView name) const;

public:
    InputActionTablePtr m_root; // root context
};

//---

END_BOOMER_NAMESPACE(game)