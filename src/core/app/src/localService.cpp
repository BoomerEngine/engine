/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: services #]
***/

#include "build.h"
#include "application.h"
#include "localService.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(DependsOnServiceMetadata);
RTTI_END_TYPE();

DependsOnServiceMetadata::DependsOnServiceMetadata()
{}

///---

RTTI_BEGIN_TYPE_CLASS(TickBeforeMetadata);
RTTI_END_TYPE();

TickBeforeMetadata::TickBeforeMetadata()
{}

///---

RTTI_BEGIN_TYPE_CLASS(TickAfterMetadata);
RTTI_END_TYPE();

TickAfterMetadata::TickAfterMetadata()
{}

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IService);
RTTI_END_TYPE();

IService::IService()
{}

IService::~IService()
{}

///---

END_BOOMER_NAMESPACE()
