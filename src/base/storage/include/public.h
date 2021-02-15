/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_storage_glue.inl"

namespace base
{
    namespace storage
    {

        class XMLData;
        typedef RefPtr<XMLData> XMLDataPtr;
        typedef res::Ref< XMLData> XMLDataRef;

        class TableData;
        typedef RefPtr<TableData> TableDataPtr;

        class Table;
        typedef RefPtr<Table> TablePtr;

        class TableBuilder;
        class TableEntry;

        typedef std::function<bool(const TableEntry & child)> TTableArrayIterator;
        typedef std::function<bool(const char * key, const TableEntry& child)> TTableIterator;

    } // storage
} // base

