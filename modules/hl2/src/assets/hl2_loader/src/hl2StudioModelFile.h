/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

namespace hl2
{
    namespace mdl
    {

#define STUDIO_VERSION		48

#ifndef _XBOX
#define MAXSTUDIOTRIANGLES	65536	// TODO: tune this
#define MAXSTUDIOVERTS		65536	// TODO: tune this
#define	MAXSTUDIOFLEXVERTS	10000	// max number of verts that can be flexed per mesh.  TODO: tune this
#else
        #define MAXSTUDIOTRIANGLES	25000
#define MAXSTUDIOVERTS		10000
#define	MAXSTUDIOFLEXVERTS	1000
#endif
#define MAXSTUDIOSKINS		32		// total textures
#define MAXSTUDIOBONES		128		// total bones actually used
#define MAXSTUDIOFLEXDESC	1024	// maximum number of low level flexes (actual morph targets)
#define MAXSTUDIOFLEXCTRL	96		// maximum number of flexcontrollers (input sliders)
#define MAXSTUDIOPOSEPARAM	24
#define MAXSTUDIOBONECTRLS	4
#define MAXSTUDIOANIMBLOCKS 256

#define MAXSTUDIOBONEBITS	7		// NOTE: MUST MATCH MAXSTUDIOBONES

// NOTE!!! : Changing this number also changes the vtx file format!!!!!
#define MAX_NUM_BONES_PER_VERT 3

//Adrian - Remove this when we completely phase out the old event system.
#define NEW_EVENT_STYLE ( 1 << 10 )

#pragma pack(push)
#pragma pack(4)

        typedef base::Vector3 Vector;
        typedef base::Quat Quaternion;
        typedef base::Vector3 RadianEuler;
        typedef base::Float16 float16;
        typedef base::Vector2 Vector2D;
        typedef base::Vector4 Vector4DAligned;
        typedef base::Vector4 Vector4D;

        class Quaternion64
        {
        public:
            uint64_t x:21;
            uint64_t y:21;
            uint64_t z:21;
            uint64_t wneg:1;

            Quaternion toQuat() const
            {
                Quaternion tmp;

                // shift to -1048576, + 1048575, then round down slightly to -1.0 < x < 1.0
                tmp.x = ((int)x - 1048576) * (1 / 1048576.5f);
                tmp.y = ((int)y - 1048576) * (1 / 1048576.5f);
                tmp.z = ((int)z - 1048576) * (1 / 1048576.5f);
                tmp.w = sqrt( 1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z );
                if (wneg)
                    tmp.w = -tmp.w;
                return tmp;
            }
        };

        class Quaternion48
        {
        public:
            unsigned short x:16;
            unsigned short y:16;
            unsigned short z:15;
            unsigned short wneg:1;

            Quaternion toQuat() const
            {
                Quaternion tmp;

                tmp.x = ((int)x - 32768) * (1 / 32768.0);
                tmp.y = ((int)y - 32768) * (1 / 32768.0);
                tmp.z = ((int)z - 16384) * (1 / 16384.0);
                tmp.w = sqrt( 1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z );
                if (wneg)
                    tmp.w = -tmp.w;
                return tmp;
            }
        };

        class Quaternion32
        {
        public:
            inline Quaternion toQuat() const
            {
                Quaternion tmp;

                tmp.x = ((int)x - 1024) * (1 / 1024.0);
                tmp.y = ((int)y - 512) * (1 / 512.0);
                tmp.z = ((int)z - 512) * (1 / 512.0);
                tmp.w = sqrt( 1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z );
                if (wneg)
                    tmp.w = -tmp.w;
                return tmp;
            }

            uint32_t x:11;
            uint32_t y:10;
            uint32_t z:10;
            uint32_t wneg:1;
        };

        class Vector48
        {
        public:
            Vector toVector() const
            {
                return Vector(x.value(), y.value(), z.value());
            }

            float16 x;
            float16 y;
            float16 z;
        };

#define JIGGLE_IS_FLEXIBLE				0x01
#define JIGGLE_IS_RIGID					0x02
#define JIGGLE_HAS_YAW_CONSTRAINT		0x04
#define JIGGLE_HAS_PITCH_CONSTRAINT		0x08
#define JIGGLE_HAS_ANGLE_CONSTRAINT		0x10
#define JIGGLE_HAS_LENGTH_CONSTRAINT	0x20
#define JIGGLE_HAS_BASE_SPRING			0x40
#define JIGGLE_IS_BOING					0x80		// simple squash and stretch sinusoid "boing"

        struct mstudiojigglebone_t
        {
            int				flags;

            // general params
            float			length;					// how from from bone base, along bone, is tip
            float			tipMass;

            // flexible params
            float			yawStiffness;
            float			yawDamping;
            float			pitchStiffness;
            float			pitchDamping;
            float			alongStiffness;
            float			alongDamping;

            // angle constraint
            float			angleLimit;				// maximum deflection of tip in radians

            // yaw constraint
            float			minYaw;					// in radians
            float			maxYaw;					// in radians
            float			yawFriction;
            float			yawBounce;

            // pitch constraint
            float			minPitch;				// in radians
            float			maxPitch;				// in radians
            float			pitchFriction;
            float			pitchBounce;

            // base spring
            float			baseMass;
            float			baseStiffness;
            float			baseDamping;
            float			baseMinLeft;
            float			baseMaxLeft;
            float			baseLeftFriction;
            float			baseMinUp;
            float			baseMaxUp;
            float			baseUpFriction;
            float			baseMinForward;
            float			baseMaxForward;
            float			baseForwardFriction;

            // boing
            float			boingImpactSpeed;
            float			boingImpactAngle;
            float			boingDampingRate;
            float			boingFrequency;
            float			boingAmplitude;
        };

        struct mstudioaimatbone_t
        {
            int				parent;
            int				aim;		// Might be bone or attach
            Vector aimvector;
            Vector upvector;
            Vector	basepos;
        };

// bones
        struct mstudiobone_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            int		 			parent;		// parent bone
            int					bonecontroller[6];	// bone controller index, -1 == none

            // default values
            Vector pos;
            Quaternion quat;
            RadianEuler rot;
            // compression scale
            Vector posscale;
            Vector rotscale;

            float               poseToBone[12];
            Quaternion qAlignment;
            int					flags;
            int					proctype;
            int					procindex;		// procedural rule
            mutable int			physicsbone;	// index into physically simulated bone
            inline void *pProcedure( ) const { if (procindex == 0) return nullptr; else return  (void *)(((uint8_t *)this) + procindex); };
            int					surfacepropidx;	// index into string tablefor property name
            inline char * const pszSurfaceProp( void ) const { return ((char *)this) + surfacepropidx; }
            int					contents;		// See BSPFlags.h for the contents flags

            int					unused[8];		// remove as appropriate
        };

        struct mstudiolinearbone_t
        {
            int numbones;

            int flagsindex;
            inline int flags( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((int *)(((uint8_t *)this) + flagsindex) + i); };
            inline int *pflags( int i ) { DEBUG_CHECK( i >= 0 && i < numbones); return ((int *)(((uint8_t *)this) + flagsindex) + i); };

            int	parentindex;
            inline int parent( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((int *)(((uint8_t *)this) + parentindex) + i); };

            int	posindex;
            inline Vector pos( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((Vector *)(((uint8_t *)this) + posindex) + i); };

            int quatindex;
            inline Quaternion quat( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((Quaternion *)(((uint8_t *)this) + quatindex) + i); };

            int rotindex;
            inline RadianEuler rot( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((RadianEuler *)(((uint8_t *)this) + rotindex) + i); };

            int posetoboneindex;
            inline const float* poseToBone( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((const float**)(((uint8_t *)this) + posetoboneindex) + i); };

            int	posscaleindex;
            inline Vector posscale( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((Vector *)(((uint8_t *)this) + posscaleindex) + i); };

            int	rotscaleindex;
            inline Vector rotscale( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((Vector *)(((uint8_t *)this) + rotscaleindex) + i); };

            int	qalignmentindex;
            inline Quaternion qalignment( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return *((Quaternion *)(((uint8_t *)this) + qalignmentindex) + i); };

            int unused[6];
        };

        //-----------------------------------------------------------------------------
// The component of the bone used by mstudioboneflexdriver_t
//-----------------------------------------------------------------------------
        enum StudioBoneFlexComponent_t
        {
            STUDIO_BONE_FLEX_INVALID = -1,	// Invalid
            STUDIO_BONE_FLEX_TX = 0,		// Translate X
            STUDIO_BONE_FLEX_TY = 1,		// Translate Y
            STUDIO_BONE_FLEX_TZ = 2			// Translate Z
        };


//-----------------------------------------------------------------------------
// Component is one of Translate X, Y or Z [0,2] (StudioBoneFlexComponent_t)
//-----------------------------------------------------------------------------
        struct mstudioboneflexdrivercontrol_t
        {
            int m_nBoneComponent;		// Bone component that drives flex, StudioBoneFlexComponent_t
            int m_nFlexControllerIndex;	// Flex controller to drive
            float m_flMin;				// Min value of bone component mapped to 0 on flex controller
            float m_flMax;				// Max value of bone component mapped to 1 on flex controller
        };


//-----------------------------------------------------------------------------
// Drive flex controllers from bone components
//-----------------------------------------------------------------------------
        struct mstudioboneflexdriver_t
        {
            int m_nBoneIndex;			// Bone to drive flex controller
            int m_nControlCount;		// Number of flex controllers being driven
            int m_nControlIndex;		// Index into data where controllers are (relative to this)

            inline mstudioboneflexdrivercontrol_t *pBoneFlexDriverControl( int i ) const
            {
                DEBUG_CHECK( i >= 0 && i < m_nControlCount );
                return (mstudioboneflexdrivercontrol_t *)(((uint8_t *)this) + m_nControlIndex) + i;
            }

            int unused[3];
        };


#define BONE_CALCULATE_MASK			0x1F
#define BONE_PHYSICALLY_SIMULATED	0x01	// bone is physically simulated when physics are active
#define BONE_PHYSICS_PROCEDURAL		0x02	// procedural when physics is active
#define BONE_ALWAYS_PROCEDURAL		0x04	// bone is always procedurally animated
#define BONE_SCREEN_ALIGN_SPHERE	0x08	// bone aligns to the screen, not constrained in motion.
#define BONE_SCREEN_ALIGN_CYLINDER	0x10	// bone aligns to the screen, constrained by it's own axis.

#define BONE_USED_MASK				0x0007FF00
#define BONE_USED_BY_ANYTHING		0x0007FF00
#define BONE_USED_BY_HITBOX			0x00000100	// bone (or child) is used by a hit box
#define BONE_USED_BY_ATTACHMENT		0x00000200	// bone (or child) is used by an attachment point
#define BONE_USED_BY_VERTEX_MASK	0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0	0x00000400	// bone (or child) is used by the toplevel model via skinned vertex
#define BONE_USED_BY_VERTEX_LOD1	0x00000800
#define BONE_USED_BY_VERTEX_LOD2	0x00001000
#define BONE_USED_BY_VERTEX_LOD3	0x00002000
#define BONE_USED_BY_VERTEX_LOD4	0x00004000
#define BONE_USED_BY_VERTEX_LOD5	0x00008000
#define BONE_USED_BY_VERTEX_LOD6	0x00010000
#define BONE_USED_BY_VERTEX_LOD7	0x00020000
#define BONE_USED_BY_BONE_MERGE		0x00040000	// bone is available for bone merge to occur against it

#define BONE_USED_BY_VERTEX_AT_LOD(lod) ( BONE_USED_BY_VERTEX_LOD0 << (lod) )
#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ( ( BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK ) | BONE_USED_BY_VERTEX_AT_LOD(lod) )

#define MAX_NUM_LODS 8

#define BONE_TYPE_MASK				0x00F00000
#define BONE_FIXED_ALIGNMENT		0x00100000	// bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

#define BONE_HAS_SAVEFRAME_POS		0x00200000	// Vector48
#define BONE_HAS_SAVEFRAME_ROT		0x00400000	// Quaternion64

// bone controllers
        struct mstudiobonecontroller_t
        {
            int					bone;	// -1 == 0
            int					type;	// X, Y, Z, XR, YR, ZR, M
            float				start;
            float				end;
            int					rest;	// uint8_t index value at rest
            int					inputfield;	// 0-3 user set controller, 4 mouth
            int					unused[8];
        };

// intersection boxes
        struct mstudiobbox_t
        {
            int					bone;
            int					group;				// intersection group
            Vector				bbmin;				// bounding box
            Vector				bbmax;
            int					szhitboxnameindex;	// offset to the name of the hitbox.
            int					unused[8];

            const char* pszHitboxName()
            {
                if( szhitboxnameindex == 0 )
                    return "";

                return ((const char*)this) + szhitboxnameindex;
            }
        };

// demand loaded sequence groups
        struct mstudiomodelgroup_t
        {
            int					szlabelindex;	// textual name
            inline char * const pszLabel( void ) const { return ((char *)this) + szlabelindex; }
            int					sznameindex;	// file name
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
        };

        struct mstudiomodelgrouplookup_t
        {
            int					modelgroup;
            int					indexwithingroup;
        };

// events
        struct mstudioevent_t
        {
            float				cycle;
            int					event;
            int					type;
            inline const char * pszOptions( void ) const { return options; }
            char				options[64];

            int					szeventindex;
            inline char * const pszEventName( void ) const { return ((char *)this) + szeventindex; }
        };

#define	ATTACHMENT_FLAG_WORLD_ALIGN 0x10000

// attachment
        struct mstudioattachment_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            uint32_t		flags;
            int					localbone;
            float local[12]; // attachment point
            int					unused[8];
        };

#define IK_SELF 1
#define IK_WORLD 2
#define IK_GROUND 3
#define IK_RELEASE 4
#define IK_ATTACHMENT 5
#define IK_UNLATCH 6

        struct mstudioikerror_t
        {
            Vector		pos;
            Quaternion	q;
        };

        union mstudioanimvalue_t;

        struct mstudiocompressedikerror_t
        {
            float	scale[6];
            short	offset[6];
            inline mstudioanimvalue_t *pAnimvalue( int i ) const { if (offset[i] > 0) return  (mstudioanimvalue_t *)(((uint8_t *)this) + offset[i]); else return nullptr; };
        };

        struct mstudioikrule_t
        {
            int			index;

            int			type;
            int			chain;

            int			bone;

            int			slot;	// iktarget slot.  Usually same as chain.
            float		height;
            float		radius;
            float		floor;
            Vector		pos;
            Quaternion	q;

            int			compressedikerrorindex;
            inline mstudiocompressedikerror_t *pCompressedError() const { return (mstudiocompressedikerror_t *)(((uint8_t *)this) + compressedikerrorindex); };
            int			unused2;

            int			iStart;
            int			ikerrorindex;
            inline mstudioikerror_t *pError( int i ) const { return  (ikerrorindex) ? (mstudioikerror_t *)(((uint8_t *)this) + ikerrorindex) + (i - iStart) : nullptr; };

            float		start;	// beginning of influence
            float		peak;	// start of full influence
            float		tail;	// end of full influence
            float		end;	// end of all influence

            float		unused3;	//
            float		contact;	// frame footstep makes ground concact
            float		drop;		// how far down the foot should drop when reaching for IK
            float		top;		// top of the foot box

            int			unused6;
            int			unused7;
            int			unused8;

            int			szattachmentindex;		// name of world attachment
            inline char * const pszAttachment( void ) const { return ((char *)this) + szattachmentindex; }

            int			unused[7];
        };


        struct mstudioiklock_t
        {
            int			chain;
            float		flPosWeight;
            float		flLocalQWeight;
            int			flags;

            int			unused[4];
        };


        struct mstudiolocalhierarchy_t
        {
            int			iBone;			// bone being adjusted
            int			iNewParent;		// the bones new parent

            float		start;			// beginning of influence
            float		peak;			// start of full influence
            float		tail;			// end of full influence
            float		end;			// end of all influence

            int			iStart;			// first frame

            int			localanimindex;
            inline mstudiocompressedikerror_t *pLocalAnim() const { return (mstudiocompressedikerror_t *)(((uint8_t *)this) + localanimindex); };

            int			unused[4];
        };



// animation frames
        union mstudioanimvalue_t
        {
            struct
            {
                uint8_t	valid;
                uint8_t	total;
            } num;
            short		value;
        };

        struct mstudioanim_valueptr_t
        {
            short	offset[3];
            inline mstudioanimvalue_t *pAnimvalue( int i ) const { if (offset[i] > 0) return  (mstudioanimvalue_t *)(((uint8_t *)this) + offset[i]); else return nullptr; };
        };

#define STUDIO_ANIM_RAWPOS	0x01 // Vector48
#define STUDIO_ANIM_RAWROT	0x02 // Quaternion48
#define STUDIO_ANIM_ANIMPOS	0x04 // mstudioanim_valueptr_t
#define STUDIO_ANIM_ANIMROT	0x08 // mstudioanim_valueptr_t
#define STUDIO_ANIM_DELTA	0x10
#define STUDIO_ANIM_RAWROT2	0x20 // Quaternion64


// per bone per animation DOF and weight pointers
        struct mstudioanim_t
        {
            uint8_t				bone;
            uint8_t				flags;		// weighing options

            // valid for animating data only
            inline uint8_t				*pData( void ) const { return (((uint8_t *)this) + sizeof( struct mstudioanim_t )); };
            inline mstudioanim_valueptr_t	*pRotV( void ) const { return (mstudioanim_valueptr_t *)(pData()); };
            inline mstudioanim_valueptr_t	*pPosV( void ) const { return (mstudioanim_valueptr_t *)(pData()) + ((flags & STUDIO_ANIM_ANIMROT) != 0); };

            // valid if animation unvaring over timeline
            inline Quaternion48		*pQuat48( void ) const { return (Quaternion48 *)(pData()); };
            inline Quaternion64		*pQuat64( void ) const { return (Quaternion64 *)(pData()); };
            inline Vector48			*pPos( void ) const { return (Vector48 *)(pData() + ((flags & STUDIO_ANIM_RAWROT) != 0) * sizeof( *pQuat48() ) + ((flags & STUDIO_ANIM_RAWROT2) != 0) * sizeof( *pQuat64() ) ); };

            short				nextoffset;
            inline mstudioanim_t	*pNext( void ) const { if (nextoffset != 0) return  (mstudioanim_t *)(((uint8_t *)this) + nextoffset); else return nullptr; };
        };

        struct mstudiomovement_t
        {
            int					endframe;
            int					motionflags;
            float				v0;			// velocity at start of block
            float				v1;			// velocity at end of block
            float				angle;		// YAW rotation at end of this blocks movement
            Vector				vector;		// movement vector relative to this blocks initial angle
            Vector				position;	// relative to start of animation???
        };

        struct studiohdr_t;

// used for piecewise loading of animation data
        struct mstudioanimblock_t
        {
            int					datastart;
            int					dataend;
        };

        struct mstudioanimsections_t
        {
            int					animblock;
            int					animindex;
        };

        struct mstudioanimdesc_t
        {
            int					baseptr;
            inline studiohdr_t	*pStudiohdr( void ) const { return (studiohdr_t *)(((uint8_t *)this) + baseptr); }

            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }

            float				fps;		// frames per second
            int					flags;		// looping/non-looping flags

            int					numframes;

            // piecewise movement
            int					nummovements;
            int					movementindex;
            inline mstudiomovement_t * const pMovement( int i ) const { return (mstudiomovement_t *)(((uint8_t *)this) + movementindex) + i; };

            int					unused1[6];			// remove as appropriate (and zero if loading older versions)

            int					animblock;
            int					animindex;	 // non-zero when anim data isn't in sections
            mstudioanim_t *pAnimBlock( int block, int index ) const; // returns pointer to a specific anim block (local or external)
            mstudioanim_t *pAnim( int *piFrame, float &flStall ) const; // returns pointer to data and new frame index
            mstudioanim_t *pAnim( int *piFrame ) const; // returns pointer to data and new frame index

            int					numikrules;
            int					ikruleindex;	// non-zero when IK data is stored in the mdl
            int					animblockikruleindex; // non-zero when IK data is stored in animblock file
            mstudioikrule_t *pIKRule( int i ) const;

            int					numlocalhierarchy;
            int					localhierarchyindex;
            mstudiolocalhierarchy_t *pHierarchy( int i ) const;

            int					sectionindex;
            int					sectionframes; // number of frames used in each fast lookup section, zero if not used
            inline mstudioanimsections_t * const pSection( int i ) const { return (mstudioanimsections_t *)(((uint8_t *)this) + sectionindex) + i; }

            short				zeroframespan;	// frames per span
            short				zeroframecount; // number of spans
            int					zeroframeindex;
            uint8_t				*pZeroFrameData( ) const { if (zeroframeindex) return (((uint8_t *)this) + zeroframeindex); else return nullptr; };
            mutable float		zeroframestalltime;		// saved during read stalls
        };

        struct mstudioikrule_t;

        struct mstudioautolayer_t
        {
            short				iSequence;
            short				iPose;

            int					flags;
            float				start;	// beginning of influence
            float				peak;	// start of full influence
            float				tail;	// end of full influence
            float				end;	// end of all influence
        };

        struct mstudioactivitymodifier_t
        {
            int					sznameindex;
            inline char			*pszName() { return (sznameindex) ? (char *)(((uint8_t *)this) + sznameindex ) : nullptr; }
        };

// sequence descriptions
        struct mstudioseqdesc_t
        {
            int					baseptr;
            inline studiohdr_t	*pStudiohdr( void ) const { return (studiohdr_t *)(((uint8_t *)this) + baseptr); }

            int					szlabelindex;
            inline char * const pszLabel( void ) const { return ((char *)this) + szlabelindex; }

            int					szactivitynameindex;
            inline char * const pszActivityName( void ) const { return ((char *)this) + szactivitynameindex; }

            int					flags;		// looping/non-looping flags

            int					activity;	// initialized at loadtime to game DLL values
            int					actweight;

            int					numevents;
            int					eventindex;
            inline mstudioevent_t *pEvent( int i ) const { DEBUG_CHECK( i >= 0 && i < numevents); return (mstudioevent_t *)(((uint8_t *)this) + eventindex) + i; };

            Vector				bbmin;		// per sequence bounding box
            Vector				bbmax;

            int					numblends;

            // Index into array of shorts which is groupsize[0] x groupsize[1] in length
            int					animindexindex;

            inline int			anim( int x, int y ) const
            {
                if ( x >= groupsize[0] )
                {
                    x = groupsize[0] - 1;
                }

                if ( y >= groupsize[1] )
                {
                    y = groupsize[ 1 ] - 1;
                }

                int offset = y * groupsize[0] + x;
                short *blends = (short *)(((uint8_t *)this) + animindexindex);
                int value = (int)blends[ offset ];
                return value;
            }

            int					movementindex;	// [blend] float array for blended movement
            int					groupsize[2];
            int					paramindex[2];	// X, Y, Z, XR, YR, ZR
            float				paramstart[2];	// local (0..1) starting value
            float				paramend[2];	// local (0..1) ending value
            int					paramparent;

            float				fadeintime;		// ideal cross fate in time (0.2 default)
            float				fadeouttime;	// ideal cross fade out time (0.2 default)

            int					localentrynode;		// transition node at entry
            int					localexitnode;		// transition node at exit
            int					nodeflags;		// transition rules

            float				entryphase;		// used to match entry gait
            float				exitphase;		// used to match exit gait

            float				lastframe;		// frame that should generation EndOfSequence

            int					nextseq;		// auto advancing sequences
            int					pose;			// index of delta animation between end and nextseq

            int					numikrules;

            int					numautolayers;	//
            int					autolayerindex;
            inline mstudioautolayer_t *pAutolayer( int i ) const { DEBUG_CHECK( i >= 0 && i < numautolayers); return (mstudioautolayer_t *)(((uint8_t *)this) + autolayerindex) + i; };

            int					weightlistindex;
            inline float		*pBoneweight( int i ) const { return ((float *)(((uint8_t *)this) + weightlistindex) + i); };
            inline float		weight( int i ) const { return *(pBoneweight( i)); };

            // FIXME: make this 2D instead of 2x1D arrays
            int					posekeyindex;
            float				*pPoseKey( int iParam, int iAnim ) const { return (float *)(((uint8_t *)this) + posekeyindex) + iParam * groupsize[0] + iAnim; }
            float				poseKey( int iParam, int iAnim ) const { return *(pPoseKey( iParam, iAnim )); }

            int					numiklocks;
            int					iklockindex;
            inline mstudioiklock_t *pIKLock( int i ) const { DEBUG_CHECK( i >= 0 && i < numiklocks); return (mstudioiklock_t *)(((uint8_t *)this) + iklockindex) + i; };

            // Key values
            int					keyvalueindex;
            int					keyvaluesize;
            inline const char * KeyValueText( void ) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : nullptr; }

            int					cycleposeindex;		// index of pose parameter to use as cycle index

            int					activitymodifierindex;
            int					numactivitymodifiers;
            inline mstudioactivitymodifier_t *pActivityModifier( int i ) const { DEBUG_CHECK( i >= 0 && i < numactivitymodifiers); return activitymodifierindex != 0 ? (mstudioactivitymodifier_t *)(((uint8_t *)this) + activitymodifierindex) + i : nullptr; };

            int					unused[5];		// remove/add as appropriate (grow back to 8 ints on version change!)

            mstudioseqdesc_t(){}
        private:
            // No copy constructors allowed
            mstudioseqdesc_t(const mstudioseqdesc_t& vOther);
        };


        struct mstudioposeparamdesc_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            int					flags;	// ????
            float				start;	// starting value
            float				end;	// ending value
            float				loop;	// looping range, 0 for no looping, 360 for rotations, etc.
        };

        struct mstudioflexdesc_t
        {
            int					szFACSindex;
            inline char * const pszFACS( void ) const { return ((char *)this) + szFACSindex; }
        };



        struct mstudioflexcontroller_t
        {
            int					sztypeindex;
            inline char * const pszType( void ) const { return ((char *)this) + sztypeindex; }
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            mutable int			localToGlobal;	// remapped at load time to master list
            float				min;
            float				max;
        };


        enum FlexControllerRemapType_t
        {
            FLEXCONTROLLER_REMAP_PASSTHRU = 0,
            FLEXCONTROLLER_REMAP_2WAY,	// Control 0 -> ramps from 1-0 from 0->0.5. Control 1 -> ramps from 0-1 from 0.5->1
            FLEXCONTROLLER_REMAP_NWAY,	// StepSize = 1 / (control count-1) Control n -> ramps from 0-1-0 from (n-1)*StepSize to n*StepSize to (n+1)*StepSize. A second control is needed to specify amount to use
            FLEXCONTROLLER_REMAP_EYELID
        };


        class CStudioHdr;
        struct mstudioflexcontrollerui_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }

            // These are used like a union to save space
            // Here are the possible configurations for a UI controller
            //
            // SIMPLE NON-STEREO:	0: control	1: unused	2: unused
            // STEREO:				0: left		1: right	2: unused
            // NWAY NON-STEREO:		0: control	1: unused	2: value
            // NWAY STEREO:			0: left		1: right	2: value

            int					szindex0;
            int					szindex1;
            int					szindex2;

            inline const mstudioflexcontroller_t *pController( void ) const
            {
                return !stereo ? (mstudioflexcontroller_t *)( (char *)this + szindex0 ) : nullptr;
            }
            inline char * const	pszControllerName( void ) const { return !stereo ? pController()->pszName() : nullptr; }
            inline int			controllerIndex( const CStudioHdr &cStudioHdr ) const;

            inline const mstudioflexcontroller_t *pLeftController( void ) const
            {
                return stereo ? (mstudioflexcontroller_t *)( (char *)this + szindex0 ) : nullptr;
            }
            inline char * const	pszLeftName( void ) const { return stereo ? pLeftController()->pszName() : nullptr; }
            inline int			leftIndex( const CStudioHdr &cStudioHdr ) const;

            inline const mstudioflexcontroller_t *pRightController( void ) const
            {
                return stereo ? (mstudioflexcontroller_t *)( (char *)this + szindex1 ): nullptr;
            }
            inline char * const	pszRightName( void ) const { return stereo ? pRightController()->pszName() : nullptr; }
            inline int			rightIndex( const CStudioHdr &cStudioHdr ) const;

            inline const mstudioflexcontroller_t *pNWayValueController( void ) const
            {
                return remaptype == FLEXCONTROLLER_REMAP_NWAY ? (mstudioflexcontroller_t *)( (char *)this + szindex2 ) : nullptr;
            }
            inline char * const	pszNWayValueName( void ) const { return remaptype == FLEXCONTROLLER_REMAP_NWAY ? pNWayValueController()->pszName() : nullptr; }
            inline int			nWayValueIndex( const CStudioHdr &cStudioHdr ) const;

            // Number of controllers this ui description contains, 1, 2 or 3
            inline int			Count() const { return ( stereo ? 2 : 1 ) + ( remaptype == FLEXCONTROLLER_REMAP_NWAY ? 1 : 0 ); }
            inline const mstudioflexcontroller_t *pController( int index ) const;

            unsigned char		remaptype;	// See the FlexControllerRemapType_t enum
            bool				stereo;		// Is this a stereo control?
            uint8_t				unused[2];
        };


// this is the memory image of vertex anims (16-bit fixed point)
        struct mstudiovertanim_t
        {
            unsigned short		index;
            uint8_t				speed;	// 255/max_length_in_flex
            uint8_t				side;	// 255/left_right

        protected:
            // JasonM changing this type a lot, to prefer fixed point 16 bit...
            union
            {
                short			delta[3];
                float16			flDelta[3];
            };

            union
            {
                short			ndelta[3];
                float16			flNDelta[3];
            };

        public:
            inline void ConvertToFixed( float flVertAnimFixedPointScale )
            {
                delta[0] = flDelta[0].value() / flVertAnimFixedPointScale;
                delta[1] = flDelta[1].value() / flVertAnimFixedPointScale;
                delta[2] = flDelta[2].value() / flVertAnimFixedPointScale;
                ndelta[0] = flNDelta[0].value() / flVertAnimFixedPointScale;
                ndelta[1] = flNDelta[1].value() / flVertAnimFixedPointScale;
                ndelta[2] = flNDelta[2].value() / flVertAnimFixedPointScale;
            }

            inline Vector GetDeltaFixed( float flVertAnimFixedPointScale )
            {
                return Vector( delta[0] * flVertAnimFixedPointScale, delta[1] * flVertAnimFixedPointScale, delta[2] * flVertAnimFixedPointScale );
            }
            inline Vector GetNDeltaFixed( float flVertAnimFixedPointScale )
            {
                return Vector( ndelta[0] * flVertAnimFixedPointScale, ndelta[1] * flVertAnimFixedPointScale, ndelta[2] * flVertAnimFixedPointScale );
            }
            inline void GetDeltaFixed4DAligned( Vector4DAligned *vFillIn, float flVertAnimFixedPointScale )
            {
                vFillIn->x = delta[0] * flVertAnimFixedPointScale;
                vFillIn->y = delta[1] * flVertAnimFixedPointScale;
                vFillIn->z = delta[2] * flVertAnimFixedPointScale;
                vFillIn->w = 0.0f;
            }
            inline void GetNDeltaFixed4DAligned( Vector4DAligned *vFillIn, float flVertAnimFixedPointScale )
            {
                vFillIn->x = ndelta[0] * flVertAnimFixedPointScale;
                vFillIn->y = ndelta[1] * flVertAnimFixedPointScale;
                vFillIn->z = ndelta[2] * flVertAnimFixedPointScale;
                vFillIn->w = 0.0f;
            }
            inline Vector GetDeltaFloat() const
            {
                return Vector (flDelta[0].value(), flDelta[1].value(), flDelta[2].value());
            }
            inline Vector GetNDeltaFloat() const
            {
                return Vector (flNDelta[0].value(), flNDelta[1].value(), flNDelta[2].value());
            }
            inline void SetDeltaFixed( const Vector& vInput, float flVertAnimFixedPointScale )
            {
                delta[0] = vInput.x / flVertAnimFixedPointScale;
                delta[1] = vInput.y / flVertAnimFixedPointScale;
                delta[2] = vInput.z / flVertAnimFixedPointScale;
            }
            inline void SetNDeltaFixed( const Vector& vInputNormal, float flVertAnimFixedPointScale )
            {
                ndelta[0] = vInputNormal.x / flVertAnimFixedPointScale;
                ndelta[1] = vInputNormal.y / flVertAnimFixedPointScale;
                ndelta[2] = vInputNormal.z / flVertAnimFixedPointScale;
            }
        };


// this is the memory image of vertex anims (16-bit fixed point)
        struct mstudiovertanim_wrinkle_t : public mstudiovertanim_t
        {
            short	wrinkledelta;

            inline Vector4D GetDeltaFixed( float flVertAnimFixedPointScale )
            {
                return Vector4D( delta[0] * flVertAnimFixedPointScale, delta[1] * flVertAnimFixedPointScale, delta[2] * flVertAnimFixedPointScale, wrinkledelta * flVertAnimFixedPointScale );
            }

            inline void GetDeltaFixed4DAligned( Vector4DAligned *vFillIn, float flVertAnimFixedPointScale )
            {
                vFillIn->x = delta[0] * flVertAnimFixedPointScale;
                vFillIn->y = delta[1] * flVertAnimFixedPointScale;
                vFillIn->z = delta[2] * flVertAnimFixedPointScale;
                vFillIn->w = wrinkledelta * flVertAnimFixedPointScale;
            }

            inline float GetWrinkleDeltaFixed( float flVertAnimFixedPointScale )
            {
                return wrinkledelta * flVertAnimFixedPointScale;
            }
        };


        enum StudioVertAnimType_t
        {
            STUDIO_VERT_ANIM_NORMAL = 0,
            STUDIO_VERT_ANIM_WRINKLE,
        };

        struct mstudioflex_t
        {
            int					flexdesc;	// input value

            float				target0;	// zero
            float				target1;	// one
            float				target2;	// one
            float				target3;	// zero

            int					numverts;
            int					vertindex;

            inline	mstudiovertanim_t *pVertanim( int i ) const { DEBUG_CHECK( vertanimtype == STUDIO_VERT_ANIM_NORMAL ); return (mstudiovertanim_t *)(((uint8_t *)this) + vertindex) + i; };
            inline	mstudiovertanim_wrinkle_t *pVertanimWrinkle( int i ) const { DEBUG_CHECK( vertanimtype == STUDIO_VERT_ANIM_WRINKLE ); return  (mstudiovertanim_wrinkle_t *)(((uint8_t *)this) + vertindex) + i; };

            inline	uint8_t *pBaseVertanim( ) const { return ((uint8_t *)this) + vertindex; };
            inline	int	VertAnimSizeBytes() const { return ( vertanimtype == STUDIO_VERT_ANIM_NORMAL ) ? sizeof(mstudiovertanim_t) : sizeof(mstudiovertanim_wrinkle_t); }

            int					flexpair;	// second flex desc
            unsigned char		vertanimtype;	// See StudioVertAnimType_t
            unsigned char		unusedchar[3];
            int					unused[6];
        };


        struct mstudioflexop_t
        {
            int		op;
            union
            {
                int		index;
                float	value;
            } d;
        };

        struct mstudioflexrule_t
        {
            int					flex;
            int					numops;
            int					opindex;
            inline mstudioflexop_t *iFlexOp( int i ) const { return  (mstudioflexop_t *)(((uint8_t *)this) + opindex) + i; };
        };

// 16 Uint8s
        struct mstudioboneweight_t
        {
            float	weight[MAX_NUM_BONES_PER_VERT];
            char	bone[MAX_NUM_BONES_PER_VERT];
            uint8_t	numbones;

//	uint8_t	material;
//	short	firstref;
//	short	lastref;
        };

// NOTE: This is exactly 48 Uint8s
        struct mstudiovertex_t
        {
            mstudioboneweight_t	m_BoneWeights;
            Vector				m_vecPosition;
            Vector				m_vecNormal;
            Vector2D			m_vecTexCoord;
        };

// skin info
        struct mstudiotexture_t
        {
            int						sznameindex;
            inline char * const		pszName( void ) const { return ((char *)this) + sznameindex; }
            int						flags;
            int						used;
            int						unused1;
            mutable uint32_t material;  // fixme: this needs to go away . .isn't used by the engine, but is used by studiomdl
            mutable uint32_t clientmaterial;	// gary, replace with client material pointer if used

            int						unused[10];
        };

// eyeball
        struct mstudioeyeball_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            int		bone;
            Vector	org;
            float	zoffset;
            float	radius;
            Vector	up;
            Vector	forward;
            int		texture;

            int		unused1;
            float	iris_scale;
            int		unused2;

            int		upperflexdesc[3];	// index of raiser, neutral, and lowerer flexdesc that is set by flex controllers
            int		lowerflexdesc[3];
            float	uppertarget[3];		// angle (radians) of raised, neutral, and lowered lid positions
            float	lowertarget[3];

            int		upperlidflexdesc;	// index of flex desc that actual lid flexes look to
            int		lowerlidflexdesc;
            int		unused[4];			// These were used before, so not guaranteed to be 0
            bool	m_bNonFACS;			// Never used before version 44
            char	unused3[3];
            int		unused4[7];
        };


// ikinfo
        struct mstudioiklink_t
        {
            int		bone;
            Vector	kneeDir;	// ideal bending direction (per link, if applicable)
            Vector	unused0;	// unused
        };

        struct mstudioikchain_t
        {
            int				sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            int				linktype;
            int				numlinks;
            int				linkindex;
            inline mstudioiklink_t *pLink( int i ) const { return (mstudioiklink_t *)(((uint8_t *)this) + linkindex) + i; };
            // FIXME: add unused entries
        };


        struct mstudioiface_t
        {
            unsigned short a, b, c;		// Indices to vertices
        };


        struct mstudiomodel_t;

        struct mstudio_modelvertexdata_t
        {
            const Vector4D			*GlobalTangentS( int i ) const;
            const mstudiovertex_t		*GlobalVertex( int i ) const;
            bool				HasTangentData( void ) const;
            int					GetGlobalVertexIndex( int i ) const;
            int					GetGlobalTangentIndex( int i ) const;

            // base of external vertex data stores
            const void			*pVertexData;
            const void			*pTangentData;
        };

        struct mstudio_meshvertexdata_t
        {
            // indirection to this mesh's model's vertex data
            const uint32_t unused;

            // used for fixup calcs when culling top level lods
            // expected number of mesh verts at desired lod
            int					numLODVertexes[MAX_NUM_LODS];
        };

        struct mstudiomesh_t
        {
            int					material;

            int					modelindex;
            mstudiomodel_t *pModel() const;

            int					numvertices;		// number of unique vertices/normals/texcoords
            int					vertexoffset;		// vertex mstudiovertex_t

            int					numflexes;			// vertex animation
            int					flexindex;
            inline mstudioflex_t *pFlex( int i ) const { return (mstudioflex_t *)(((uint8_t *)this) + flexindex) + i; };

            // special codes for material operations
            int					materialtype;
            int					materialparam;

            // a unique ordinal for this mesh
            int					meshid;

            Vector				center;

            mstudio_meshvertexdata_t vertexdata;

            int					unused[8]; // remove as appropriate
        };

        struct mstudiofixup_t
        {
            uint32_t lodIndex;
            uint32_t vertexIndex;
            uint32_t vertexCount;
        };

        // little-endian "IDSV"
#define MODEL_VERTEX_FILE_ID		(('V'<<24)+('S'<<16)+('D'<<8)+'I')
#define MODEL_VERTEX_FILE_VERSION	4
// this id (IDCV) is used once the vertex data has been compressed (see CMDLCache::CreateThinVertexes)
#define MODEL_VERTEX_FILE_THIN_ID	(('V'<<24)+('C'<<16)+('D'<<8)+'I')

        struct vertexFileHeader_t
        {
            int		id;								// MODEL_VERTEX_FILE_ID
            int		version;						// MODEL_VERTEX_FILE_VERSION
            int		checksum;						// same as studiohdr_t, ensures sync
            int		numLODs;						// num of valid lods
            int		numLODVertexes[MAX_NUM_LODS];	// num verts for desired root lod
            int		numFixups;						// num of vertexFileFixup_t
            int		fixupTableStart;				// offset from base to fixup table
            int		vertexDataStart;				// offset from base to vertex block
            int		tangentDataStart;				// offset from base to tangent block

        public:

            // Accessor to fat vertex data
            const mstudiovertex_t *GetVertexData() const
            {
                if ( ( id == MODEL_VERTEX_FILE_ID ) && ( vertexDataStart != 0 ) )
                    return ( mstudiovertex_t * ) ( vertexDataStart + (uint8_t *)this );
                else
                return nullptr;
            }
            // Accessor to (fat) tangent vertex data (tangents aren't stored in compressed data)
            const Vector4D *GetTangentData() const
            {
                if ( ( id == MODEL_VERTEX_FILE_ID ) && ( tangentDataStart != 0 ) )
                    return ( Vector4D * ) ( tangentDataStart + (uint8_t *)this );
                else
                return nullptr;
            }

            const mstudiofixup_t* GetFixupData() const
            {
                return (const mstudiofixup_t*)((uint8_t*)this + fixupTableStart);
            }

            /*// Accessor to thin vertex data
            const  thinModelVertices_t *GetThinVertexData() const
            {
                if ( ( id == MODEL_VERTEX_FILE_THIN_ID ) && ( vertexDataStart != 0 ) )
                    return ( thinModelVertices_t * ) ( vertexDataStart + (uint8_t *)this );
                else
                return NULL;
            }*/
        };

// studio models
        struct mstudiomodel_t
        {
            inline const char * pszName( void ) const { return name; }
            char				name[64];

            int					type;

            float				boundingradius;

            int					nummeshes;
            int					meshindex;
            inline mstudiomesh_t *pMesh( int i ) const { return (mstudiomesh_t *)(((uint8_t *)this) + meshindex) + i; };

            // cache purposes
            int					numvertices;		// number of unique vertices/normals/texcoords
            int					vertexindex;		// vertex Vector
            int					tangentsindex;		// tangents Vector

            // Access thin/fat mesh vertex data (only one will return a non-nullptr result)
            const mstudio_modelvertexdata_t		*GetVertexData(		void *pModelData = nullptr );
            //const thinModelVertices_t			*GetThinVertexData(	void *pModelData = nullptr );

            int					numattachments;
            int					attachmentindex;

            int					numeyeballs;
            int					eyeballindex;
            inline  mstudioeyeball_t *pEyeball( int i ) { return (mstudioeyeball_t *)(((uint8_t *)this) + eyeballindex) + i; };

            mstudio_modelvertexdata_t vertexdata;

            int					unused[6];		// remove as appropriate
        };

        inline bool mstudio_modelvertexdata_t::HasTangentData( void ) const
        {
            return (pTangentData != nullptr);
        }

        inline int mstudio_modelvertexdata_t::GetGlobalVertexIndex( int i ) const
        {
            mstudiomodel_t *modelptr = (mstudiomodel_t *)((uint8_t *)this - offsetof(mstudiomodel_t, vertexdata));
            DEBUG_CHECK( ( modelptr->vertexindex % sizeof( mstudiovertex_t ) ) == 0 );
            return ( i + ( modelptr->vertexindex / sizeof( mstudiovertex_t ) ) );
        }

        inline int mstudio_modelvertexdata_t::GetGlobalTangentIndex( int i ) const
        {
            mstudiomodel_t *modelptr = (mstudiomodel_t *)((uint8_t *)this - offsetof(mstudiomodel_t, vertexdata));
            DEBUG_CHECK( ( modelptr->tangentsindex % sizeof( Vector4D ) ) == 0 );
            return ( i + ( modelptr->tangentsindex / sizeof( Vector4D ) ) );
        }


        inline const mstudiovertex_t *mstudio_modelvertexdata_t::GlobalVertex( int i ) const
        {
            return (mstudiovertex_t *)pVertexData + i;
        }

        inline const Vector4D *mstudio_modelvertexdata_t::GlobalTangentS( int i ) const
        {
            // NOTE: The tangents vector is 16-Uint8s in a separate array
            // because it only exists on the high end, and if I leave it out
            // of the mstudiovertex_t, the vertex is 64-Uint8s (good for low end)
            return (Vector4D *)pTangentData + i;
        }

        inline mstudiomodel_t *mstudiomesh_t::pModel() const
        {
            return (mstudiomodel_t *)(((uint8_t *)this) + modelindex);
        }


// a group of studio model data
        enum studiomeshgroupflags_t
        {
            MESHGROUP_IS_FLEXED			= 0x1,
            MESHGROUP_IS_HWSKINNED		= 0x2,
            MESHGROUP_IS_DELTA_FLEXED	= 0x4
        };



// ----------------------------------------------------------
// ----------------------------------------------------------

// body part index
        struct mstudiobodyparts_t
        {
            int					sznameindex;
            inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
            int					nummodels;
            int					base;
            int					modelindex; // index into models array
            inline mstudiomodel_t *pModel( int i ) const { return (mstudiomodel_t *)(((uint8_t *)this) + modelindex) + i; };
        };


        struct mstudiomouth_t
        {
            int					bone;
            Vector				forward;
            int					flexdesc;
        };

        struct mstudiohitboxset_t
        {
            int					sznameindex;
            inline char * const	pszName( void ) const { return ((char *)this) + sznameindex; }
            int					numhitboxes;
            int					hitboxindex;
            inline mstudiobbox_t *pHitbox( int i ) const { return (mstudiobbox_t *)(((uint8_t *)this) + hitboxindex) + i; };
        };


//-----------------------------------------------------------------------------
// Src bone transforms are transformations that will convert .dmx or .smd-based animations into .mdl-based animations
// NOTE: The operation you should apply is: pretransform * bone transform * posttransform
//-----------------------------------------------------------------------------
        struct mstudiosrcbonetransform_t
        {
            int			sznameindex;
            inline const char *pszName( void ) const { return ((char *)this) + sznameindex; }
            float pretransform[12];
            float posttransform[12];
        };

// model vertex data accessor (defined here so vertexFileHeader_t can be used)
        inline const mstudio_modelvertexdata_t * mstudiomodel_t::GetVertexData( void *pModelData )
        {

            if ( !vertexdata.pVertexData )
                return nullptr;

            return &vertexdata;
        }

// model thin vertex data accessor (defined here so vertexFileHeader_t can be used)
        /*inline const thinModelVertices_t * mstudiomodel_t::GetThinVertexData( void *pModelData )
        {
            const vertexFileHeader_t * pVertexHdr = CacheVertexData( pModelData );
            if ( !pVertexHdr )
                return nullptr;

            return pVertexHdr->GetThinVertexData();
        }*/

// apply sequentially to lod sorted vertex and tangent pools to re-establish mesh order
        struct vertexFileFixup_t
        {
            int		lod;				// used to skip culled root lod
            int		sourceVertexID;		// absolute index from start of vertex/tangent blocks
            int		numVertexes;
        };

// This flag is set if no hitbox information was specified
#define STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX				0x00000001

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_ENV_CUBEMAP					0x00000002

// Use this when there are translucent parts to the model but we're not going to sort it
#define STUDIOHDR_FLAGS_FORCE_OPAQUE						0x00000004

// Use this when we want to render the opaque parts during the opaque pass
// and the translucent parts during the translucent pass
#define STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS					0x00000008

// This is set any time the .qc files has $staticprop in it
// Means there's no bones and no transforms
#define STUDIOHDR_FLAGS_STATIC_PROP							0x00000010

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_FB_TEXTURE						0x00000020

// This flag is set by studiomdl.exe if a separate "$shadowlod" entry was present
//  for the .mdl (the shadow lod is the last entry in the lod list if present)
#define STUDIOHDR_FLAGS_HASSHADOWLOD						0x00000040

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_BUMPMAPPING					0x00000080

// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
// instead of overriding them with the default one (necessary for translucent shadows)
#define STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS				0x00000100

// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
// instead of overriding them with the default one (necessary for translucent shadows)
#define STUDIOHDR_FLAGS_OBSOLETE							0x00000200

#define STUDIOHDR_FLAGS_UNUSED								0x00000400

// NOTE:  This flag is set at mdl build time
#define STUDIOHDR_FLAGS_NO_FORCED_FADE						0x00000800

// NOTE:  The npc will lengthen the viseme check to always include two phonemes
#define STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE				0x00001000

// This flag is set when the .qc has $constantdirectionallight in it
// If set, we use constantdirectionallightdot to calculate light intensity
// rather than the normal directional dot product
// only valid if STUDIOHDR_FLAGS_STATIC_PROP is also set
#define STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT		0x00002000

// Flag to mark delta flexes as already converted from disk format to memory format
#define STUDIOHDR_FLAGS_FLEXES_CONVERTED					0x00004000

// Indicates the studiomdl was built in preview mode
#define STUDIOHDR_FLAGS_BUILT_IN_PREVIEW_MODE				0x00008000

// Ambient boost (runtime flag)
#define STUDIOHDR_FLAGS_AMBIENT_BOOST						0x00010000

// Don't cast shadows from this model (useful on first-person models)
#define STUDIOHDR_FLAGS_DO_NOT_CAST_SHADOWS					0x00020000

// alpha textures should cast shadows in vrad on this model (ONLY prop_static!)
#define STUDIOHDR_FLAGS_CAST_TEXTURE_SHADOWS				0x00040000


// flagged on load to indicate no animation events on this model
#define STUDIOHDR_FLAGS_VERT_ANIM_FIXED_POINT_SCALE			0x00200000

// NOTE! Next time we up the .mdl file format, remove studiohdr2_t
// and insert all fields in this structure into studiohdr_t.
        struct studiohdr2_t
        {
            // NOTE: For forward compat, make sure any methods in this struct
            // are also available in studiohdr_t so no leaf code ever directly references
            // a studiohdr2_t structure
            int numsrcbonetransform;
            int srcbonetransformindex;

            int	illumpositionattachmentindex;
            inline int			IllumPositionAttachmentIndex() const { return illumpositionattachmentindex; }

            float flMaxEyeDeflection;
            inline float		MaxEyeDeflection() const { return flMaxEyeDeflection != 0.0f ? flMaxEyeDeflection : 0.866f; } // default to cos(30) if not set

            int linearboneindex;
            inline mstudiolinearbone_t *pLinearBones() const { return (linearboneindex) ? (mstudiolinearbone_t *)(((uint8_t *)this) + linearboneindex) : nullptr; }

            int sznameindex;
            inline char *pszName() { return (sznameindex) ? (char *)(((uint8_t *)this) + sznameindex ) : nullptr; }

            int m_nBoneFlexDriverCount;
            int m_nBoneFlexDriverIndex;
            inline mstudioboneflexdriver_t *pBoneFlexDriver( int i ) const { DEBUG_CHECK( i >= 0 && i < m_nBoneFlexDriverCount ); return (mstudioboneflexdriver_t *)(((uint8_t *)this) + m_nBoneFlexDriverIndex) + i; }

            int reserved[56];
        };


        struct studiohdr_t
        {
            int					id;
            int					version;

            int					checksum;		// this has to be the same in the phy and vtx files to load!

            inline const char *	pszName( void ) const { if (studiohdr2index && pStudioHdr2()->pszName()) return pStudioHdr2()->pszName(); else return name; }
            char				name[64];
            int					length;


            Vector				eyeposition;	// ideal eye position

            Vector				illumposition;	// illumination center

            Vector				hull_min;		// ideal movement hull size
            Vector				hull_max;

            Vector				view_bbmin;		// clipping bounding box
            Vector				view_bbmax;

            int					flags;

            int					numbones;			// bones
            int					boneindex;
            inline mstudiobone_t *pBone( int i ) const { DEBUG_CHECK( i >= 0 && i < numbones); return (mstudiobone_t *)(((uint8_t *)this) + boneindex) + i; };
            int					RemapSeqBone( int iSequence, int iLocalBone ) const;	// maps local sequence bone to global bone
            int					RemapAnimBone( int iAnim, int iLocalBone ) const;		// maps local animations bone to global bone

            int					numbonecontrollers;		// bone controllers
            int					bonecontrollerindex;
            inline mstudiobonecontroller_t *pBonecontroller( int i ) const { DEBUG_CHECK( i >= 0 && i < numbonecontrollers); return (mstudiobonecontroller_t *)(((uint8_t *)this) + bonecontrollerindex) + i; };

            int					numhitboxsets;
            int					hitboxsetindex;

            // Look up hitbox set by index
            mstudiohitboxset_t	*pHitboxSet( int i ) const
            {
                DEBUG_CHECK( i >= 0 && i < numhitboxsets);
                return (mstudiohitboxset_t *)(((uint8_t *)this) + hitboxsetindex ) + i;
            };

            // Calls through to hitbox to determine size of specified set
            inline mstudiobbox_t *pHitbox( int i, int set ) const
            {
                mstudiohitboxset_t const *s = pHitboxSet( set );
                if ( !s )
                    return nullptr;

                return s->pHitbox( i );
            };

            // Calls through to set to get hitbox count for set
            inline int			iHitboxCount( int set ) const
            {
                mstudiohitboxset_t const *s = pHitboxSet( set );
                if ( !s )
                    return 0;

                return s->numhitboxes;
            };

            // file local animations? and sequences
//private:
            int					numlocalanim;			// animations/poses
            int					localanimindex;		// animation descriptions
            inline mstudioanimdesc_t *pLocalAnimdesc( int i ) const { if (i < 0 || i >= numlocalanim) i = 0; return (mstudioanimdesc_t *)(((uint8_t *)this) + localanimindex) + i; };

            int					numlocalseq;				// sequences
            int					localseqindex;
            inline mstudioseqdesc_t *pLocalSeqdesc( int i ) const { if (i < 0 || i >= numlocalseq) i = 0; return (mstudioseqdesc_t *)(((uint8_t *)this) + localseqindex) + i; };

//public:
            bool				SequencesAvailable() const;
            int					GetNumSeq() const;
            mstudioanimdesc_t	&pAnimdesc( int i ) const;
            mstudioseqdesc_t	&pSeqdesc( int i ) const;
            int					iRelativeAnim( int baseseq, int relanim ) const;	// maps seq local anim reference to global anim index
            int					iRelativeSeq( int baseseq, int relseq ) const;		// maps seq local seq reference to global seq index

//private:
            mutable int			activitylistversion;	// initialization flag - have the sequences been indexed?
            mutable int			eventsindexed;
//public:
            int					GetSequenceActivity( int iSequence );
            void				SetSequenceActivity( int iSequence, int iActivity );
            int					GetActivityListVersion( void );
            void				SetActivityListVersion( int version ) const;
            int					GetEventListVersion( void );
            void				SetEventListVersion( int version );

            // raw textures
            int					numtextures;
            int					textureindex;
            inline mstudiotexture_t *pTexture( int i ) const { DEBUG_CHECK( i >= 0 && i < numtextures ); return (mstudiotexture_t *)(((uint8_t *)this) + textureindex) + i; };


            // raw textures search paths
            int					numcdtextures;
            int					cdtextureindex;
            inline char			*pCdtexture( int i ) const { return (((char *)this) + *((int *)(((uint8_t *)this) + cdtextureindex) + i)); };

            // replaceable textures tables
            int					numskinref;
            int					numskinfamilies;
            int					skinindex;
            inline short		*pSkinref( int i ) const { return (short *)(((uint8_t *)this) + skinindex) + i; };

            int					numbodyparts;
            int					bodypartindex;
            inline mstudiobodyparts_t	*pBodypart( int i ) const { return (mstudiobodyparts_t *)(((uint8_t *)this) + bodypartindex) + i; };

            // queryable attachable points
//private:
            int					numlocalattachments;
            int					localattachmentindex;
            inline mstudioattachment_t	*pLocalAttachment( int i ) const { DEBUG_CHECK( i >= 0 && i < numlocalattachments); return (mstudioattachment_t *)(((uint8_t *)this) + localattachmentindex) + i; };
//public:
            int					GetNumAttachments( void ) const;
            const mstudioattachment_t &pAttachment( int i ) const;
            int					GetAttachmentBone( int i );
            // used on my tools in hlmv, not persistant
            void				SetAttachmentBone( int iAttachment, int iBone );

            // animation node to animation node transition graph
//private:
            int					numlocalnodes;
            int					localnodeindex;
            int					localnodenameindex;
            inline char			*pszLocalNodeName( int iNode ) const { DEBUG_CHECK( iNode >= 0 && iNode < numlocalnodes); return (((char *)this) + *((int *)(((uint8_t *)this) + localnodenameindex) + iNode)); }
            inline uint8_t			*pLocalTransition( int i ) const { DEBUG_CHECK( i >= 0 && i < (numlocalnodes * numlocalnodes)); return (uint8_t *)(((uint8_t *)this) + localnodeindex) + i; };

//public:
            int					EntryNode( int iSequence );
            int					ExitNode( int iSequence );
            char				*pszNodeName( int iNode );
            int					GetTransition( int iFrom, int iTo ) const;

            int					numflexdesc;
            int					flexdescindex;
            inline mstudioflexdesc_t *pFlexdesc( int i ) const { DEBUG_CHECK( i >= 0 && i < numflexdesc); return (mstudioflexdesc_t *)(((uint8_t *)this) + flexdescindex) + i; };

            int					numflexcontrollers;
            int					flexcontrollerindex;
            inline mstudioflexcontroller_t *pFlexcontroller( int i ) const { DEBUG_CHECK( numflexcontrollers == 0 || ( i >= 0 && i < numflexcontrollers ) ); return (mstudioflexcontroller_t *)(((uint8_t *)this) + flexcontrollerindex) + i; };

            int					numflexrules;
            int					flexruleindex;
            inline mstudioflexrule_t *pFlexRule( int i ) const { DEBUG_CHECK( i >= 0 && i < numflexrules); return (mstudioflexrule_t *)(((uint8_t *)this) + flexruleindex) + i; };

            int					numikchains;
            int					ikchainindex;
            inline mstudioikchain_t *pIKChain( int i ) const { DEBUG_CHECK( i >= 0 && i < numikchains); return (mstudioikchain_t *)(((uint8_t *)this) + ikchainindex) + i; };

            int					nummouths;
            int					mouthindex;
            inline mstudiomouth_t *pMouth( int i ) const { DEBUG_CHECK( i >= 0 && i < nummouths); return (mstudiomouth_t *)(((uint8_t *)this) + mouthindex) + i; };

//private:
            int					numlocalposeparameters;
            int					localposeparamindex;
            inline mstudioposeparamdesc_t *pLocalPoseParameter( int i ) const { DEBUG_CHECK( i >= 0 && i < numlocalposeparameters); return (mstudioposeparamdesc_t *)(((uint8_t *)this) + localposeparamindex) + i; };
//public:
            int					GetNumPoseParameters( void ) const;
            const mstudioposeparamdesc_t &pPoseParameter( int i );
            int					GetSharedPoseParameter( int iSequence, int iLocalPose ) const;

            int					surfacepropindex;
            inline char * const pszSurfaceProp( void ) const { return ((char *)this) + surfacepropindex; }

            // Key values
            int					keyvalueindex;
            int					keyvaluesize;
            inline const char * KeyValueText( void ) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : nullptr; }

            int					numlocalikautoplaylocks;
            int					localikautoplaylockindex;
            inline mstudioiklock_t *pLocalIKAutoplayLock( int i ) const { DEBUG_CHECK( i >= 0 && i < numlocalikautoplaylocks); return (mstudioiklock_t *)(((uint8_t *)this) + localikautoplaylockindex) + i; };

            int					GetNumIKAutoplayLocks( void ) const;
            const mstudioiklock_t &pIKAutoplayLock( int i );
            int					CountAutoplaySequences() const;
            int					CopyAutoplaySequences( unsigned short *pOut, int outCount ) const;
            int					GetAutoplayList( unsigned short **pOut ) const;

            // The collision model mass that jay wanted
            float				mass;
            int					contents;

            // external animations, models, etc.
            int					numincludemodels;
            int					includemodelindex;
            inline mstudiomodelgroup_t *pModelGroup( int i ) const { DEBUG_CHECK( i >= 0 && i < numincludemodels); return (mstudiomodelgroup_t *)(((uint8_t *)this) + includemodelindex) + i; };
            // implementation specific call to get a named model
            const studiohdr_t	*FindModel( void **cache, char const *modelname ) const;

            // implementation specific back pointer to virtual data
            mutable uint32_t		virtualModelPtr;
            //virtualmodel_t		*GetVirtualModel( void ) const;

            // for demand loaded animation blocks
            int					szanimblocknameindex;
            inline char * const pszAnimBlockName( void ) const { return ((char *)this) + szanimblocknameindex; }
            int					numanimblocks;
            int					animblockindex;
            inline mstudioanimblock_t *pAnimBlock( int i ) const { DEBUG_CHECK( i > 0 && i < numanimblocks); return (mstudioanimblock_t *)(((uint8_t *)this) + animblockindex) + i; };
            mutable uint32_t		animblockModelPtr;
            uint8_t *				GetAnimBlock( int i ) const;

            int					bonetablebynameindex;
            inline const uint8_t	*GetBoneTableSortedByName() const { return (uint8_t *)this + bonetablebynameindex; }

            // used by tools only that don't cache, but persist mdl's peer data
            // engine uses virtualModel to back link to cache pointers
            uint32_t pVertexBasePtr;
            uint32_t pIndexBasePtr;

            // if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
            // this value is used to calculate directional components of lighting
            // on static props
            uint8_t				constdirectionallightdot;

            // set during load of mdl data to track *desired* lod configuration (not actual)
            // the *actual* clamped root lod is found in studiohwdata
            // this is stored here as a global store to ensure the staged loading matches the rendering
            uint8_t				rootLOD;

            // set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
            // to be set as root LOD:
            //	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
            //	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
            uint8_t				numAllowedRootLODs;

            uint8_t				unused[1];

            int					unused4; // zero out if version < 47

            int					numflexcontrollerui;
            int					flexcontrolleruiindex;
            mstudioflexcontrollerui_t *pFlexControllerUI( int i ) const { DEBUG_CHECK( i >= 0 && i < numflexcontrollerui); return (mstudioflexcontrollerui_t *)(((uint8_t *)this) + flexcontrolleruiindex) + i; }

            float				flVertAnimFixedPointScale;
            inline float		VertAnimFixedPointScale() const { return ( flags & STUDIOHDR_FLAGS_VERT_ANIM_FIXED_POINT_SCALE ) ? flVertAnimFixedPointScale : 1.0f / 4096.0f; }

            int					unused3[1];

            // FIXME: Remove when we up the model version. Move all fields of studiohdr2_t into studiohdr_t.
            int					studiohdr2index;
            studiohdr2_t*		pStudioHdr2() const { return (studiohdr2_t *)( ( (uint8_t *)this ) + studiohdr2index ); }

            // Src bone transforms are transformations that will convert .dmx or .smd-based animations into .mdl-based animations
            int					NumSrcBoneTransforms() const { return studiohdr2index ? pStudioHdr2()->numsrcbonetransform : 0; }
            const mstudiosrcbonetransform_t* SrcBoneTransform( int i ) const { DEBUG_CHECK( i >= 0 && i < NumSrcBoneTransforms()); return (mstudiosrcbonetransform_t *)(((uint8_t *)this) + pStudioHdr2()->srcbonetransformindex) + i; }

            inline int			IllumPositionAttachmentIndex() const { return studiohdr2index ? pStudioHdr2()->IllumPositionAttachmentIndex() : 0; }

            inline float		MaxEyeDeflection() const { return studiohdr2index ? pStudioHdr2()->MaxEyeDeflection() : 0.866f; } // default to cos(30) if not set

            inline mstudiolinearbone_t *pLinearBones() const { return studiohdr2index ? pStudioHdr2()->pLinearBones() : nullptr; }

            inline int			BoneFlexDriverCount() const { return studiohdr2index ? pStudioHdr2()->m_nBoneFlexDriverCount : 0; }
            inline const mstudioboneflexdriver_t* BoneFlexDriver( int i ) const { DEBUG_CHECK( i >= 0 && i < BoneFlexDriverCount() ); return studiohdr2index ? pStudioHdr2()->pBoneFlexDriver( i ) : nullptr; }

            // NOTE: No room to add stuff? Up the .mdl file format version
            // [and move all fields in studiohdr2_t into studiohdr_t and kill studiohdr2_t],
            // or add your stuff to studiohdr2_t. See NumSrcBoneTransforms/SrcBoneTransform for the pattern to use.
            int					unused2[1];
        };

#define OPTIMIZED_MODEL_FILE_VERSION 7

#pragma pack(1)

        namespace OptimizedModel
        {
            struct BoneStateChangeHeader_t
            {
                int hardwareID;
                int newBoneID;
            };

            struct Vertex_t
            {
                // these index into the mesh's vert[origMeshVertID]'s bones
                unsigned char boneWeightIndex[MAX_NUM_BONES_PER_VERT];
                unsigned char numBones;

                unsigned short origMeshVertID;

                // for sw skinned verts, these are indices into the global list of bones
                // for hw skinned verts, these are hardware bone indices
                char boneID[MAX_NUM_BONES_PER_VERT];
            };

            enum StripHeaderFlags_t {
                STRIP_IS_TRILIST	= 0x01,
                STRIP_IS_TRISTRIP	= 0x02
            };

// a strip is a piece of a stripgroup that is divided by bones
// (and potentially tristrips if we remove some degenerates.)
            struct StripHeader_t
            {
                // indexOffset offsets into the mesh's index array.
                int numIndices;
                int indexOffset;

                // vertexOffset offsets into the mesh's vert array.
                int numVerts;
                int vertOffset;

                // use this to enable/disable skinning.
                // May decide (in optimize.cpp) to put all with 1 bone in a different strip
                // than those that need skinning.
                short numBones;

                unsigned char flags;

                int numBoneStateChanges;
                int boneStateChangeOffset;
                inline BoneStateChangeHeader_t *pBoneStateChange( int i ) const
                {
                    return (BoneStateChangeHeader_t *)(((uint8_t *)this) + boneStateChangeOffset) + i;
                };
            };

            enum StripGroupFlags_t
            {
                STRIPGROUP_IS_FLEXED		= 0x01,
                STRIPGROUP_IS_HWSKINNED		= 0x02,
                STRIPGROUP_IS_DELTA_FLEXED	= 0x04,
                STRIPGROUP_SUPPRESS_HW_MORPH = 0x08,	// NOTE: This is a temporary flag used at run time.
            };

            struct StripGroupHeader_t
            {
                // These are the arrays of all verts and indices for this mesh.  strips index into this.
                int numVerts;
                int vertOffset;
                inline Vertex_t *pVertex( int i ) const
                {
                    return (Vertex_t *)(((uint8_t *)this) + vertOffset) + i;
                };

                int numIndices;
                int indexOffset;
                inline unsigned short *pIndex( int i ) const
                {
                    return (unsigned short *)(((uint8_t *)this) + indexOffset) + i;
                };

                int numStrips;
                int stripOffset;
                inline StripHeader_t *pStrip( int i ) const
                {
                    return (StripHeader_t *)(((uint8_t *)this) + stripOffset) + i;
                };

                unsigned char flags;
            };

            enum MeshFlags_t {
                // these are both material properties, and a mesh has a single material.
                    MESH_IS_TEETH	= 0x01,
                MESH_IS_EYES	= 0x02
            };

            struct MeshHeader_t
            {
                int numStripGroups;
                int stripGroupHeaderOffset;
                inline StripGroupHeader_t *pStripGroup( int i ) const
                {
                    StripGroupHeader_t *pDebug = (StripGroupHeader_t *)(((uint8_t *)this) + stripGroupHeaderOffset) + i;
                    return pDebug;
                };
                unsigned char flags;
            };

            struct ModelLODHeader_t
            {
                int numMeshes;
                int meshOffset;
                float switchPoint;
                inline MeshHeader_t *pMesh( int i ) const
                {
                    MeshHeader_t *pDebug = (MeshHeader_t *)(((uint8_t *)this) + meshOffset) + i;
                    return pDebug;
                };
            };

            struct ModelHeader_t
            {
                int numLODs; // garymcthack - this is also specified in FileHeader_t
                int lodOffset;
                inline ModelLODHeader_t *pLOD( int i ) const
                {
                    ModelLODHeader_t *pDebug = ( ModelLODHeader_t *)(((uint8_t *)this) + lodOffset) + i;
                    return pDebug;
                };
            };

            struct BodyPartHeader_t
            {
                int numModels;
                int modelOffset;
                inline ModelHeader_t *pModel( int i ) const
                {
                    ModelHeader_t *pDebug = (ModelHeader_t *)(((uint8_t *)this) + modelOffset) + i;
                    return pDebug;
                };
            };

            struct MaterialReplacementHeader_t
            {
                short materialID;
                int replacementMaterialNameOffset;
                inline const char *pMaterialReplacementName( void )
                {
                    const char *pDebug = (const char *)(((uint8_t *)this) + replacementMaterialNameOffset);
                    return pDebug;
                }
            };

            struct MaterialReplacementListHeader_t
            {
                int numReplacements;
                int replacementOffset;
                inline MaterialReplacementHeader_t *pMaterialReplacement( int i ) const
                {
                    MaterialReplacementHeader_t *pDebug = ( MaterialReplacementHeader_t *)(((uint8_t *)this) + replacementOffset) + i;
                    return pDebug;
                }
            };

            struct FileHeader_t
            {
                // file version as defined by OPTIMIZED_MODEL_FILE_VERSION
                int version;

                // hardware params that affect how the model is to be optimized.
                int vertCacheSize;
                unsigned short maxBonesPerStrip;
                unsigned short maxBonesPerTri;
                int maxBonesPerVert;

                // must match checkSum in the .mdl
                int checkSum;

                int numLODs; // garymcthack - this is also specified in ModelHeader_t and should match

                // one of these for each LOD
                int materialReplacementListOffset;
                MaterialReplacementListHeader_t *pMaterialReplacementList( int lodID ) const
                {
                    MaterialReplacementListHeader_t *pDebug =
                        (MaterialReplacementListHeader_t *)(((uint8_t *)this) + materialReplacementListOffset) + lodID;
                    return pDebug;
                }

                int numBodyParts;
                int bodyPartOffset;
                inline BodyPartHeader_t *pBodyPart( int i ) const
                {
                    BodyPartHeader_t *pDebug = (BodyPartHeader_t *)(((uint8_t *)this) + bodyPartOffset) + i;
                    return pDebug;
                };
            };

        } // OptimizedModel

#pragma pack(pop)

        //---

        // is this a valid file ?
        extern bool ValidateFile(const studiohdr_t& file);

    } // mdl
} // hl2