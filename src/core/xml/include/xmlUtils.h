/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(xml)

// error reporting context
class CORE_XML_API ILoadingReporter
{
public:
    virtual ~ILoadingReporter();

    // report error in the loaded document
    virtual void onError(uint32_t line, uint32_t pos, const char* text) = 0;

    // get the default implementation of the reporter - outputs stuff to log
    static ILoadingReporter& GetDefault();
};

// load and parse XML document from file on disk (may be text or binary XML)
// NOTE: the loaded document is read only, youy will have to convert it to writable version to make changes
extern CORE_XML_API DocumentPtr LoadDocument(ILoadingReporter& ctx, StringView absoluteFilePath);

// parse XML document from text, the input buffer will be copied
// NOTE: the loaded document is read only, youy will have to convert it to writable version to make changes
extern CORE_XML_API DocumentPtr LoadDocument(ILoadingReporter& ctx, const char* text);

// load and parse XML document from memory buffer disk (may be text or binary XML)
// NOTE: the loaded document is read only, youy will have to convert it to writable version to make changes
extern CORE_XML_API DocumentPtr LoadDocument(ILoadingReporter& ctx, const Buffer& mem);

// save document to file on disk, document can be saved both in binary or text format
extern CORE_XML_API bool SaveDocument(const IDocument* ptr, StringView absoluteFilePath, bool binaryFormat = false);

// save document to file on disk without creating any in-memory representation, better for large XMLs
extern CORE_XML_API bool SaveDocument(const IDocument* ptr, IWriteFileHandle* file, bool binaryFormat = false);

// create an empty document that can be filled with data
extern CORE_XML_API DocumentPtr CreateDocument(StringView rootNodeName);

//---

// copy nodes from one document to other
// NOTE: the destination node must exist
extern CORE_XML_API void CopyInnerNodes(const IDocument& srcDoc, NodeID srcNodeId, IDocument& destDoc, NodeID destNodeId);

// copy nodes from one document to other
// NOTE: the destination node must exist
extern CORE_XML_API void CopyNodes(const IDocument& srcDoc, NodeID srcNodeId, IDocument& destDoc, NodeID destNodeParentId);

END_BOOMER_NAMESPACE_EX(xml)
