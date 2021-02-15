/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#pragma once

#include "rttiType.h"
#include "rttiProperty.h"
#include "rttiFunctionPointer.h"
#include "base/containers/include/array.h"
#include "base/containers/include/stringID.h"
#include "base/containers/include/hashMap.h"

namespace base
{

    namespace reflection
    {
        class ClassBuilder;
    }

    namespace rtti
    {
        class Property;
        class Function;
        class Interface;

        /// property for templated object
        struct BASE_OBJECT_API TemplateProperty
        {
            Type type;
            StringID name;
            StringID category;
            void* defaultValue = nullptr;
            PropertyEditorData editorData;
            const Property* nativeProperty = nullptr;

            TemplateProperty(Type type);
            ~TemplateProperty();
        };

        /// type describing a class, can be a native or abstract class, depends
        class BASE_OBJECT_API IClassType : public IType
        {
        public:
            IClassType(StringID name, uint32_t size, uint32_t alignment, PoolTag pool);
            virtual ~IClassType();

            // can we create objects from this class ?
            virtual bool isAbstract() const = 0;

            // get default object for this class
            // NOTE: does not exist for abstract or runtime only classes
            virtual const void* defaultObject() const = 0;

            // create a default object
            virtual const void* createDefaultObject() const = 0;

            // destroy default object
            virtual void destroyDefaultObject() const = 0;

            // IRTTI TYPE INTERFACE IMPLEMENTATION
            virtual bool compare(const void* data1, const void* data2) const override;
            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override final;
            virtual bool parseFromString(StringView txt, void* data, uint32_t flags = 0) const override final;
            virtual void copy(void* dest, const void* src) const override;

            // TYPE SERIALIZATON
            virtual void writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const override final;
            virtual void readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const override final;
            virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
            virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

            // DATA VIEW
            virtual DataViewResult describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const override final;
            virtual DataViewResult readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const override final;
            virtual DataViewResult writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const override final;


            //--

            // get metadata by type, if the metadata is not found in this class the parent class is checked
            virtual const IMetadata* metadata(ClassType metadataType) const override;

            // collect metadata from this class and base classes
            virtual void collectMetadataList(Array<const IMetadata*>& outMetadataList) const override;

            // get local (this class only) properties
            typedef Array< const Property* > TConstProperties;
            INLINE const TConstProperties& localProperties() const { return (const TConstProperties&) m_localProperties; }

            // get local (this class only) functions
            typedef Array< const Function* > TConstFunctions;
            INLINE const TConstFunctions& localFunctions() const { return (const TConstFunctions&)m_localFunctions; }

            // get the declared base class of this class
            INLINE ClassType baseClass() const { return m_baseClass; }

            // get short class name (ie. engine::scene::Mesh -> "Mesh")
            INLINE StringID shortName() const { return m_shortName; }

            // get ALL properties (from this class and base classes)
            const TConstProperties& allProperties() const;

            // get ALL functions (from this class and base classes)
            const TConstFunctions& allFunctions() const;

            // get all template properties for this class
            typedef Array<TemplateProperty> TConstTemplateProperties;
            const TConstTemplateProperties& allTemplateProperties() const;

            // find property with given name (recursive)
            const Property* findProperty(StringID propertyName) const;

            // find function by name (recursive)
            const Function* findFunction(StringID functionName) const;

            // find function by name without cache check (recursive)
            const Function* findFunctionNoCache(StringID functionName) const;

            // patch all pointers to resources, returns list of patched properties (top-level only)
            bool patchResourceReferences(void* data, res::IResource* currentResource, res::IResource* newResource, base::Array<base::StringID>* outPatchedProperties) const;

            //---------------

            // get assigned user index, used for indexed systems (app services, game systems)
            // by default returns -1
            INLINE short userIndex() const { return m_userIndex; }

            // assign user index to class
            // NOTE: can only be done once
            void assignUserIndex(short index) const;

            //---------------

            // is this class derived from given base class
            bool is(ClassType otherClass) const;

            // is this class derived from given base class
            template< typename T >
            INLINE bool is() const
            {
                return is(T::GetStaticClass());
            }

            //---------------

            // bind a base class
            // NOTE: works only on a non-const (buildable) RTTI class, do not call after RTII initialized for a given module
            void baseClass(ClassType baseClass);

            // add a property to this class
            // NOTE: works only on a non-const (buildable) RTTI class, do not call after RTII initialized for a given module
            void addProperty(Property* property);

            // add a function to this class
            // NOTE: works only on a non-const (buildable) RTTI class, do not call after RTII initialized for a given module
            void addFunction(Function* function);

            //---------------

            // memory pool class belongs to (changed with RTTI_DECLARE_POOL)
            INLINE PoolTag pool() const { return m_memoryPool; }

            // allocate memory (from proper pool) for object of this type
            void* allocateClassMemory(uint32_t size, uint32_t alignment) const;

            // free memory (to proper pool) used by object of this type
            void freeClassMemory(void* ptr) const;

            //---------------

            template< typename T >
            INLINE RefPtr<T> create() const
            {
                DEBUG_CHECK_EX(is<T>(), "Unrelated types");
                DEBUG_CHECK_EX(sizeof(T) <= size(), "Trying to allocate bigger type from smaller class");
                auto* mem = (T*)allocateClassMemory(size(), alignment());
                construct(mem);
                return NoAddRef(mem);
            }

            template< typename T >
            INLINE T* createPointer() const
            {
                DEBUG_CHECK_EX(is<T>(), "Unrelated types");
                DEBUG_CHECK_EX(sizeof(T) <= size(), "Trying to allocate bigger type from smaller class");
                auto* mem = (T*)allocateClassMemory(size(), alignment());
                construct(mem);
                return mem;
            }

            //--

        protected:
            mutable short m_userIndex;

            StringID m_shortName;
            PoolTag m_memoryPool;

            typedef HashMap<StringID, const Function*> TFunctionCache;

            typedef Array< Property* > TNonConstProperties;
            TNonConstProperties m_localProperties;

            typedef Array< Function* > TNonConstFunctions;
            TNonConstFunctions m_localFunctions;

            mutable TConstProperties m_allProperties;
            mutable bool m_allPropertiesCached = false;

            mutable TConstTemplateProperties m_allTemplateProperties;
            mutable bool m_allTemplatePropertiesCached = false;

            mutable TConstFunctions m_allFunctions;
            mutable TFunctionCache m_allFunctionsMap;
            mutable bool m_allFunctionsCached = false;

            mutable SpinLock m_allTablesLock;

            const IClassType* m_baseClass;

            //--

            SpinLock m_resourceRelatedPropertiesLock;
            bool m_resourceRelatedPropertiesUpdated = false;
            Array<Property*> m_resourceRelatedProperties;

            friend reflection::ClassBuilder;

            //--

            bool handlePropertyMissing(TypeSerializationContext& context, StringID name, Type dataType, const void* data) const;
            bool handlePropertyTypeChange(TypeSerializationContext& context, StringID name, Type dataType, const void* data, Type currentType, void* currentData) const;

            virtual void cacheTypeData() override;
            virtual void releaseTypeReferences() override;
        };

    } // rtti

    // shared-ptr cast for RTTI SharedPointers
    template< class _DestType, class _SrcType >
    INLINE RefPtr< _DestType > rtti_cast(const RefPtr< _SrcType >& srcObj)
    {
        if (srcObj && srcObj->cls()->is(_DestType::GetStaticClass()))
            return srcObj.staticCast<_DestType>();

        return nullptr;
    }

    // normal-ptr cast for RTTI SharedPointers
    template< class _DestType, class _SrcType >
    INLINE _DestType* rtti_cast(_SrcType* srcObj)
    {
        if (srcObj && srcObj->cls()->is(_DestType::GetStaticClass()))
            return static_cast<_DestType*>(srcObj);

        return nullptr;
    }

    // normal-ptr cast for RTTI SharedPointers
    template< class _DestType, class _SrcType >
    INLINE const _DestType* rtti_cast(const _SrcType* srcObj)
    {
        if (srcObj && srcObj->cls()->is(_DestType::GetStaticClass()))
            return static_cast<const _DestType*>(srcObj);

        return nullptr;
    }

} // base
    
