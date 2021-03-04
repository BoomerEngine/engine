/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationFieldPacking.h"
#include "replicationDataModel.h"
#include "replicationDataModelRepository.h"
#include "replicationRttiExtensions.h"
#include "replicationBitWriter.h"
#include "replicationBitReader.h"

#include "core/containers/include/stringParser.h"
#include "core/object/include/rttiArrayType.h"
#include "core/object/include/rttiHandleType.h"
#include "core/object/include/rttiProperty.h"

BEGIN_BOOMER_NAMESPACE_EX(replication)

//--

RTTI_BEGIN_TYPE_ENUM(DataModelType);
    RTTI_ENUM_OPTION(Object);
    RTTI_ENUM_OPTION(Struct);
    RTTI_ENUM_OPTION(Function);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(DataModelFieldType);
    RTTI_ENUM_OPTION(Packed);
    RTTI_ENUM_OPTION(Struct);
    RTTI_ENUM_OPTION(StringID);
    RTTI_ENUM_OPTION(StringBuf);
    RTTI_ENUM_OPTION(TypeRef);
    RTTI_ENUM_OPTION(ObjectPtr);
    RTTI_ENUM_OPTION(WeakObjectPtr);
    RTTI_ENUM_OPTION(ResourceRef);
RTTI_END_TYPE();

//--

IDataModelMapper::~IDataModelMapper()
{}

DataMappedID IDataModelMapper::mapTypeRef(Type type)
{
    if (!type)
        return 0;

    // slight hack...
    if (type->scripted())
        return mapPath(type->name().view(), "."); // script uses the "." as type name separator
    else
        return mapPath(type->name().view(), "::"); // engine uses "::" as separator in type names
}

DataMappedID IDataModelMapper::mapResourcePath(const StringBuf& path)
{
    return mapPath(path.view(), "/");
}

//--

IDataModelResolver::~IDataModelResolver()
{}

bool IDataModelResolver::resolveStringID(DataMappedID id, StringID& outStringID)
{
    if (!id)
    {
        outStringID = StringID();
        return true;
    }

    BaseTempString<128> txt;
    if (!resolveString(id, txt))
        return false; // fails if the string with non zero ID was empty

    // TODO: validate string ID ?

    outStringID = StringID(txt);
    return true;
}

bool IDataModelResolver::resolveTypeRef(DataMappedID id, bool isScripted, Type& outType)
{
    if (!id)
    {
        outType = nullptr;
        return true;
    }

    BaseTempString<200> txt;
    if (!resolvePath(id, isScripted ? "." : "::", txt))
        return false; // fails if the string with non zero ID was empty

    auto typeRef  = RTTI::GetInstance().findType(StringID::Find(txt));
    if (!typeRef)
        return false;

    // only allowed classes for now
    if (typeRef->metaType() != MetaType::Class)
        return false;

    outType = typeRef;
    return true;
}

bool IDataModelResolver::resolveResourcePath(DataMappedID id, StringBuf& outPath)
{
    if (!id)
    {
        outPath = StringBuf();
        return true;
    }

    BaseTempString<200> txt;
    if (!resolvePath(id, "/", txt))
        return false;

    auto path = StringBuf(txt.c_str());
    if (path.empty()) // building failed
        return false;

    outPath = path;
    return true;
}

//--

DataModel::DataModel(StringID name, DataModelType modelType)
    : m_name(name)
    , m_type(modelType)
{}

void DataModel::buildFromFunction(const Function* functionType, DataModelRepository& repository)
{
    // TODO
}

void DataModel::buildFromType(ClassType structType, DataModelRepository& repository)
{
    // build model
    for (auto prop  : structType->allProperties())
    {
        // skip properties that are not replicated
        auto replicationMetadata  = prop->findMetadata<SetupMetadata>();
        if (!replicationMetadata)
        {
            TRACE_INFO("Property '{}.{}' not replicated", prop->name(), prop->parent()->name());
            continue;
        }

        DataModelField field;
        field.m_nativeName = prop->name();
        field.m_nativeSize = prop->type()->size();
        field.m_nativeOffset = prop->offset();
        field.m_nativeType = prop->type();


        // get the element type
        Type dataType = prop->type();
        if (prop->type()->metaType() == MetaType::Array)
        {
            dataType = prop->type().innerType();
            field.m_isArray = true;
        }
        else
        {
            if (replicationMetadata->packing().m_maxCount > 0)
            {
                TRACE_ERROR("Property '{}.{}' has 'maxCount' set even it's not an array", prop->name(), prop->parent()->name());
                continue;
            }
        }

        // max length can only be set for strings
        if (dataType != GetTypeObject<StringBuf>() && replicationMetadata->packing().m_maxLength > 0)
        {
            TRACE_ERROR("Property '{}.{}' has 'maxLength' set even it's not a string", prop->name(), prop->parent()->name());
            continue;
        }

        // store rest of the crap
        if (dataType == GetTypeObject<StringBuf>())
        {
            field.m_type = DataModelFieldType::StringBuf;
            field.m_packing.m_maxLength = replicationMetadata->packing().m_maxLength;
        }
        else if (dataType == GetTypeObject<StringID>())
        {
            field.m_type = DataModelFieldType::StringID;
        }
        else if (dataType->metaType() == MetaType::ClassRef)
        {
            field.m_type = DataModelFieldType::TypeRef;
        }
        else if (dataType->metaType() == MetaType::ResourceRef)
        {
            field.m_type = DataModelFieldType::ResourceRef;
        }
        else if (dataType->metaType() == MetaType::StrongHandle)
        {
            field.m_type = DataModelFieldType::ObjectPtr;
        }
        else if (dataType->metaType() == MetaType::WeakHandle)
        {
            field.m_type = DataModelFieldType::WeakObjectPtr;
        }
        else if (dataType->metaType() == MetaType::Class && replicationMetadata->packing().m_mode == PackingMode::Default)
        {
            auto innerClassType  = dataType.toClass();
            field.m_type = DataModelFieldType::Struct;
            field.m_structModel = repository.buildModelForType(innerClassType);
            if (!field.m_structModel)
            {
                TRACE_ERROR("Property '{}.{}' uses compound type '{}' that is not packable", prop->name(), prop->parent()->name(), prop->type());
                continue;
            }
        }
        else
        {
            if (!replicationMetadata->packing().checkTypeCompatibility(dataType, field.m_isArray))
            {
                TRACE_ERROR("Property '{}.{}' packing mode '{}' is not compatible with it's type '{}'", prop->name(), prop->parent()->name(), replicationMetadata->packing(), prop->type());
                continue;
            }

            field.m_packing = replicationMetadata->packing();
            field.m_type = DataModelFieldType::Packed;
        }

        // arrays
        if (prop->type()->metaType() == MetaType::Array)
            field.m_packing.m_maxCount = replicationMetadata->packing().m_maxCount;

        // add field to model
        m_fields.pushBack(field);
    }
}

template< typename T >
const T& GetFieldValue(const DataModelField& field, const void* fieldData)
{
    ASSERT(sizeof(T) <= field.m_nativeSize);
    return *(const T*)fieldData;
}

void DataModel::encodeArrayFieldData(const DataModelField& field, const void* fieldData, IDataModelMapper& mapper, BitWriter& w) const
{
    w.reserve(32);

    ASSERT(field.m_isArray);

    auto arrayType  = static_cast<const IArrayType*>(field.m_nativeType.ptr());
    ASSERT(arrayType->metaType() == MetaType::Array);

    auto count  = arrayType->arraySize(fieldData);
    if (field.m_packing.m_maxCount && count > field.m_packing.m_maxCount) // limit the count to allowed range
        count = field.m_packing.m_maxCount;

    w.writeAdaptiveNumber(count);

    for (uint32_t i=0; i<count; ++i)
    {
        auto fieldElement  = arrayType->arrayElementData(fieldData, i);
        encodeFieldData(field, fieldElement, mapper, w);
    }
}

void DataModel::encodeFieldData(const DataModelField& field, const void* fieldData, IDataModelMapper& mapper, BitWriter& w) const
{
    w.reserve(32);

    switch (field.m_type)
    {
        case DataModelFieldType::Packed:
        {
            field.m_packing.packData(fieldData, field.m_nativeSize, w);
            break;
        }

        case DataModelFieldType::Struct:
        {
            ASSERT(field.m_structModel != nullptr);
            field.m_structModel->encodeFromNativeData(fieldData, mapper, w);
            break;
        }

        case DataModelFieldType::StringID:
        {
            auto &value = GetFieldValue<StringID>(field, fieldData);
            auto id  = mapper.mapString(value.view());
            w.writeAdaptiveNumber(id);
            break;
        }

        case DataModelFieldType::StringBuf:
        {
            auto &value = GetFieldValue<StringBuf>(field, fieldData);
            if (value.empty())
            {
                w.writeBit(0);
            }
            else
            {
                w.writeBit(1);

                auto length  = value.length();
                if (field.m_packing.m_maxLength && length > field.m_packing.m_maxLength)
                {
                    TRACE_WARNING("Truncating string {} -> {} at field in {}. This may be undesired.", length, field.m_packing.m_maxLength, field.m_structModel->name());
                    length = field.m_packing.m_maxLength;
                }

                w.writeAdaptiveNumber(length);

                w.align();
                w.writeBlock(value.c_str(), length);
            }

            break;
        }

        case DataModelFieldType::TypeRef:
        {
            auto &value = GetFieldValue<ClassType>(field, fieldData);
            auto id  = mapper.mapTypeRef(value.ptr());
            w.writeBit(value ? value->scripted() : false);
            w.writeAdaptiveNumber(id);
            break;
        }

        case DataModelFieldType::ObjectPtr:
        {
            auto &value = GetFieldValue<ObjectPtr>(field, fieldData);
            auto id  = mapper.mapObject(value.get());
            w.writeAdaptiveNumber(id);
            break;
        }

        case DataModelFieldType::WeakObjectPtr:
        {
            auto &value = GetFieldValue<ObjectWeakPtr>(field, fieldData);
            auto id  = mapper.mapObject(value.unsafe());
            w.writeAdaptiveNumber(id);
            break;
        }

        case DataModelFieldType::ResourceRef:
        {
            /*auto &value = GetFieldValue<res::BaseReference>(field, fieldData);

            if (value.empty())
            {*/
                w.writeBit(0);
            /*}
            else
            {
                w.writeBit(1);

                auto typeId = mapper.mapTypeRef(ClassType(value.key().cls()));
                w.writeAdaptiveNumber(typeId);
                auto pathId  = mapper.mapResourcePath(value.key().path());
                w.writeAdaptiveNumber(pathId);
            }*/
            break;
        }
    }
}

//--

template< typename T >
T& GetFieldValue(const DataModelField& field, void* fieldData)
{
    ASSERT(sizeof(T) <= field.m_nativeSize);
    return *(T*)fieldData;
}

bool DataModel::decodingError(const DataModelField& field, StringView message) const
{
    TRACE_SPAM("DataModel: Failed decoding of field '{}' in model '{}': {}", field.m_nativeName, m_name, message);
    return false;
}

bool DataModel::decodeArrayFieldData(const DataModelField& field, void* fieldData, IDataModelResolver& mapper, BitReader& r) const
{
    ASSERT(field.m_isArray);

    auto arrayType  = static_cast<const IArrayType*>(field.m_nativeType.ptr());
    ASSERT(arrayType->metaType() == MetaType::Array);

    auto currentSize  = arrayType->arraySize(fieldData);

    BitReader::WORD count = 0;
    if (!r.readAdaptiveNumber(count))
        return decodingError(field, "Unable to decode array count");

    if (field.m_packing.m_maxCount && count > field.m_packing.m_maxCount)
        return decodingError(field, TempString("Invalid array count {}, max count {}", count, field.m_packing.m_maxCount));

    for (uint32_t i=currentSize; i<count; ++i)
        if (!arrayType->createArrayElement(fieldData, i))
            return decodingError(field, TempString("Unable to create array element {}, count {}", i, count));

    for (uint32_t i=currentSize; i>count; --i)
        if (!arrayType->removeArrayElement(fieldData, i-1))
            return decodingError(field, TempString("Unable to delete array element {}, count {}", i, count));

    for (uint32_t i=0; i<count; ++i)
    {
        auto fieldElement  = arrayType->arrayElementData(fieldData, i);
        if (!decodeFieldData(field, fieldElement, mapper, r))
            return decodingError(field, TempString("Unable to decode array element {} of {}", i, count));
    }

    return true;
}

bool DataModel::decodeFieldData(const DataModelField& field, void* fieldData, IDataModelResolver& resolver, BitReader& r) const
{
    switch (field.m_type)
    {
        case DataModelFieldType::Packed:
        {
            if (!field.m_packing.unpackData(fieldData, field.m_nativeSize, r))
                return decodingError(field, "Packed data decoding error");
            return true;
        }

        case DataModelFieldType::Struct:
        {
            ASSERT(field.m_structModel != nullptr);
            if (!field.m_structModel->decodeToNativeData(fieldData, resolver, r))
                return decodingError(field, "Struct data decoding error");
            break;
        }

        case DataModelFieldType::StringID:
        {
            BitReader::WORD id;
            if (!r.readAdaptiveNumber(id))
                return decodingError(field, "Unable to load ID for packed string");

            auto &value = GetFieldValue<StringID>(field, fieldData);
            if (!resolver.resolveStringID(id, value))
                return decodingError(field, "StringID decoding error");

            break;
        }

        case DataModelFieldType::StringBuf:
        {
            auto &value = GetFieldValue<StringBuf>(field, fieldData);

            bool hasData = false;
            if (!r.readBit(hasData))
                return decodingError(field, "StringBuf flag decoding error");

            if (hasData)
            {
                BitReader::WORD length = 0;
                if (!r.readAdaptiveNumber(length))
                    return decodingError(field, "StringBuf length decoding error");

                if (field.m_packing.m_maxCount && length > field.m_packing.m_maxCount)
                    return decodingError(field, TempString("StringBuf length is larger than allowed {} > {}", length, field.m_packing.m_maxCount));

                r.align();

                auto str  = (char*) alloca(length + 1);
                if (!r.readBlock(str, length))
                    return decodingError(field, "StringBuf buffer decoding error");

                str[length] = 0;

                value = StringBuf(str);
            }
            else
            {
                value = StringBuf();
            }

            break;
        }

        case DataModelFieldType::TypeRef:
        {
            bool scripted = false;
            if (!r.readBit(scripted))
                return decodingError(field, "Scripted flag decoding error");

            BitReader::WORD id = 0;
            if (!r.readAdaptiveNumber(id))
                return decodingError(field, "Type id decoding error");

            Type rttiType = nullptr;
            if (!resolver.resolveTypeRef(id, scripted, rttiType))
                return decodingError(field, "Type ref decoding error");

            auto &value = GetFieldValue<ClassType>(field, fieldData);
            value = rttiType.toClass();
            break;
        }

        case DataModelFieldType::ObjectPtr:
        {
            BitReader::WORD id = 0;
            if (!r.readAdaptiveNumber(id))
                return decodingError(field, "Object id decoding error");

            ObjectPtr ptr;
            if (!resolver.resolveObject(id, ptr))
                return decodingError(field, "Object reference decoding error");

            auto &value = GetFieldValue<ObjectPtr>(field, fieldData);
            value = ptr;

            break;
        }

        case DataModelFieldType::WeakObjectPtr:
        {
            BitReader::WORD id = 0;
            if (!r.readAdaptiveNumber(id))
                return decodingError(field, "Object id decoding error");

            ObjectPtr ptr;
            if (!resolver.resolveObject(id, ptr))
                return decodingError(field, "Object reference decoding error");

            auto &value = GetFieldValue<ObjectWeakPtr>(field, fieldData);
            value = ptr;

            break;
        }

        case DataModelFieldType::ResourceRef:
        {
            //auto &value = GetFieldValue<res::BaseReference>(field, fieldData);

            bool hasData = false;
            if (!r.readBit(hasData))
                return decodingError(field, "ResourceRef flag decoding error");

            if (hasData)
            {
                BitReader::WORD typeId = 0;
                if (!r.readAdaptiveNumber(typeId))
                    return decodingError(field, "ResourceRef type id decoding error");

                BitReader::WORD pathId = 0;
                if (!r.readAdaptiveNumber(pathId))
                    return decodingError(field, "ResourceRef path id decoding error");

                Type rttiType = nullptr;
                if (!resolver.resolveTypeRef(typeId, false, rttiType))
                    return decodingError(field, "ResourceRef type decoding error");

                /*auto resourceClass = rttiType.toSpecificClass<res::IResource>();
                if (!resourceClass)
                    return decodingError(field, TempString("ResourceRef type '{}' is not a resource class", rttiType->name()));*/

                StringBuf path;
                if (!resolver.resolveResourcePath(pathId, path))
                    return decodingError(field, "ResourceRef path resolving error");

                // TODO:
                //value.set(path);
            }
            else
            {
                //value.reset();
            }

            break;
        }
    }

    return true;
}

//--

void DataModel::encodeFromNativeData(const void* data, IDataModelMapper& mapper, BitWriter& w) const
{
    // TODO: build a VM/JIT to do this

    for (auto& field : m_fields)
    {
        auto fieldData  = OffsetPtr(data, field.m_nativeOffset);
        if (field.m_isArray)
            encodeArrayFieldData(field, fieldData, mapper, w);
        else
            encodeFieldData(field, fieldData, mapper, w);
    }
}

void DataModel::encodeFromFunctionCall(const FunctionCallingParams& params, IDataModelMapper& mapper, BitWriter& w) const
{
    for (auto& field : m_fields)
    {
        auto fieldData  = params.m_argumentsPtr[field.m_nativeOffset];
        if (field.m_isArray)
            encodeArrayFieldData(field, fieldData, mapper, w);
        else
            encodeFieldData(field, fieldData, mapper, w);
    }
}

bool DataModel::decodeToNativeData(void* data, IDataModelResolver& resolve, BitReader& r) const
{
    for (auto& field : m_fields)
    {
        auto fieldData  = OffsetPtr(data, field.m_nativeOffset);
        if (field.m_isArray)
        {
            if (!decodeArrayFieldData(field, fieldData, resolve, r))
            {
                return false;
            }
        }
        else
        {
            if (!decodeFieldData(field, fieldData, resolve, r))
            {
                TRACE_SPAM("DataModel: Failed decoding of array field '{}' in model '{}'", field.m_nativeName, m_name);
                return false;
            }
        }
    }

    return true;
}

bool DataModel::decodeToFunctionCall(FunctionCallingParams& params, IDataModelResolver& resolve, BitReader& r) const
{
    for (auto& field : m_fields)
    {
        auto fieldData  = params.m_argumentsPtr[field.m_nativeOffset];
        if (field.m_isArray)
        {
            if (!decodeArrayFieldData(field, fieldData, resolve, r))
            {
                TRACE_SPAM("DataModel: Failed decoding of field '{}' in model '{}'", field.m_nativeName, m_name);
                return false;
            }
        }
        else
        {
            if (!decodeFieldData(field, fieldData, resolve, r))
            {
                TRACE_SPAM("DataModel: Failed decoding of array field '{}' in model '{}'", field.m_nativeName, m_name);
                return false;
            }
        }
    }

    return true;
}

//--

void DataModelField::print(IFormatStream& f) const
{
    f.appendf("{}", m_isArray ? "array " : "");
    f.appendf("{} ({})", m_type, m_nativeName);

    f.appendf(" {}", m_packing);

    if (m_type == DataModelFieldType::Struct)
        f.appendf(" (type: {})", m_structModel ? m_structModel->name() : StringID());

    if (m_nativeSize > 0)
        f.appendf(" @ {} (size: {})", m_nativeOffset, m_nativeSize);
}

void DataModel::print(IFormatStream& f) const
{
    f.appendf("Model for '{}' ({}), {} field(s)", m_name, m_type, m_fields.size());
    for (uint32_t i=0; i<m_fields.size(); ++i)
        f.append("\n").appendf("  [{}]: {}", i, m_fields[i]);
}

//--

END_BOOMER_NAMESPACE_EX(replication)
