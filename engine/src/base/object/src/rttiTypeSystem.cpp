/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiTypeSystem.h"
#include "rttiTypeSystem.h"
#include "rttiNativeClassType.h"
#include "rttiProperty.h"
#include "rttiFunction.h"

#include "rttiEnumType.h"
#include "rttiClassType.h"
#include "rttiClassRefType.h"
#include "rttiDynamicArrayType.h"
#include "rttiNativeArrayType.h"
#include "rttiHandleType.h"

#include "base/system/include/scopeLock.h"
#include "base/containers/include/stringParser.h"

namespace base
{
    namespace rtti
    {

        extern void RegisterFundamentalTypes(TypeSystem& typeSystem);
        extern void RegisterObjectTypes(TypeSystem& typeSystem);

        TypeSystem& TypeSystem::GetInstance()
        {
            static TypeSystem* instance = new TypeSystem();
            return *instance;
        }

        ClassType TypeSystem::findFactoryClass(StringID name, ClassType requiredBaseClass)
        {
            ASSERT(requiredBaseClass != nullptr);

			if (name.empty())
			{
				TRACE_ERROR("No class specified when a class derived from '{}' is required", requiredBaseClass->name());
				return nullptr;
			}

            auto classPtr = findClass(name);
            if (!classPtr)
            {
                TRACE_ERROR("Unknown class '{}' (requires class derived from '{}')", name, requiredBaseClass->name());
                return nullptr;
            }

            if (!classPtr->is(requiredBaseClass))
            {
                TRACE_ERROR("Class '{}' does not derive from required base '{}'", name, requiredBaseClass->name());
                return nullptr;
            }

            if (classPtr->isAbstract())
            {
                TRACE_ERROR("Class '{}' is abstract and can't be used as a factory class", name);
                return nullptr;
            }

            return classPtr;
        }

        TypeSystem::TypeSystem()
            : m_fullyInitialized(false)
        {
            // register the basic types: int, float, string, etc
            RegisterFundamentalTypes(*this);

            // register handle types
            registerDynamicTypeCreator(StrongHandleType::TypePrefix, &StrongHandleType::ParseType);
            registerDynamicTypeCreator(WeakHandleType::TypePrefix, &WeakHandleType::ParseType);

            // register array types
            registerDynamicTypeCreator(prv::DynamicArrayType::TypePrefix, &prv::DynamicArrayType::ParseType);
            registerDynamicTypeCreator(prv::NativeArrayType::TypePrefix, &prv::NativeArrayType::ParseType);

            // register the class ref type
            registerDynamicTypeCreator(ClassRefType::TypePrefix, &ClassRefType::ParseType);

            // register the IObject
            RegisterObjectTypes(*this);

            // we are fully initialized now
            m_fullyInitialized = true;
        }

        void TypeSystem::deinit()
        {
            ScopeLock<> lock(m_typesLock);

            // release default objects for classes
            for (auto ptr : m_classes)
                ptr->destroyDefaultObject();

            // delete all type references
            for (auto ptr : m_typeList)
                const_cast<IType*>(ptr.ptr())->releaseTypeReferences();

            // delete all types
            for (auto ptr : m_typeList)
                delete ptr.ptr();

            // cleanup
            m_types.clear();
            m_typeList.clear();
            m_alternativeTypes.clear();
            m_childClasses.clear();
            m_typeCreators.clear();
            m_types.clear();
            m_classes.clear();
            m_nativeMap.clear();
            m_properties.clear();
        }

        void TypeSystem::updateCaches()
        {
            for (auto classPtr : m_classes)
            {
                classPtr->allProperties();
                classPtr->allFunctions();
            }
        }

        void TypeSystem::enumBaseTypes(Array<Type>& outAllTypes)
        {
            ScopeLock<> lock(m_typesLock);

            outAllTypes.reserve(m_typeList.size());

            for (auto type : m_typeList)
            {
                // skip types that are created on-demand
                if (type.metaType() == MetaType::StrongHandle || type.metaType() == MetaType::WeakHandle || type.metaType() == MetaType::Array || type.metaType() == MetaType::ClassRef || type.metaType() == MetaType::ResourceRef)
                    continue;

                outAllTypes.pushBack(type);
            }
        }

        Type TypeSystem::findType(StringID typeName)
        {
            // nullptr type?
            if (typeName.empty())
                return nullptr;

            ScopeLock<> lock(m_typesLock);

            Type type = nullptr;
            if (m_types.find(typeName, type))
                return type;

            if (m_alternativeTypes.find(typeName, type))
                return type;

            return createDynamicType_NoLock(typeName.c_str());
        }

        ClassType TypeSystem::findClass(StringID name)
        {
            ScopeLock<> lock(m_typesLock);

            Type type = nullptr;
            if (m_types.find(name, type))
                return type.toClass();

            if (m_alternativeTypes.find(name, type))
                return type.toClass();

            return nullptr;
        }

        const Function* TypeSystem::findGlobalFunction(StringID name)
        {
            ScopeLock<> lock(m_functionLock);

            const Function* func = nullptr;
            if (m_functionMap.find(name, func))
                return func;

            return nullptr;
        }

        const EnumType* TypeSystem::findEnum(StringID name)
        {
            ScopeLock<> lock(m_typesLock);

            Type type = nullptr;
            if (m_types.find(name, type))
            {
                if (type->metaType() == MetaType::Enum)
                    return static_cast<const EnumType*>(type.ptr());
            }

            return nullptr;
        }

        const Property* TypeSystem::findProperty(uint64_t propertyHash)
        {
            ScopeLock<> lock(m_propertiesLock);

            const rtti::Property* prop = nullptr;
            if (m_properties.find(propertyHash, prop))
                return prop;

            return nullptr;
        }

        void TypeSystem::enumClasses(ClassType baseClass, Array< ClassType >& outClasses, const TClassFilter& filter /*= TClassFilter()*/, bool allowAbstract /* = false */, bool assingClassIndices /*= false*/)
        {
            ScopeLock<> lock(m_typesLock);

            // try to use cached results if possible
            if (!filter && !allowAbstract && !assingClassIndices)
            {
                auto existingList  = m_childClasses.find(baseClass);
                if (existingList)
                {
                    outClasses.pushBack(existingList->typedData(), existingList->size());
                    return;
                }
            }

            // enumerate classes manually
            for (auto testClass : m_classes)
            {
                if (testClass->isAbstract() && !allowAbstract)
                    continue;

                if (baseClass && !testClass->is(baseClass))
                    continue;

                if (filter && !filter(testClass))
                    continue;

                outClasses.pushBack(testClass);
            }

            // assign numerical indices to the classes
            if (assingClassIndices)
            {
                for (uint16_t i = 0; i < outClasses.size(); ++i)
                    const_cast<IClassType*>(outClasses[i].ptr())->assignUserIndex((short)i);
            }

            // store in cache for later use
            if (!filter && !allowAbstract && !assingClassIndices)
                m_childClasses.set(baseClass, outClasses);
        }

        void TypeSystem::enumDerivedClasses(ClassType baseClass, Array< ClassType >& outClasses)
        {
            ScopeLock<> lock(m_typesLock);

            for (auto testClass : m_classes)
                if (baseClass == testClass->baseClass())
                    outClasses.pushBack(testClass);
        }

        void TypeSystem::registerAlternativeTypeName(Type type, StringID alternativeName)
        {
            ScopeLock<> lock(m_typesLock);

            Type currentType = nullptr;
            if (m_alternativeTypes.find(alternativeName, currentType))
            {
                FATAL_ERROR(TempString("Alternative name '{}' is already used by type '{}'", alternativeName, currentType->name()).c_str());
            }
            else
            {
                m_alternativeTypes.set(alternativeName, type);
            }
        }

        void TypeSystem::registerType(Type type)
        {
            ASSERT_EX(type->name(), "Trying to register type with no name");

            ScopeLock<> lock(m_typesLock);

            Type currentType = nullptr;
            if (m_types.find(type->name(), currentType))
            {
                FATAL_ERROR(TempString("Name '{}' is already used as type name", type->name()).c_str());
                return;
            }

            m_types.set(type->name(), type);
            m_typeList.pushBack(type);

            if (auto classType = type.toClass())
                m_classes.pushBack(classType);

            auto typeHash = type->traits().nativeHash;
            if (typeHash)
            {
                ScopeLock<> lock2(m_nativeMapLock);
                m_nativeMap.set(typeHash, type);
            }

            if (m_fullyInitialized)
            {
                const_cast<IType*>(type.ptr())->cacheTypeData();

                if (auto metadata  = static_cast<const ShortTypeNameMetadata*>(type->MetadataContainer::metadata(ShortTypeNameMetadata::GetStaticClass())))
                {
                    if (auto shortName = StringID(metadata->shortName()))
                        registerAlternativeTypeName(type, shortName);
                }
            }

            if (type->scripted())
            {
                TRACE_INFO("Type '{}' registered", type->name());
            }
            else
            {
                TRACE_SPAM("Type '{}' registered", type->name());
            }
        }

        void TypeSystem::registerProperty(const rtti::Property* prop)
        {
            ScopeLock<> lock(m_propertiesLock);

            DEBUG_CHECK_EX(m_properties.find(prop->hash()) == nullptr, "Property already registered");
            m_properties.set(prop->hash(), prop);
        }

        void TypeSystem::registerGlobalFunction(const Function* function)
        {
            ScopeLock<> lock(m_functionLock);

            if (m_functionMap.contains(function->name()))
            {
                TRACE_ERROR("Global function '{}' was already registered", function->name());
            }
            else
            {
                TRACE_INFO("Registered global function '{}'", function->name());
                m_functionMap.set(function->name(), function);
            }
        }

        void TypeSystem::registerDynamicTypeCreator(const char* keyword, TDynamicTypeCreationFunction creatorFunction)
        {
            m_typeCreators.pushBack(std::make_pair(keyword, creatorFunction));
        }

        Type TypeSystem::mapNativeType(uint64_t hashCode) const
        {
            ScopeLock<> lock2(m_nativeMapLock);

            Type mappedType = nullptr;
            m_nativeMap.find(hashCode, mappedType);
            return mappedType;
        }

        StringID TypeSystem::mapNativeTypeName(uint64_t hashCode) const
        {
            ScopeLock<> lock2(m_nativeMapLock);

            Type mappedType = nullptr;
            m_nativeMap.find(hashCode, mappedType);

            return mappedType ? mappedType->name() : StringID();
        }

        //---

        Type TypeSystem::createDynamicType_NoLock(const char* typeName)
        {
            for (auto it : m_typeCreators)
            {
                StringParser parser(typeName);

                if (parser.parseKeyword(it.first))
                {
                    if (auto createdType = it.second(parser, *this))
                    {
                        const_cast<IType*>(createdType.ptr())->cacheTypeData();

                        m_typeList.pushBack(createdType);
                        registerAlternativeTypeName(createdType, base::StringID(typeName));
                        return createdType;
                    }
                }
            }

            return nullptr;
        }


        //--
    }
}

