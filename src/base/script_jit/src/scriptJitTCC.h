/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "scriptJitGeneralC.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//--

/// TCC based script JIT
class BASE_SCRIPT_JIT_API JITTCC : public JITGeneralC
{
    RTTI_DECLARE_VIRTUAL_CLASS(JITTCC, JITGeneralC);

public:
    JITTCC();

    virtual bool compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& data, StringView outputModulePath, const Settings& settings) override final;

private:
    static StringBuf FindTCCCompiler();
    static StringBuf FindGCCCompiler();
};

//--

END_BOOMER_NAMESPACE(base::script)