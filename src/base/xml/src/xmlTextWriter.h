/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\prv #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::xml)

/// helper class to save the XML documents
class TextWriter : public base::NoCopy
{
public:
    TextWriter(IFormatStream& builder);
    ~TextWriter();

    /// write given node, recursive
    void writeNode(const IDocument& doc, NodeID id, uint32_t depth = 0);

private:
    IFormatStream& m_builder;
};

END_BOOMER_NAMESPACE(base::xml)