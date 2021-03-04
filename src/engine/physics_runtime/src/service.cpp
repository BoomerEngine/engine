/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "service.h"
#include "scene.h"
#include "core/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE()

//----

PhysicsSceneDesc::PhysicsSceneDesc()
    : m_type(PhysicsSceneType::Game)
{}

//----

class EngineAllocatorCallback : public physx::PxAllocatorCallback
{
public:
    virtual void* allocate(size_t size, const char*, const char*, int) override
    {
        return GlobalPool<POOL_PHYSICS_RUNTIME>::Alloc(size, 16);
    }

    virtual void deallocate(void* ptr)
    {
        GlobalPool<POOL_PHYSICS_RUNTIME>::Free(ptr);
    }
};

class EngineErrorCallback : public physx::PxErrorCallback
{
public:
    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        if (0 != (code & physx::PxErrorCode::eOUT_OF_MEMORY))
        {
            FATAL_ERROR(TempString("{}({}): OutOfPhysicsMemory: {}", file, line, message));
        }
        else if (0 != (code & physx::PxErrorCode::eINTERNAL_ERROR))
        {
            FATAL_ERROR(TempString("{}({}): InternalPhysicsError: {}", file, line, message));
        }
        else if (0 != (code & physx::PxErrorCode::eABORT))
        {
            FATAL_ERROR(TempString("{}({}): PhysicsAbort: {}", file, line, message));
            //logging::Log::Print(logging::OutputLevel::Error, file, line, nullptr, message);
        }
        else if (0 != (code & physx::PxErrorCode::eINVALID_PARAMETER))
        {
            //TRACE_ERROR(TempString("{}({}): Error: {}", file, line, message));
            logging::Log::Print(logging::OutputLevel::Info, file, line, nullptr, nullptr, message);
        }
        else if (0 != (code & physx::PxErrorCode::eINVALID_OPERATION))
        {
            //TRACE_ERROR(TempString("{}({}): Error: {}", file, line, message));
            logging::Log::Print(logging::OutputLevel::Error, file, line, nullptr, nullptr, message);
        }
        else if (0 != (code & physx::PxErrorCode::eDEBUG_WARNING))
        {
            //TRACE_WARNING(TempString("{}({}): Warning: {}", file, line, message));
            logging::Log::Print(logging::OutputLevel::Warning, file, line, nullptr, nullptr, message);
        }
        else if (0 != (code & physx::PxErrorCode::eDEBUG_INFO))
        {
            //TRACE_INFO(TempString("{}({}): {}", file, line, message));
            logging::Log::Print(logging::OutputLevel::Info, file, line, nullptr, nullptr, message);
        }
    }
};

//----

RTTI_BEGIN_TYPE_CLASS(PhysicsService);
RTTI_END_TYPE();

PhysicsService::PhysicsService()
    : m_foundation(nullptr)
    , m_pvd(nullptr)
    , m_physics(nullptr)
{}

static EngineErrorCallback theErrorCallback;
static EngineAllocatorCallback theAllocatorCallback;

app::ServiceInitializationResult PhysicsService::onInitializeService(const app::CommandLine& cmdLine)
{
    // create the PhysX foundation
    TRACE_INFO("Compiled with PhysX version {}.{}.{}", PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX);
    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, theAllocatorCallback, theErrorCallback);
    if (!m_foundation)
    {
        TRACE_ERROR("Failed to create PhysX Foundation class!");
        return app::ServiceInitializationResult::FatalError;
    }

    // create the PVD interface
#ifndef BUILD_RELEASE
    m_pvd = PxCreatePvd(*m_foundation);
    if (!m_pvd)
    {
        TRACE_WARNING("Unable to create PVD interface, debug connections will not be possible");
    }
    else
    {
        auto transport  = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        if (!m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL))
        {
            TRACE_WARNING("Failed to connect to PVD");
        }
        else
        {
            TRACE_INFO("Connected to PVD");
        }
    }
#endif

    // should we track allocations ?
#ifdef BUILD_DEBUG
    auto trackAllocations = cmdLine.singleValueBool("trackPhysXAllocations", true);
#else
    auto trackAllocations = cmdLine.singleValueBool("trackPhysXAllocations", false);
#endif
    // setup tolerances
    physx::PxTolerancesScale tolerances;

    // create physics interface
    m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, tolerances, trackAllocations, m_pvd);
    if (!m_physics)
    {
        TRACE_ERROR("Failed to create PhysX Physics interface");
        return app::ServiceInitializationResult::FatalError;
    }

    // PhysX initialized
    return app::ServiceInitializationResult::Finished;
}

void PhysicsService::onShutdownService()
{
    // close physics
    if (m_physics)
    {
        TRACE_INFO("Closing PhysX Physics...");
//                m_physics->release();
        m_physics = nullptr;
        TRACE_INFO("Closed PhysX Physics");
    }

    // close the PVD
    if (m_pvd)
    {
        TRACE_INFO("Closing PhysX PVD...");
        m_pvd->release();
        m_pvd = nullptr;
        TRACE_INFO("Closed PhysX PVD");
    }

    // close the foundation object
    if (m_foundation)
    {
        TRACE_INFO("Closing PhysX Foundation...");
        m_foundation->release();
        m_foundation = nullptr;
        TRACE_INFO("Closed PhysX Foundation");
    }
}

void PhysicsService::onSyncUpdate()
{}

PhysicsScenePtr PhysicsService::createScene(const PhysicsSceneDesc& desc)
{
    return CreateUniquePtr<PhysicsScene>(nullptr);
}

END_BOOMER_NAMESPACE()
