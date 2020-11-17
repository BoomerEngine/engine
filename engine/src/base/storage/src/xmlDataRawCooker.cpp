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
#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace storage
    {
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

        // a simple raw cooker for the XML files
        class XMLRawCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(XMLRawCooker, base::res::IResourceCooker);

            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override
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

        RTTI_BEGIN_TYPE_CLASS(XMLRawCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<XMLData>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("xml");
        RTTI_END_TYPE();

        ///--

    } // storage
} // base