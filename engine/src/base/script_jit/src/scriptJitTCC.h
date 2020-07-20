/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "scriptJitGeneralC.h"

namespace base
{
    namespace script
    {
        //--

        /// TCC based script JIT
        class BASE_SCRIPT_JIT_API JITTCC : public JITGeneralC
        {
            RTTI_DECLARE_VIRTUAL_CLASS(JITTCC, JITGeneralC);

        public:
            JITTCC();

            virtual bool compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& data, const io::AbsolutePath& outputModulePath, const Settings& settings) override final;

        private:
            static io::AbsolutePath FindTCCCompiler();
            static io::AbsolutePath FindGCCCompiler();
        };

        //--

    } // script
} // base