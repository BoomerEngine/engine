/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml #]
***/

#include "build.h"
#include "xmlUtils.h"
#include "xmlDocument.h"
#include "xmlDynamicDocument.h"
#include "xmlStaticRapidXMLDocument.h"

namespace base
{
    namespace xml
    {

        //--

        class DefaultLoadingReporter : public ILoadingReporter
        {
        public:
            virtual void onError(uint32_t line, uint32_t pos, const char* text) override final
            {
                TRACE_ERROR("Parsing error at line {}, position {}: {}", line, pos, text);
            }

        };

        ILoadingReporter::~ILoadingReporter()
        {}

        ILoadingReporter& ILoadingReporter::GetDefault()
        {
            static DefaultLoadingReporter theDefaultReporter;
            return theDefaultReporter;
        }

        //--

        DocumentPtr LoadDocument(ILoadingReporter& ctx, StringView absoluteFilePath)
        {
            auto buffer  = io::LoadFileToBuffer(absoluteFilePath);
            return LoadDocument(ctx, buffer);
        }

        DocumentPtr LoadDocument(ILoadingReporter& ctx, const char* text)
        {
            if (!text || !*text)
                return nullptr;

            auto length  = strlen(text);
            if (length >= UINT32_MAX)
                return nullptr;

            Buffer buf;
            buf.init(POOL_TEMP, length+1, 1, text);
            buf.data()[length] = 0;

            return LoadDocument(ctx, buf);
        }

        DocumentPtr LoadDocument(ILoadingReporter& ctx, const Buffer& mem)
        {
            // to small
            if (!mem || mem.size() < 7)
                return nullptr;
            
            // Check the data header
            auto header  = (const char*)mem.data();
            if (0 == strncmp(header, "<?xml ", 6))
            { 
                return StaticRapidXMLDocument::Load(ctx, mem);
            }
            else if (0 == strncmp(header, "BINXML", 6))
            {
                //return StaticBinaryXMLDocument::Load(ctx, mem);
                return nullptr;
            }
            else
            {
                TRACE_ERROR("Unable to load XML data from buffer");
                return nullptr;
            }
        }

        bool SaveDocument(const IDocument& ptr, StringView absoluteFilePath, bool binaryFormat/*= false*/)
        {
            if (binaryFormat)
            {
                auto data  = ptr.saveAsBinary(ptr.root());
                return io::SaveFileFromBuffer(absoluteFilePath, data);
            }
            else
            {
                StringBuilder txt;
                ptr.saveAsText(txt, ptr.root());
                return io::SaveFileFromString(absoluteFilePath, txt.toString());
            }
        }

        DocumentPtr CreateDocument(StringView rootNodeName)
        {
            return CreateSharedPtr<DynamicDocument>(rootNodeName);
        }

        void CopyNodes(const IDocument& srcDoc, NodeID srcNodeId, IDocument& destDoc, NodeID destNodeParentId)
        {
            auto destNodeId  = destDoc.createNode(destNodeParentId, srcDoc.nodeName(srcNodeId));
            CopyInnerNodes(srcDoc, srcNodeId, destDoc, destNodeId);
        }

        void CopyInnerNodes(const IDocument& srcDoc, NodeID srcNodeId, IDocument& destDoc, NodeID destNodeId)
        {
            // copy value
            destDoc.nodeValue(destNodeId, srcDoc.nodeValue(srcNodeId));

            // create child nodes
            auto srcChildId  = srcDoc.nodeFirstChild(srcNodeId);
            while (srcChildId != 0)
            {
                CopyNodes(srcDoc, srcChildId, destDoc, destNodeId);
                srcChildId = srcDoc.nodeSibling(srcChildId);
            }

            // create attributes
            auto srcAttrId  = srcDoc.nodeFirstAttribute(srcNodeId);
            while (srcAttrId != 0)
            {
                destDoc.nodeAttribute(destNodeId,
                    srcDoc.attributeName(srcAttrId),
                    srcDoc.attributeValue(srcAttrId));

                srcAttrId = srcDoc.nextAttribute(srcAttrId);
            }
        }

        //--

    } // xml
} // base
