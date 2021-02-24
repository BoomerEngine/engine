/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "rendering/device/include/renderingShaderService.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

class LocalIncludeHandler;

//--

// runtime shader compiler
class RENDERING_COMPILER_API ShaderRuntimeCompiler : public IShaderRuntimeCompiler
{
    RTTI_DECLARE_VIRTUAL_CLASS(ShaderRuntimeCompiler, IShaderRuntimeCompiler);

public:
    ShaderRuntimeCompiler();

    virtual ShaderDataPtr compileFile(base::StringView filePath, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies) override final;
    virtual ShaderDataPtr compileCode(base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies) override final;

private:
    base::StringBuf m_shaderPath; // /data/shaders

    //--

    struct CachedFileContent
    {
        base::io::TimeStamp timestamp;
        base::Buffer content;
    };

    base::Mutex m_sourceFileLock;
    base::HashMap<base::StringBuf, CachedFileContent> m_sourceFileMap;

    bool loadSourceCode(base::StringView path, base::Array<ShaderDependency>* outDependencies, base::StringBuf* outFullPath, base::Buffer& outCode);

    //--

    ShaderDataPtr compileInternal(base::StringView filePath, base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies);

    //--

    friend class LocalIncludeHandler;
};

//--

END_BOOMER_NAMESPACE(rendering::shadercompiler)
