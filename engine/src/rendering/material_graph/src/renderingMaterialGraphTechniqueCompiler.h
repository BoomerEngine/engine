/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "base/depot/include/depotStructure.h"

namespace rendering
{

    //---

    /// compiler material technique
    struct MaterialCompiledTechnique;
    extern MaterialCompiledTechnique* CompileTechnique(const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, base::parser::IErrorReporter& err, base::parser::IIncludeHandler& includes);

    //---

    /// a local compiler used to compile a single technique of given material
    class MaterialTechniqueCompiler : public base::parser::IErrorReporter, base::parser::IIncludeHandler
    {
    public:
        MaterialTechniqueCompiler(base::depot::DepotStructure& depot, const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique);
        virtual ~MaterialTechniqueCompiler();

        //--

        struct UsedFile
        {
            base::StringBuf depotPath;
            uint64_t timestamp = 0;
            uint64_t crc = 0;
            base::Buffer content;
        };

        struct ReportedError
        {
            base::StringBuf filePath;
            uint32_t linePos = 0;
            uint32_t charPos = 0;
            base::StringBuf text;
        };

        INLINE const base::Array<UsedFile>& usedFiles() const { return m_usedFiles; }
        INLINE const base::Array<ReportedError>& errors() const { return m_errors; }

        //--

        // do the compilation, can take some time
        bool compile() CAN_YIELD; // NOTE: slow

    private:
        MaterialCompilationSetup m_setup;

        MaterialGraphContainerPtr m_graph;
        MaterialTechniquePtr m_technique;

        base::depot::DepotStructure& m_depot;
        base::StringBuf m_contextName;

        base::Array<UsedFile> m_usedFiles;
        base::Array<ReportedError> m_errors;

        bool m_firstErrorPrinted = false;

        //--

        // IErrorReporter
        virtual void reportError(const base::parser::Location& loc, base::StringView<char> message) override final;
        virtual void reportWarning(const base::parser::Location& loc, base::StringView<char> message) override final;

        // IIncludeHandler
        virtual bool loadInclude(bool global, base::StringView<char> path, base::StringView<char> referencePath, base::Buffer& outContent, base::StringBuf& outPath) override final;

        bool queryResolvedPath(base::StringView<char> relativePath, base::StringView<char> contextFileSystemPath, bool global, base::StringBuf& outResourcePath) const;
        bool checkFileExists(base::StringView<char> depotPath) const;

        base::Buffer loadFileContent(base::StringView<char> depotPath);
    };

    //---

} // rendering

