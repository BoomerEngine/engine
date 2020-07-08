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
#include "base/object/include/serializationMapper.h"
#include "base/object/include/nullWriter.h"

namespace base
{
    namespace res
    {
        //----

        RTTI_BEGIN_TYPE_CLASS(ResourceDataVersionMetadata);
        RTTI_END_TYPE();

        ResourceDataVersionMetadata::ResourceDataVersionMetadata()
            : m_version(0)
        {}

        //----

        RTTI_BEGIN_TYPE_CLASS(ResourceExtensionMetadata);
        RTTI_END_TYPE();

        ResourceExtensionMetadata::ResourceExtensionMetadata()
            : m_ext(nullptr)
        {}

        //---

        RTTI_BEGIN_TYPE_CLASS(ResourceManifestExtensionMetadata);
        RTTI_END_TYPE();

        ResourceManifestExtensionMetadata::ResourceManifestExtensionMetadata()
            : m_ext(nullptr)
        {}

        //---

        RTTI_BEGIN_TYPE_CLASS(ResourceBakedOnlyMetadata);
        RTTI_END_TYPE();

        ResourceBakedOnlyMetadata::ResourceBakedOnlyMetadata()
        {}

        //---

        RTTI_BEGIN_TYPE_CLASS(ResourceDescriptionMetadata);
        RTTI_END_TYPE();

        ResourceDescriptionMetadata::ResourceDescriptionMetadata()
            : m_desc("")
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceTagColorMetadata);
        RTTI_END_TYPE();

        ResourceTagColorMetadata::ResourceTagColorMetadata()
            : m_color(0,0,0,0)
        {}        

        //--

        RTTI_BEGIN_TYPE_CLASS(RawTextData);
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Raw Text Data");
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4rawtext");
            RTTI_PROPERTY(m_data);
            RTTI_PROPERTY(m_crc);
        RTTI_END_TYPE();

        RawTextData::RawTextData()
        {}

        RawTextData::RawTextData(const StringBuf& data)
            : m_data(data)
        {
            m_crc = m_data.view().calcCRC64();
        }

        RawTextData::~RawTextData()
        {}
        
        //--

        RTTI_BEGIN_TYPE_CLASS(RawBinaryData);
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Raw Binary Data");
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4rawdata");
            RTTI_PROPERTY(m_data);
            RTTI_PROPERTY(m_crc);
        RTTI_END_TYPE();

        RawBinaryData::RawBinaryData()
        {}

        RawBinaryData::RawBinaryData(const Buffer& data)
            : m_data(data)
        {
            CRC64 crc;
            crc.append(data.data(), data.size());
            m_crc = crc.crc();
        }

        RawBinaryData::~RawBinaryData()
        {}

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

        void CookerErrorReporter::reportError(const parser::Location& loc, StringView<char> message)
        {
            parser::Location absoluteLoc;
            translateContextPath(loc, absoluteLoc);

            m_numErrors += 1;

            base::logging::Log::Print(base::logging::OutputLevel::Error, absoluteLoc.contextName().c_str(), absoluteLoc.line(), "", TempString("{}", message).c_str());
        }

        void CookerErrorReporter::reportWarning(const parser::Location& loc, StringView<char> message)
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

        bool CookerIncludeHandler::checkFileExists(StringView<char> path) const
        {
            uint64_t fileSize = 0;
            if (!m_cooker.queryFileInfo(path, nullptr, &fileSize, nullptr))
                return false;
            return 0 != fileSize;
        }

        bool CookerIncludeHandler::resolveIncludeFile(bool global, StringView<char> path, StringView<char> referencePath, StringBuf& outPath) const
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

        bool CookerIncludeHandler::loadInclude(bool global, StringView<char> path, StringView<char> referencePath, Buffer& outContent, StringBuf& outPath)
        {
            if (!resolveIncludeFile(global, path, referencePath, outPath))
                return false;

            outContent = m_cooker.loadToBuffer(outPath);
            m_retainedBuffers.pushBack(outContent);
            return outContent != nullptr;
        }

        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceCookedClassMetadata);
        RTTI_END_TYPE();

        ResourceCookedClassMetadata::ResourceCookedClassMetadata()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceSourceFormatMetadata);
        RTTI_END_TYPE();

        ResourceSourceFormatMetadata::ResourceSourceFormatMetadata()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceCookerBakingOnlyMetadata);
        RTTI_END_TYPE();
        
        ResourceCookerBakingOnlyMetadata::ResourceCookerBakingOnlyMetadata()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceCookerVersionMetadata);
        RTTI_END_TYPE();

        ResourceCookerVersionMetadata::ResourceCookerVersionMetadata()
            : m_version(0)
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceManifest);
        RTTI_METADATA(ResourceExtensionMetadata); // disable normal saving as resources
        RTTI_METADATA(ResourceCookerVersionMetadata).version(0);
        RTTI_END_TYPE();

        IResourceManifest::IResourceManifest()
        {}

        IResourceManifest::~IResourceManifest()
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceCooker);
        RTTI_METADATA(ResourceCookerVersionMetadata).version(0);
        RTTI_END_TYPE();

        IResourceCooker::IResourceCooker()
        {}

        IResourceCooker::~IResourceCooker()
        {}

        void IResourceCooker::reportManifestClasses(base::Array<base::SpecificClassType<IResourceManifest>>& outManifestClasses) const
        {
            // nothing
        }

        //--

        class RawTextDataImporter : public IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RawTextDataImporter, IResourceCooker);

        public:
            virtual res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
            {
                StringBuf data;
                if (!cooker.loadToString(cooker.queryResourcePath().path(), data))
                    return false;

                return base::CreateSharedPtr<RawTextData>(data);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(RawTextDataImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<RawTextData>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("*");
        RTTI_END_TYPE();

        //--

        class RawBinaryDataImporter : public IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RawBinaryDataImporter, IResourceCooker);

        public:
            virtual res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
            {
                Buffer data = cooker.loadToBuffer(cooker.queryResourcePath().path());
                if (!data)
                    return false;

                return base::CreateSharedPtr<RawBinaryData>(data);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(RawBinaryDataImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<RawBinaryData>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("*");
        RTTI_END_TYPE();

        //--

    } // res
} // base