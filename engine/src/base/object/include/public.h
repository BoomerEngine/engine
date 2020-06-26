/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue code
#include "base_object_glue.inl"

namespace base
{
    //--

    /// object pointer types
    class IObject;
    typedef RefPtr<IObject> ObjectPtr;
    typedef RefWeakPtr<IObject> ObjectWeakPtr;

    /// observer of object's state
    class IObjectObserver;

    /// data view of object
    class IDataView;
    typedef RefPtr<IDataView> DataViewPtr;
    class DataProxy;
    typedef RefPtr<DataProxy> DataProxyPtr;

    // actions
    class IAction;
    typedef RefPtr<IAction> ActionPtr;
    class ActionHistory;
    typedef RefPtr<ActionHistory> ActionHistoryPtr;

    //--

    namespace stream
    {
        class IStream;

        class IBinaryReader;
        typedef RefPtr<IBinaryReader> ReaderPtr;

        class IBinaryWriter;
        typedef RefPtr<IBinaryWriter> WriterPtr;

        class ITextWriter;
        typedef RefPtr<ITextWriter> TextWritterPtr;

        class ITextReader;
        typedef RefPtr<ITextReader> TextReaderPtr;

        class LoadingResult;
        struct LoadingDependency;

        class ILoader;
        class ISaver;

        // mapping indices
        typedef uint16_t MappedNameIndex;
        typedef uint16_t MappedTypeIndex;
        typedef uint16_t MappedPropertyIndex;
        typedef uint16_t MappedPathIndex;
        typedef uint32_t MappedObjectIndex; // yup, it happened 64K+ objects in one files
        typedef uint16_t MappedBufferIndex;

        // Data mapping runtime -> persistent, interface used by binary serialization
        class IDataMapper;

        // Data mapping persistent -> runtime, interface used by binary serialization
        class IDataUnmapper;

        /// loading policy for loading references from the files
        enum class ResourceLoadingPolicy : uint8_t
        {
            AsIs, // load as it was saved
            NeverLoad,
            AlwaysLoad,
        };

        class BASE_OBJECT_API IDataBufferLatentLoader : public IReferencable
        {
        public:
            virtual ~IDataBufferLatentLoader() {};
            virtual uint32_t size() const = 0;
            virtual uint64_t crc() const = 0;
            virtual Buffer loadAsync() const = 0;
            virtual bool resident() const = 0;
        };

        typedef RefPtr< IDataBufferLatentLoader> DataBufferLatentLoaderPtr;

    } // stream

    //--

    namespace res
    {
        class IResource;
        class IResourceLoader;
    } // res

    //--

    namespace rtti
    {
        /// Meta type (type of type)
        enum class MetaType
        {
            Void,           // empty type (used to represent invalid type of no value)
            Simple,         // simple type (indivisible into smaller pieces)
            Enum,           // rtti::EnumType
            Bitfield,       // rtti::BitfieldType
            Class,          // rtti::ClassType
            Array,          // rtti::IArrayType - array type, may have different implementations
            StrongHandle,   // rtti::StringHandleType - handle to object that keeps it alive
            WeakHandle,     // rtti::WeakHandleType - handle to object that does not keep it alive
            ResourceRef,    // rtti::IReferenceType - resource reference
            AsyncResourceRef,  // rtti::IAsyncReferenceType - async resource reference
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
        class IType;

        //--

        struct TypeSerializationContext;

        //--

        struct DataViewInfo;
        struct DataViewMemberInfo;
        struct DataViewOptionInfo;

    } // rtti

    //--

} // base

//---

#include "rttiTypeSystem.h"
#include "rttiTypeRef.h"
#include "rttiClassRefType.h"

#include "object.h"
#include "objectObserver.h"

//---