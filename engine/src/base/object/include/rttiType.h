/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "base/containers/include/stringID.h"
#include "rttiMetadata.h"

namespace base
{
    class Type;
    
    namespace rtti
    {
        class Property;
        class IMetadata;
        class IClassType;

        class ITextWriter;
        class ITextReader;

        /// type conversion "matrix" values
        enum class TypeConversionClass : char
        {
            TypeUnknown = 0,
            Typebool = 1,
            Typechar,
            Typeshort,
            Typeint,
            Typeint64_t,
            Typeuint8_t,
            Typeuint16_t,
            Typeuint32_t,
            Typeuint64_t,
            Typefloat,
            Typedouble,
            TypeStringBuf,
            TypeStringID,
            TypeStrongHandle,
            TypeWeakHandle,
            TypeEnum,
            TypeClassRef,
            TypeSyncRef,
            TypeAsyncRef,

            MAX,
        };

        ////-----

        /// General type serialization context
        struct TypeSerializationContext : public base::NoCopy
        {
            const IClassType* classContext = nullptr;
            const Property* propertyContext = nullptr; // property being deserialized/serialized
            IObject* objectContext = nullptr; // parent object, can be used to handle property data conversion/missing properties
        };

        ////-----

        /// flags for printToText/printFromText
        enum PrintToFlags : uint32_t
        {
            PrintToFlag_TextSerializaitonStructElement = FLAG(1),
            PrintToFlag_TextSerializaitonArrayElement = FLAG(2),
            PrintToFlag_Editor = FLAG(3),
        };

        ////-----

        /// Type traits
        struct TypeRuntimeTraits
        {
            TypeConversionClass convClass = TypeConversionClass::TypeUnknown; // runtime conversion class
            MetaType metaType = MetaType::Void; // general class of the type
            uint64_t nativeHash = 0; // typeid().hash() of the type
            uint32_t size = 0; // size of the type in memory
            uint32_t alignment = 1; // alignment required for the type
            mem::PoolID pool = POOL_DEFAULT; // pool to allocate memory from when creating an instance of this type

            bool scripted = false; // type was defined in scripts
            bool initializedFromZeroMem = false; // type does not require construct() if initialized from zero memory
            bool requiresConstructor = true; // call to construct() is not needed in all cases (no undefined state)
            bool requiresDestructor = true; // call to destroy() is not needed in all cases (no memory leaks)
            bool simpleCopyCompare = false; // use memcpy/memcmp instead of full copy() compare() calls
            bool hashable = false; // is this type hashable ? (can be used as Key in hashmap)

        };

        ////-----

        /// Base type in the RTTI system
        class BASE_OBJECT_API IType : public MetadataContainer
        {
        public:
            IType(StringID name = StringID::EMPTY());
            virtual ~IType();

            ///----

            /// DataInternface: Construct valid data in given memory
            /// Equivalent of constructor call
            /// NOTE: zero-filled memory is ALSO a good initial state, in this case the Construct() can be omitted
            virtual void construct(void *object) const = 0;

            /// DataInternface: Destroy data from given memory
            /// Equivalent of destructor call, usually not called if NeedsCleaning() returns false
            virtual void destruct(void *object) const = 0;

            /// DataInternface: Compare two values, returns true if values are equal
            virtual bool compare(const void* data1, const void* data2) const = 0;

            /// DataInternface: Copy value from one place in memory to other place
            /// Equivalent of assignment operator
            /// NOTE: the target memory MUST be properly initialized before we copy value into it
            virtual void copy(void* dest, const void* src) const = 0;

            //----
            // Binary serialization

            /// DataInternface: Write data to file
            virtual bool writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const = 0;

            /// DataInternface: Read data from file
            virtual bool readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const = 0;

            //----
            // Text serialization

            /// Serialize value of this type using the generic text writer
            /// By default the value is retrieved via ReadValue and saved as text
            virtual bool writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const = 0;

            /// Deserialize value of this type using the generic text reader
            /// By default the text value is read and parsed via the WriteValue
            virtual bool readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const = 0;

            //----
            // To/From text

            /// Print data of this type into a string buffer
            /// NOTE: we try to print in a "minimal parsable interpretation" so no unnecessary stuff that we can't parse back, especially no debug info
            /// NOTE: this is NOT a serialization replacement, so types like pointers to objects may print null/or other stuff if they feel like not printing the pointer itself
            /// NOTE: the partOfLargerStream flag is set if the output of the print will be part of a larger text stream, that basically means we need to behave better
            ///   e.g. a multi word string "Ala ma kota" may print as Ala ma kota (no quotes) when printed as a single value, but when in struct it should behave itself:
            //    (txt="Ala ma kota")(size=5), etc. 
            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const;

            /// TRY to parse value from a provided StringView, NOTE that we must parse EVERY character of that string view, ie. "5 cats" will not parse as integer "5".
            /// NOTE: in a similar fassion, if the partOfLargerStream is set we may expect to have for example escaped string, etc
            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags = 0) const;

            //----
            // Data view

            /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
            virtual bool describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const;

            /// Read data from memory
            virtual bool readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const;

            /// Write data to memory
            virtual bool writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const;

            //----
            // Runtime hashing

            // append data to CRC calculator
            virtual void calcCRC64(CRC64& crc, const void* data) const;

            //----
            // Type traits

            /// get type traits
            INLINE const TypeRuntimeTraits& traits() const { return m_traits; }

            /// get type traits
            INLINE TypeRuntimeTraits& traits() { return m_traits; }

            /// Get size in the memory, always well known
            INLINE uint32_t size() const { return m_traits.size; }

            /// Get required memory alignment in case the storage for typed data is allocated dynamically
            /// NOTE: this is basically _alignof + some hacks
            INLINE uint32_t alignment() const { return m_traits.alignment; }

            /// Get meta type (type of the type), this can be used to static_cast to derived classes
            INLINE MetaType metaType() const { return m_traits.metaType; }

            /// is this a scripted type ?
            INLINE bool scripted() const { return m_traits.scripted; }

            /// Get name of the RTTI type, internal, can be stored in files
            INLINE StringID name() const { return m_name; }

            //---
            // RTTI system native

            /// (shutdown only) release all internal references to other type
            virtual void releaseTypeReferences();

            /// (scripts only) cache type related data for faster performance
            virtual void cacheTypeData();

            //--

        protected:
            TypeRuntimeTraits m_traits;
            StringID m_name;
        };

        // type naming, slight hack to put it here but this allows way less includes cross-visibility
        // to be honest, those functions are needed ONLY in the reflection part because type knows how to format its name, but to move it there would require to have a copy paste somewhere..
        extern BASE_OBJECT_API StringID FormatStrongHandleTypeName(StringID className);
        extern BASE_OBJECT_API StringID FormatWeakHandleTypeName(StringID className);
        extern BASE_OBJECT_API StringID FormatDynamicArrayTypeName(StringID innerTypeName);
        extern BASE_OBJECT_API StringID FormatNativeArrayTypeName(StringID innerTypeName, uint32_t maxSize);
        extern BASE_OBJECT_API StringID FormatClassRefTypeName(StringID className);

        // helpers
        extern BASE_OBJECT_API const char* GetEnumValueName(const rtti::EnumType* enumType, int64_t enumValue);
        extern BASE_OBJECT_API bool GetEnumNameValue(const rtti::EnumType* enumType, StringID name, int64_t& outEnumValue);

    } // rtti

} // base