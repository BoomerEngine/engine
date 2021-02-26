/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\view #]
***/

#pragma once

#include "core/containers/include/stringID.h"
#include "core/containers/include/array.h"
#include "rttiProperty.h"

BEGIN_BOOMER_NAMESPACE_EX(rtti)

//--

enum class DataViewRequestFlagBit : uint8_t
{
    MemberList = FLAG(0), // return list of members for structures
    OptionsList = FLAG(1), // return list of options for enumerators
    ObjectInfo = FLAG(2), // information about pointed object
    PropertyEditorData = FLAG(3), // return editor data assigned to the property
    TypeMetadata = FLAG(4), // return metadata assigned to the view type
    CheckIfResetable = FLAG(5), // check if the current value of the property is resettable to some base value (done via DataView in the end)
    PropertyMetadataEx = FLAG(6), // return metadata assigned to the view type
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
    ResetableToBaseValue = FLAG(12), // property has some "overridden" value that can be reset to base value 
};

typedef DirectFlags<DataViewRequestFlagBit> DataViewRequestFlags;
typedef DirectFlags<DataViewInfoFlagBit> DataViewInfoFlags;

//--

struct DataViewMemberInfo
{
    StringID name;
    StringID category;
    Type type; // used ONLY to merge different member lists
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
    Array<const IMetadata*> typeMetadata;
    PropertyEditorData editorData;
};

//--

// eat property name from view path, will modify the path to the remainder if successful
extern CORE_OBJECT_API bool ParsePropertyName(StringView& view, StringView& outPropertyName);

// eat array count from view path, will modify the path to the remainder if successful
extern CORE_OBJECT_API bool ParseArrayIndex(StringView& view, uint32_t& outArrayCount);

// get parent path for given path
extern CORE_OBJECT_API bool ExtractParentPath(StringView& view, StringView& outChild);

//--

END_BOOMER_NAMESPACE_EX(rtti)
