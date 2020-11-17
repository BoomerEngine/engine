/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationDataModel.h"
#include "replicationDataModelRepository.h"

namespace base
{
    namespace replication
    {
        //--

        DataModelRepository::DataModelRepository()
        {}

        DataModelRepository::~DataModelRepository()
        {}

        static bool IsObjectClass(Type type)
        {
            return type.toSpecificClass<IObject>();
        }

        const DataModel* DataModelRepository::buildModelForType(Type type)
        {
            // we can only build replication model for structures/classes
            // TODO: CONSIDER having a single type replication as well
            if (!type || type->metaType() != rtti::MetaType::Class)
                return nullptr;

            auto lock = CreateLock(m_lock);

            // get existing model
            DataModel* model = nullptr;
            if (m_compoundModels.find(type->name(), model))
                return model;

            // create new model
            auto dataModelType = IsObjectClass(type) ? DataModelType::Object : DataModelType::Struct;
            model = new DataModel(type->name(), dataModelType);
            m_compoundModels[type->name()] = model; // NOTE: we self-register because we may be recursive and even get to ourselves e.g. struct T { array<T> }

            // initialize model from type, we skip invalid properties
            // NOTE: we don't fail here, in case of errors we may just end up with an empty model
            model->buildFromType(type.toClass(), *this);

            // done, DUMP IT
            TRACE_INFO("Generated data model: {}", IndirectPrint(model));
            return model;
        }

        const DataModel* DataModelRepository::buildModelForFunction(const rtti::Function* func)
        {
            auto lock = CreateLock(m_lock);

            // get existing model
            DataModel* model = nullptr;
            if (m_functionModels.find(func->fullName(), model))
                return model;

            // create new model
            model = new DataModel(StringID(func->fullName().c_str()), DataModelType::Function);
            m_functionModels[func->fullName()] = model;

            // initialize model from type, we skip invalid properties
            // NOTE: we don't fail here, in case of errors we may just end up with an empty model
            model->buildFromFunction(func, *this);

            // done, DUMP IT
            TRACE_INFO("Generated function model: {}", IndirectPrint(model));
            return model;
        }

        //--

    } // replication
} // base