/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "replicationFieldPacking.h"

BEGIN_BOOMER_NAMESPACE_EX(replication)

//--

/// model type
enum class DataModelType : uint8_t
{
    Object, // object replication, will support a little bit more
    Struct, // structure replication, mostly messages
    Function, // function call replication, some restriction
};

//--

/// type of the field
enum class DataModelFieldType : uint8_t
{
    Packed, // use packing data
    Struct, // save as struct (uses the struct packing)
    StringID, // save as indexed string
    StringBuf, // save as pure string
    TypeRef, // save as type ref
    ObjectPtr, // save as object pointer
    WeakObjectPtr, // save as object pointer
    ResourceRef, // save as resource ref
};

/// a replicated property info
struct DataModelField
{
    DataModelFieldType m_type = DataModelFieldType::Packed;
    bool m_isArray = false;
    const DataModel* m_structModel = nullptr;
    FieldPacking m_packing;

    StringID m_nativeName; // for debug only
    Type m_nativeType = nullptr;
    uint32_t m_nativeOffset = 0;
    uint32_t m_nativeSize = 0; // cached

    //--

    void print(IFormatStream& f) const;
};

//--

/// data mapper for data encoding, allows to cache "knowledge" and map large data to simple IDs
class CORE_REPLICATION_API IDataModelMapper : public NoCopy
{
public:
    virtual ~IDataModelMapper();

    /// map a string to a unique number (type ref as well)
    virtual DataMappedID mapString(StringView txt) = 0;

    /// map path (a/b/c/d) to a unique number (type ref as well)
    virtual DataMappedID mapPath(StringView path, const char* pathSeparators) = 0;

    /// map reference to object
    virtual DataMappedID mapObject(const IObject* obj) = 0;

    //--

    // map type name, uses the path mapper with ":" or "." separator
    DataMappedID mapTypeRef(Type type);

    // map resource path, uses the path mapper with "/" or "\" separator
    DataMappedID mapResourcePath(const StringBuf& path);
};

//--

/// resolver of mapped IDs to data
class CORE_REPLICATION_API IDataModelResolver : public NoCopy
{
public:
    virtual ~IDataModelResolver();

    /// resolve ID to a string
    virtual bool resolveString(DataMappedID id, IFormatStream& f) = 0;

    /// resolve ID to path, path separator must be specified manually as it's not stored
    virtual bool resolvePath(DataMappedID id, const char* pathSeparator, IFormatStream& f) = 0;

    /// resolve object ID to object instance, NOTE: may fail
    virtual bool resolveObject(DataMappedID id, ObjectPtr& outObject) = 0;

    //--

    // resolve into string ID
    bool resolveStringID(DataMappedID id, StringID& outStringID);

    // map type name, uses the path mapper with ":" or "." separator
    // NOTE: this fails auto matically  if type is not found
    bool resolveTypeRef(DataMappedID id, bool isScripted, Type& outType);

    // map resource path, uses the path mapper with "/" or "\" separator
    // NOTE: this fails auto matically  if path is not valid path
    bool resolveResourcePath(DataMappedID id, StringBuf& outPath);
};

//--

/// a data model for a structure
class CORE_REPLICATION_API DataModel : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_NET_MODEL)

public:
    DataModel(StringID name, DataModelType modelType);

    /// get the type of this model (class/struct/function, etc)
    INLINE DataModelType type() const { return m_type; }

    /// get name of the model (struct name/function name)
    INLINE StringID name() const { return m_name; }

    /// get the model checksum
    INLINE uint32_t checksum() const { return m_checksum; }

    /// get field informations
    INLINE const Array<DataModelField>& fields() const { return m_fields; }

    //--

    // initialize from a native type
    // NOTE: invalid/incompatible properties are skipped
    void buildFromType(ClassType structType, DataModelRepository& repository);

    //--

    // encode data with this model using native data layout
    void encodeFromNativeData(const void* data, IDataModelMapper& mapper, BitWriter& w) const;

    //--

    /// decode data with this model using native data layout
    /// NOTE: memory for data MUST BE PREALLOCATED!
    /// NOTE: this function may return false if there are errors in the bit stream, the goal is TO NEVER CRASH
    bool decodeToNativeData(void* data, IDataModelResolver& resolver, BitReader& r) const;

    //--

    // debug dump
    void print(IFormatStream& f) const;

private:
    Array<DataModelField> m_fields;

    uint32_t m_checksum = 0;
    DataModelType m_type;
    StringID m_name;

    //--

    void encodeArrayFieldData(const DataModelField& field, const void* fieldData, IDataModelMapper& mapper, BitWriter& w) const;
    void encodeFieldData(const DataModelField& field, const void* fieldData, IDataModelMapper& mapper, BitWriter& w) const;

    bool decodeArrayFieldData(const DataModelField& field, void* fieldData, IDataModelResolver& mapper, BitReader& r) const;
    bool decodeFieldData(const DataModelField& field, void* fieldData, IDataModelResolver& mapper, BitReader& r) const;

    bool decodingError(const DataModelField& field, StringView message) const;
};

END_BOOMER_NAMESPACE_EX(replication)
