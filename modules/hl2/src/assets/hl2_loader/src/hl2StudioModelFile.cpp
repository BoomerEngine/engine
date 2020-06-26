/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2StudioModelFile.h"

namespace hl2
{
    namespace mdl
    {

// Insert this code anywhere that you need to allow for conversion from an old STUDIO_VERSION
// to a new one.
// If we only support the current version, this function should be empty.
        inline bool Studio_ConvertStudioHdrToNewVersion( studiohdr_t *pStudioHdr )
        {
            //( STUDIO_VERSION == 48 ); //  put this to make sure this code is updated upon changing version.

            if (pStudioHdr->studiohdr2index == -1)
                pStudioHdr->studiohdr2index = 0;

            int version = pStudioHdr->version;
            if ( version == STUDIO_VERSION )
                return true;

            bool bResult = true;
            if (version < 46)
            {
                // some of the anim index data is incompatible
                for (int i = 0; i < pStudioHdr->numlocalanim; i++)
                {
                    mstudioanimdesc_t *pAnim = (mstudioanimdesc_t *)pStudioHdr->pLocalAnimdesc( i );

                    // old ANI files that used sections (v45 only) are not compatible
                    if ( pAnim->sectionframes != 0 )
                    {
                        // zero most everything out
                        memset( &(pAnim->numframes), 0, (uint8_t *)(pAnim + 1) - (uint8_t *)&(pAnim->numframes) );

                        pAnim->numframes = 1;
                        pAnim->animblock = -1; // disable animation fetching
                        bResult = false;
                    }
                }
            }

            if (version < 47)
            {
                // used to contain zeroframe cache data
                if (pStudioHdr->unused4 != 0)
                {
                    pStudioHdr->unused4 = 0;
                    bResult = false;
                }
                for (int i = 0; i < pStudioHdr->numlocalanim; i++)
                {
                    mstudioanimdesc_t *pAnim = (mstudioanimdesc_t *)pStudioHdr->pLocalAnimdesc( i );
                    pAnim->zeroframeindex = 0;
                    pAnim->zeroframespan = 0;
                }
            }
            else if (version == 47)
            {
                for (int i = 0; i < pStudioHdr->numlocalanim; i++)
                {
                    mstudioanimdesc_t *pAnim = (mstudioanimdesc_t *)pStudioHdr->pLocalAnimdesc( i );
                    if (pAnim->zeroframeindex != 0)
                    {
                        pAnim->zeroframeindex = 0;
                        pAnim->zeroframespan = 0;
                        bResult = false;
                    }
                }
            }

            // for now, just slam the version number since they're compatible
            TRACE_INFO("Original file version: {}", pStudioHdr->version);
            pStudioHdr->version = STUDIO_VERSION;

            return bResult;
        }

        bool ValidateFile(const studiohdr_t& file)
        {
            if (file.version > STUDIO_VERSION)
            {
                TRACE_ERROR("Studio model file is in unsupported version {}", file.version);
                return false;
            }

            return Studio_ConvertStudioHdrToNewVersion(const_cast<studiohdr_t*>(&file));
        }

    } // mdl
} // hl2