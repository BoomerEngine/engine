/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: services #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"

BEGIN_BOOMER_NAMESPACE(base::app)

class App;
struct AppInitContext;

// Results for service initialization
enum class ServiceInitializationResult
{
    // module finished initialization, no more work for us
    Finished = 0,

    // module failed initialization and the app cannot continue
    FatalError = 1,

    // module failed initialization but the app can continue
    // service will be unavailable
    Silenced = 2,
};

//--

// Service metadata to specify what is the service's dependency
class BASE_APP_API DependsOnServiceMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(DependsOnServiceMetadata, rtti::IMetadata);

public:
    DependsOnServiceMetadata();

    // add class to list
    template< typename T >
    INLINE DependsOnServiceMetadata& dependsOn()
    {
        m_classList.pushBackUnique(T::GetStaticClass());
		return *this;
    }

    // get list of classes the service having this metadata depends on
    INLINE const base::Array<ClassType>& classes() const
    {
        return m_classList;
    }

private:
    base::Array<ClassType> m_classList;
};

//--

// Service metadata to specify what other service must tick before we can tick
class BASE_APP_API TickBeforeMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(TickBeforeMetadata, rtti::IMetadata);

public:
    TickBeforeMetadata();

    // add class to list
    template< typename T >
    INLINE TickBeforeMetadata& tickBefore()
    {
        m_classList.pushBackUnique(T::GetStaticClass());
		return *this;
    }

    // get list of classes the service having this metadata depends on
    INLINE const base::Array<ClassType>& classes() const
    {
        return m_classList;
    }

private:
    base::Array<ClassType> m_classList;
};

//--

// Service metadata to specify what other service must tick after we can tick
class BASE_APP_API TickAfterMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(TickAfterMetadata, rtti::IMetadata);

public:
    TickAfterMetadata();

    // add class to list
    template< typename T >
    INLINE TickAfterMetadata& tickAfter()
    {
        m_classList.pushBackUnique(T::GetStaticClass());
		return *this;
    }

    // get list of classes the service having this metadata depends on
    INLINE const base::Array<ClassType>& classes() const
    {
        return m_classList;
    }

private:
    base::Array<ClassType> m_classList;
};

//--

// application service, created with the application, accessible via the app handle
// app->GetService<DepotService>()->LoadResource, 
// app->GetService<RendererService>()->CreateFrame, etc
// the services are NOT ticked with any time delta related to the project(s)/game(s) running, they are updated periodically though
// services can be used either directly or an per-game/per-project instance of something can be created (like creating a PhysicsScene from PhysicsService for game simulation)
// NOTE: some services may refuse to be created under some conditions
// NOTE: services may have dependencies on each other but this does not determine update order or anything (usually it does not have to)
// NOTE: services are object in order to be passable to scripts
// NOTE: services are very long lived objects
class BASE_APP_API ILocalService : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ILocalService, IObject);

public:
    ILocalService();
    virtual ~ILocalService();

    /// application is initializing, service can perform a self contained initialization that does not depend on any other service
    /// service can emit background initialization jobs (like connecting to remote server) that can continue once we enter next application state
    virtual ServiceInitializationResult onInitializeService(const CommandLine& cmdLine) = 0;

    /// application is shutting down, last chance for cleanup
    /// this method is called in order determined by the resolve steps based on OnResolveShutdownDependency
    /// note: this function MAY be called while the background initialization jobs are processing
    virtual void onShutdownService() = 0;

    /// update service, called from main platform thread every tick
    /// simulated games are GUARANTEED not to have any async update at this time
    /// this is the safest place in the whole universe ;)
    virtual void onSyncUpdate() = 0;
};

END_BOOMER_NAMESPACE(base::app)