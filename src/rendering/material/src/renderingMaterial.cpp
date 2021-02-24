/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterial.h"
#include "renderingMaterialTemplate.h"
#include "renderingMaterialRuntimeService.h"
#include "base/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

RTTI_BEGIN_TYPE_ENUM(MaterialPass);
    RTTI_ENUM_OPTION(DepthPrepass);
    RTTI_ENUM_OPTION(WireframeSolid);
    RTTI_ENUM_OPTION(WireframePassThrough);
    //RTTI_ENUM_OPTION(ConstantColor);
    RTTI_ENUM_OPTION(SelectionFragments);
    //RTTI_ENUM_OPTION(NoLighting);
    RTTI_ENUM_OPTION(Forward);
    RTTI_ENUM_OPTION(ShadowDepth);
    RTTI_ENUM_OPTION(MaterialDebug);
RTTI_END_TYPE();

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterial);
RTTI_END_TYPE();

///----

IMaterial::IMaterial()
{
}

IMaterial::~IMaterial()
{
    if (auto trackedBase = m_trackedMaterial.lock())
        trackedBase->unregisterDependentMaterial(this);
}

   
void IMaterial::registerDependentMaterial(IMaterial* material)
{
    if (material)
    {
        auto lock = base::CreateLock(m_runtimeDependenciesLock);
        m_runtimeDependencies.pushBackUnique(material);
        //TRACE_INFO("RegisterDependentMaterial at {}, child {}", this, material);
    }
}

void IMaterial::unregisterDependentMaterial(IMaterial* material)
{
    auto lock = base::CreateLock(m_runtimeDependenciesLock);

    //TRACE_INFO("UnregisterDependentMaterial at {}, child {}", this, material);

    auto index = m_runtimeDependencies.find(material);
    if (index != INDEX_NONE)
        m_runtimeDependencies[index] == nullptr;
}

void IMaterial::changeTrackedMaterial(IMaterial* material)
{
    if (m_trackedMaterial != material)
    {
        if (auto oldMaterial = m_trackedMaterial.lock())
            oldMaterial->unregisterDependentMaterial(this);

        m_trackedMaterial = material;

        if (material)
            material->registerDependentMaterial(this);
    }
}

void IMaterial::notifyDataChanged()
{
    auto lock = base::CreateLock(m_runtimeDependenciesLock);

    for (const auto& dep : m_runtimeDependencies)
        if (auto material = dep.lock())
            material->notifyDataChanged();
}

void IMaterial::notifyBaseMaterialChanged()
{
    auto lock = base::CreateLock(m_runtimeDependenciesLock);

    for (const auto& dep : m_runtimeDependencies)
        if (auto material = dep.lock())
            material->notifyBaseMaterialChanged();
}    

///---

END_BOOMER_NAMESPACE(rendering)