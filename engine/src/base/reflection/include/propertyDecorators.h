/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "base/containers/include/stringID.h"
#include "base/containers/include/inplaceArray.h"

#include "base/object/include/rttiTypeSystem.h"
#include "base/object/include/rttiType.h"
#include "base/object/include/rttiClassRef.h"

namespace base
{
    //--

    /// custom editor to use for property
    class BASE_REFLECTION_API PropertyCustomEditorMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyCustomEditorMetadata, rtti::IMetadata);

    public:
        PropertyCustomEditorMetadata(StringView<char> editorName="");

        StringID customEditorName;
    };

    //--

    /// comment for editable property
    class BASE_REFLECTION_API PropertyCommentMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyCommentMetadata, rtti::IMetadata);

    public:
        PropertyCommentMetadata(StringView<char> txt = "");

        StringBuf text;
    };

    //--

    /// units for the value (meters/deg, etc)
    class BASE_REFLECTION_API PropertyUnitsMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyUnitsMetadata, rtti::IMetadata);

    public:
        PropertyUnitsMetadata(StringView<char> txt = "");

        StringBuf text;
    };

    //--

    /// allowed range of values for the property
    class BASE_REFLECTION_API PropertyNumberRangeMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyNumberRangeMetadata, rtti::IMetadata);

    public:
        PropertyNumberRangeMetadata(double min_=0.0, double max_=1.0);

        double min = 0.0;
        double max = 1.0;
    };

    //--

    /// number of precision digits to display for floating point values
    class BASE_REFLECTION_API PropertyPrecisionDigitsMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyPrecisionDigitsMetadata, rtti::IMetadata);

    public:
        PropertyPrecisionDigitsMetadata(uint8_t digits_ = 2);

        uint8_t digits = 2;
    };

    //--

    /// edit the value of the property using a trackbar
    class BASE_REFLECTION_API PropertyHasTrackbarMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyHasTrackbarMetadata, rtti::IMetadata);

    public:
        PropertyHasTrackbarMetadata();
    };

    //--

    /// include alpha in editor for a color
    class BASE_REFLECTION_API PropertyHasAlphaMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyHasAlphaMetadata, rtti::IMetadata);

    public:
        PropertyHasAlphaMetadata();
    };

    //--

    /// property can't be reset to base value (ie. name of the object, resource etc)
    class BASE_REFLECTION_API PropertyHasNoDefaultValueMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyHasNoDefaultValueMetadata, rtti::IMetadata);

    public:
        PropertyHasNoDefaultValueMetadata();
    };

    //--

    /// edit the value of the property using a dragger
    class BASE_REFLECTION_API PropertyHasDragMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PropertyHasDragMetadata, rtti::IMetadata);

    public:
        PropertyHasDragMetadata(bool wrap_=false);

        bool wrap = false;
    };

    //--    

} // base
