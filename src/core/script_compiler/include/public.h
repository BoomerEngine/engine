/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_script_compiler_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(script)

/// error handler
class CORE_SCRIPT_COMPILER_API IErrorHandler : public NoCopy
{
public:
    virtual ~IErrorHandler();

    /// report error
    virtual void reportError(const StringBuf& fullPath, uint32_t line, StringView message) = 0;

    /// report warning
    virtual void reportWarning(const StringBuf& fullPath, uint32_t line, StringView message) = 0;
};

END_BOOMER_NAMESPACE_EX(script)
