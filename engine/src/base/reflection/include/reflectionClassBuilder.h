/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "reflectionTypeName.h"
#include "reflectionPropertyBuilder.h"
#include "reflectionFunctionBuilder.h"

namespace base
{ 
    namespace rtti
    {
        class IClassType;
        class Property;
        class IMetadata;
        class NativeClass;
    }

    namespace reflection    
    {
        class PropertyBuilder;
        class FunctionBuilder;
        class InterfaceBuilder;
        class ClassBuilder;
        class CustomTypeBuilder;

        // trait builder
        class BASE_REFLECTION_API TypeTraitBuilder : public NoCopy
        {
        public:
            TypeTraitBuilder();

            INLINE TypeTraitBuilder& zeroInitializationValid()
            {
                m_traitZeroInit = true;
                return *this;
            }

            INLINE TypeTraitBuilder& noConstructor()
            {
                m_traitNoConstructor = true;
                return *this;
            }

            INLINE TypeTraitBuilder& noDestructor()
            {
                m_traitNoDestructor = true;
                return *this;
            }

            INLINE TypeTraitBuilder& fastCopyCompare()
            {
                m_traitFastCopyCompare = true;
                return *this;
            }

        private:
            bool m_traitZeroInit;
            bool m_traitNoConstructor;
            bool m_traitNoDestructor;
            bool m_traitFastCopyCompare;

            void apply(rtti::TypeRuntimeTraits& traits);

            friend class ClassBuilder;
            friend class CustomTypeBuilder;
        };

        // helper class that can add stuff to the class type
        class BASE_REFLECTION_API ClassBuilder : public NoCopy
        {
        public:
            ClassBuilder(rtti::NativeClass* classPtr);
            ~ClassBuilder();

            // apply changes to target class
            // this atomically sets up the base class and the properties
            void submit(); 

            // set current property category
            void category(const char* category);

            // create a builder for a class property 
            PropertyBuilder& addProperty(const char* rawName, Type type, uint32_t dataOffset);

            // create metadata
            rtti::IMetadata& addMetadata(ClassType classType);

            // create function builder
            FunctionBuilder& addFunction(const char* rawName);

            //---

            template< typename T >
            INLINE PropertyBuilder& addProperty(const char* rawName, T& ptr)
            {
                auto offset = range_cast<uint32_t>((uint64_t)&ptr);
                return addProperty(rawName, GetTypeObject<T>(), offset);
            }

            template< typename T >
            INLINE T& addMetadata()
            {
                static_assert(std::is_base_of<rtti::IMetadata, T>::value, "Not a metadata class");
                return static_cast<T&>(addMetadata(ClassID<T>()));
            }

            template< typename T >
            INLINE FunctionBuilderClass<T> addFunction(const char* name)
            {
                return FunctionBuilderClass<T>(addFunction(name));
            }

            INLINE FunctionBuilderStatic addStaticFunction(const char* name)
            {
                return FunctionBuilderStatic(addFunction(name));
            }

            INLINE TypeTraitBuilder& addTrait()
            {
                return m_traits;
            }

            INLINE rtti::NativeClass& type()
            {
                return *m_classPtr;
            }

            void addOldName(const char* oldName);

        private:
            rtti::NativeClass* m_classPtr;
            Array< PropertyBuilder > m_properties;
            Array< FunctionBuilder* > m_functions;
            Array< StringID > m_oldNames;
            StringBuf m_categoryName;
            TypeTraitBuilder m_traits;
        };

        //--

        // helper class that can add stuff to the custom type
        class BASE_REFLECTION_API CustomTypeBuilder : public NoCopy
        {
        public:
            CustomTypeBuilder(rtti::CustomType* customType);
            ~CustomTypeBuilder();

            // apply changes to target class
            // this atomically sets up the base class and the properties
            void submit();

            //--

            INLINE TypeTraitBuilder& addTrait()
            {
                return m_traits;
            }

            INLINE rtti::CustomType& type()
            {
                return *m_type;
            }

            //--

        private:
            rtti::CustomType* m_type;
            TypeTraitBuilder m_traits;
        };


    } // reflection

} // base