/***
* Boomer Engine v4
* Written by Lukasz "Krawiec" Krawczyk
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_replication_glue.inl"

namespace base
{
    namespace replication
    {

        class BitWriter;
        class BitReader;

        struct FieldPacking;
        class DataModel;

        class DataModelRepository;
        typedef RefPtr<DataModelRepository> DataModelRepositoryPtr;

        class IDataModelMapper;
        class IDataModelResolver;

        typedef uint16_t DataMappedID;
        typedef uint32_t QuantizedValue;

    } // replication
} // base

#include "replicationRttiExtensions.h"