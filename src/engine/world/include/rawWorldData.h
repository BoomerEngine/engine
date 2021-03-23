/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// Uncooked scene
/// Layer is composed of a list of node templates
class ENGINE_WORLD_API RawWorldData : public IResource, public IWorldParametersSource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(RawWorldData, IResource);

public:
    RawWorldData();

    INLINE const Array<WorldParametersPtr>& parameters() const { return m_parametres; }

private:
    Array<WorldParametersPtr> m_parametres;

    void createParameters();

    virtual Array<WorldParametersPtr> compileWorldParameters() const override final;

    virtual void onPostLoad() override;
};

//---

END_BOOMER_NAMESPACE()
