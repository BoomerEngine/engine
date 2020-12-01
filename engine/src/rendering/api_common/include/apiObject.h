/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingObjectID.h"

namespace rendering
{
    namespace api
    {
        //--

        // lower class for the API-side object
		// NOTE: API objects are NOT reference counted, they are destroyed once host-side object dies and the frame that happens finishes
        class RENDERING_API_COMMON_API IBaseObject : public base::NoCopy
        {
			RTTI_DECLARE_POOL(POOL_API_OBJECTS)

        public:
			IBaseObject(IBaseThread* drv, ObjectType type);
            virtual ~IBaseObject(); // called on rendering thread once all frames this object was used are done rendering

			// object owner - thread that created it
			INLINE IBaseThread* owner() const { return m_owner; }

			// type of the object
            INLINE ObjectType objectType() const { return m_type; }

			// handle to this object
            INLINE ObjectID handle() const { return m_handle; }

			//--

			// calculate or estimate GPU memory usage of this object
			virtual uint64_t calcMemoryUsage() const { return 0; }

			// describe this object in more details
			virtual void additionalPrint(ImageFormat& f) const { }

			// can this object be deleted ?
			// TODO: add internal flag if used more often
			INLINE bool canDelete() const { return m_type != ObjectType::OutputRenderTargetView; }

			// called THE MOMENT client releases last reference to the client-side object
			// NOTE: this may be called from any thread!
			virtual void disconnectFromClient() {};

			//--

			virtual const IBaseCopiableObject* toCopiable() const { return nullptr; }
			virtual IBaseCopiableObject* toCopiable() { return nullptr; }

			//--

			void print(base::IFormatStream& f) const;

        private:
			IBaseThread* m_owner = nullptr;
            ObjectType m_type = ObjectType::Unknown;
            ObjectID m_handle;
        };

        //--

		// object + async copy interface
		class RENDERING_API_COMMON_API IBaseCopiableObject : public IBaseObject
		{
			RTTI_DECLARE_POOL(POOL_API_OBJECTS)

		public:
			IBaseCopiableObject(IBaseThread* drv, ObjectType type);
			virtual ~IBaseCopiableObject(); // called on rendering thread once all frames this object was used are done rendering

			// generate atoms for async update of this resource 
			// Atoms are the independent regions of resource that should be copied to
			virtual bool generateCopyAtoms(const ResourceCopyRange& range, base::Array<ResourceCopyAtom>& outAtoms, uint32_t& outStagingAreaSize, uint32_t& outStagingAreaAlignment) const = 0;

			// apply copied content (in form of atoms) to this resource
			virtual void applyCopyAtoms(const base::Array<ResourceCopyAtom>& atoms, Frame* frame, const StagingArea& area) = 0;

			///--

			virtual const IBaseCopiableObject* toCopiable() const override final { return this; }
			virtual IBaseCopiableObject* toCopiable()  override final { return this; }

			///--
		};

		//--

    } // gl4
} // rendering