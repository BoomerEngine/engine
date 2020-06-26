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
        class RENDERING_COMPILER_API ShaderLibraryCookerErrorReporter : public base::parser::IErrorReporter
        {
        public:
            ShaderLibraryCookerErrorReporter(base::res::IResourceCookerInterface& cooker);

            INLINE uint32_t errors() const { return m_numErrors; }
            INLINE uint32_t warnings() const { return m_numWarnings; }

            void translateContextPath(const base::parser::Location& loc, base::parser::Location& outAbsoluteLocation);

            virtual void reportError(const base::parser::Location& loc, base::StringView<char> message) override;
            virtual void reportWarning(const base::parser::Location& loc, base::StringView<char> message) override;

        private:
            base::res::IResourceCookerInterface& m_cooker;
            uint32_t m_numErrors;
            uint32_t m_numWarnings;
        };

        //--

        // include handler that loads appropriate dependencies and uses cooker to load files
        class RENDERING_COMPILER_API ShaderLibraryCookerIncludeHandler : public base::parser::IIncludeHandler
        {
        public:
            ShaderLibraryCookerIncludeHandler(base::res::IResourceCookerInterface& cooker);

            void addGlobalIncludePath(const base::StringBuf& path);

            bool checkFileExists(const base::StringBuf& path) const;
            bool resolveIncludeFile(bool global, base::StringView<char> path, base::StringView<char> referencePath, base::StringBuf& outPath) const;

            virtual bool loadInclude(bool global, base::StringView<char> path, base::StringView<char> referencePath, base::Buffer& outContent, base::StringBuf& outPath) override;

        private:
            base::res::IResourceCookerInterface& m_cooker;

            base::Array<base::StringBuf> m_includePaths;
            mutable base::Array<base::Buffer> m_retainedBuffers;
        };

        //--

        // compile a shader library file (and a bunch of shaders) from given token stream
        extern RENDERING_COMPILER_API ShaderLibraryPtr CompileShaderLibrary(
            base::StringView<char> code, // code to compile
            base::StringView<char> contextPath, // context path to print with errors 
            base::parser::IIncludeHandler* includeHandler, // handler for any files we want to include
            base::parser::IErrorReporter& err, // where to report any errors
            base::StringID nativeGeneratorName, // GLSL, HLSL, NULL etc
            base::HashMap<base::StringID, base::StringBuf>* defines = nullptr);  // defines and their values

        //--

    } // compiler
} // rendering
