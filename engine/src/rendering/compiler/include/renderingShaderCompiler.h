/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

namespace rendering
{
    namespace compiler
    {
        //--

        // parsing error reporter for cooker based shader compilation
        class RENDERING_COMPILER_API ShaderCompilerDefaultErrorReporter : public base::parser::IErrorReporter
        {
        public:
			ShaderCompilerDefaultErrorReporter(base::res::IResourceCookerInterface& cooker);

            INLINE uint32_t errors() const { return m_numErrors; }
            INLINE uint32_t warnings() const { return m_numWarnings; }

            void translateContextPath(const base::parser::Location& loc, base::parser::Location& outAbsoluteLocation);

            virtual void reportError(const base::parser::Location& loc, base::StringView message) override;
            virtual void reportWarning(const base::parser::Location& loc, base::StringView message) override;

        private:
            base::res::IResourceCookerInterface& m_cooker;
            uint32_t m_numErrors;
            uint32_t m_numWarnings;
        };

        //--

        // include handler that loads appropriate dependencies and uses cooker to load files
        class RENDERING_COMPILER_API ShaderCompilerDefaultIncludeHandler : public base::parser::IIncludeHandler
        {
        public:
			ShaderCompilerDefaultIncludeHandler(base::res::IResourceCookerInterface& cooker);

            void addGlobalIncludePath(const base::StringBuf& path);

            bool checkFileExists(const base::StringBuf& path) const;
            bool resolveIncludeFile(bool global, base::StringView path, base::StringView referencePath, base::StringBuf& outPath) const;

            virtual bool loadInclude(bool global, base::StringView path, base::StringView referencePath, base::Buffer& outContent, base::StringBuf& outPath) override;

        private:
            base::res::IResourceCookerInterface& m_cooker;

            base::Array<base::StringBuf> m_includePaths;
            mutable base::Array<base::Buffer> m_retainedBuffers;
        };

        //--

        // compile a shader data from given token stream
		// NOTE: no permutation exploration is performed here, if a define is not defined it's an error
        extern RENDERING_COMPILER_API ShaderDataPtr CompileShader(
            base::StringView code, // code to compile
            base::StringView contextPath, // context path to print with errors (Z:/projects/awesome/shaders/box.fx)
			base::StringView sourceContentDebugPath, // debug name/path that describes what we are compiling (added to code dumps)
            base::parser::IIncludeHandler* includeHandler, // handler for any files we want to include (ShaderCompilerDefaultIncludeHandler in cooking)
            base::parser::IErrorReporter& err, // where to report any errors (ShaderCompilerDefaultErrorReporter in cooking)
            base::HashMap<base::StringID, base::StringBuf>* defines = nullptr);  // defines and their values

        //--

    } // compiler
} // rendering
