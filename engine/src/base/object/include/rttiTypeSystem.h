/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti #]
***/

#pragma once

#include "base/containers/include/stringID.h"
#include "base/containers/include/array.h"
#include "base/containers/include/hashMap.h"

#include "rttiTypeRef.h"

#include <functional>

namespace base
{
    namespace rtti
    {
        //--

        /// RTTI SYSTEM interface
        class BASE_OBJECT_API TypeSystem : public ISingleton
        {
        public:
            /// Update internal caches
            void updateCaches();

            /// Find type by name (any type)
            /// NOTE: array and pointer types may be created if the name fits
            Type findType(StringID typeName);

            /// Find a class only type definition (a little bit faster than general FindType)
            ClassType findClass(StringID name);

            /// Find an enum only type definition (a little bit faster than general FindType)
            const EnumType* findEnum(StringID name);

            /// Find class property using the property hash (much faster than looking it up manually)
            const Property* findProperty(uint64_t propertyHash);

            /// Find global function
            const Function* findGlobalFunction(StringID name);

            /// Enumerate classes derived from given base class
            typedef std::function<bool(ClassType )> TClassFilter;
            typedef Array<ClassType > TClassList;
            void enumClasses(ClassType baseClass, TClassList& outClasses, const TClassFilter& filter = TClassFilter(), bool allowAbstract = false, bool assingClassIndices = false);

            /// Enumerate classes directly derived from given IClassType
            void enumDerivedClasses(ClassType baseClass, TClassList& outClasses);

            /// Enumerate all base types (non dynamically created) 
            void enumBaseTypes(Array<Type>& outAllTypes);

            //---

            /// Register type in the RTTI system
            void registerType(Type type);

            /// Register alternative name for type (usually old)
            void registerAlternativeTypeName(Type type, StringID alternativeName);

            /// Register class property
            void registerProperty(const rtti::Property* prop);

            /// Register global function
            void registerGlobalFunction(const Function* function);

            //--

            /// Register a Dynamic Type Creator. Dynamic type are usually a type that composition of multiple type. Like DynArray or THandle.
            typedef Type (*TDynamicTypeCreationFunction)(StringParser& typeNameString, TypeSystem& typeSystem);
            void registerDynamicTypeCreator(const char* keyword, TDynamicTypeCreationFunction creatorFunction);

            //---

            /// Given a native type hash (from typeid().hash_code()) find a matching RTTI type object
            /// NOTE: use std::remove_cv before calculating the hash
            Type mapNativeType(uint64_t hashCode) const;

            /// Given a native type hash (from typeid().hash_code()) find a matching RTTI type name
            /// NOTE: use std::remove_cv before calculating the hash
            StringID mapNativeTypeName(uint64_t hashCode) const;

            //---

            template< typename T >
            INLINE TClassList enumChildClasses(const TClassFilter& filter = TClassFilter(), bool allowAbstract = false)
            {
                TClassList ret;
                enumClasses(T::GetStaticClass(), ret, filter, allowAbstract);
                return ret;
            }

            /// Enumerate classes derived from given base class
            template< typename T >
            INLINE void enumClasses(Array< SpecificClassType<T> >& outClasses, const TClassFilter& filter = TClassFilter(), bool allowAbstract = false, bool assingClassIndices = false)
            {
                enumClasses(T::GetStaticClass(), (Array<ClassType>&)outClasses, filter, allowAbstract, assingClassIndices);
            }

            /// Enumerate classes derived from given base class
            template< typename T >
            INLINE void enumClasses(ClassType baseClass, Array< SpecificClassType<T> >& outClasses, const TClassFilter& filter = TClassFilter(), bool allowAbstract = false, bool assingClassIndices = false)
            {
                base::InplaceArray<base::ClassType, 128> tempClasses;
                enumClasses(baseClass, tempClasses);

                outClasses.reserve(tempClasses.size());
                for (const auto& cls : tempClasses)
                {
                    if (cls->is<T>())
                        outClasses.pushBack(cls.cast<T>());
                }
            }

            //---

            static TypeSystem& GetInstance();

            //--

            /// find a class by name that can be used to create object
            ClassType findFactoryClass(StringID name, ClassType requiredBaseClass);

        protected:
            TypeSystem();

            virtual void deinit() override;

            typedef HashMap<StringID, Type>  TTypeMap;
            typedef Array<Type>  TTypeList;
            typedef HashMap<uint64_t, const Property*>  TPropertyMap;
            typedef HashMap<uint64_t, Type>  TNativeTypeMap;
            typedef Array<ClassType>  TStorageClassList;
            typedef Array<std::pair< const char*, TDynamicTypeCreationFunction > > TDynamicTypeCreators;
            typedef HashMap<ClassType, TClassList> TChildClasses;
            typedef HashMap<StringID, const Function*> TFunctionMap;

            Mutex m_typesLock;
            TTypeMap m_types;
            TTypeMap m_alternativeTypes;
            TTypeList m_typeList;
            TStorageClassList m_classes;

            Mutex m_functionLock;
            TFunctionMap m_functionMap;

            Mutex m_nativeMapLock;
            TNativeTypeMap m_nativeMap;

            Mutex m_propertiesLock;
            TPropertyMap m_properties;

            TDynamicTypeCreators m_typeCreators;

            TChildClasses m_childClasses;

            bool m_fullyInitialized;

            Type createDynamicType_NoLock(const char* typeName);
        };

        //--

        /// copy value from one place to other, does type conversion when needed
        /// returns false if type conversion was not possible
        /// supported conversions:
        ///   same type to same type always works
        ///   numerical types (int8,16,32,64, uint8,16,32,64)
        ///   real types: float -> double, double -> float
        ///   handle types (via rtti_cast)
        ///   resource refs (of matching classes, ie Ref<StaticTexture> -> Ref<ITexture>, actual class is only checked if resource is loaded)
        ///   async resource refs (of matching classes, ie AsyncRef<StaticTexture> -> AsyncRef<ITexture>, downcasting is not possible as actual class is not known)
        ///   stringBuf <-> stringId
        ///   enum <-> stringID (if enum does not have a option with that name then we fail the conversion)
        ///   enum <-> (u)int64 (raw value)
        ///   bitfield <-> (u)int64 (raw value)
        ///   classTypes <-> specficClassTypes
        ///   SIMPLE string <-> type conversions (numbers, boolean)
        extern BASE_OBJECT_API bool ConvertData(const void* srcData, Type srcType, void* destData, Type destType);

        //--

    } // rtti
} // base

/// this is one of the major singletons, it deserves a global access
using RTTI = base::rtti::TypeSystem;