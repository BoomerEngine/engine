/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4ComputePipeline.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)
					if (glProgram != glActiveProgram)
					{
						GL_PROTECT(glBindProgramPipeline(glProgram));
						glActiveProgram = glProgram;
					}

					return true;
				}

				return false;
			}

			//--

		} // gl4
    } // gl4
} // rendering
