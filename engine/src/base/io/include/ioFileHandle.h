/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

namespace base
{
    namespace io
    {
        // an abstract file handle, allows for sync operations directly
        // is used by the async io as well
        class BASE_IO_API IFileHandle : public IReferencable
        {
        public:
            virtual ~IFileHandle();

            //----

            //! get the user debug label about the origin of this file (for printing errors)
            virtual const StringBuf& originInfo() const = 0;

            //! get size of the file
            virtual uint64_t size() const = 0;

            //! get current position in file
            virtual uint64_t pos() const = 0;

            //! change position in file, returns false if not set
            virtual bool pos(uint64_t newPosition) = 0;

            //----

            //! is the file opened for reading ?
            virtual bool isReadingAllowed() const = 0;

            //! is the file opened for writing ?
            virtual bool isWritingAllowed() const = 0;

            //! write data to file (if opened as writer, returns number of bytes written)
            //! NOTE: this does not yield the fiber and does not go though the IO queue
            virtual uint64_t writeSync(const void* data, uint64_t size) = 0;

            //! read data from file (if opened as reader, returns number of bytes read)
            //! NOTE: this does not yield the fiber and does not go though the IO queue
            virtual uint64_t readSync(void* data, uint64_t size) = 0;

            //----

            //! create an async IO job to access the data at given offset and given size, the memory to load the data into is provided
            /// asynchronous IO request will be created, current fiber will be yielded and only woken back up when the IO request finishes
            virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) = 0;

            //----

        protected:
            IFileHandle();
        };

    } // io
} // base
