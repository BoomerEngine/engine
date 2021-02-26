/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(io)

// an abstract file handle for async read operations
class CORE_IO_API IAsyncFileHandle : public IReferencable
{
public:
    virtual ~IAsyncFileHandle();

    //----

    //! get size of the file at the time the handle was created
    virtual uint64_t size() const = 0;

    //! create an async IO job to access the data at given offset and given size, the memory to load the data into is provided
    /// asynchronous IO request will be created, current fiber will be yielded and only woken back up when the IO request finishes
    virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) = 0;

    //----

protected:
    IAsyncFileHandle();
};

END_BOOMER_NAMESPACE_EX(io)
