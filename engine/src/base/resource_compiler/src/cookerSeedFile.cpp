/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooking #]
***/

#include "build.h"
#include "cookerSeedFile.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/xml/include/xmlWrappers.h"
#include "base/resource/include/resourcePath.h"

namespace base
{
    namespace res
    {
        
        ///--

        RTTI_BEGIN_TYPE_CLASS(SeedFile);
            RTTI_METADATA(ResourceExtensionMetadata).extension("v4seed");
            RTTI_METADATA(ResourceDescriptionMetadata).description("Seed File");
            RTTI_METADATA(ResourceTagColorMetadata).color(0x20, 0xb0, 0x50);
            RTTI_PROPERTY(m_files);
        RTTI_END_TYPE();

        SeedFile::SeedFile()
        {}

        SeedFile::SeedFile(TSeedFileList&& files)
            : m_files(std::move(files))
        {}

        ///--

        /// cooker for loading seed files from XML files
        class SeedFileXMLCooker : public IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SeedFileXMLCooker, IResourceCooker);

        public:
            virtual ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override
            {
                // load the XML with the file data
                auto xml = LoadXML(cooker);
                if (!xml)
                    return nullptr;

                // parse entries
                TSeedFileList files;
                for (xml::NodeIterator it(xml, "file"); it; ++it)
                {
                    // get relative resource path
                    const auto relativePath = it->attribute("path");
                    if (!relativePath)
                        continue;

                    // get class
                    SpecificClassType<IResource> resourceClass;
                    if (const auto className = it->attribute("class"))
                    {
                        resourceClass = RTTI::GetInstance().findClass(StringID::Find(className)).cast<IResource>();
                        if (!resourceClass)
                        {
                            TRACE_ERROR("Unrecognized class name '{}' for resource '{}'", className, relativePath);
                        }
                    }
                    else
                    {
                        const auto pathExtension = relativePath.afterLast(".");
                        resourceClass = IResource::FindResourceClassByExtension(pathExtension);
                        if (!resourceClass)
                        {
                            TRACE_ERROR("Unable to identify resource class for '{}'", relativePath);
                        }
                    }

                    // resolve to absolute path based on current resource location
                    base::StringBuf depotPath;
                    if (cooker.queryResolvedPath(relativePath, cooker.queryResourcePath().path(), true, depotPath))
                    {
                        files.emplaceBack(ResourceKey(ResourcePath(depotPath), resourceClass));
                    }
                    else
                    {
                        TRACE_ERROR("Unable to resolve relative path '{}'", relativePath);
                    }
                }
            }
        };

        ///--

    } // res
} // base


