/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: stubs #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

// helper class responsible for allocating stubs
class BASE_OBJECT_API StubFactory : public NoCopy
{
public:
	StubFactory();
	~StubFactory();

	// register a factory function for given stub type
	void registerType(StubTypeValue id, uint32_t size, uint32_t alignment, const std::function<void(void*)>& initStubFunc, const std::function<void(void*)>& cleanupStubFunc);

	// register a stub class type when class is directly known
	template< typename T >
	INLINE void registerTypeNoDestructor()
	{
		registerType(T::StaticType(), sizeof(T), __alignof(T),
			[](void* ptr) { new (ptr) T(); },
			{} );
	}

	// register a stub class type when class is directly known
	template< typename T >
	INLINE void registerTypeWithDescrutor()
	{
		registerType(T::StaticType(), sizeof(T), __alignof(T),
			[](void* ptr) { new (ptr) T(); },
			[](void* ptr) { ((T*)ptr)->~T(); });
	}

	struct StubClass
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		uint32_t size = 0;
		uint32_t alignment = 0;
		std::function<void(void*)> initFunc;
		std::function<void(void*)> cleanupFunc;
	};

	// fetch class information
	const StubClass* classInfo(StubTypeValue id) const;

protected:
	Array<StubClass*> m_classes;
};

//---

END_BOOMER_NAMESPACE(base)