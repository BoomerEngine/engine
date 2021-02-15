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
        //--

        // an abstract file reader
        class BASE_IO_API IReadFileHandle : public IReferencable
        {
        public:
            virtual ~IReadFileHandle();

            //----

            //! get size of the file
            virtual uint64_t size() const = 0;

            //! get current position in file
            virtual uint64_t pos() const = 0;

            //! change position in file, returns false if not set
            virtual bool pos(uint64_t newPosition) = 0;

            //! read data from file (if opened as reader, returns number of bytes read)
            //! NOTE: this does not yield the fiber and does not go though the IO queue
            virtual uint64_t readSync(void* data, uint64_t size) = 0;

            //----

        protected:
            IReadFileHandle();
        };

        //--

        // an abstract file writer
        class BASE_IO_API IWriteFileHandle : public IReferencable
        {
        public:
            virtual ~IWriteFileHandle();

            //----

            //! get size of the file
            virtual uint64_t size() const = 0;

            //! get current position in file
            virtual uint64_t pos() const = 0;

            //! change position in file, returns false if not set
            virtual bool pos(uint64_t newPosition) = 0;

            //! write data to file (if opened as writer, returns number of bytes written)
            //! NOTE: this does not yield the fiber and does not go though the IO queue
            virtual uint64_t writeSync(const void* data, uint64_t size) = 0;

            //----

            //! discard all of the written content - prevents temporary output file from being swapped with the target one
            virtual void discardContent() = 0;
            
            //----

        protected:
            IWriteFileHandle();
        };

        ///--

    } // io
} // base
