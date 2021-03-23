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
class GAME_COMMON_API GameInputDefinitions : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameInputDefinitions, IResource);

public:
    GameInputDefinitions();

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
