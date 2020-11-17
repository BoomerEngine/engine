/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking #]
***/

#include "build.h"
#include "resource.h"
#include "resourceCookingInterface.h"
#include "base/parser/include/textToken.h"
#include "base/xml/include/xmlUtils.h"

namespace base
{
    namespace res
    {

        //--

        IResourceCookerInterface::~IResourceCookerInterface()
        {}

        //--

        CookerErrorReporter::CookerErrorReporter(IResourceCookerInterface& cooker)
            : m_cooker(cooker)
        {}

        void CookerErrorReporter::translateContextPath(const parser::Location& loc, parser::Location& outAbsoluteLocation)
        {
            StringBuf absoluteFile;
            if (m_cooker.queryContextName(loc.contextName(), absoluteFile))
                outAbsoluteLocation = parser::Location(absoluteFile, loc.line(), loc.charPos());
            else
                outAbsoluteLocation = loc;
        }

        void CookerErrorReporter::reportError(const parser::Location& loc, StringView message)
        {
            parser::Location absoluteLoc;
            translateContextPath(loc, absoluteLoc);

            m_numErrors += 1;

            base::logging::Log::Print(base::logging::OutputLevel::Error, absoluteLoc.contextName().c_str(), absoluteLoc.line(), "", TempString("{}", message).c_str());
        }

        void CookerErrorReporter::reportWarning(const parser::Location& loc, StringView message)
        {
            parser::Location absoluteLoc;
            translateContextPath(loc, absoluteLoc);

            m_numWarnings += 1;

            base::logging::Log::Print(base::logging::OutputLevel::Warning, absoluteLoc.contextName().c_str(), absoluteLoc.line(), "", TempString("{}", message).c_str());
        }

        //--

        CookerIncludeHandler::CookerIncludeHandler(IResourceCookerInterface& cooker)
            : m_cooker(cooker)
        {
        }

        bool CookerIncludeHandler::checkFileExists(StringView path) const
        {
            return m_cooker.queryFileExists(path);
        }

        bool CookerIncludeHandler::resolveIncludeFile(bool global, StringView path, StringView referencePath, StringBuf& outPath) const
        {
            if (m_cooker.queryResolvedPath(path, referencePath, !global, outPath))
                if (checkFileExists(outPath))
                    return true;

            for (auto& includeBase : m_includePaths)
            {
                if (m_cooker.queryResolvedPath(path, includeBase, !global, outPath))
                    if (checkFileExists(outPath))
                        return true;
            }

            return false;
        }

        bool CookerIncludeHandler::loadInclude(bool global, StringView path, StringView referencePath, Buffer& outContent, StringBuf& outPath)
        {
            if (!resolveIncludeFile(global, path, referencePath, outPath))
                return false;

            outContent = m_cooker.loadToBuffer(outPath);
            m_retainedBuffers.pushBack(outContent);
            return outContent != nullptr;
        }

        //--

        class LocalCookerLoadingReporter : public xml::ILoadingReporter
        {
        public:
            LocalCookerLoadingReporter(IResourceCookerInterface& cooker, const base::StringBuf& filePath)
                : m_path(filePath)
            {
            }

            INLINE bool hasErrors() const
            {
                return m_numErrors > 0;
            }

            virtual void onError(uint32_t line, uint32_t pos, const char* text) override
            {
                logging::Log().Print(logging::OutputLevel::Error, m_path.c_str(), line, "", text);
                m_numErrors += 1;
            }

        private:
            base::StringBuf m_path;
            uint32_t m_numErrors = 0;
        };

        xml::DocumentPtr LoadXML(IResourceCookerInterface& cooker, StringView path /*= ""*/)
        {
            // load the xml content
            const auto& xmlFilePath = cooker.queryResourcePath();
            auto rawContent = cooker.loadToBuffer(xmlFilePath);
            if (!rawContent)
                return nullptr;

            /// get the full context path
            StringBuf contextName;
            cooker.queryContextName(xmlFilePath, contextName);

            /// parse the document
            LocalCookerLoadingReporter errorReporter(cooker, contextName);
            auto ret = xml::LoadDocument(errorReporter, rawContent);

            // do not use the document if we had errors
            if (errorReporter.hasErrors())
                ret.reset();

            return ret;
        }

    } // res
} // base