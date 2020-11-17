/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"
#include "base/containers/include/array.h"
#include "base/containers/include/stringID.h"

namespace base
{
    namespace rtti
    {
        //--

        namespace prv
        {
            template< typename T >
            struct CtorHelper
            {
                static void Func(void* ptr) { new (ptr) T(); }
            };

            template< typename T >
            struct DtorHelper
            {
                static void Func(void* ptr) { ((T*)ptr)->~T(); }
            };

            template< typename T >
            struct CopyHelper
            {
                static void Func(void* a, const void* b)
                {
                    *(T*)a = *(const T*)b;
                }
            };

            template< typename T >
            struct CopyConstructHelper
            {
                static void Func(void* a, const void* b)
                {
                    new (a) T(*(const T*)b);
                }
            };

            template< typename T >
            struct CompareHelper
            {
                static bool Func(const void* a, const void* b)
                {
                    return *(const T*)b == *(const T*)a;
                }
            };

            template< typename T >
            struct HashHelper
            {
                static void Func(CRC64& crc, const void* a)
                {
                    ((const T*)a)->calcCRC64(crc);
                }
            };

            template< typename T >
            struct HashHelperStd
            {
                static void Func(CRC64& crc, const void* a)
                {
                    crc << std::hash<T>{}(*(const T*)a);
                }
            };

            template< typename T >
            struct HashHelperHasher
            {
                static void Func(CRC64& crc, const void* a)
                {
                    crc << Hasher<T>::CalcHash(*(const T*)a);
                }
            };

            template< typename T >
            struct BinarySerializationHelper
            {
                static void WriteBinaryFunc(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData)
                {
                    ((const T*)data)->writeBinary(file);
                }

                static void ReadBinaryFunc(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data)
                {
                    ((T*)data)->readBinary(file);
                }
            };

            template< typename T >
            struct XMLSerializationHelper
            {
                static void WriteXMLFunc(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
                {
                    ((const T*)data)->writeXML(node);
                }

                static void ReadXMLFunc(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
                {
                    ((T*)data)->readXML(node);
                }
            };

            template< typename T >
            struct PrintHelper
            {
                static void PrintToTextFunc(IFormatStream& f, const void* data, uint32_t flags)
                {
                    ((const T*)data)->print(f);
                }
            };

            template< typename T >
            struct ToStringFromStringHelper
            {
                static void PrintToTextFunc(IFormatStream& f, const void* data, bool partOfLargerStream)
                {
                    ((const T*)data)->print(f);
                }

                static bool ParseFromStringFunc(StringView txt, void* data, bool partOfLargerStream)
                {
                    return T::Parse(txt, *(T*)data);
                }
            };

            template< typename T >
            struct DataViewHelper
            {
                static bool DescribeDataViewFunc(StringView viewPath, const void* viewData, DataViewInfo& outInfo)
                {
                    return (*(const T*)viewData)->describeDataView(viewPath, outInfo);
                }

                static bool ReadDataViewFunc(StringView viewPath, const void* viewData, void* targetData, Type targetType)
                {
                    return (*(const T*)viewData)->readDataView(viewPath, targetData, targetType);
                }

                static bool WriteDataViewFunc(StringView viewPath, void* viewData, const void* sourceData, Type sourceType)
                {
                    return (*(T*)viewData)->writeDataView(viewPath, targetData, targetType);
                }
            };

        } // prv

        //--

        // a custom type wrapper, allows to define any type via a list of function pointers without sub classing this class as it's not always possible
        // technically all function bindings can be empty for a type that is JUST for runtime, any way:
        // BASICS:
        //   - if we don't have constructor function bound we assume we can leave with zero initialized memory
        //   - if we don't have destructor function bound we assume we can leave with not calling destructor
        //   - if we don't have copy function bound we assume we can live with memcpy
        //   - if we don't have compare function bound we assume we can live with memcmp
        // SERIALIZATION:
        //   - if we don't have binary serialization bound we assume we can just do a byte dump (surprisingly it works for many simple types)
        //   - if we don't have custom text serialization bound we assume we can just use ToString/FromString results
        // STRING REPRESENTATION:
        //   - if we don't have custom Print/Parse function bound than we just don't print and fail to parse
        // DATA VIEW:
        //   - if we don't have custom data view functions than we assume that type is atomic and does not have internal parts (so we can only copy whole value)
        class BASE_OBJECT_API CustomType : public IType
        {
        public:
            CustomType(const char* name, uint32_t size, uint32_t alignment = 1, uint64_t nativeHash = 0);
            virtual ~CustomType();

            //--

            typedef void (*TConstructFunc)(void* ptr);
            typedef void (*TDestructFunc)(void* ptr);
            typedef bool (*TCompareFunc)(const void* a, const void* b); // if empty we assume "compare via memcmp"
            typedef void (*TCopyFunc)(void* dest, const void* src); // if empty we assume "copy via memcpy"

            typedef void (*TWriteBinaryFunc)(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData);
            typedef void (*TReadBinaryFunc)(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data);

            typedef void (*TWriteXMLFunc)(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData);
            typedef void (*TReadXMLFunc)(TypeSerializationContext& typeContext, const xml::Node& node, void* data);

            typedef void (*TPrintToTextFunc)(IFormatStream& f, const void* data, uint32_t flags);
            typedef bool (*TParseFromStringFunc)(StringView txt, void* data, uint32_t flags);

            typedef DataViewResult (*TDescribeDataViewFunc)(StringView viewPath, const void* viewData, DataViewInfo& outInfo);
            typedef DataViewResult(*TReadDataViewFunc)(StringView viewPath, const void* viewData, void* targetData, Type targetType);
            typedef DataViewResult(*TWriteDataViewFunc)(StringView viewPath, void* viewData, const void* sourceData, Type sourceType);

            //--

            TConstructFunc funcConstruct = nullptr;
            TDestructFunc funcDestruct = nullptr;
            TCompareFunc funcComare = nullptr;
            TCopyFunc funcCopy = nullptr;

            TWriteBinaryFunc funcWriteBinary = nullptr;
            TReadBinaryFunc funcReadBinary = nullptr;

            TWriteXMLFunc funcWriteXML = nullptr;
            TReadXMLFunc funcReadXML = nullptr;

            TPrintToTextFunc funcPrintToText = nullptr;
            TParseFromStringFunc funcParseFromText = nullptr;

            TDescribeDataViewFunc funcDescribeView = nullptr;
            TReadDataViewFunc funcReadDataView = nullptr;
            TWriteDataViewFunc funcWriteDataView = nullptr;

            //--

            template< typename T >
            void bindCtorDtor()
            {
                if (!std::is_trivially_constructible<T>::value)
                {
                    funcConstruct = &prv::CtorHelper<T>::Func;
                    m_traits.requiresConstructor = true;
                }
                else
                {
                    funcConstruct = nullptr; // zero initialize
                }

                if (!std::is_trivially_destructible<T>::value)
                {
                    funcDestruct = &prv::DtorHelper<T>::Func;
                    m_traits.requiresDestructor = true;
                }
                else
                {
                    funcDestruct = nullptr; // do not destruct memory
                }
            }

            template< typename T >
            void bindCopy()
            {
                if (!std::is_trivially_copy_assignable<T>::value)
                {
                    funcCopy = &prv::CopyHelper<T>::Func;
                }

                if (!funcCopy && !funcComare)
                    m_traits.simpleCopyCompare = true;
                else
                    m_traits.simpleCopyCompare = false;
            }

            template< typename T >
            void bindCompare()
            {
                funcComare = &prv::CompareHelper<T>::Func;
                m_traits.simpleCopyCompare = false;
            }

            template< typename T >
            void bindBinarySerialization()
            {
                funcReadBinary = &prv::BinarySerializationHelper<T>::ReadBinaryFunc;
                funcWriteBinary = &prv::BinarySerializationHelper<T>::WriteBinaryFunc;
            }

            template< typename T >
            void bindXMLSerialization()
            {
                funcReadXML = &prv::XMLSerializationHelper<T>::ReadXMLFunc;
                funcWriteXML = &prv::XMLSerializationHelper<T>::WriteXMLFunc;
            }

            template< typename T >
            void bindToStringFromString()
            {
                funcPrintToText = &prv::ToStringFromStringHelper<T>::PrintToTextFunc;
                funcParseFromText = &prv::ToStringFromStringHelper<T>::ParseFromStringFunc;
            }

            template< typename T >
            void bindPrint()
            {
                funcPrintToText = &prv::PrintHelper<T>::PrintToTextFunc;
            }

            template< typename T >
            void bindDataView()
            {
                funcDescribeView = &prv::DataViewHelper<T>::DescribeDataViewFunc;
                funcReadDataView = &prv::DataViewHelper<T>::ReadDataViewFunc;
                funcWriteDataView = &prv::DataViewHelper<T>::WriteDataViewFunc;
            }

        protected:
            virtual void construct(void* object) const override;
            virtual void destruct(void* object) const override;
            virtual bool compare(const void* data1, const void* data2) const override;
            virtual void copy(void* dest, const void* src) const override;

            virtual void writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const override;
            virtual void readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const override;

            virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
            virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override;
            virtual bool parseFromString(StringView txt, void* data, uint32_t flags = 0) const override;

            virtual DataViewResult describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const override;
            virtual DataViewResult readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const override;
            virtual DataViewResult writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const override;
        };

        //--

    } // rtti
} // base