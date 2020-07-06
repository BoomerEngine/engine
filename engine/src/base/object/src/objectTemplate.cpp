/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "objectTemplate.h"
#include "rttiNativeClassType.h"

namespace base
{
    //---

    IObjectTemplate::IObjectTemplate()
    {}

    IObjectTemplate::~IObjectTemplate()
    {}

    //--

    SpecificClassType<IObjectTemplate> IObjectTemplate::GetStaticClass()
    {
        static ClassType objectType = RTTI::GetInstance().findClass("base::IObjectTemplate"_id);
        return SpecificClassType<IObjectTemplate>(*objectType.ptr());
    }

    ClassType IObjectTemplate::cls() const
    {
        return GetStaticClass();
    }

    //--

    bool IObjectTemplate::hasPropertyOverride(StringID name) const
    {
        return m_overridenProperties.contains(name);
    }

    void IObjectTemplate::togglePropertyOverride(StringID name, bool flag)
    {
        if (flag)
        {
            if (m_overridenProperties.insert(name))
                markModified();
        }
        else
        {
            if (m_overridenProperties.remove(name))
                markModified();
        }
    }

    //--

    void IObjectTemplate::toggleOverrideState(bool flag)
    {
        if (m_override != flag)
        {
            m_override = flag;
            markModified();

            postEvent("OnFullStructureChange"_id);
        }
    }

    //---

    DataViewResult IObjectTemplate::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (m_override)
        {
            if (viewPath.empty())
            {
                
            }
        }

        return IObject::describeDataView(viewPath, outInfo);
    }

    DataViewResult IObjectTemplate::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        return IObject::readDataView(viewPath, targetData, targetType);
    }

    DataViewResult IObjectTemplate::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType)
    {
        return IObject::writeDataView(viewPath, sourceData, sourceType);
    }

    //---

    void IObjectTemplate::RegisterType(rtti::TypeSystem& typeSystem)
    {
        const auto base = typeSystem.findClass("base::IObject"_id);
        auto cls = MemNew(rtti::NativeClass, "base::IObjectTemplate", sizeof(IObjectTemplate), alignof(IObjectTemplate), typeid(IObjectTemplate).hash_code()).ptr;
        cls->baseClass(base);
        typeSystem.registerType(cls);
    }

    //---

} // base

