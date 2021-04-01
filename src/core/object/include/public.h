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

/// resource - file based object
class IResource;

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

// async buffer
class IAsyncFileBufferLoader;
typedef RefPtr<IAsyncFileBufferLoader> AsyncFileBufferLoaderPtr;

//--


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
class IFunction;
class NativeFunction;
class MonoFunction;
class Property;
class CustomType;
class IClassType;
class NativeClass;
class IType;

//--

class SerializationReader;
class SerializationWriter;

//--

struct TypeSerializationContext;

//--

struct DataViewInfo;
struct DataViewMemberInfo;
struct DataViewOptionInfo;

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
    ErrorIncompatibleMultiView, // field with the same name in different views in a multi view is something completly different 
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
INLINE DataViewResult::DataViewResult(const DataViewResult & other) = default;
INLINE DataViewResult::DataViewResult(DataViewResult && other) = default;
INLINE DataViewResult::~DataViewResult() = default;
INLINE DataViewResult& DataViewResult::operator=(const DataViewResult & other) = default;
INLINE DataViewResult& DataViewResult::operator=(DataViewResult && other) = default;

//---

/// copy value from one property to other property, handles type conversions, inlined objects etc
extern CORE_OBJECT_API bool CopyPropertyValue(const IObject* srcObject, const Property* srcProperty, IObject* targetObject, const Property* targetProperty);

//---

// save object to standalone xml document, the root node will contain the "class" 
// NOTE: to save object into existing XML use the object->writeXML()
extern CORE_OBJECT_API xml::DocumentPtr SaveObjectToXML(const IObject* object, StringView rootNodeName = "object");

// save object to standalone xml file on disk, the root node will contain the "class" 
// NOTE: to save object into existing XML use the object->writeXML()
extern CORE_OBJECT_API bool SaveObjectToXMLFile(StringView path, const IObject* object, StringView rootNodeName = "object", bool binaryXMLFormat=false);
//--

// load object from an XML file (absolute path)
extern CORE_OBJECT_API ObjectPtr LoadObjectFromXMLFile(StringView absolutePath, SpecificClassType<IObject> expectedClass = nullptr);

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

// load object from standalone XML document file
template< typename T >
INLINE RefPtr<T> LoadObjectFromXMLFile(StringView absolutePath)
{
    return rtti_cast<T>(LoadObjectFromXMLFile(absolutePath, T::GetStaticClass()));
}

//---

static const uint32_t VER_INITIAL = 1;

static const uint32_t VER_ABSOLUTE_DEPOT_PATHS = 2;

static const uint32_t VER_THREAD_SAFE_GRAPHS = 3;

static const uint32_t VER_NEW_RESOURCE_ID = 4;

//--

static const uint32_t VER_CURRENT = VER_NEW_RESOURCE_ID;

END_BOOMER_NAMESPACE()

//---

