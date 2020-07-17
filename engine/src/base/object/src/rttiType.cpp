/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiDataView.h"
#include "streamOpcodeWriter.h"
#include "rttiDataHolder.h"
#include "rttiClassType.h"
#include "dataView.h"

#include "base/containers/include/stringBuilder.h"

namespace base
{
    namespace rtti
    {

        ///---

        IType::IType(StringID name)
            : m_name(name)
        {}

        IType::~IType()
        {}

        void IType::cacheTypeData()
        {}

        void IType::releaseTypeReferences()
        {}

        void IType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            // nothing
        }

        bool IType::parseFromString(StringView<char> txt, void* data, uint32_t flags) const
        {
            // unparsable
            return false;
        }

        //--

        DataViewResult IType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (viewPath.empty())
            {
                outInfo.dataPtr = viewData;
                outInfo.dataType = this; // that's all we know :D

                if (outInfo.requestFlags.test(DataViewRequestFlagBit::TypeMetadata))
                    collectMetadataList(outInfo.metadata);

                outInfo.flags |= DataViewInfoFlagBit::LikeValue;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
                return DataViewResultCode::ErrorUnknownProperty;

            return DataViewResultCode::ErrorIllegalAccess;
        }

        DataViewResult IType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (viewPath.empty())
            {
                if (!rtti::ConvertData(viewData, this, targetData, targetType))
                    return DataViewResultCode::ErrorTypeConversion;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                if (propertyName == "__type")
                {
                     // TODO
                }
                else if (propertyName == "__text")
                {
                    StringBuf text;
                    {
                        StringBuilder f;
                        printToText(f, viewData);
                        text = f.toString();
                    }

                    static const auto textType = RTTI::GetInstance().findType("StringBuf"_id);

                    if (!rtti::ConvertData(&text, textType, targetData, targetType))
                        return DataViewResultCode::ErrorTypeConversion;

                    return DataViewResultCode::OK;
                    
                }

                return DataViewResultCode::ErrorUnknownProperty;
            }

            return DataViewResultCode::ErrorIllegalAccess;
        }

        DataViewResult IType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (viewPath.empty())
            {
                if (!rtti::ConvertData(sourceData, sourceType, viewData, this))
                    return DataViewResultCode::ErrorTypeConversion;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                return DataViewResultCode::ErrorUnknownProperty;
            }

            return DataViewResultCode::ErrorIllegalAccess;
        }

        ///---

        void TypeSerializationContext::print(IFormatStream& f) const
        {
            bool written = false;

            if (directObjectContext)
            {
                f.appendf("object {}", *directObjectContext);
                written = true;
            }

            if (classContext)
            {
                if (written) f.append(", ");
                f.appendf("class '{}'", classContext->name());
                written = true;
            }

            if (propertyContext)
            {
                if (written) f.append(", ");
                f.appendf("property '{}'", propertyContext->name());
            }
        }

        ///---

    } // rtti
} // base