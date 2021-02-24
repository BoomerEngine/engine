/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4VertexLayout.h"
#include "gl4Utils.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)
					GL_PROTECT(glDeleteVertexArrays(1, &m_glVertexLayout));
					m_glVertexLayout = 0;
				}
			}

			GLuint VertexBindingLayout::object()
			{
				if (m_glVertexLayout)
					return m_glVertexLayout;

				// create the VAO
				PC_SCOPE_LVL0(CreateVertexLayout);
				GL_PROTECT(glCreateVertexArrays(1, &m_glVertexLayout));

				// create the attribute mapping from all active streams
				uint32_t attributeIndex = 0;
				for (uint32_t i : vertexStreams().indexRange())
				{
					const auto& vertexStream = vertexStreams()[i];

					GL_PROTECT(glVertexArrayBindingDivisor(m_glVertexLayout, i, vertexStream.instanced ? 1 : 0));

					for (uint32_t j : vertexStream.elements.indexRange())
					{
						const auto& vertexAttrib = vertexStream.elements[j];

						const auto glFormat = TranslateImageFormat(vertexAttrib.format);
						const auto& formatInfo = GetImageFormatInfo(vertexAttrib.format);

						// convert to old school format
						GLenum glBaseFormat = 0;
						GLuint glNumComponents = 0;
						GLboolean glFormatNormalized = GL_FALSE;
						DecomposeVertexFormat(glFormat, glBaseFormat, glNumComponents, glFormatNormalized);


						if (formatInfo.formatClass == ImageFormatClass::INT || formatInfo.formatClass == ImageFormatClass::UINT)
						{
							GL_PROTECT(glVertexArrayAttribIFormat(m_glVertexLayout, attributeIndex, glNumComponents, glBaseFormat, vertexAttrib.offset));
						}
						else
						{
                            GL_PROTECT(glVertexArrayAttribFormat(m_glVertexLayout, attributeIndex, glNumComponents, glBaseFormat, glFormatNormalized, vertexAttrib.offset));

						}

						GL_PROTECT(glVertexArrayAttribBinding(m_glVertexLayout, attributeIndex, i)); // attribute {attributeIndex} in buffer {i}
						GL_PROTECT(glEnableVertexArrayAttrib(m_glVertexLayout, attributeIndex)); // we use consecutive attribute indices
						attributeIndex += 1;
					}
				}

				return m_glVertexLayout;
			}

			//--

		} // gl4
    } // gl4
} // rendering
