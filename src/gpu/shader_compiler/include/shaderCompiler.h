/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "gpu/device/include/shaderService.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

class LocalIncludeHandler;

//--

// runtime shader compiler
class GPU_SHADER_COMPILER_API ShaderCompiler : public IShaderRuntimeCompiler
{
    RTTI_DECLARE_VIRTUAL_CLASS(ShaderCompiler, IShaderRuntimeCompiler);

public:
    ShaderCompiler();

    virtual ShaderDataPtr compileFile(StringView filePath, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies) override final;
    virtual ShaderDataPtr compileCode(StringView code, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies) override final;

private:
    StringBuf m_path; // /data/s

    //--

    struct CachedFileContent
    {
        io::TimeStamp timestamp;
        Buffer content;
    };

    Mutex m_sourceFileLock;
    HashMap<StringBuf, CachedFileContent> m_sourceFileMap;

    bool loadSourceCode(StringView path, Array<ShaderDependency>* outDependencies, StringBuf* outFullPath, Buffer& outCode);

    //--

    ShaderDataPtr compileInternal(StringView filePath, StringView code, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies);

    //--

    friend class LocalIncludeHandler;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)
