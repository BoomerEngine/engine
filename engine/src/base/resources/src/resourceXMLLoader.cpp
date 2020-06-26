/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml #]
***/

#include "build.h"
#include "resourceXMLLoader.h"
#include "resourceXMLRuntimeLoader.h"
#include "resource.h"

#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/serializationLoader.h"
#include "base/system/include/algorithms.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioMemoryPool.h"
#include "base/object/include/memoryReader.h"
#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/containers/include/hashSet.h"

namespace base
{
    namespace res
    {
        namespace xml
        {

            XMLLoader::XMLLoader()
            {}

            XMLLoader::~XMLLoader()
            {}

            static mem::PoolID POOL_XML("Engine.XML");

            bool XMLLoader::extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies)
            {
                // TODO!
                return false;
            }

            bool XMLLoader::loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result)
            {
                PC_SCOPE_LVL1(LoadObjectsXML);

                // TODO: UTF8 support

                // load the header to determine if this is indeed XML
                char header[6];
                memzero(&header, sizeof(header));
                file.read(&header, sizeof(header));

                // not an xml :)
                if (0 != strncmp(header, "<?xml ", ARRAY_COUNT(header)))
                {
                    TRACE_ERROR("File does not contain XML header");
                    return false;
                }

                // create the storage
                auto dataSize = file.size() + 10;
                auto buffer = Buffer::Create(POOL_XML, file.size() + 10);
                if (!buffer)
                {
                    TRACE_ERROR("Failed to allocate memory for loaded data");
                    return false;
                }

                auto bufferPtr = buffer.data();

                // load data
                memcpy(bufferPtr + 0, header, sizeof(header));
                file.read(bufferPtr + sizeof(header), dataSize - sizeof(header));
                if (file.isError())
                {
                    TRACE_ERROR("Reading XML content form file failed");
                    return false;
                }

                // zero terminate the buffer
                bufferPtr[dataSize] = 0;

                // load the content
                auto loader  = prv::LoaderState::LoadContent(buffer, context);
                if (!loader)
                {
                    TRACE_ERROR("Internal error loading XML file");
                    return false;
                }

                // post process loaded objects
                loader->postLoad();

                // extract result
                loader->extractResults(result);
                return true;
            }

            static void ExtractPathNodes(const base::xml::IDocument& doc, const base::xml::NodeID node, HashSet<res::ResourceKey>& outDependencyPaths)
            {
                auto nodeName  = doc.nodeName(node);
                if (nodeName == "ref")
                {
                    auto async  = doc.nodeAttributeOfDefault(node, "async");
                    if (async != "true")
                    {
                        auto path  = doc.nodeAttributeOfDefault(node, "path");
                        if (!path.empty() && path != "<none>")
                        {
                            // get the resource class from the entry
                            SpecificClassType<IResource> resourceClass = nullptr;
                            auto className  = doc.nodeAttributeOfDefault(node, "class");
                            if (!className.empty())
                                resourceClass = RTTI::GetInstance().findClass(StringID(className)).cast<IResource>();

                            // if not found, try to resolve by extension
                            if (!resourceClass)
                            {
                                auto fileExtension  = path.afterFirst(".");
                                resourceClass = res::IResource::FindResourceClassByExtension(fileExtension);
                            }

                            // report dependency
                            if (resourceClass)
                            {
                                res::ResourceKey key(ResourcePath(StringBuf(path)), resourceClass);
                                outDependencyPaths.insert(key);
                            }
                        }
                    }
                }
                else
                {
                    auto childNode  = doc.nodeFirstChild(node);
                    while (childNode)
                    {
                        ExtractPathNodes(doc, childNode, outDependencyPaths);
                        childNode = doc.nodeSibling(childNode);
                    }
                }
            }

        } // xml
    } // res
} // base
