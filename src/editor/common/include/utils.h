/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// run a LOOGN action under a modal progress dialog, if dialog does not auto close it's left opened, if no progress dialog is specified a default one is used
/// NOTE: dialog has initial small delay before it is shown (around 0.2-0.3s so a very short actions don't bother user with popping window)
extern EDITOR_COMMON_API void RunLongAction(ui::IElement* owner, ProgressDialog* dlg, const TLongJobFunc& func, StringView title = "", bool canCancel = false);

///---

/// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
extern EDITOR_COMMON_API bool SaveToXML(ui::IElement* owner, StringView category, const std::function<ObjectPtr()>& makeXMLFunc, StringBuf* currentFileName = nullptr);

/// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
extern EDITOR_COMMON_API bool SaveToXML(ui::IElement* owner, StringView category, const ObjectPtr& objectPtr, StringBuf* currentFileName = nullptr);

/// helper function to load object from XML on user's disk 
extern EDITOR_COMMON_API ObjectPtr LoadFromXML(ui::IElement* owner, StringView category, SpecificClassType<IObject> expectedObjectClass);

/// helper function to load object from XML on user's disk 
template< typename T >
static INLINE RefPtr<T> LoadFromXML(ui::IElement* owner, StringView category)
{ 
    return rtti_cast<T>(LoadFromXML(owner, category, T::GetStaticClass()));
}

//---

/// check if depot file can be loaded into given class
extern EDITOR_COMMON_API bool CanLoadAsClass(StringView depotPath, ClassType cls);

/// check if depot file can be loaded into given class
template< typename T >
INLINE bool CanLoadAsClass(StringView depotPath)
{
    return CanLoadAsClass(depotPath, T::GetStaticClass());
}

//---

END_BOOMER_NAMESPACE_EX(ed)

