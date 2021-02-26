/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"

#include "renderingShader.h"
#include "renderingShaderData.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ShaderObject);
RTTI_END_TYPE();

ShaderObject::ShaderObject(ObjectID id, IDeviceObjectHandler* impl, const ShaderMetadata* metadata)
    : IDeviceObject(id, impl)
    , m_metadata(AddRef(metadata))
{
    DEBUG_CHECK(metadata);
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
