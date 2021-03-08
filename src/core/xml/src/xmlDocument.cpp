/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml #]
***/

#include "build.h"
#include "xmlDocument.h"
#include "xmlTextWriter.h"
#include "xmlUtils.h"

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(xml)

//----

IDocument::IDocument()
{}

IDocument::~IDocument()
{}

//----

const char* IDocument::HEADER_TEXT()
{
    return "<?xml version=\"1.0\" standalone=\"yes\"?>\n";
}

//----

DocumentPtr IDocument::createWritableCopy(NodeID id)
{
    if (!id)
        return DocumentPtr();

    auto copyDoc  = CreateDocument(StringBuf("root"));
    if (!copyDoc)
        return nullptr;

    CopyInnerNodes(*this, id, *copyDoc, copyDoc->root());
    return copyDoc;
}

DocumentPtr IDocument::createReadOnlyCopy(NodeID id)
{
    return createWritableCopy(id); // TEMPSHIT
}

//----

Buffer IDocument::saveAsBinary(NodeID id) const
{
    return Buffer();
}

void IDocument::saveAsText(IFormatStream& f, NodeID id) const
{
    TextWriter writer(f);
    writer.writeNode(*this, id);
}

//----

END_BOOMER_NAMESPACE_EX(xml)
