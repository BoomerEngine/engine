/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: background #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

/// simple background job
class BASE_FIBERS_API IBackgroundJob : public IReferencable
{
    RTTI_DECLARE_POOL(POOL_FIBERS)

public:
    virtual ~IBackgroundJob();

    inline bool canceled() const { return m_cancelFlag.load(); }

    virtual void cancel();
    virtual void run() = 0;

private:
    std::atomic<bool> m_cancelFlag = false;
};

//--

// run background job
extern BASE_FIBERS_API void RunBackground(IBackgroundJob* job, StringID bucket = "default"_id);

//--

END_BOOMER_NAMESPACE(base)

