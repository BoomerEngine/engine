/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

// The Canvas class is heavily based on nanovg project by Mikko Mononen
// Adaptations were made to fit the rest of the source code in here

//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#pragma once
#include "canvasStyle.h"

namespace base
{
    namespace canvas
    {
		//--

#pragma pack(push)
#pragma pack(4)
		struct Attributes
		{
			Vector2 base;
			Vector2 extent;
			Color innerColor;
			Color outerColor;
			float radius = 0.0f;
			float feather = 0.0f;
			float lineWidth = 0.0f;

			//--

			bool operator==(const Attributes& other) const;
			bool operator!=(const Attributes& other) const;

			static uint32_t CalcHash(const Attributes& style);

		private:
			uint32_t _padding0 = 0;
		};
#pragma pack(pop)

		static_assert(sizeof(Attributes) == 40, "Make sure there are no holes");

		//--

        /// Simple structure to hold renderable canvas geometry (vertices + draw commands)
		/// Can be placed (instanced) in canvas with a custom transformation
		/// NOTE: data cached here depends on the particular UVs and atlas placements or images in the storage!
        struct BASE_CANVAS_API Geometry
        {
            RTTI_DECLARE_POOL(POOL_CANVAS)

        public:
            Geometry();
			Geometry(const Geometry& other);
			Geometry(Geometry&& other);
			Geometry& operator=(const Geometry& other);
			Geometry& operator=(Geometry&& other);
			~Geometry();

            //--

            INLINE bool empty() const { return batches.empty(); }
			INLINE operator bool() const { return !batches.empty(); }

            //--

			// reset geometry without freeing memory
			void reset();

			// count used memory
			uint32_t calcMemorySize() const;

			//--

			Vector2 boundsMin;
			Vector2 boundsMax;

			Array<Vertex> vertices;
			Array<Batch> batches;
			Array<Attributes> attributes;
			Array<uint8_t> customData;

			//--
			
			// directly append a batch to the geometry
			void appendVertexBatch(const Vertex* vertices, uint32_t numVertices, const Batch& setup = Batch(), const RenderStyle* style = nullptr);

			// directly append a batch to the geometry indexed triangle list batch to the geometry
			void appendIndexedBatch(const Vertex* vertices, const uint16_t* indices, uint32_t numIndices, const Batch& setup = Batch(), const RenderStyle* style = nullptr);

			//--

		private:
			int appendStyle(const RenderStyle& style);
			void applyStyle(Vertex* vertices, uint32_t numVertices, const RenderStyle& style);
        };

		//--

    } // canvas
} // base
