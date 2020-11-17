/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "propertyDecorators.h" 

namespace base
{
    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyCustomEditorMetadata);
        RTTI_PROPERTY(customEditorName);
    RTTI_END_TYPE();

    PropertyCustomEditorMetadata::PropertyCustomEditorMetadata(StringView editorName)
        : customEditorName(editorName)
    {}
    
    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyCommentMetadata);
        RTTI_PROPERTY(text);
    RTTI_END_TYPE();

    PropertyCommentMetadata::PropertyCommentMetadata(StringView txt)
        : text(txt)
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyUnitsMetadata);
        RTTI_PROPERTY(text);
    RTTI_END_TYPE();

    PropertyUnitsMetadata::PropertyUnitsMetadata(StringView txt)
        : text(txt)
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyNumberRangeMetadata);
        RTTI_PROPERTY(min);
        RTTI_PROPERTY(max);
    RTTI_END_TYPE();

    PropertyNumberRangeMetadata::PropertyNumberRangeMetadata(double min_, double max_)
        : min(min_)
        , max(max_)
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyPrecisionDigitsMetadata);
        RTTI_PROPERTY(digits);
    RTTI_END_TYPE();

    PropertyPrecisionDigitsMetadata::PropertyPrecisionDigitsMetadata(uint8_t digits_)
        : digits(digits_)
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyHasTrackbarMetadata);
    RTTI_END_TYPE();

    PropertyHasTrackbarMetadata::PropertyHasTrackbarMetadata()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyHasDragMetadata);
        RTTI_PROPERTY(wrap);
    RTTI_END_TYPE();

    PropertyHasDragMetadata::PropertyHasDragMetadata(bool wrap_)
        : wrap(wrap_)
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyHasAlphaMetadata);
    RTTI_END_TYPE();

    PropertyHasAlphaMetadata::PropertyHasAlphaMetadata()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(PropertyHasNoDefaultValueMetadata);
    RTTI_END_TYPE();

    PropertyHasNoDefaultValueMetadata::PropertyHasNoDefaultValueMetadata()
    {}

    //--

} // base