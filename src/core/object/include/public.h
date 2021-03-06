/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue code
#include "core_object_glue.inl"

#define DECLARE_GLOBAL_EVENT(x, ...) \
    inline static const ::boomer::StringID x = #x##_id;

BEGIN_BOOMER_NAMESPACE()

//--

/// maximum number of template properties in any given class
static const uint32_t MAX_TEMPLATE_PROPERTIES = 128;

//--

DECLARE_GLOBAL_EVENT(EVENT_OBJECT_PROPERTY_CHANGED);
DECLARE_GLOBAL_EVENT(EVENT_OBJECT_STRUCTURE_CHANGED);

//--

/// stubs
struct IStub;
class IStubWriter;
class IStubReader;

typedef uint16_t StubTypeValue;

//---

/// object pointer types
class IObject;
typedef RefPtr<IObject> ObjectPtr;
typedef RefWeakPtr<IObject> ObjectWeakPtr;

/// observer of object's state
class IObjectObserver;

/// abstract data view
class IDataView;
typedef RefPtr<IDataView> DataViewPtr;

/// data view of an object
class DataViewNative;
typedef RefPtr<DataViewNative> DataViewNativePtr;

// actions
class IAction;
typedef RefPtr<IAction> ActionPtr;
class ActionHistory;
typedef RefPtr<ActionHistory> ActionHistoryPtr;

/// direct object template
class IObjectDirectTemplate;
typedef RefPtr<IObjectDirectTemplate> ObjectDirectTemplatePtr;
typedef RefWeakPtr<IObjectDirectTemplate> ObjectDirectTemplateWeakPtr;

// global events
class IGlobalEventListener;
typedef RefPtr<IGlobalEventListener> GlobalEventListenerPtr;

// selection
class Selectable;
struct EncodedSelectable;

END_BOOMER_NAMESPACE()

//--

BEGIN_BOOMER_NAMESPACE_EX(stream)

class OpcodeStream;
class OpcodeIterator;
class OpcodeReader;
class OpcodeWriter;

class LoadingResult;
struct LoadingDependency;

typedef uint16_t MappedNameIndex;
typedef uint16_t MappedTypeIndex;
typedef uint16_t MappedPropertyIndex;
typedef uint16_t MappedPathIndex;
typedef uint32_t MappedObjectIndex; // yup, it happened 64K+ objects in one files
typedef uint16_t MappedBufferIndex;
      

class CORE_OBJECT_API IDataBufferLatentLoader : public IReferencable
{
public:
    virtual ~IDataBufferLatentLoader() {};
    virtual uint32_t size() const = 0;
    virtual uint64_t crc() const = 0;
    virtual Buffer loadAsync() const = 0;
    virtual bool resident() const = 0;
};

typedef RefPtr< IDataBufferLatentLoader> DataBufferLatentLoaderPtr;

END_BOOMER_NAMESPACE_EX(stream)

BEGIN_BOOMER_NAMESPACE()

class IResource;
class ResourceLoader;

END_BOOMER_NAMESPACE()

BEGIN_BOOMER_NAMESPACE()

/// Meta type (type of type)
enum class MetaType
{
    Void,           // empty type (used to represent invalid type of no value)
    Simple,         // simple type (indivisible into smaller pieces)
    Enum,           // EnumType
    Bitfield,       // BitfieldType
    Class,          // ClassType
    Array,          // IArrayType - array type, may have different implementations
    StrongHandle,   // StringHandleType - handle to object that keeps it alive
    WeakHandle,     // WeakHandleType - handle to object that does not keep it alive
    ResourceRef,    // IReferenceType - resource reference
    ResourceAsyncRef,  // IAsyncReferenceType - async resource reference
    ClassRef,       // SpecificClassType - reference to class
};

// forward declarations of RTTI types
class IType;
class AccessPath;
class IClassType;
class EnumType;
class BitfieldType;
class IArrayType;
class IHandleType;
class TypeSystem;
class Function;
class Property;
class CustomType;
class IClassType;
class NativeClass;
class IType;

//--

struct TypeSerializationContext;

//--

struct DataViewInfo;
struct DataViewMemberInfo;
struct DataViewOptionInfo;

END_BOOMER_NAMESPACE()

BEGIN_BOOMER_NAMESPACE()

enum class DataViewResultCode : uint8_t
{
    OK,
    ErrorUnknownProperty, // property (or part of path) is unknown (ie. property does not exist)
    ErrorIllegalAccess, // you can't access that property like that (ie. hidden property)
    ErrorIllegalOperation, // operation is not permitted - ie. resizing static array
    ErrorTypeConversion, // property exists and is accessible but was not readable as the type that was requested
    ErrorManyValues, // property has many different values and a single value cannot be established
    ErrorReadOnly, // property is marked as read only, no writes are possible
    ErrorNullObject, // trying to access null object
    ErrorIndexOutOfRange, // trying to access array's index that is out of range
    ErrorInvalidValid, // value to write is invalid
};

struct CORE_OBJECT_API DataViewResult
{
    DataViewResultCode code = DataViewResultCode::ErrorIllegalOperation;
    StringBuf additionalInfo;

    INLINE DataViewResult();
    INLINE DataViewResult(const DataViewResult& other);
    INLINE DataViewResult(DataViewResult&& other);
    INLINE DataViewResult& operator=(const DataViewResult& other);
    INLINE DataViewResult& operator=(DataViewResult&& other);
    INLINE ~DataViewResult();

    INLINE DataViewResult(DataViewResultCode code_) : code(code_) {};

    INLINE bool valid() const { return code == DataViewResultCode::OK; }

    void print(IFormatStream& f) const;
};

END_BOOMER_NAMESPACE()

//---

#include "rttiTypeSystem.h"
#include "rttiTypeRef.h"
#include "rttiClassRefType.h"
#include "rttiClassRef.h"

#include "object.h"

#include "globalEventDispatch.h"
#include "globalEventTable.h"

//---

BEGIN_BOOMER_NAMESPACE()

INLINE DataViewResult::DataViewResult() = default;
INLINE DataViewResult::DataViewResult(const DataViewResult& other) = default;
INLINE DataViewResult::DataViewResult(DataViewResult&& other) = default;
INLINE DataViewResult::~DataViewResult() = default;
INLINE DataViewResult& DataViewResult::operator=(const DataViewResult& other) = default;
INLINE DataViewResult& DataViewResult::operator=(DataViewResult && other) = default;

struct CORE_OBJECT_API DataViewErrorResult
{
    DataViewResult result;

    INLINE DataViewErrorResult() {};
    INLINE DataViewErrorResult(const DataViewErrorResult& other) = default;
    INLINE DataViewErrorResult(DataViewErrorResult&& other) = default;
    INLINE DataViewErrorResult& operator=(const DataViewErrorResult& other) = default;
    INLINE DataViewErrorResult& operator=(DataViewErrorResult&& other) = default;

    INLINE DataViewErrorResult(DataViewResult&& other) : result(std::move(other)) {};
    INLINE DataViewErrorResult(const DataViewResult& other) : result(other) {};

    INLINE DataViewErrorResult& operator=(DataViewResult&& other) { result = std::move(other); return *this; }
    INLINE DataViewErrorResult& operator=(const DataViewResult& other) { result = other; return *this; }

    // NOTE: true only if we have error
    INLINE operator bool() const { return result.code != DataViewResultCode::OK; }

    INLINE operator DataViewResult() const { return result; }

    void print(IFormatStream& f) const;
};

//---

/// copy value from one property to other property, handles type conversions, inlined objects etc
extern CORE_OBJECT_API bool CopyPropertyValue(const IObject* srcObject, const Property* srcProperty, IObject* targetObject, const Property* targetProperty);

//---

// save object to standalone xml document, the root node will contain the "class" 
// NOTE: to save object into existing XML use the object->writeXML()
extern CORE_OBJECT_API xml::DocumentPtr SaveObjectToXML(const IObject* object, StringView rootNodeName = "object");

// load object from standalone XML document
extern CORE_OBJECT_API ObjectPtr LoadObjectFromXML(const xml::IDocument* doc, SpecificClassType<IObject> expectedClass = nullptr);

// load object from a node in XML document
extern CORE_OBJECT_API ObjectPtr LoadObjectFromXML(const xml::Node& node, SpecificClassType<IObject> expectedClass = nullptr);

// load object from standalone XML document
template< typename T >
INLINE RefPtr<T> LoadObjectFromXML(const xml::IDocument* doc)
{
    return rtti_cast<T>(LoadObjectFromXML(doc, T::GetStaticClass()));
}

//---

static INLINE DataViewErrorResult HasError(DataViewResult&& result)
{
    return DataViewErrorResult(std::move(result));
}

static INLINE DataViewErrorResult HasError(const DataViewResult& result)
{
    return DataViewErrorResult(std::move(result));
}

//---

static const uint32_t VER_INITIAL = 1;

static const uint32_t VER_ABSOLUTE_DEPOT_PATHS = 2;

static const uint32_t VER_THREAD_SAFE_GRAPHS = 3;

//--

static const uint32_t VER_CURRENT = VER_THREAD_SAFE_GRAPHS;

END_BOOMER_NAMESPACE()

//---

