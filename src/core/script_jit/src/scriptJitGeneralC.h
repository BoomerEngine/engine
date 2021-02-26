/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

class JITTypeLib;
class JITConstCache;

/// General C based compiler
class CORE_SCRIPT_JIT_API JITGeneralC : public IJITCompiler
{
    RTTI_DECLARE_VIRTUAL_CLASS(JITGeneralC, IJITCompiler);

public:
    JITGeneralC();
    virtual ~JITGeneralC();

    virtual bool compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& data, StringView outputModulePath, const Settings& settings) override;

protected:
    mem::LinearAllocator m_mem;

    bool m_emitExceptions;
    bool m_emitLines;

    const PortableData* m_data;
    UniquePtr<JITTypeLib> m_typeLib;
    UniquePtr<JITConstCache> m_constCache;

    //--

    struct ExportedFunction : public NoCopy
    {
        uint64_t codeHash = 0;
        const JITType* jitClass = nullptr;
        StringView functionName;
        StringView jitName; // general wrapper
        StringView jitLocalName; // fast wrapper
        const StubFunction* stub = nullptr;
        StringBuf code; // non empty only if valid JIT was generated
    };

    Array<ExportedFunction*> m_exportedFunctions;
    HashMap<const StubFunction*, ExportedFunction*> m_exportedFunctionMap;

    void printFunctionExports(IFormatStream& f) const;
    void printFunctionBodies(IFormatStream& f) const;

    void exportFunction(const StubFunction* func);
    bool generateCode();

    void printFunctionSignature(IFormatStream& f, const StubFunction* func) const;

    StringBuf writeTempSourceFile() const;
};

//--

END_BOOMER_NAMESPACE_EX(script)
