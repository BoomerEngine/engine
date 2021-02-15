/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#include "build.h"
#include "instanceBufferLayout.h"
#include "instanceBufferLayoutBuilder.h"
#include "instanceVar.h"

namespace base
{

    InstanceBufferLayoutBuilder::InstanceBufferLayoutBuilder()
    {
        m_layout = RefNew<InstanceBufferLayout>();
        startGroup("main"_id);
    }

    InstanceBufferLayoutPtr InstanceBufferLayoutBuilder::extractLayout()
    {
        endGroup();

        auto ret = m_layout;

        // collect variables that require complex actions
        ret->m_complexVarsList = nullptr;
        for (auto& var : ret->m_vars)
        {
            // TODO: extend!
            if (var.m_type->traits().requiresDestructor)
            {
                var.m_nextComplexType = ret->m_complexVarsList;
                ret->m_complexVarsList = &var;
            }
        }

        // start new layout
        m_layout = RefNew<InstanceBufferLayout>();
        startGroup("main"_id);
        return ret;
    }

    void InstanceBufferLayoutBuilder::endGroup()
    {
        if (!m_layout->m_groups.empty())
        {
            auto& group = m_layout->m_groups.back();
            group.m_numVars = m_layout->m_vars.size() - group.m_firstVar;
        }
    }

    void InstanceBufferLayoutBuilder::startGroup(StringID name)
    {
        endGroup();

        auto& group = m_layout->m_groups.emplaceBack();
        group.m_name = name;
        group.m_firstVar = m_layout->m_vars.size();
        group.m_numVars = 0;
    }

    uint32_t InstanceBufferLayoutBuilder::addLayoutVar(Type type, uint32_t arraySize)
    {
        auto typeAlignment = type->alignment();
        auto typeSize = type ->size();

        auto offset = base::Align(m_layout->m_size, typeAlignment);

        InstanceBufferLayout::Var v;
        v.m_arraySize = arraySize;
        v.m_arrayStride = (arraySize == 1) ? typeSize : base::Align(typeSize, typeAlignment);
        v.m_offset = offset;
        v.m_type = type;
        m_layout->m_vars.pushBack(v);

        m_layout->m_alignment = std::max(m_layout->m_alignment, typeAlignment);
        m_layout->m_size = offset + arraySize * v.m_arrayStride;

        return offset;
    }

    void InstanceBufferLayoutBuilder::placeVariable(InstanceVarBase& var)
    {
        ASSERT_EX(var.offset() == InstanceVarBase::INVALID_OFFSET, "Variable already registered in the instance buffer layout");
        ASSERT_EX(var.type() != nullptr, "RTTI type for the variable was not recognized");

        auto typeAlignment = var.type()->alignment();
        var.m_offset = var.m_offset = addLayoutVar(var.type(), 1);
        var.m_type = var.m_type;
    }

    void InstanceBufferLayoutBuilder::placeArray(InstanceArrayBase& var, uint32_t size)
    {
        ASSERT_EX(var.offset() == InstanceVarBase::INVALID_OFFSET, "Variable already registered in the instance buffer layout");
        ASSERT_EX(var.type() != nullptr, "RTTI type for the variable was not recognized");

        var.m_size = size;

        if (size > 0)
        {
            auto typeAlignment = var.type()->alignment();
            var.m_offset = var.m_offset = addLayoutVar(var.type(), size);
            var.m_type = var.m_type;
        }
    }

    //--

    InstanceBufferLayoutBuilderArrayPlacer::InstanceBufferLayoutBuilderArrayPlacer(InstanceBufferLayoutBuilder* layout, InstanceArrayBase* arr)
        : m_layout(layout)
        , m_var(arr)
    {}

    void InstanceBufferLayoutBuilderArrayPlacer::operator<<(uint32_t size)
    {
        m_layout->placeArray(*m_var, size);
    }

    //--

} // base