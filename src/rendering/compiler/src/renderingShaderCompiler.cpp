/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"

#include "renderingShaderProgram.h"
#include "renderingShaderFunction.h"
#include "renderingShaderStaticExecution.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderProgram.h"
#include "renderingShaderCodeLibrary.h"
#include "renderingShaderFunctionFolder.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderCompiler.h"
#include "renderingShaderExporter.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resource.h"
#include "base/parser/include/textFilePreprocessor.h"
#include "base/io/include/ioSystem.h"

#include "rendering/device/include/renderingShaderData.h"
#include "base/parser/include/public.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//---

// parsing error reporter for cooker based shader compilation
class LocalErrorReporter : public base::parser::IErrorReporter
{
public:
    virtual void reportError(const base::parser::Location& loc, base::StringView message) override
    {
        base::logging::Log::Print(base::logging::OutputLevel::Error, loc.contextName().c_str(), loc.line(), "", "", base::TempString("{}", message));
    }

    virtual void reportWarning(const base::parser::Location& loc, base::StringView message) override
    {
        base::logging::Log::Print(base::logging::OutputLevel::Warning, loc.contextName().c_str(), loc.line(), "", "", base::TempString("{}", message));
    }
};

//---

// include handler that loads appropriate dependencies
class LocalIncludeHandler : public base::parser::IIncludeHandler
{
public:
	LocalIncludeHandler(ShaderRuntimeCompiler* compiler, base::Array<ShaderDependency>* outDependencies)
		: m_compiler(compiler)
		, m_dependencies(outDependencies)
	{}

	virtual bool loadInclude(bool global, base::StringView path, base::StringView referencePath, base::Buffer& outContent, base::StringBuf& outPath) override
	{
        base::StringBuf code;

		if (global)
            return m_compiler->loadSourceCode(path, m_dependencies, &outPath, outContent);

		base::StringBuf resolvedPath;
		if (!base::ApplyRelativePath(referencePath, path, resolvedPath))
			return false;

		const auto shortResolvedPath = resolvedPath.view().afterFirst("/shaders/");
		return m_compiler->loadSourceCode(shortResolvedPath, m_dependencies, &outPath, outContent);
	}

private:
	ShaderRuntimeCompiler* m_compiler = nullptr;
	base::Array<ShaderDependency>* m_dependencies = nullptr;
};

//---

RTTI_BEGIN_TYPE_CLASS(ShaderRuntimeCompiler);
RTTI_END_TYPE();

ShaderRuntimeCompiler::ShaderRuntimeCompiler()
{
    // determine where are the source files
	const auto engineRootDirectory = base::io::SystemPath(base::io::PathCategory::EngineDir);
	m_shaderPath = base::StringBuf(base::TempString("{}data/shaders/", engineRootDirectory));
	TRACE_INFO("Engine shaders directory: '{}'", m_shaderPath);			
}

void StoreDependency(base::Array<ShaderDependency>* outDependencies, base::StringView path, base::io::TimeStamp timestamp)
{
	if (outDependencies)
	{
		bool exists = false;
		for (auto& dep : *outDependencies)
		{
			if (0 == dep.path.view().caseCmp(path))
			{
				dep.timestamp = timestamp;
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			auto& entry = outDependencies->emplaceBack();
			entry.path = base::StringBuf(path);
			entry.timestamp = timestamp;
		}
	}
}

bool ShaderRuntimeCompiler::loadSourceCode(base::StringView path, base::Array<ShaderDependency>* outDependencies, base::StringBuf* outFullPath, base::Buffer& outCode)
{
	const auto fullPath = base::StringBuf(base::TempString("{}{}", m_shaderPath, path));

	// report full file path
	if (outFullPath)
		*outFullPath = fullPath;

	// get current timestamp of the file, will tell if we can use cached content
	base::io::TimeStamp timestamp;
	if (!base::io::FileTimeStamp(fullPath, timestamp))
	{
		TRACE_ERROR("Missing shader file '{}'", path);
		return false;
	}

	// try to use existing content
	{
		auto lock = CreateLock(m_sourceFileLock);

		if (const auto* data = m_sourceFileMap.find(fullPath))
		{
			if (data->timestamp == timestamp)
			{
				StoreDependency(outDependencies, path, timestamp);
				outCode = data->content;
				return true;
			}
		}
	}

	// load the content to string
	auto content = base::io::LoadFileToBuffer(fullPath);
	if (!content)
	{
        TRACE_ERROR("Unable to load shader file '{}'", path);
        return false;
	}

	// store content in map
    {
		CachedFileContent entry;
		entry.timestamp = timestamp;
		entry.content = content;
 
		auto lock = CreateLock(m_sourceFileLock);
		m_sourceFileMap[fullPath] = entry;
    }

	// store dependency
	StoreDependency(outDependencies, path, timestamp);

	// return loaded code
	outCode = content;
	return true;
}

ShaderDataPtr ShaderRuntimeCompiler::compileFile(base::StringView filePath, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies)
{
	base::StringBuf fullPath;

	base::Buffer code;
	if (!loadSourceCode(filePath, &outDependencies, &fullPath, code))
		return nullptr;

	base::StringView codeView(code);
	return compileInternal(fullPath, codeView, defines, outDependencies);
}

ShaderDataPtr ShaderRuntimeCompiler::compileCode(base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies)
{
	static std::atomic<uint32_t> localTempFileCounter = 0;

	const auto tempFileIndex = localTempFileCounter++;
	const auto tempDir = base::io::SystemPath(base::io::PathCategory::LocalTempDir);
	const auto tempFilePath = base::StringBuf(base::TempString("{}.dynamicShaders/shader_{}.txt", tempDir, tempFileIndex));

	base::io::SaveFileFromString(tempFilePath, code);

	auto ret = compileInternal("", code, defines, outDependencies);

	if (ret)
		base::io::DeleteFile(tempFilePath);

	return ret;
}

//--

base::Mutex GGlobalShaderCompilationMutex;

ShaderDataPtr ShaderRuntimeCompiler::compileInternal(base::StringView filePath, base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines, base::Array<ShaderDependency>& outDependencies)
{
	auto lock = CreateLock(GGlobalShaderCompilationMutex);

    base::mem::LinearAllocator mem(POOL_SHADER_COMPILATION);
    base::ScopeTimer timer;

	// create handlers
	LocalIncludeHandler includeHandler(this, &outDependencies);
	LocalErrorReporter errorReporter;

	{
		// setup preprocessor
		base::parser::TextFilePreprocessor parser(mem, includeHandler, errorReporter, base::parser::ICommentEater::StandardComments(), parser::GetShaderLanguageDefinition());

		// inject given defines
		if (nullptr != defines)
		{
			for (uint32_t i = 0; i < defines->size(); ++i)
			{
				parser.defineSymbol(defines->keys()[i].view(), defines->values()[i]);
				TRACE_DEEP("Defined static shader permutation '{}' = '{}'", defines->keys()[i], defines->values()[i]);
			}
		}

		// process the code into tokens
		// NOTE: may fail if undefined symbols are encountered
		if (!parser.processContent(code, filePath))
			return nullptr;

		// parse and compile code
		TypeLibrary typeLibrary(mem);
		CodeLibrary codeLibrary(mem, typeLibrary);
		if (!codeLibrary.parseContent(parser.tokens(), errorReporter))
			return false;

		// assemble final data
		AssembledShader data;
		if (!AssembleShaderStubs(mem, codeLibrary, data, filePath, "", errorReporter))
			return false;

		// prints stats
		TRACE_INFO("Compiled shader file '{}' in {}, used {} ({} allocs). Generated {} of data",
			filePath, TimeInterval(timer.timeElapsed()), MemSize(mem.totalUsedMemory()), mem.numAllocations(), MemSize(data.blob.size()));

		// create the data object
		return base::RefNew<ShaderData>(data.blob, data.metadata);
	}
}
       
//--

END_BOOMER_NAMESPACE(rendering::shadercompiler)
