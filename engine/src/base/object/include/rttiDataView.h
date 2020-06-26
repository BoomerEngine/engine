/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\view #]
***/

#pragma once

#include "base/containers/include/stringID.h"
#include "base/containers/include/array.h"
#include "rttiProperty.h"

namespace base
{
    namespace rtti
    {

        //--

        enum class DataViewRequestFlagBit : uint8_t
        {
            MemberList = FLAG(0), // return list of members for structures
            OptionsList = FLAG(1), // return list of options for enums
            ObjectInfo = FLAG(2), // information about pointed object
            PropertyMetadata = FLAG(3), // return metadata assigned to the property (only works for views that are properties)
            TypeMetadata = FLAG(4), // return metadata assigned to the view type
        };

        enum class DataViewInfoFlagBit : uint16_t
        {
            LikeArray = FLAG(0), // element is like array - has indices
            LikeStruct = FLAG(1), // element is like a struct
            LikeValue = FLAG(2), // element behaves like a simple value, we should hope for ToString/FromString to work
            
            Object = FLAG(3), // element is a pointer to object, we can query the raw pointer if we ask for it
            Inlined = FLAG(4), // element is inlined object that can be edited directly in the same view
            Advanced = FLAG(6), // element is considered part of "advanced" set of properties
            ReadOnly = FLAG(7), // element is for sure read only, writes will be discarded
            Constrainded = FLAG(8), // value on this item is constrained (enum)
            HasSetup = FLAG(9), // we have some more setup how to edit this item
            DynamicArray = FLAG(10), // array element count can be changed
            VerticalEditor = FLAG(11), // prefer vertical editor data
        };

        typedef DirectFlags<DataViewRequestFlagBit> DataViewRequestFlags;
        typedef DirectFlags<DataViewInfoFlagBit> DataViewInfoFlags;

        //--

        struct DataViewMemberInfo
        {
            StringID name;
            StringID category;
        };

        struct DataViewOptionInfo
        {
            StringID name;
            StringBuf hint;
        };

        struct DataViewInfo
        {
            DataViewRequestFlags requestFlags; // what to fill when returning the info
            StringID categoryFilter; // filter for reported members

            //--
            // basic info, that is always set

            DataViewInfoFlags flags; // info about stuff
            Type dataType = nullptr; // true RTTI type of data (if known)
            const void* dataPtr = nullptr; // data pointer at the end of eval (ha ha ha, use at your own peril...)
            ClassType objectClass = nullptr;
            const IObject* objectPtr = nullptr;
            uint32_t arraySize = 0; // size of array (only if array)

            //--
            // filled on request

            Array<DataViewMemberInfo> members;
            Array<DataViewOptionInfo> options;
            Array<const IMetadata*> metadata;
        };

        struct BASE_OBJECT_API DataViewCommand
        {
            StringID command;
            int arg0 = -1;
            int arg1 = -1;
            Type argT;

            //--

            static SpecificClassType<DataViewCommand> GetStaticClass();
            inline base::ClassType nativeClass() const { return GetStaticClass(); }
            inline base::ClassType cls() const { return GetStaticClass(); }

            // initialize class (since we have no automatic reflection in this project)
            static void RegisterType(rtti::TypeSystem& typeSystem);
        };

        struct BASE_OBJECT_API DataViewBaseValue
        {
            bool differentThanBase = false;

            // TODO: base value history ?

            static SpecificClassType<DataViewBaseValue> GetStaticClass();
            inline base::ClassType nativeClass() const { return GetStaticClass(); }
            inline base::ClassType cls() const { return GetStaticClass(); }

            // initialize class (since we have no automatic reflection in this project)
            static void RegisterType(rtti::TypeSystem& typeSystem);
        };

        //--

        // eat property name from view path, will modify the path to the remainder if successful
        extern BASE_OBJECT_API bool ParsePropertyName(StringView<char>& view, StringView<char>& outPropertyName);

        // eat array count from view path, will modify the path to the remainder if successful
        extern BASE_OBJECT_API bool ParseArrayIndex(StringView<char>& view, uint32_t& outArrayCount);

        // get parent path for given path
        extern BASE_OBJECT_API bool ExtractParentPath(StringView<char>& view, StringView<char>& outChild);

        //--

    } // rtti
} // base