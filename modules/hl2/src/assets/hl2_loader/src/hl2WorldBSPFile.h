/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace hl2
{
    namespace bsp
    {
        //--

        // ------------------------------------------------------------------------------------------------ //
        // Displacement neighbor rules
        // ------------------------------------------------------------------------------------------------ //
        //
        // Each displacement is considered to be in its own space:
        //
        //               NEIGHBOREDGE_TOP
        //
        //                   1 --- 2
        //                   |     |
        // NEIGHBOREDGE_LEFT |     | NEIGHBOREDGE_RIGHT
        //                   |     |
        //                   0 --- 3
        //
        //   			NEIGHBOREDGE_BOTTOM
        //
        //
        // Edge edge of a displacement can have up to two neighbors. If it only has one neighbor
        // and the neighbor fills the edge, then SubNeighbor 0 uses CORNER_TO_CORNER (and SubNeighbor 1
        // is undefined).
        //
        // CORNER_TO_MIDPOINT means that it spans [bottom edge,midpoint] or [left edge,midpoint] depending
        // on which edge you're on.
        //
        // MIDPOINT_TO_CORNER means that it spans [midpoint,top edge] or [midpoint,right edge] depending
        // on which edge you're on.
        //
        // Here's an illustration (where C2M=CORNER_TO_MIDPOINT and M2C=MIDPOINT_TO_CORNER
        //
        //
        //				 C2M			  M2C
        //
        //       1 --------------> x --------------> 2
        //
        //       ^                                   ^
        //       |                                   |
        //       |                                   |
        //  M2C  |                                   |	M2C
        //       |                                   |
        //       |                                   |
        //
        //       x                 x                 x
        //
        //       ^                                   ^
        //       |                                   |
        //       |                                   |
        //  C2M  |                                   |	C2M
        //       |                                   |
        //       |                                   |
        //
        //       0 --------------> x --------------> 3
        //
        //               C2M			  M2C
        //
        //
        // The CHILDNODE_ defines can be used to refer to a node's child nodes (this is for when you're
        // recursing into the node tree inside a displacement):
        //
        // ---------
        // |   |   |
        // | 1 | 0 |
        // |   |   |
        // |---x---|
        // |   |   |
        // | 2 | 3 |
        // |   |   |
        // ---------
        //
        // ------------------------------------------------------------------------------------------------ //

        // These can be used to index g_ChildNodeIndexMul.
        enum
        {
            CHILDNODE_UPPER_RIGHT=0,
            CHILDNODE_UPPER_LEFT=1,
            CHILDNODE_LOWER_LEFT=2,
            CHILDNODE_LOWER_RIGHT=3
        };

        // Corner indices. Used to index m_CornerNeighbors.
        enum
        {
            CORNER_LOWER_LEFT=0,
            CORNER_UPPER_LEFT=1,
            CORNER_UPPER_RIGHT=2,
            CORNER_LOWER_RIGHT=3
        };


        // These edge indices must match the edge indices of the CCoreDispSurface.
        enum
        {
            NEIGHBOREDGE_LEFT=0,
            NEIGHBOREDGE_TOP=1,
            NEIGHBOREDGE_RIGHT=2,
            NEIGHBOREDGE_BOTTOM=3
        };

        // These denote where one dispinfo fits on another.
        // Note: tables are generated based on these indices so make sure to update
        //       them if these indices are changed.
        enum NeighborSpan
        {
            CORNER_TO_CORNER=0,
            CORNER_TO_MIDPOINT=1,
            MIDPOINT_TO_CORNER=2
        };

        // These define relative orientations of displacement neighbors.
        enum NeighborOrientation
        {
            ORIENTATION_CCW_0=0,
            ORIENTATION_CCW_90=1,
            ORIENTATION_CCW_180=2,
            ORIENTATION_CCW_270=3
        };

        //--

        enum Lump
        {
            LUMP_ENTITIES		= 0,		// *
            LUMP_PLANES			= 1,		// *
            LUMP_TEXDATA		= 2,		//
            LUMP_VERTEXES		= 3,		// *
            LUMP_VISIBILITY		= 4,		// *
            LUMP_NODES			= 5,		// *
            LUMP_TEXINFO		= 6,		// *
            LUMP_FACES			= 7,		// *
            LUMP_LIGHTING		= 8,		// *
            LUMP_OCCLUSION		= 9,
            LUMP_LEAFS			= 10,		// *
            LUMP_EDGES			= 12,		// *
            LUMP_SURFEDGES		= 13,		// *
            LUMP_MODELS			= 14,		// *
            LUMP_WORLDLIGHTS	= 15,		//
            LUMP_LEAFFACES		= 16,		// *
            LUMP_LEAFBRUSHES	= 17,		// *
            LUMP_BRUSHES		= 18,		// *
            LUMP_BRUSHSIDES		= 19,		// *
            LUMP_AREAS			= 20,		// *
            LUMP_AREAPORTALS	= 21,		// *
            LUMP_PORTALS		= 22,
            LUMP_CLUSTERS		= 23,
            LUMP_PORTALVERTS	= 24,
            LUMP_CLUSTERPORTALS = 25,
            LUMP_DISPINFO		= 26,
            LUMP_ORIGINALFACES	= 27,
            LUMP_PHYSCOLLIDE	= 29,
            LUMP_VERTNORMALS	= 30,
            LUMP_VERTNORMALINDICES		= 31,
            LUMP_DISP_LIGHTMAP_ALPHAS	= 32,
            LUMP_DISP_VERTS = 33,						// CDispVerts
            LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
            LUMP_GAME_LUMP = 35,
            LUMP_LEAFWATERDATA	= 36,
            LUMP_PRIMITIVES		= 37,
            LUMP_PRIMVERTS		= 38,
            LUMP_PRIMINDICES	= 39,
            LUMP_PAKFILE		= 40,
            LUMP_CLIPPORTALVERTS= 41,
            LUMP_CUBEMAPS		= 42,
            LUMP_TEXDATA_STRING_DATA	= 43,
            LUMP_TEXDATA_STRING_TABLE	= 44,
            LUMP_OVERLAYS				= 45,
            LUMP_LEAFMINDISTTOWATER		= 46,
            LUMP_FACE_MACRO_TEXTURE_INFO = 47,
            LUMP_DISP_TRIS = 48
        };

        enum ContentFlag
        {
            CONTENTS_EMPTY = 0,        // No contents
            CONTENTS_SOLID = 0x1,        // an eye is never valid in a solid
            CONTENTS_WINDOW = 0x2,        // translucent, but not watery (glass)
            CONTENTS_AUX = 0x4,
            CONTENTS_GRATE = 0x8,    // alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
            CONTENTS_SLIME = 0x10,
            CONTENTS_WATER = 0x20,
            CONTENTS_MIST = 0x40,
            CONTENTS_OPAQUE = 0x80,    // things that cannot be seen through (may be non-solid though)
            LAST_VISIBLE_CONTENTS = 0x80,

            ALL_VISIBLE_CONTENTS = (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS - 1)),

            CONTENTS_TESTFOGVOLUME = 0x100,

            // unused
            // NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
            // if not visible, then grab from the bottom.
            CONTENTS_UNUSED3 = 0x200,
            CONTENTS_UNUSED4 = 0x400,
            CONTENTS_UNUSED5 = 0x800,
            CONTENTS_UNUSED6 = 0x1000,
            CONTENTS_UNUSED7 = 0x2000,


            // hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
            CONTENTS_MOVEABLE = 0x4000,

            // remaining contents are non-visible, and don't eat brushes
            CONTENTS_AREAPORTAL = 0x8000,

            CONTENTS_PLAYERCLIP = 0x10000,
            CONTENTS_MONSTERCLIP = 0x20000,

            // currents can be added to any other contents, and may be mixed
            CONTENTS_CURRENT_0 = 0x40000,
            CONTENTS_CURRENT_90 = 0x80000,
            CONTENTS_CURRENT_180 = 0x100000,
            CONTENTS_CURRENT_270 = 0x200000,
            CONTENTS_CURRENT_UP = 0x400000,
            CONTENTS_CURRENT_DOWN = 0x800000,

            CONTENTS_ORIGIN = 0x1000000,    // removed before bsping an entity

            CONTENTS_MONSTER = 0x2000000,    // should never be on a brush, only in game
            CONTENTS_DEBRIS = 0x4000000,
            CONTENTS_DETAIL = 0x8000000,    // brushes to be added after vis leafs
            CONTENTS_TRANSLUCENT = 0x10000000,    // auto set if any surface has trans
            CONTENTS_LADDER = 0x20000000,
            CONTENTS_HITBOX = 0x40000000,    // use accurate hitboxes on trace
        };
        
        enum SurfFlag
        {
            SURF_LIGHT = 0x0001,        // value will hold the light strength
            SURF_SLICK = 0x0002,        // effects game physics
            SURF_SKY = 0x0004,        // don't draw, but add to skybox
            SURF_WARP = 0x0008,        // turbulent water warp
            SURF_TRANS = 0x0010,
            SURF_WET = 0x0020,    // the surface is wet
            SURF_FLOWING = 0x0040,    // scroll towards angle
            SURF_NODRAW = 0x0080,    // don't bother referencing the texture
            SURF_HINT = 0x0100,    // make a primary bsp splitter
            SURF_SKIP = 0x0200,    // completely ignore, allowing non-closed brushes
            SURF_NOLIGHT = 0x0400,    // Don't calculate light
            SURF_BUMPLIGHT = 0x0800,    // calculate three lightmaps for the surface for bumpmapping
            SURF_HITBOX = 0x8000,    // surface is part of a hitbox
        };

        // Lumps that have versions are listed here
        enum LumpVersion
        {
            LUMP_LIGHTING_VERSION = 1,
            LUMP_FACES_VERSION = 1,
            LUMP_OCCLUSION_VERSION = 1,
        };

        //--

        struct lump_t
        {
            int		fileofs, filelen;
            int		version;		// default to zero
            char	fourCC[4];		// default to ( char )0, ( char )0, ( char )0, ( char )0
        };

        struct dheader_t
        {
            static const int HEADER_LUMPS = 64;

            int			ident;
            int			version;
            lump_t		lumps[HEADER_LUMPS];
            int			mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)
        };

        struct dgamelumpheader_t
        {
            int lumpCount;

            // dclientlump_ts follow this
        };

        // This is expected to be a four-CC code ('lump')
        typedef unsigned int GameLumpId_t;

        struct dgamelump_t
        {
            GameLumpId_t	id;
            unsigned short flags;		// currently unused, but you never know!
            unsigned short version;
            int	fileofs;
            int filelen;
        };

        extern int g_MapRevision;

        typedef base::Vector3 Vector;

        struct dmodel_t
        {
            Vector      mins, maxs;
            Vector		origin;			// for sounds or lights
            int			headnode;
            int			firstface, numfaces;	// submodels just draw faces
            // without walking the bsp tree
        };

        struct dphysmodel_t
        {
            int			modelIndex;
            int			dataSize;
            int			keydataSize;
            int			solidCount;
        };

        struct dvertex_t
        {
            Vector	point;
        };

        // planes (x&~1) and (x&~1)+1 are always opposites
        struct dplane_t
        {
            Vector	normal;
            float	dist;
            int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
        };

        struct dnode_t
        {
            int			    planenum;
            int			    children[2];	// negative numbers are -(leafs+1), not nodes
            short		    mins[3];		// for frustom culling
            short		    maxs[3];
            unsigned short	firstface;
            unsigned short	numfaces;	// counting both sides
            short			area;		// If all leaves below this node are in the same area, then
        };

        struct texinfo_t
        {
            float		textureVecsTexelsPerWorldUnits[2][4];			// [s/t][xyz offset]
            float		lightmapVecsLuxelsPerWorldUnits[2][4];			// [s/t][xyz offset] - length is in units of texels/area
            int			flags;				// miptex flags + overrides
            int			texdata;			// Pointer to texture name, size, etc.
        } ;

        struct dtexdata_t
        {
            Vector		reflectivity;
            int			nameStringTableID;				// index into g_StringTable for the texture name
            int			width, height;					// source image
            int			view_width, view_height;		//
        };

        enum
        {
            TEXTURE_NAME_LENGTH	 = 128, // changed from 64 BSPVERSION 8
            OCCLUDER_FLAGS_INACTIVE = 0x1,
            DISPTRI_TAG_SURFACE = (1<<0),
            DISPTRI_TAG_WALKABLE = (1<<1),
            DISPTRI_TAG_BUILDABLE = (1<<2),
            OVERLAY_BSP_FACE_COUNT = 64,
            MAXLIGHTMAPS = 4,
            ANGLE_UP = -1,
            ANGLE_DOWN = -2,
            DVIS_PVS = 0,
            DVIS_PAS = 1,
        };

        struct doccluderdata_t
        {
            int			flags;
            int			firstpoly;				// index into doccluderpolys
            int			polycount;
            Vector		mins;
            Vector		maxs;
        };

        struct doccluderpolydata_t
        {
            int			firstvertexindex;		// index into doccludervertindices
            int			vertexcount;
            int			planenum;
        };

        struct CDispSubNeighbor
        {
        public:

            unsigned short		GetNeighborIndex() const		{ return m_iNeighbor; }
            NeighborSpan		GetSpan() const					{ return (NeighborSpan)m_Span; }
            NeighborSpan		GetNeighborSpan() const			{ return (NeighborSpan)m_NeighborSpan; }
            NeighborOrientation	GetNeighborOrientation() const	{ return (NeighborOrientation)m_NeighborOrientation; }

            bool				IsValid() const				{ return m_iNeighbor != 0xFFFF; }
            void				SetInvalid()				{ m_iNeighbor = 0xFFFF; }


        public:
            unsigned short		m_iNeighbor;		// This indexes into ddispinfos.
            // 0xFFFF if there is no neighbor here.

            unsigned char		m_NeighborOrientation;		// (CCW) rotation of the neighbor wrt this displacement.

            // These use the NeighborSpan type.
            unsigned char		m_Span;						// Where the neighbor fits onto this side of our displacement.
            unsigned char		m_NeighborSpan;				// Where we fit onto our neighbor.
        };


        // NOTE: see the section above titled "displacement neighbor rules".
        class CDispNeighbor
        {
        public:
            void				SetInvalid()	{ m_SubNeighbors[0].SetInvalid(); m_SubNeighbors[1].SetInvalid(); }

            // Returns false if there isn't anything touching this edge.
            bool				IsValid()		{ return m_SubNeighbors[0].IsValid() || m_SubNeighbors[1].IsValid(); }


        public:
            // Note: if there is a neighbor that fills the whole side (CORNER_TO_CORNER),
            //       then it will always be in CDispNeighbor::m_Neighbors[0]
            CDispSubNeighbor	m_SubNeighbors[2];
        };

        enum
        {
            MAX_DISP_CORNER_NEIGHBORS = 4,
            MIN_MAP_DISP_POWER = 2,
            MAX_MAP_DISP_POWER = 4,
            MAX_MAP_DISPINFO = 2048,
        };

#define MAX_MAP_DISP_VERTS		( MAX_MAP_DISPINFO * ((1<<MAX_MAP_DISP_POWER)+1) * ((1<<MAX_MAP_DISP_POWER)+1) )
#define MAX_MAP_DISP_TRIS		( (1 << MAX_MAP_DISP_POWER) * (1 << MAX_MAP_DISP_POWER) * 2 )
#define NUM_DISP_POWER_VERTS(power)	( ((1 << (power)) + 1) * ((1 << (power)) + 1) )
#define NUM_DISP_POWER_TRIS(power)	( (1 << (power)) * (1 << (power)) * 2 )
#define MAX_DISPVERTS			NUM_DISP_POWER_VERTS( MAX_MAP_DISP_POWER )
#define MAX_DISPTRIS			NUM_DISP_POWER_TRIS( MAX_MAP_DISP_POWER )

        class CDispCornerNeighbors
        {
        public:
            void			SetInvalid()	{ m_nNeighbors = 0; }

        public:
            unsigned short	m_Neighbors[MAX_DISP_CORNER_NEIGHBORS];	// indices of neighbors.
            unsigned char	m_nNeighbors;
        };


        class CDispVert
        {
        public:
            Vector		m_vVector;		// Vector field defining displacement volume.
            float		m_flDist;		// Displacement distances.
            float		m_flAlpha;		// "per vertex" alpha values.
        };


#define PAD_NUMBER(number, boundary) \
	( ((number) + ((boundary)-1)) / (boundary) ) * (boundary)

        class CDispTri
        {
        public:
            unsigned short m_uiTags;		// Displacement triangle tags.
        };

        class ddispinfo_t
        {
        public:
            int			NumVerts() const		{ return NUM_DISP_POWER_VERTS(power); }
            int			NumTris() const			{ return NUM_DISP_POWER_TRIS(power); }

        public:
            Vector		startPosition;						// start position used for orientation -- (added BSPVERSION 6)
            int			m_iDispVertStart;					// Index into LUMP_DISP_VERTS.
            int			m_iDispTriStart;					// Index into LUMP_DISP_TRIS.

            int         power;                              // power - indicates size of map (2^power + 1)
            int         minTess;                            // minimum tesselation allowed
            float       smoothingAngle;                     // lighting smoothing angle
            int         contents;                           // surface contents

            unsigned short	m_iMapFace;						// Which map face this displacement comes from.

            int			m_iLightmapAlphaStart;				// Index into ddisplightmapalpha.
            // The count is m_pParent->lightmapTextureSizeInLuxels[0]*m_pParent->lightmapTextureSizeInLuxels[1].

            int			m_iLightmapSamplePositionStart;		// Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.

            CDispNeighbor			m_EdgeNeighbors[4];		// Indexed by NEIGHBOREDGE_ defines.
            CDispCornerNeighbors	m_CornerNeighbors[4];	// Indexed by CORNER_ defines.

            enum { ALLOWEDVERTS_SIZE = PAD_NUMBER( MAX_DISPVERTS, 32 ) / 32 };
            unsigned long	m_AllowedVerts[ALLOWEDVERTS_SIZE];	// This is built based on the layout and sizes of our neighbors
            // and tells us which vertices are allowed to be active.
        };

        // note that edge 0 is never used, because negative edge nums are used for
        // counterclockwise use of the edge in a face
        struct dedge_t
        {
            unsigned short	v[2];		// vertex numbers
        };

        struct dprimitive_t
        {
            unsigned char type;
            unsigned short	firstIndex;
            unsigned short	indexCount;
            unsigned short	firstVert;
            unsigned short	vertCount;
        };

        struct dprimvert_t
        {
            Vector		pos;
        };

        enum dprimitive_type
        {
            PRIM_TRILIST=0,
            PRIM_TRISTRIP=1,
        };

        struct colorRGBExp32
        {
            unsigned char r, g, b;
            signed char exponent;
        };

        struct colorVec
        {
            unsigned r, g, b, a;
        };

        struct dface_t
        {
            // BSP_VERSION_CHANGE: Start removing here if you up the BSP version to 19
            // For computing lighting information (R_LightVec)
            //colorRGBExp32	m_AvgLightColor[MAXLIGHTMAPS];
            // BSP_VERSION_CHANGE: Stop removing here if you up the BSP version to 19

            unsigned short	planenum;
            unsigned char side;	// faces opposite to the node's plane direction
            unsigned char onNode; // 1 of on node, 0 if in leaf

            int			firstedge;		// we must support > 64k edges
            short		numedges;
            short		texinfo;
            // This is a union under the assumption that a fog volume boundary (ie. water surface)
            // isn't a displacement map.
            // FIXME: These should be made a union with a flags or type field for which one it is
            // if we can add more to this.
//	union
//	{
            short       dispinfo;
            // This is only for surfaces that are the boundaries of fog volumes
            // (ie. water surfaces)
            // All of the rest of the surfaces can look at their leaf to find out
            // what fog volume they are in.
            short		surfaceFogVolumeID;
//	};

            // lighting info
            unsigned char styles[MAXLIGHTMAPS];
            int			lightofs;		// start of [numstyles*surfsize] samples
            float       area;

            // TODO: make these unsigned chars?
            int			m_LightmapTextureMinsInLuxels[2];
            int			m_LightmapTextureSizeInLuxels[2];

            int         origFace;       // reference the original face this face was derived from

            // non-polygon primitives (strips and lists)
            unsigned short	numPrims;
            unsigned short	firstPrimID;

            unsigned int	smoothingGroups;
        };

        struct dleaf_t
        {
            int				contents;			// OR of all brushes (not needed?)

            short			cluster;
            short			area;

            short			mins[3];			// for frustum culling
            short			maxs[3];

            unsigned short	firstleafface;
            unsigned short	numleaffaces;

            unsigned short	firstleafbrush;
            unsigned short	numleafbrushes;
            short			leafWaterDataID; // -1 for not in water
        };

        struct dbrushside_t
        {
            unsigned short	planenum;		// facing out of the leaf
            short	texinfo;
            short			dispinfo;		// displacement info (BSPVERSION 7)
            short			bevel;			// is the side a bevel plane? (BSPVERSION 7)
        };

        struct dbrush_t
        {
            int			firstside;
            int			numsides;
            int			contents;
        };

        // the visibility lump consists of a header with a count, then
        // byte offsets for the PVS and PHS of each cluster, then the raw
        // compressed bit vectors
        struct dvis_t
        {
            int			numclusters;
            int			bitofs[8][2];	// bitofs[numclusters][2]
        };

        // each area has a list of portals that lead into other areas
        // when portals are closed, other areas may not be visible or
        // hearable even if the vis info says that it should be
        struct dareaportal_t
        {
            unsigned short	m_PortalKey;		// Entities have a key called portalnumber (and in vbsp a variable
            // called areaportalnum) which is used
            // to bind them to the area portals by comparing with this value.

            unsigned short	otherarea;		// The area this portal looks into.

            unsigned short	m_FirstClipPortalVert;	// Portal geometry.
            unsigned short	m_nClipPortalVerts;

            int				planenum;
        };

        struct darea_t
        {
            int		numareaportals;
            int		firstareaportal;
        };

        struct dportal_t
        {
            int		firstportalvert;
            int		numportalverts;
            int		planenum;
            unsigned short	cluster[2];
        };

        struct dcluster_t
        {
            int		firstportal;
            int		numportals;
        };

        struct dleafwaterdata_t
        {
            float	surfaceZ;
            float	minZ;
            short	surfaceTexInfoID;
        };

        class CFaceMacroTextureInfo
        {
        public:
            // This looks up into g_TexDataStringTable, which looks up into g_TexDataStringData.
            // 0xFFFF if the face has no macro texture.
            unsigned short m_MacroTextureNameID;
        };

        // lights that were used to illuminate the world
        enum emittype_t
        {
            emit_surface,		// 90 degree spotlight
            emit_point,			// simple point light source
            emit_spotlight,		// spotlight with penumbra
            emit_skylight,		// directional light with no falloff (surface must trace to SKY texture)
            emit_quakelight,	// linear falloff, non-lambertian
            emit_skyambient,	// spherical light source with no falloff (surface must trace to SKY texture)
        };

        struct dworldlight_t
        {
            Vector		origin;
            Vector		intensity;
            Vector		normal;			// for surfaces and spotlights
            int			cluster;
            emittype_t	type;
            int			style;
            float		stopdot;		// start of penumbra for emit_spotlight
            float		stopdot2;		// end of penumbra for emit_spotlight
            float		exponent;		//
            float		radius;			// cutoff distance
            // falloff for emit_spotlight + emit_point:
            // 1 / (constant_attn + linear_attn * dist + quadratic_attn * dist^2)
            float		constant_attn;
            float		linear_attn;
            float		quadratic_attn;
            int			flags;
            int			texinfo;		//
            int			owner;			// entity that this light it relative to
        };

        struct dcubemapsample_t
        {
            int			origin[3];			// position of light snapped to the nearest integer
            // the filename for the vtf file is derived from the position
            unsigned char size;				// 0 - default
            // otherwise, 1<<(size-1)
        };

        struct doverlay_t
        {
            int			nId;
            short		nTexInfo;
            short		nFaceCount;
            int			aFaces[OVERLAY_BSP_FACE_COUNT];
            float		flU[2];
            float		flV[2];
            Vector		vecUVPoints[4];
            Vector		vecOrigin;
            Vector		vecBasisNormal;
        };

        struct epair_t
        {
            epair_t	*next;
            char	*key;
            char	*value;
        };

        //--

        enum
        {
            STATIC_PROP_NAME_LENGTH  = 128,

            // Flags field
            // These are automatically computed
                STATIC_PROP_FLAG_FADES	= 0x1,
            STATIC_PROP_USE_LIGHTING_ORIGIN	= 0x2,

            // These are set in WC
                STATIC_PROP_NO_SHADOW	= 0x10,

            // This mask includes all flags settable in WC
                STATIC_PROP_WC_MASK		= 0x10,
        };

        struct StaticPropDictLump_t
        {
            char	m_Name[STATIC_PROP_NAME_LENGTH];		// model name
        };

        struct StaticPropLump_t
        {
            // v4
            Vector          Origin;            // origin
            Vector          Angles;            // orientation (pitch roll yaw)

            // v4
            unsigned short  PropType;          // index into model name dictionary
            unsigned short  FirstLeaf;         // index into leaf array
            unsigned short  LeafCount;
            unsigned char   Solid;             // solidity type
            unsigned char   Flags;
            int             Skin;              // model skin numbers
            float           FadeMinDist;
            float           FadeMaxDist;
            Vector          LightingOrigin;    // for lighting

            // since v5
            float           ForcedFadeScale;   // fade distance scale

            // v6 and v7 only
            unsigned short  MinDXLevel;        // minimum DirectX version to be visible
            unsigned short  MaxDXLevel;        // maximum DirectX version to be visible
            // since v8
            unsigned char   MinCPULevel;
            unsigned char   MaxCPULevel;
            unsigned char   MinGPULevel;
            unsigned char   MaxGPULevel;
            // since v7
            unsigned int    DiffuseModulation; // per instance color and alpha modulation
            // v9 and v10 only
            unsigned int    DisableX360;       // if true, don't show on XBox 360 (4-bytes long)
            // since v10
            unsigned int    FlagsEx;           // Further bitflags.
            // since v11
            float           UniformScale;      // Prop scale
        };

        struct StaticPropLeafLump_t
        {
            unsigned short	m_Leaf;
        };


//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_STATIC_PROP_LIGHTING lump
//-----------------------------------------------------------------------------
        struct StaticPropLightstylesLump_t
        {
            colorRGBExp32	m_Lighting;
        };


        //--

        /// BSP file
        class File : public base::res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(File, base::res::IResource);

        public:
            File();
            ~File();

            ///--

            /// get data counts
            INLINE uint32_t numFaces() const { return m_header->lumps[LUMP_FACES].filelen / sizeof(dface_t); }
            INLINE uint32_t numModels() const { return m_header->lumps[LUMP_MODELS].filelen / sizeof(dmodel_t); }
            INLINE uint32_t numTextureInfos() const { return m_header->lumps[LUMP_TEXINFO].filelen / sizeof(texinfo_t); }
            INLINE uint32_t numTextureDatas() const { return m_header->lumps[LUMP_TEXDATA].filelen / sizeof(dtexdata_t); }

            /// get the data
            INLINE const char* entities() const { return base::OffsetPtr((const char*)m_header, m_header->lumps[LUMP_ENTITIES].fileofs); }
            INLINE const dplane_t* planes() const { return base::OffsetPtr((const dplane_t*)m_header, m_header->lumps[LUMP_PLANES].fileofs); }
            INLINE const dface_t* faces() const { return base::OffsetPtr((const dface_t*)m_header, m_header->lumps[LUMP_FACES].fileofs); }
            INLINE const dvertex_t* vertices() const { return base::OffsetPtr((const dvertex_t*)m_header, m_header->lumps[LUMP_VERTEXES].fileofs); }
            INLINE const dedge_t* edges() const { return base::OffsetPtr((const dedge_t*)m_header, m_header->lumps[LUMP_EDGES].fileofs); }
            INLINE const int* surfEdges() const { return base::OffsetPtr((const int*)m_header, m_header->lumps[LUMP_SURFEDGES].fileofs); }
            INLINE const dtexdata_t* textureDatas() const { return base::OffsetPtr((const dtexdata_t*)m_header, m_header->lumps[LUMP_TEXDATA].fileofs); }
            INLINE const texinfo_t* textureInfos() const { return base::OffsetPtr((const texinfo_t*)m_header, m_header->lumps[LUMP_TEXINFO].fileofs); }
            INLINE const int* textureStringIndices() const { return base::OffsetPtr((const int*)m_header, m_header->lumps[LUMP_TEXDATA_STRING_TABLE].fileofs); }
            INLINE const char* textureStringData() const { return base::OffsetPtr((const char*)m_header, m_header->lumps[LUMP_TEXDATA_STRING_DATA].fileofs); }
            INLINE const dmodel_t* models() const { return base::OffsetPtr((const dmodel_t*)m_header, m_header->lumps[LUMP_MODELS].fileofs); }
            INLINE const lump_t& zipLump() const { return m_header->lumps[LUMP_PAKFILE]; }
            INLINE const void* zipData() const { return base::OffsetPtr((const void*)m_header, m_header->lumps[LUMP_PAKFILE].fileofs); }

            /// get game lump data
            base::Buffer gameLumpData(uint32_t fourCC) const;
            uint32_t gameLumpVersion(uint32_t fourCC) const;

            ///--

            // load the content
            static const base::RefPtr<File> LoadFromFile(base::res::IResourceCookerInterface& cooker);

        private:
            base::Buffer m_buffer;
            const dheader_t* m_header;

            virtual void onPostLoad() override;
        };

        //--

    } // bsp
} // hl2