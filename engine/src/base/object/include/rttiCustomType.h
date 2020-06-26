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
                static bool WriteBinaryFunc(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData)
                {
                    return ((const T*)data)->writeBinary(file);
                }

                static bool ReadBinaryFunc(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data)
                {
                    return ((T*)data)->readBinary(file);
                }
            };

            template< typename T >
            struct TextSerializationHelper
            {
                static bool WriteTextFunc(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData)
                {
                    return ((const T*)data)->writeText(stream);
                }

                static bool ReadTextFunc(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data)
                {
                    return ((T*)data)->readText(stream);
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

                static bool ParseFromStringFunc(StringView<char> txt, void* data, bool partOfLargerStream)
                {
                    return T::Parse(txt, *(T*)data);
                }
            };

            template< typename T >
            struct DataViewHelper
            {
                static bool DescribeDataViewFunc(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo)
                {
                    return (*(const T*)viewData)->describeDataView(viewPath, outInfo);
                }

                static bool ReadDataViewFunc(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType)
                {
                    return (*(const T*)viewData)->readDataView(viewPath, targetData, targetType);
                }

                static bool WriteDataViewFunc(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType)
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
            typedef void (*TCalcHashFunc)(CRC64& crc, const void* data); // if empty we assume "hash by memory"

            typedef bool (*TWriteBinaryFunc)(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData);
            typedef bool (*TReadBinaryFunc)(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data);
            typedef bool (*TWriteTextFunc)(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData);
            typedef bool (*TReadTextFunc)(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data);

            typedef void (*TPrintToTextFunc)(IFormatStream& f, const void* data, uint32_t flags);
            typedef bool (*TParseFromStringFunc)(StringView<char> txt, void* data, uint32_t flags);

            typedef bool (*TDescribeDataViewFunc)(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo);
            typedef bool (*TReadDataViewFunc)(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType);
            typedef bool (*TWriteDataViewFunc)(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType);

            //--

            TConstructFunc funcConstruct = nullptr;
            TDestructFunc funcDestruct = nullptr;
            TCompareFunc funcComare = nullptr;
            TCopyFunc funcCopy = nullptr;
            TCalcHashFunc funcHash = nullptr;

            TWriteBinaryFunc funcWriteBinary = nullptr;
            TReadBinaryFunc funcReadBinary = nullptr;
            TWriteTextFunc funcWriteText = nullptr;
            TReadTextFunc funcReadText = nullptr;

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
            void bindHash()
            {
                funcHash = &prv::HashHelper<T>::Func;
            }

            template< typename T >
            void bindStdHash()
            {
                funcHash = &prv::HashHelperStd<T>::Func;
            }

            template< typename T >
            void bindHasher()
            {
                funcHash = &prv::HashHelperHasher<T>::Func;
            }

            template< typename T >
            void bindBinarySerialization()
            {
                funcReadBinary = &prv::BinarySerializationHelper<T>::ReadBinaryFunc;
                funcWriteBinary = &prv::BinarySerializationHelper<T>::WriteBinaryFunc;
            }

            template< typename T >
            void bindTextSerialization()
            {
                funcReadText = &prv::TextSerializationHelper<T>::ReadTextFunc;
                funcWriteText = &prv::TextSerializationHelper<T>::WriteTextFunc;
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
            virtual void calcCRC64(CRC64& crc, const void* data) const override final;

            virtual bool writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const override;
            virtual bool readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const override;
            virtual bool writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const override;
            virtual bool readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const override;

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override;
            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags = 0) const override;

            virtual bool describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const override;
            virtual bool readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const override;
            virtual bool writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const override;
        };

        //--

    } // rtti
} // base