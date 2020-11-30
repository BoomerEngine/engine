/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
*/

#include "build.h"
#include "renderingShaderCompiler.h"

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/parser/include/textToken.h"
#include "base/parser/include/textFilePreprocessor.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource/include/resourceCooker.h"

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingShaderFile.h"

namespace rendering
{
    namespace compiler
    {
    
        //--

        ShaderCompilerDefaultErrorReporter::ShaderCompilerDefaultErrorReporter(base::res::IResourceCookerInterface& cooker)
            : m_cooker(cooker)
            , m_numErrors(0)
            , m_numWarnings(0)
        {}

        void ShaderCompilerDefaultErrorReporter::translateContextPath(const base::parser::Location& loc, base::parser::Location& outAbsoluteLocation)
        {
            base::StringBuf absoluteFile;
            if (m_cooker.queryContextName(loc.contextName(), absoluteFile))
                outAbsoluteLocation = base::parser::Location(absoluteFile, loc.line(), loc.charPos());
            else
                outAbsoluteLocation = loc;
        }

        void ShaderCompilerDefaultErrorReporter::reportError(const base::parser::Location& loc, base::StringView message)
        {
            base::parser::Location absoluteLoc;
            translateContextPath(loc, absoluteLoc);

            m_numErrors += 1;

            //TRACE_ERROR("{}: error: {}", absoluteLoc, message);
            base::logging::Log::Print(base::logging::OutputLevel::Error, absoluteLoc.contextName().c_str(), absoluteLoc.line(), "shader", base::TempString("{}", message));
        }

        void ShaderCompilerDefaultErrorReporter::reportWarning(const base::parser::Location& loc, base::StringView message)
        {

            base::parser::Location absoluteLoc;
            translateContextPath(loc, absoluteLoc);

            m_numWarnings += 1;

            //TRACE_WARNING("{}: warning: {}", absoluteLoc, message);
            base::logging::Log::Print(base::logging::OutputLevel::Warning, absoluteLoc.contextName().c_str(), absoluteLoc.line(), "shader", base::TempString("{}", message));
        }

        //--

        ShaderCompilerDefaultIncludeHandler::ShaderCompilerDefaultIncludeHandler(base::res::IResourceCookerInterface& cooker)
            : m_cooker(cooker)
        {
            m_includePaths.pushBack("/engine/shaders/");
        }

        void ShaderCompilerDefaultIncludeHandler::addGlobalIncludePath(const base::StringBuf& path)
        {
            if (path.empty())
                return;

            base::StringBuf safePath;
            if (path.endsWith("/"))
                safePath = path;
            else
                safePath = base::TempString("{}/", path);

            safePath = safePath.toLower();

            m_includePaths.pushBackUnique(safePath);
        }

        bool ShaderCompilerDefaultIncludeHandler::checkFileExists(const base::StringBuf& path) const
        {
            return m_cooker.queryFileExists(path);
        }

        bool ShaderCompilerDefaultIncludeHandler::resolveIncludeFile(bool global, base::StringView path, base::StringView referencePath, base::StringBuf& outPath) const
        {
            if (global)
            {
                for (auto& includeBase : m_includePaths)
                {
                    if (m_cooker.queryResolvedPath(path, includeBase, true, outPath))
                        if (checkFileExists(outPath))
                            return true;
                }
            }
            else
            {
                if (m_cooker.queryResolvedPath(path, referencePath, true, outPath))
                    if (checkFileExists(outPath))
                        return true;
            }            

            return false;
        }

        bool ShaderCompilerDefaultIncludeHandler::loadInclude(bool global, base::StringView path, base::StringView referencePath, base::Buffer& outContent, base::StringBuf& outPath)
        {
            if (!resolveIncludeFile(global, path, referencePath, outPath))
                return false;

            outContent = m_cooker.loadToBuffer(outPath);
            m_retainedBuffers.pushBack(outContent);
            return outContent != nullptr;
        }

        //--

        // a resource cooker to compiler CSL files into a shader libraries
        class ShaderLibraryCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ShaderLibraryCooker, base::res::IResourceCooker);

        public:
            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override
            {
                auto mainFilePath = cooker.queryResourcePath();

                // load the root file content
                auto code = cooker.loadToBuffer(mainFilePath);
                if (code.empty())
                {
                    TRACE_ERROR("Unable to compile shaders '{}' because the main shader file can't be loaded", mainFilePath);
                    return nullptr;
                }

                // forward include requests and errors to cooker
                ShaderCompilerDefaultIncludeHandler includeHandler(cooker);
                ShaderCompilerDefaultErrorReporter errorHandler(cooker);

                // compile the loaded code and return a compiled shader object
                auto data = CompileShader(code, mainFilePath, mainFilePath, &includeHandler, errorHandler);
				if (!data)
				{
					TRACE_ERROR("No shader data were produced from '{}'", mainFilePath);
					return nullptr;
				}

				// store in a resource-like wrapper
				return base::RefNew<ShaderFile>(data);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(ShaderLibraryCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<ShaderFile>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("csl").addSourceExtension("fx");
        RTTI_END_TYPE();

        //--

    } // pipeline
} // rendering

