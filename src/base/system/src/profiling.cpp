/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\profiling #]
***/

#include "build.h"
#include "profiling.h"

// use easy profiler for now
#define BUILD_WITH_EASY_PROFILER
#include "thirdparty/easyprofiler/profiler.h"

BEGIN_BOOMER_NAMESPACE(base::profiler)

//--

#if defined(PLATFORM_POSIX)
pthread_key_t GTlsCurrentBlock = 0;
#elif defined(PLATFORM_WINAPI)
uint32_t GTlsCurrentBlock = 0;
#endif

//--

static bool GRegisterBlockInfo = false;

BlockInfo::BlockInfo(const char* name, const char* id, const char* file, uint32_t line, uint8_t lvl, uint32_t color /*= Color::Default)*/)
    : m_name(name)
    , m_level(lvl)
    , m_color(color)
    , m_internal(nullptr)
{
    if (GRegisterBlockInfo)
        m_internal = ::profiler::registerDescription(::profiler::ON, id, name, file, line, ::profiler::block_type_t::Block, color, false);
}

//--

uint8_t Block::st_level = 0; // disabled

void Block::Initialize(uint8_t level)
{
    // set profiling level
    st_level = level;
    GRegisterBlockInfo = true;

    // start Easy Profier
    ::profiler::startListen();
    EASY_MAIN_THREAD;

    // allocate the current block TLS entry
#if defined(PLATFORM_POSIX)
    pthread_key_create(&GTlsCurrentBlock, NULL);
#elif defined(PLATFORM_WINAPI)
    GTlsCurrentBlock = TlsAlloc();
#endif
}

void Block::InitializeDisabled()
{
    // allocate the current block TLS entry
#if defined(PLATFORM_POSIX)
    pthread_key_create(&GTlsCurrentBlock, NULL);
#elif defined(PLATFORM_WINAPI)
    GTlsCurrentBlock = TlsAlloc();
#endif
}

#if defined(PLATFORM_POSIX)
    __attribute__ ((noinline)) Block* GetCurrentBlock()
    {
        return (Block*)pthread_getspecific(GTlsCurrentBlock);
    }

    __attribute__ ((noinline)) void SetCurrentBlock(Block* block)
    {
        pthread_setspecific(GTlsCurrentBlock, block);
    }
#elif defined(PLATFORM_WINAPI)
    __declspec(noinline) Block* GetCurrentBlock()
    {
        return (Block*)TlsGetValue(GTlsCurrentBlock);
    }

    __declspec(noinline) void SetCurrentBlock(Block* block)
    {
        TlsSetValue(GTlsCurrentBlock, block);
    }
#endif

Block::Block(const BlockInfo& info)
    : m_parent(nullptr)
    , m_enabled(info.m_level < st_level)
    , m_started(false)
    , m_info(&info)
    , m_startTime(0)
{
    if (m_enabled)
    {
        start();

        m_parent = GetCurrentBlock();
        ASSERT(!m_parent || m_parent->m_info != nullptr);
        SetCurrentBlock(this);
    }
}

Block::~Block()
{
    if (m_enabled)
    {
        SetCurrentBlock(m_parent);
        stop();
    }
}

void Block::start()
{
    if (!m_started)
    {
        m_startTime = ::profiler::now();
        m_started = true;
    }
}

void Block::stop()
{
    if (m_started)
    {
        if (m_info->m_internal)
        {
            auto end  = ::profiler::now();
            ::profiler::storeBlock((const ::profiler::BaseBlockDescriptor *) m_info->m_internal, "", m_startTime, end);
        }
        m_started = false;
    }
}

void Block::startChain()
{
    if (m_parent)
        m_parent->startChain();

    start();
}

void Block::stopChain()
{
    stop();

    if (m_parent)
        m_parent->stopChain();
}

Block* Block::CaptureAndYield()
{
    auto block  = GetCurrentBlock();
    SetCurrentBlock(nullptr);

    if (block)
        block->stopChain();

    return block;
}

void Block::Resume(Block* block)
{
    if (block)
    {
        block->startChain();
        SetCurrentBlock(block);
    }
}

END_BOOMER_NAMESPACE(base::profiler)