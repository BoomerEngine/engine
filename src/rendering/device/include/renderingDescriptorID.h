/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#pragma once

namespace rendering
{
    ///--

    /// Global, unique ID of the parameter layout
    /// This class identifies the layout of the resource entries in the parameter block (descriptor)
    /// Layout itself is the only reasonable way to identify stuff, we don't need a name as there may be many different
    class RENDERING_DEVICE_API DescriptorID
    {
    public:
        INLINE DescriptorID() {}
        INLINE explicit DescriptorID(uint16_t id_) : id(id_) {};

        // is the ID empty (unassigned)
        INLINE bool empty() const { return id == 0; }
        INLINE operator bool() const { return id != 0; }

        // get the numerical value, can be used to index things
        INLINE uint16_t value() const { return id; }

        // compare
        INLINE bool operator==(DescriptorID other) const { return id == other.id; }
        INLINE bool operator!=(DescriptorID other) const { return id != other.id; }

        // get a string representation of the layout, usually something like C-C-IR-IR-IW, etc describing the descriptor layout
        // NOTE: slow  (global lock), for debug only
        void print(base::IFormatStream& f) const;

        // get the layout description
        // NOTE: slow (global lock), don't call to often
        const DescriptorInfo& layout() const;

        // get the memory size required to upload data with this layout to renderer
        uint32_t memorySize() const;

        ///---

        /// register a layout ID form a manual layout
        /// NOTE: this may return existing one if it matches the layout
        static DescriptorID Register(const DescriptorInfo& info, const DescriptorInfo** outInfoPtr = nullptr);

        /// register a layout ID form a string description, this allows to specify the layouts at compile time
        /// NOTE: this may return existing one if it matches the layout
        /// NOTE: layout format is very simple, e.g: "CBV-CBV-ISRV-ISRV-BUAV"
        static DescriptorID Register(base::StringView layoutDesc, const DescriptorInfo** outInfoPtr = nullptr);

        /// get a layout from list of descriptor entries
        static DescriptorID FromDescriptor(const DescriptorEntry* data, uint32_t count, const DescriptorInfo** outInfoPtr = nullptr);

        /// get a layout from a list of descriptor entry types
        static DescriptorID FromTypes(const DeviceObjectViewType* types, uint32_t numTypes, const DescriptorInfo** outInfoPtr = nullptr);

    private:
        uint16_t id;
    };

    ///--

	static const uint32_t DESCRIPTOR_ENTRY_MEMORY_SIZE = 64;

	///--

} // rendering
