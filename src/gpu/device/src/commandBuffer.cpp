/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#include "build.h"
#include "commands.h"
#include "commandBuffer.h"

#include "core/system/include/thread.h"
#include "core/memory/include/linearAllocator.h"
#include "core/memory/include/pageAllocator.h"
#include "core/memory/include/pageCollection.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

namespace helper
{
    ConfigProperty<uint32_t> cvCommandBufferPageSize("Rendering.Commands", "CommandBufferPageSize", 64 * 1024);
    ConfigProperty<uint32_t> cvCommandBufferNumPreallocatedPages("Rendering.Commands", "NumPreallocatedPages", 256);
    ConfigProperty<uint32_t> cvCommandBufferNumRetainedPages("Rendering.Commands", "NumRetainedPages", 256);

    // wrapper for page allocator for command buffers
    class CommandBuferPageAllocator : public ISingleton
    {
        DECLARE_SINGLETON(CommandBuferPageAllocator);

    public:
        INLINE mem::PageAllocator& allocator() { return m_allocator; }

        CommandBuferPageAllocator()
        {
            const auto pageSize = std::clamp<uint32_t>(cvCommandBufferPageSize.get(), 16U << 10, 16U << 20);
            const auto numPreallocatedPages = std::max<uint32_t>(cvCommandBufferNumPreallocatedPages.get(), 0);
            const auto numRetainedPages = std::max<uint32_t>(cvCommandBufferNumRetainedPages.get(), 0);
            m_allocator.initialize(POOL_GPU_COMMANDS, pageSize, numPreallocatedPages, numRetainedPages);
        }

    private:
        mem::PageAllocator m_allocator;

        virtual void deinit() override
        {
        }
    };

} // helper

//--

#if 0
CommandBuffer::CommandBuffer(mem::PageCollection* pages, StringView name, uint8_t* firstPagePtr, uint8_t* firstPageEndPtr)
    : m_pages(pages)
    , m_currentWritePtr(firstPagePtr)
    , m_currentWriteEndPtr(firstPageEndPtr)
{
    // write the "hello" command
    auto* helloCmd = (OpHello*)m_currentWritePtr;
    m_commands = m_lastCommand = helloCmd->setup(CommandCode::Hello);

    // set the string
    memcpy(helloCmd->name, name.data(), name.length());
    helloCmd->name[name.length()] = 0;
    m_currentWritePtr += sizeof(OpHello) + name.length();

    // write the "end of buffer" guard
    auto* endCmd = (OpNewBuffer*)m_currentWritePtr;
    endCmd->setup(CommandCode::NewBuffer, helloCmd);
}
#endif

CommandBuffer::CommandBuffer()
{
    // create page allocator
    auto& pageAllocator = helper::CommandBuferPageAllocator::GetInstance().allocator();
    m_pages = mem::PageCollection::CreateFromAllocator(pageAllocator);

    // allocate first page
    m_currentWritePtr = (uint8_t*) m_pages->allocatePage();
    m_currentWriteEndPtr = m_currentWritePtr + m_pages->pageSize();

    // write the "hello" command
    auto* helloCmd = (OpHello*)m_currentWritePtr;
    m_commands = m_lastCommand = helloCmd->setup(CommandCode::Hello);
    m_currentWritePtr += sizeof(OpHello);

    // write the "end of buffer" guard
    auto* endCmd = (OpNewBuffer*)m_currentWritePtr;
    endCmd->setup(CommandCode::NewBuffer, helloCmd);
}

CommandBuffer::~CommandBuffer()
{}

void CommandBuffer::finishRecording()
{
    const auto wasFinished = (0 != m_finished.exchange(1));
    DEBUG_CHECK_EX(!wasFinished, "Buffer already finished recording");
}

void CommandBuffer::release()
{
    DEBUG_CHECK_EX(m_finished.load() != 0, "Cannot release non finished buffer");

    if (m_firstChildBuffer)
    {
        const auto* cur = m_firstChildBuffer;
        while (cur)
        {
            auto* next = cur->nextChildBuffer;
            cur->childBuffer->release();
            cur = next;
        }
        m_firstChildBuffer = nullptr;
    }

    DEBUG_CHECK_EX(m_writingThread == 0, "There is still a thread with CommandWriter that opened this command buffer for writing");
    m_pages->reset(); // this releases memory for this command buffer as well

    this->~CommandBuffer();
    mem::FreeBlock(this);
}

bool CommandBuffer::enumChildren(const std::function<bool(CommandBuffer * buffer)>& enumFunc)
{
    const auto* cur = m_firstChildBuffer;
    while (cur)
    {
        if (enumFunc(cur->childBuffer))
            return true;
        cur = cur->nextChildBuffer;
    }
    return false;
}

bool CommandBuffer::visitHierarchy(const std::function<bool(CommandBuffer * buffer)>& enumFunc)
{
    if (enumFunc(this))
        return true;

    const auto* cur = m_firstChildBuffer;
    while (cur)
    {
        if (cur->childBuffer->visitHierarchy(enumFunc))
            return true;
        cur = cur->nextChildBuffer;
    }

    return false;
}

CommandBuffer* CommandBuffer::Alloc()
{
    return new CommandBuffer;

    /*if (auto* pages = mem::PageCollection::CreateFromAllocator(pageAllocator))
    {
        if (pages->pageSize() >= 4096)
        {
            // get the first memory range
            auto* ptr = (uint8_t*)pages->allocatePage();
            auto* ptrEnd = ptr + pages->pageSize() - sizeof(OpNewBuffer);

            // allocate memory for the command buffer object
            auto* mem = AlignPtr(ptr, alignof(CommandBuffer));
            ptr = mem + sizeof(CommandBuffer);

            // create the command buffer object and pass rest of the page for it to use
            return new (mem) CommandBuffer(pages, name, ptr, ptrEnd);
        }
    }

    return nullptr; // OOM ?*/
}

void CommandBuffer::beginWriting()
{
    DEBUG_CHECK_EX(m_writingThread == 0, TempString("Command buffer is already being written to by thread '{}'", m_writingThread));
    m_writingThread = GetCurrentThreadID();
}

void CommandBuffer::endWriting()
{
    DEBUG_CHECK_EX(m_writingThread != 0, "Command buffer is not being written to by any thread");
    m_writingThread = 0;
}

//--      

static bool ValidateOpcodes()
{
#define RENDER_COMMAND_OPCODE(x) \
    static_assert(sizeof(Op##x) != 0, "Opcode has no message struct declared");\
    static_assert(alignof(Op##x) <= 4, "Can't use aligned types in command buffer data, use simple PODs");
    //static_assert(std::is_trivially_move_constructible<Op##x>::value, "Opcode has complex constructor but we don't call constructors on the opcodes :P");
#include "commandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE
    return true;
}

void ValidateCommandBuffer(CommandBuffer* buffer)
{
    static bool opcodesValid = ValidateOpcodes();


#if 0
#ifdef BUILD_DEBUG
    while (auto baseOp  = reader.get())
    {}

    if (m_totalConstantsUploadSize > 0)
    {
        auto prev= m_constantUploadHead;
        for (auto cur = m_constantUploadHead; cur; cur = cur->next)
        {
            uint32_t mergedOffset = 0;

            const void* constData = cur->externalDataPtr ? cur->externalDataPtr : cur->data;
            ASSERT(cur->offset + cur->dataSize <= m_totalConstantsUploadSize);

            prev = cur;
        }
    }

    for (auto cur = m_triansienBufferAllocHead; cur; cur = cur->next)
    {
        // allocate the resident storage for the buffer on the GPU
        auto initialData = (cur->initializationDataSize != 0)
                                    ? (cur->initializationDataExternal ? cur->initializationDataExternal : cur->initializationDataInlined)
                                    : nullptr;
        if (initialData != nullptr)
        {
            ASSERT(cur->initializationDataSize <= cur->buffer.size());
        }

        // report the buffer updates
    }

    for (auto cur = m_dynamicBufferUpdatesHead; cur; cur = cur->next)
    {
        ASSERT(cur->dataBlockSize <= cur->view.size());
    }
#endif
#endif
}

END_BOOMER_NAMESPACE_EX(gpu)
