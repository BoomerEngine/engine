/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "inputContext.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// Null input context
class CORE_INPUT_API InputContextNull : public IInputContext
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputContextNull, IInputContext);

public:
    InputContextNull(uint64_t nativeWindow=0, uint64_t nativeDisplay=0);

    //--

    virtual void resetInput() override final;
    virtual void processState() override final;
    virtual void processMessage(const void* msg) override final;
};

///---

END_BOOMER_NAMESPACE()
