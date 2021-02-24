/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: xml #]
***/

#include "build.h"
#include "xmlDataDocument.h"

#include "base/io/include/ioFileHandle.h"
#include "base/memory/include/buffer.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlUtils.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE(base::storage)

///--

// a simple error reporter for XML loading
class FileLoadingReporter : public xml::ILoadingReporter
{
public:
    FileLoadingReporter(const base::StringBuf& filePath)
        : m_path(filePath)
    {
    }

    virtual void onError(uint32_t line, uint32_t pos, const char *text) override
    {
        if (pos != 0)
        {
            TRACE_ERROR("{}({},{}): {}", m_path, line, pos, text);
        }
        else
        {
            TRACE_ERROR("{}({}): {}", m_path, line, text);
        }
    }

private:
    base::StringBuf m_path;
};

/*// a simple raw cooker for the XML files
class XMLRawCooker : public base::res::
{
    RTTI_DECLARE_VIRTUAL_CLASS(XMLRawCooker, base::res::);

    {
        // load the xml content
        const auto& xmlFilePath  = cooker.queryResourcePath();
        auto rawContent  = cooker.loadToBuffer(xmlFilePath);
        if (!rawContent)
            return nullptr;

        /// get the full context path
        StringBuf contextName;
        cooker.queryContextName(xmlFilePath, contextName);

        /// parse the document
        FileLoadingReporter errorReporter(contextName);
        auto document  = xml::LoadDocument(errorReporter, rawContent);
        if (!document)
        {
            TRACE_ERROR("Failed to load xml file from '{}'", cooker.queryResourcePath());
            return nullptr;
        }

        // pack data into the font
        return base::RefNew<XMLData>(document);
    }
};
*/

///--

END_BOOMER_NAMESPACE(base::storage)
