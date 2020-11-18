/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#pragma once

#include "base/reflection/include/variantTable.h"
#include "rendering/device/include/renderingDeviceObject.h"

namespace rendering
{
    namespace scene
    {
        //---

		// data holder for frame 2 frame stuff, cached engine side
		class RENDERING_SCENE_API CameraContext : public base::IReferencable
		{
		public:
			CameraContext(const base::StringBuf& cameraName = "Camera");
            ~CameraContext();

			/// name of the rendering camera (good for hacks :P)
			INLINE const base::StringBuf& name() const { return m_name; }

			//--

		private:
			base::StringBuf m_name;

			ImageView m_currentFrameLuminance;
			ImageView m_prevFrameLuminance;
			ImageView m_temporalColorFeedback;
		};

		//---

	} // scene
} // rendering

