/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(process)

class IOutputCallback;

//-----------------------------------------------------------------------------

/// This is the abstraction of a simple system named pipe, the writing end
/// NOTE: the writing end is always created first and is the "master" of the connection
class CORE_PROCESS_API IPipeWriter : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_PIPES)

public:
    virtual ~IPipeWriter();

    //! Close pipe 
    //! NOTE: the connection is closed but the object is not destroyed
    virtual void close() = 0;

    //! Get internal pipe name
    virtual const char* name() const = 0;

    //! Is pipeline opened ?
    //! NOTE: will go to false if errors occurred
    virtual bool isOpened() const = 0;

    //! Write data to pipe, returns number of data written
    //! NOTE: function is non blocking, if the buffer is full a zero will be returned
    virtual uint32_t write(const void* data, uint32_t size) = 0;

    //---

    //! Create a pipe with given name
    //! NOTE: may fail if the pipe already exists or name is not valid
    static IPipeWriter* Create();

    //! Open existing pipe for writing
    //! NOTE: may fail if pipe is already opened for writing
    //! NOTE: name of the pipe cannot be null
    static IPipeWriter* Open(const char* pipeName);
};

//-----------------------------------------------------------------------------

/// This is the abstraction of a simple system named pipe, the reading end
/// NOTE: the reading end is always created after writing end and is the "slave" of the connection
class CORE_PROCESS_API IPipeReader : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_PIPES)

public:
    virtual ~IPipeReader();

    //! Close pipe 
    //! NOTE: the connection is closed but the object is not destroyed
    virtual void close() = 0;

    //! Get internal pipe name
    virtual const char* name() const = 0;

    //! Is pipeline opened ?
    //! NOTE: will go to false if errors occurred
    virtual bool isOpened() const = 0;

    //! Read data from pipe, returns number of data written
    //! NOTE: function is non blocking, if there's nothing to read a zero will be returned
    //! NOTE: it's not legal to call this function if pipe was created with async callback
    virtual uint32_t read(void* data, uint32_t size) = 0;

    //---

    //! Create a pipe with given name
    //! NOTE: may fail if the pipe already exists or name is not valid
    //! If the pipe name is not specified a random, unique and safe name is generated
    //! If a callback is specfied than all input on the pipe is sent directly (and asynchronously) to the callback function
    static IPipeReader* Create(IOutputCallback* callback=nullptr);

    //! Open existing pipe with provided name
    //! If a callback is specfied than all input on the pipe is sent directly (and asynchronously) to the callback function
    static IPipeReader* Open(const char* pipeName, IOutputCallback* callback=nullptr);
};

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE_EX(process)
