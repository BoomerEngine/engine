/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// helper class used to define arrays
class CORE_GRAPH_API InstanceBufferLayoutBuilderArrayPlacer
{
public:
    InstanceBufferLayoutBuilderArrayPlacer(InstanceBufferLayoutBuilder* layout, InstanceArrayBase* arr);
    InstanceBufferLayoutBuilderArrayPlacer(const InstanceBufferLayoutBuilderArrayPlacer& other) = default;
    void operator<<(uint32_t size);

private:
    InstanceBufferLayoutBuilder* m_layout;
    InstanceArrayBase* m_var;
};

/// helper class used to build the memory layout of the instance buffer
class CORE_GRAPH_API InstanceBufferLayoutBuilder : public NoCopy
{
public:
    InstanceBufferLayoutBuilder();

    /// add a variable to the instance buffer
    INLINE void operator<<(InstanceVarBase& var) { placeVariable(var); }

    /// add an array to the instance buffer
    INLINE InstanceBufferLayoutBuilderArrayPlacer operator<<(InstanceArrayBase& var) { return InstanceBufferLayoutBuilderArrayPlacer(this, &var); }

    // extract the data layout from the builder
    InstanceBufferLayoutPtr extractLayout();

    // start variable group
    void startGroup(StringID name);

public:
    InstanceBufferLayoutPtr m_layout;

    void placeVariable(InstanceVarBase& var);
    void placeArray(InstanceArrayBase& var, uint32_t size);
    void endGroup();

    uint32_t addLayoutVar(Type type, uint32_t arraySize);

    friend class InstanceBufferLayoutBuilderArrayPlacer;
};

END_BOOMER_NAMESPACE()
