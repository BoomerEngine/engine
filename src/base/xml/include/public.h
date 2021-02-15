/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_xml_glue.inl"

namespace base
{
    namespace xml
    {
        typedef size_t NodeID;
        typedef size_t AttributeID;

        class Node;

        class IDocument;
        typedef RefPtr<IDocument> DocumentPtr;

    } // xml
} // base

