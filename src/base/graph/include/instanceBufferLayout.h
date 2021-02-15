/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#pragma once

namespace base
{
    /// layout of the instance buffer
    /// determines where are the variable
    class BASE_GRAPH_API InstanceBufferLayout : public IReferencable
    {
    public:
        InstanceBufferLayout();
        ~InstanceBufferLayout();

        /// get size of the data in the data buffer
        INLINE uint32_t size() const { return m_size; }

        /// get the required alignment of the data
        INLINE uint32_t alignment() const { return m_alignment; }

        ///--

        /// initialize variables in the instance buffer
        void initializeBuffer(void* bufferMemory) const;

        /// destroy variable in the instance buffer
        void destroyBuffer(void* bufferMemory) const;

        /// copy variable in the instance buffer
        void copyBufer(void* destBufferMemory, const void* srcBufferMemory) const;

        ///--

        // create instance buffer from this layout
        InstanceBufferPtr createInstance(PoolTag poolID = POOL_INSTANCE_BUFFER);

    private:
        struct Group
        {
            StringID m_name;
            uint32_t m_firstVar;
            uint32_t m_numVars;
        };

        struct Var
        {
            Type m_type;
            uint32_t m_offset;
            uint32_t m_arraySize;
            uint32_t m_arrayStride;

            const Var* m_nextComplexType;
        };

        uint32_t m_size; // size of the data buffer
        uint32_t m_alignment; // required alignment of the data

        typedef Array<Group> TGroups;
        TGroups m_groups; // simple grouping for the variables, allows for better debug printouts

        typedef Array<Var> TVars;
        TVars m_vars; // all variables in the layout

        const Var* m_complexVarsList; // variables that have complex data and needs to be explicitly constructed/destructed

        friend class InstanceBufferLayoutBuilder;
    };

} // base