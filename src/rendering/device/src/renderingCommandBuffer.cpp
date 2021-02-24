/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#include "build.h"
#include "renderingCommands.h"
#include "renderingCommandBuffer.h"

#include "base/system/include/thread.h"
#include "base/memory/include/linearAllocator.h"
#include "base/memory/include/pageAllocator.h"
#include "base/memory/include/pageCollection.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//--

namespace helper
{
    base::ConfigProperty<uint32_t> cvCommandBufferPageSize("Rendering.Commands", "CommandBufferPageSize", 64 * 1024);
    base::ConfigProperty<uint32_t> cvCommandBufferNumPreallocatedPages("Rendering.Commands", "NumPreallocatedPages", 256);
    base::ConfigProperty<uint32_t> cvCommandBufferNumRetainedPages("Rendering.Commands", "NumRetainedPages", 256);

    // wrapper for page allocator for command buffers
    class CommandBuferPageAllocator : public base::ISingleton
    {
        DECLARE_SINGLETON(CommandBuferPageAllocator);

    public:
        INLINE base::mem::PageAllocator& allocator() { return m_allocator; }

        CommandBuferPageAllocator()
        {
            const auto pageSize = std::clamp<uint32_t>(cvCommandBufferPageSize.get(), 16U << 10, 16U << 20);
            const auto numPreallocatedPages = std::max<uint32_t>(cvCommandBufferNumPreallocatedPages.get(), 0);
            const auto numRetainedPages = std::max<uint32_t>(cvCommandBufferNumRetainedPages.get(), 0);
            m_allocator.initialize(POOL_RENDERING_COMMANDS, pageSize, numPreallocatedPages, numRetainedPages);
        }

    private:
        base::mem::PageAllocator m_allocator;

        virtual void deinit() override
        {
        }
    };

} // helper

//--

#if 0
GPUCommandBuffer::GPUCommandBuffer(base::mem::PageCollection* pages, base::StringView name, uint8_t* firstPagePtr, uint8_t* firstPageEndPtr)
    : m_pages(pages)
    , m_currentWritePtr(firstPagePtr)
    , m_currentWriteEndPtr(firstPageEndPtr)
{
    // write the "hello" command
    auto* helloCmd = (OpHello*)m_currentWritePtr;
    m_commands = m_lastCommand = helloCmd->setup(GPUCommandCode::Hello);

    // set the string
    memcpy(helloCmd->name, name.data(), name.length());
    helloCmd->name[name.length()] = 0;
    m_currentWritePtr += sizeof(OpHello) + name.length();

    // write the "end of buffer" guard
    auto* endCmd = (OpNewBuffer*)m_currentWritePtr;
    endCmd->setup(GPUCommandCode::NewBuffer, helloCmd);
}
#endif

GPUCommandBuffer::GPUCommandBuffer()
{
    // create page allocator
    auto& pageAllocator = helper::CommandBuferPageAllocator::GetInstance().allocator();
    m_pages = base::mem::PageCollection::CreateFromAllocator(pageAllocator);

    // allocate first page
    m_currentWritePtr = (uint8_t*) m_pages->allocatePage();
    m_currentWriteEndPtr = m_currentWritePtr + m_pages->pageSize();

    // write the "hello" command
    auto* helloCmd = (OpHello*)m_currentWritePtr;
    m_commands = m_lastCommand = helloCmd->setup(GPUCommandCode::Hello);
    m_currentWritePtr += sizeof(OpHello);

    // write the "end of buffer" guard
    auto* endCmd = (OpNewBuffer*)m_currentWritePtr;
    endCmd->setup(GPUCommandCode::NewBuffer, helloCmd);
}

GPUCommandBuffer::~GPUCommandBuffer()
{}

void GPUCommandBuffer::finishRecording()
{
    const auto wasFinished = (0 != m_finished.exchange(1));
    DEBUG_CHECK_EX(!wasFinished, "Buffer already finished recording");
}

void GPUCommandBuffer::release()
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

    DEBUG_CHECK_EX(m_writingThread == 0, "There is still a thread with GPUCommandWriter that opened this command buffer for writing");
    m_pages->reset(); // this releases memory for this command buffer as well

    this->~GPUCommandBuffer();
    base::mem::FreeBlock(this);
}

bool GPUCommandBuffer::enumChildren(const std::function<bool(GPUCommandBuffer * buffer)>& enumFunc)
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

bool GPUCommandBuffer::visitHierarchy(const std::function<bool(GPUCommandBuffer * buffer)>& enumFunc)
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

GPUCommandBuffer* GPUCommandBuffer::Alloc()
{
    return new GPUCommandBuffer;

    /*if (auto* pages = base::mem::PageCollection::CreateFromAllocator(pageAllocator))
    {
        if (pages->pageSize() >= 4096)
        {
            // get the first memory range
            auto* ptr = (uint8_t*)pages->allocatePage();
            auto* ptrEnd = ptr + pages->pageSize() - sizeof(OpNewBuffer);

            // allocate memory for the command buffer object
            auto* mem = base::AlignPtr(ptr, alignof(GPUCommandBuffer));
            ptr = mem + sizeof(GPUCommandBuffer);

            // create the command buffer object and pass rest of the page for it to use
            return new (mem) GPUCommandBuffer(pages, name, ptr, ptrEnd);
        }
    }

    return nullptr; // OOM ?*/
}

void GPUCommandBuffer::beginWriting()
{
    DEBUG_CHECK_EX(m_writingThread == 0, base::TempString("Command buffer is already being written to by thread '{}'", m_writingThread));
    m_writingThread = base::GetCurrentThreadID();
}

void GPUCommandBuffer::endWriting()
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
#include "renderingCommandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE
    return true;
}

void ValidateCommandBuffer(GPUCommandBuffer* buffer)
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

END_BOOMER_NAMESPACE(rendering)