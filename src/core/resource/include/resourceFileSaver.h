/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

/// read context for loading file deserialization
struct CORE_RESOURCE_API FileSavingContext
{
    //--

    // save into protected token stream (more metadata to spot issues)
    // NOTE: final version of loading code does not support the protected stream
    bool protectedStream = true;

    //--

    // root object to save
    Array<ObjectPtr> rootObject;

    //--

    // should we save given object ?
    bool shouldSave(const IObject* object) const;
};

//--

// save objects to file
CAN_YIELD extern CORE_RESOURCE_API bool SaveFile(IWriteFileHandle* file, const FileSavingContext& context, IProgressTracker* progress = nullptr);

//--

END_BOOMER_NAMESPACE_EX(res)
