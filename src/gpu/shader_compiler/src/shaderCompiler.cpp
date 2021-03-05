/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"

#include "program.h"
#include "function.h"
#include "staticExecution.h"
#include "programInstance.h"
#include "program.h"
#include "codeLibrary.h"
#include "functionFolder.h"
#include "fileParser.h"
#include "exporter.h"
#include "shaderCompiler.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/path.h"
#include "core/resource/include/resource.h"
#include "core/parser/include/textFilePreprocessor.h"
#include "core/parser/include/public.h"
#include "core/io/include/io.h"

#include "gpu/device/include/shaderData.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

// parsing error reporter for cooker based shader compilation
class LocalErrorReporter : public parser::IErrorReporter
{
public:
    virtual void reportError(const parser::Location& loc, StringView message) override
    {
        logging::Log::Print(logging::OutputLevel::Error, loc.contextName().c_str(), loc.line(), "", "", TempString("{}", message));
    }

    virtual void reportWarning(const parser::Location& loc, StringView message) override
    {
        logging::Log::Print(logging::OutputLevel::Warning, loc.contextName().c_str(), loc.line(), "", "", TempString("{}", message));
    }
};

//---

// include handler that loads appropriate dependencies
class LocalIncludeHandler : public parser::IIncludeHandler
{
public:
	LocalIncludeHandler(ShaderCompiler* compiler, Array<ShaderDependency>* outDependencies)
		: m_compiler(compiler)
		, m_dependencies(outDependencies)
	{}

	virtual bool loadInclude(bool global, StringView path, StringView referencePath, Buffer& outContent, StringBuf& outPath) override
	{
        StringBuf code;

		if (global)
            return m_compiler->loadSourceCode(path, m_dependencies, &outPath, outContent);

		StringBuf resolvedPath;
		if (!ApplyRelativePath(referencePath, path, resolvedPath))
			return false;

		const auto shortResolvedPath = resolvedPath.view().afterFirst("/shaders/");
		return m_compiler->loadSourceCode(shortResolvedPath, m_dependencies, &outPath, outContent);
	}

private:
	ShaderCompiler* m_compiler = nullptr;
	Array<ShaderDependency>* m_dependencies = nullptr;
};

//---

RTTI_BEGIN_TYPE_CLASS(ShaderCompiler);
RTTI_END_TYPE();

ShaderCompiler::ShaderCompiler()
{
    // determine where are the source files
	const auto engineRootDirectory = SystemPath(PathCategory::EngineDir);
	m_path = StringBuf(TempString("{}data/shaders/", engineRootDirectory));
	TRACE_INFO("Engine s directory: '{}'", m_path);
}

void StoreDependency(Array<ShaderDependency>* outDependencies, StringView path, TimeStamp timestamp)
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
			entry.path = StringBuf(path);
			entry.timestamp = timestamp;
		}
	}
}

bool ShaderCompiler::loadSourceCode(StringView path, Array<ShaderDependency>* outDependencies, StringBuf* outFullPath, Buffer& outCode)
{
	const auto fullPath = StringBuf(TempString("{}{}", m_path, path));

	// report full file path
	if (outFullPath)
		*outFullPath = fullPath;

	// get current timestamp of the file, will tell if we can use cached content
	TimeStamp timestamp;
	if (!FileTimeStamp(fullPath, timestamp))
	{
		TRACE_ERROR("Missing shader file '{}'", path);
		DEBUG_CHECK_EX(false, TempString("Missing shader file '{}'", path));
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
	auto content = LoadFileToBuffer(fullPath);
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

ShaderDataPtr ShaderCompiler::compileFile(StringView filePath, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies)
{
	StringBuf fullPath;

	Buffer code;
	if (!loadSourceCode(filePath, &outDependencies, &fullPath, code))
		return nullptr;

	StringView codeView(code);
	return compileInternal(fullPath, codeView, defines, outDependencies);
}

ShaderDataPtr ShaderCompiler::compileCode(StringView code, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies)
{
	static std::atomic<uint32_t> localTempFileCounter = 0;

	const auto tempFileIndex = localTempFileCounter++;
	const auto tempDir = SystemPath(PathCategory::LocalTempDir);
	const auto tempFilePath = StringBuf(TempString("{}.dynamics/shader_{}.txt", tempDir, tempFileIndex));

	SaveFileFromString(tempFilePath, code);

	auto ret = compileInternal("", code, defines, outDependencies);

	if (ret)
		DeleteFile(tempFilePath);

	return ret;
}

//--

Mutex GGlobalcompilationMutex;

ShaderDataPtr ShaderCompiler::compileInternal(StringView filePath, StringView code, HashMap<StringID, StringBuf>* defines, Array<ShaderDependency>& outDependencies)
{
	auto lock = CreateLock(GGlobalcompilationMutex);

    LinearAllocator mem(POOL_SHADER_COMPILATION);
    ScopeTimer timer;

	// create handlers
	LocalIncludeHandler includeHandler(this, &outDependencies);
	LocalErrorReporter errorReporter;

	{
		// setup preprocessor
		parser::TextFilePreprocessor parser(mem, includeHandler, errorReporter, parser::ICommentEater::StandardComments(), GetlanguageDefinition());

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
		if (!Assemblestubs(mem, codeLibrary, data, filePath, "", errorReporter))
			return false;

		// prints stats
		TRACE_INFO("Compiled shader file '{}' in {}, used {} ({} allocs). Generated {} of data",
			filePath, TimeInterval(timer.timeElapsed()), MemSize(mem.totalUsedMemory()), mem.numAllocations(), MemSize(data.blob.size()));

		// create the data object
		return RefNew<ShaderData>(data.blob, data.metadata);
	}
}
       
//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)
