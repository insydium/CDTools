// R12+ Compatibility
#ifndef _CDCOMPATIBILITY_H_
#define _CDCOMPATIBILITY_H_

#include "c4d.h"
#include "ge_math.h"
#include "ge_matrix.h"
#include "ge_vector.h"

#include "CDTypes.h"

#if API_VERSION > 14999
    #include "CDLegacy.h"
#endif

// compatibility defines
#if API_VERSION < 12000
	#define CDMAXREAL		( 9.0e18)
	#define	CDMINREAL		(-9.0e18)
	#define CDMAXLONG		( 0x7fffffff)
	#define	CDMINLONG		(-0x7fffffff)
#else
	#define CDMAXREAL		( 1.0e308)
	#define	CDMINREAL		(-1.0e308)
	#define CDMAXLONG		( 0x7fffffff)
	#define	CDMINLONG		(-0x7fffffff)
#endif

// defines for prefs colors
#define CD_WPREF_VERTEXSTART_COL						10043
#define CD_WPREF_VERTEXEND_COL							10044

#define CD_VIEWCOLOR_VERTEXSTART						36
#define CD_VIEWCOLOR_VERTEXEND							37
#define CD_VIEWCOLOR_ACTIVEPOLYBOX						45
#define CD_VIEWCOLOR_ACTIVEPOLYCHILDBOX					46
#define CD_VIEWCOLOR_SELECTION_PREVIEW					47
#define CD_VIEWCOLOR_ACTIVEPOINT						12

// savable demo flags
#define CD_VERSION_SAVABLEDEMO				(1<<12)
#define CD_VERSION_SAVABLEDEMO_ACTIVE		(1<<13)

// predefined calling points for tags and scene hooks
#define	CD_EXECUTION_PRIORITY_INITIAL					1000
#define CD_EXECUTION_PRIORITY_ANIMATION					2000
#define CD_EXECUTION_PRIORITY_ANIMATION_NLA				2010
#define CD_EXECUTION_PRIORITY_EXPRESSION				3000
#define CD_EXECUTION_PRIORITY_DYNAMICS					4000
#define CD_EXECUTION_PRIORITY_GENERATOR					5000

#if API_VERSION < 15000
    #include "c4d_memory.h"
	#if API_VERSION < 10500
		#define CDDataAllocator new(0,NULL)
	#else
		#define CDDataAllocator new(std::nothrow,__LINE__,__FILE__)
	#endif
#else
    #include "defaultallocator.h"
    #define CDDataAllocator(T, ...) new (maxon::DefaultAllocator::AllocClear(SIZEOF(T), C4D_MISC_ALLOC_LOCATION)) T(__VA_ARGS__)
#endif

template<typename T>
T* CDAlloc(LONG s)
{
#if API_VERSION < 15000
	LONG size = sizeof(T) * s;
	return (T*)GeAlloc(size);
#else
    return (T*)NewMemClear(T,s);
#endif
}

template<typename T>
void CDFree(T& ptr)
{
#if API_VERSION < 15000
    GeFree(ptr);
#else
    DeleteMem(ptr);
#endif
}

template<typename T>
T* CDMAlloc(int s)
{
	int size = sizeof(T) * s;
	return (T*)malloc(size);
}

template<typename T>
void CDMFree(T& ptr)
{
	if(ptr)
	{
		free(ptr);
		ptr = NULL;
	}
}

#if API_VERSION < 12000
class CDBaseSceneHook : public PluginSceneHook
#else
class CDBaseSceneHook : public BaseSceneHook
#endif
{
private:
	CDBaseSceneHook();
	~CDBaseSceneHook();
};


// plugin types
enum
{
	CD_COMMAND_PLUGIN		= 0,
	CD_TOOL_PLUGIN,
	CD_OBJECT_PLUGIN,
	CD_TAG_PLUGIN,
	CD_MESSAGE_PLUGIN,
	CD_SCENEHOOK_PLUGIN,
	CD_PREFS_PLUGIN
};

// undo 
enum
{
	CD_UNDO_CHANGE					= 40,
	CD_UNDO_CHANGE_NOCHILDS			= 41,
	CD_UNDO_CHANGE_SMALL			= 42,
	CD_UNDO_CHANGE_SELECTION		= 43,

	CD_UNDO_NEW						= 44,
	CD_UNDO_DELETE					= 45,

	CD_UNDO_ACTIVATE				= 46,
	CD_UNDO_DEACTIVATE				= 47,
	
	CD_UNDO_BITS					= 48
};


// execution
enum
{
	CD_EXECUTION_RESULT_OK				= 0,
	CD_EXECUTION_RESULT_USERBREAK,
	CD_EXECUTION_RESULT_MEMORYERROR
};

enum
{
	CD_EXECUTION_0						= 0,
	CD_EXECUTION_ANIMATION				= (1<<1),
	CD_EXECUTION_EXPRESSION				= (1<<2),
	CD_EXECUTION_CACHEBUILDING			= (1<<3),
	CD_EXECUTION_CAMERAONLY				= (1<<4),
	CD_EXECUTION_INDRAG					= (1<<5),
	CD_EXECUTION_INMOVE					= (1<<6),
	CD_EXECUTION_RENDER					= (1<<7)
};


// draw
enum
{
	CD_DRAWHANDLE_MINI				= 0,
	CD_DRAWHANDLE_SMALL				= 1,
	CD_DRAWHANDLE_MIDDLE			= 2,
	CD_DRAWHANDLE_BIG				= 3,
	CD_DRAWHANDLE_CUSTOM			= 4
};

enum
{
	CD_DRAWRESULT_ERROR		= 0,
	CD_DRAWRESULT_OK		= 1,
	CD_DRAWRESULT_SKIP		= 2
};

enum
{
	CD_DRAWPASS_OBJECT			= 0,
	CD_DRAWPASS_BOX				= 1,
	CD_DRAWPASS_HANDLES			= 2,
	CD_DRAWPASS_HIGHLIGHTS		= 3,
	CD_DRAWPASS_XRAY			= 4
};

enum
{
	CD_TOOLDRAW_0					= 0,
	CD_TOOLDRAW_HANDLES				= (1<<0),
	CD_TOOLDRAW_AXIS				= (1<<1),
	CD_TOOLDRAW_HIGHLIGHTS			= (1<<2)
};

enum
{
	CD_TOOLDRAWFLAGS_0				= 0,
	CD_TOOLDRAWFLAGS_INVERSE_Z		= (1<<0),
	CD_TOOLDRAWFLAGS_HIGHLIGHT		= (1<<1)
};

enum
{
	CD_SCENEHOOKDRAW_0								= 0,
	CD_SCENEHOOKDRAW_DRAW_PASS						= (1<<0),
	CD_SCENEHOOKDRAW_HIGHLIGHT_PASS_BEFORE_TOOL		= (1<<1),
	CD_SCENEHOOKDRAW_HIGHLIGHT_PASS					= (1<<2),
	CD_SCENEHOOKDRAW_HIGHLIGHT_PASS_INV				= (1<<3)
} ;

enum
{
	CD_DRAWFLAGS_0									= 0,
	CD_DRAWFLAGS_NO_THREAD							= (1<<1),
	CD_DRAWFLAGS_NO_REDUCTION						= (1<<2),
	CD_DRAWFLAGS_NO_ANIMATION						= (1<<8),
	CD_DRAWFLAGS_ONLY_ACTIVE_VIEW					= (1<<10),
	CD_DRAWFLAGS_NO_EXPRESSIONS						= (1<<12),
	CD_DRAWFLAGS_INDRAG								= (1<<13),
	CD_DRAWFLAGS_FORCEFULLREDRAW					= (1<<15),
	CD_DRAWFLAGS_ONLY_CAMERAEXPRESSION				= (1<<16),
	CD_DRAWFLAGS_INMOVE								= (1<<17),
	CD_DRAWFLAGS_ONLY_BASEDRAW						= (1<<22), // draw specific BaseDraw only
	
	CD_DRAWFLAGS_ONLY_HIGHLIGHT						= (1<<18),
	CD_DRAWFLAGS_STATICBREAK						= (1<<19), // only use in combination with DRAWFLAGS_NO_THREAD
	
	CD_DRAWFLAGS_PRIVATE_NO_WAIT_GL_FINISHED		= (1<<3),
	CD_DRAWFLAGS_PRIVATE_ONLYBACKGROUND				= (1<<4),
	CD_DRAWFLAGS_PRIVATE_NOBLIT						= (1<<9),
	CD_DRAWFLAGS_PRIVATE_OPENGLHACK					= (1<<11),
	CD_DRAWFLAGS_PRIVATE_ONLY_PREPARE				= (1<<21),
	CD_DRAWFLAGS_PRIVATE_NO_3DCLIPPING				= (1<<24)
};


// copy
enum
{
	CD_COPYFLAGS_0									= 0,
	CD_COPYFLAGS_NO_HIERARCHY						= (1<<2),
	CD_COPYFLAGS_NO_ANIMATION						= (1<<3),
	CD_COPYFLAGS_NO_BITS							= (1<<4),
	CD_COPYFLAGS_NO_MATERIALPREVIEW					= (1<<5),
	CD_COPYFLAGS_NO_BRANCHES						= (1<<7),
	CD_COPYFLAGS_DOCUMENT							= (1<<10),	// this flag is read-only, set when a complete document is copied
	CD_COPYFLAGS_NO_NGONS							= (1<<11),
	CD_COPYFLAGS_CACHE_BUILD						= (1<<13),	// this flags is read-only, set when a cache is built
	CD_COPYFLAGS_RECURSIONCHECK						= (1<<14),
	
	CD_COPYFLAGS_PRIVATE_IDENTMARKER				= (1<<0),	// private 
	CD_COPYFLAGS_PRIVATE_NO_INTERNALS				= (1<<8),	// private
	CD_COPYFLAGS_PRIVATE_NO_PLUGINLAYER				= (1<<9),	// private
	CD_COPYFLAGS_PRIVATE_UNDO						= (1<<12),	// private
	CD_COPYFLAGS_PRIVATE_CONTAINER_COPY_DIRTY		= (1<<15),	// private
	CD_COPYFLAGS_PRIVATE_CONTAINER_COPY_IDENTICAL	= (1<<16),	// private
	CD_COPYFLAGS_PRIVATE_BODYPAINT_NODATA			= (1<<29),  // private
	CD_COPYFLAGS_PRIVATE_BODYPAINT_CONVERTLAYER		= (1<<30)	// private
};


// descriptions
enum
{
	CD_DESCFLAGS_DESC_0							= 0,
	CD_DESCFLAGS_DESC_RESOLVEMULTIPLEDATA		= (1<<0),
	CD_DESCFLAGS_DESC_LOADED					= (1<<1),
	CD_DESCFLAGS_DESC_RECURSIONLOCK				= (1<<2),
	CD_DESCFLAGS_DESC_DONTLOADDEFAULT			= (1<<3), // internal: used for old plugintools
	CD_DESCFLAGS_DESC_MAPTAGS					= (1<<4),
	CD_DESCFLAGS_DESC_NEEDDEFAULTVALUE			= (1<<5) // DESC_DEFAULT needed
};

enum
{
	CD_DESCFLAGS_GET_0								= 0,
	CD_DESCFLAGS_GET_PARAM_GET						= (1<<1),
	CD_DESCFLAGS_GET_NO_GLOBALDATA					= (1<<4),
	CD_DESCFLAGS_GET_NO_GEDATADEFAULTVALUE			= (1<<5)
};

enum
{
	CD_DESCFLAGS_SET_0								= 0,
	CD_DESCFLAGS_SET_PARAM_SET						= (1<<1),
	CD_DESCFLAGS_SET_USERINTERACTION				= (1<<2),
	CD_DESCFLAGS_SET_DONTCHECKMINMAX				= (1<<3),
	CD_DESCFLAGS_SET_DONTAFFECTINHERITANCE			= (1<<6), // for render settings and post effects only (SetParameter)
	CD_DESCFLAGS_SET_FORCESET						= (1<<7) // SetParameter: force the set value without GetParameter/Compare, use only for calls where you definitely changed the value
};

enum
{
	CD_DESCFLAGS_ENABLE_0						= 0
};


// dirty
enum
{
	CD_DIRTYFLAGS_0			= 0,
	CD_DIRTY_MATRIX		= (1<<1),
	CD_DIRTY_DATA		= (1<<2),
	CD_DIRTY_SELECT		= (1<<3),
	CD_DIRTY_CACHE		= (1<<4),
	CD_DIRTY_CHILDREN	= (1<<5)
};


// OS
enum
{
	CD_MAC			= 0,
	CD_WIN,
	CD_UNIX
};


// psr track description
enum
{
	CD_POS_X	= 0,
	CD_POS_Y,
	CD_POS_Z,
	
	CD_SCA_X,
	CD_SCA_Y,
	CD_SCA_Z,
	
	CD_ROT_H,
	CD_ROT_P,
	CD_ROT_B
};

enum
{
	CD_ID_BASEOBJECT_POSITION        = 903,
	CD_ID_BASEOBJECT_ROTATION        = 904,
	CD_ID_BASEOBJECT_SCALE           = 905
};


// key qualifier
enum
{
	CD_QUALIFIER_0				= 0,
	CD_QUALIFIER_SHIFT			= (1<<0),
	CD_QUALIFIER_CTRL			= (1<<1),
	CD_QUALIFIER_MOUSEHIT		= (1<<10)
};


// modeling command
enum
{
	CD_MODELINGCOMMANDMODE_ALL					= 0,
	CD_MODELINGCOMMANDMODE_POINTSELECTION		= 1,
	CD_MODELINGCOMMANDMODE_POLYGONSELECTION		= 2,
	CD_MODELINGCOMMANDMODE_EDGESELECTION		= 3
};

enum
{
	CD_MODELINGCOMMANDFLAGS_0				= 0,
	CD_MODELINGCOMMANDFLAGS_CREATEUNDO		= (1<<0)
};


// get active object
enum
{
	CD_GETACTIVEOBJECTFLAGS_0					= 0,
	CD_GETACTIVEOBJECTFLAGS_CHILDREN			= (1<<0),
	CD_GETACTIVEOBJECTFLAGS_SELECTIONORDER		= (1<<1)
};


// animation
enum
{
	CD_ANIMATEFLAGS_0					= 0,
	CD_ANIMATEFLAGS_NO_PARTICLES		= (1<<2),
	CD_ANIMATEFLAGS_QUICK				= (1<<3), // pre R12 compatibility
	CD_ANIMATEFLAGS_NO_CHILDREN			= (1<<6),
	CD_ANIMATEFLAGS_INRENDER			= (1<<7),
	CD_ANIMATEFLAGS_NO_MINMAX       	= (1<<8),  // private
	CD_ANIMATEFLAGS_NO_NLA          	= (1<<9),  // private
	CD_ANIMATEFLAGS_NLA_SUM         	= (1<<10) // private
};

enum
{
	CD_SPLINETYPE_LINEAR		= 0,
	CD_SPLINETYPE_CUBIC			= 1,
	CD_SPLINETYPE_AKIMA			= 2,
	CD_SPLINETYPE_BSPLINE		= 3,
	CD_SPLINETYPE_BEZIER		= 4
};

// coffee types COFFEE_VTYPE_NIL
enum
{
    CD_COFFEE_NIL				= 0, 	
    CD_COFFEE_LONG				= 1, 	
    CD_COFFEE_FLOAT			= 2,
    CD_COFFEE_VECTOR			= 3,
    CD_COFFEE_VOID				= 4, 	
    CD_COFFEE_BYTES			= 5,
    CD_COFFEE_STRING			= 6,
    CD_COFFEE_CLASS			= 7,
    CD_COFFEE_OBJECT			= 8,
    CD_COFFEE_ARRAY			= 9,
    CD_COFFEE_BYTECODE		= 10,
    CD_COFFEE_CODE				= 11, 	
    CD_COFFEE_EXTCODE		= 12,
    
    CD_COFFEE_EXCEPTION	= 13,
    CD_COFFEE_SEXCEPTION	= 14,
    CD_COFFEE_DICTIONARY	= 15,
    CD_COFFEE_ENTRY			= 16,
    CD_COFFEE_INSTANCE		= 17,
    
    CD_COFFEE_NUMBER			= 99	
};



// BaseObject compatiblity functions
Vector CDGetPos(BaseObject *op);
void CDSetPos(BaseObject *op, Vector pos);
Vector CDGetScale(BaseObject *op);
void CDSetScale(BaseObject *op, Vector sca);
Vector CDGetRot(BaseObject *op);
void CDSetRot(BaseObject *op, Vector rot);
Bool CDIsBone(BaseObject *op);
Bool CDIsDirty(BaseObject *op, LONG dirtyID);

// BaseDraw compatiblity functions
void CDDrawHandle2D(BaseDraw *bd, const Vector &p, LONG type=CD_DRAWHANDLE_SMALL);
void CDDrawHandle3D(BaseDraw *bd, const Vector &vp, LONG type, LONG flags);
void CDDrawLine2D(BaseDraw *bd, Vector a, Vector b);
void CDDrawLine(BaseDraw *bd, Vector a, Vector b);
void CDDrawCircle2D(BaseDraw *bd, LONG mx, LONG my, Real rad);
void CDDrawCircle(BaseDraw *bd, Matrix m);
void CDDrawPolygon(BaseDraw *bd, Vector *p, Vector *f, Bool quad);
void CDSetBaseDrawMatrix(BaseDraw *bd, BaseObject *op, const Matrix &mg);
Bool CDDrawViews(LONG flags, BaseDraw *bd = NULL);

Vector CDGetViewColor(LONG colid);

// SplineObject compatiblity functions
Real CDUniformToNatural(SplineObject *spl, Real val);
Real CDGetSplineLength(SplineObject *spl);
Vector CDGetSplinePoint(SplineObject *spln, Real t, LONG segment=0, const Vector *padr=NULL);
SplineObject* CDAllocateSplineObject(LONG pcnt, LONG type);

// UVWTag compatiblity functions
void CDGetUVWStruct(UVWTag *uvTag, UVWStruct &uv, LONG ind);
void CDSetUVWStruct(UVWTag *uvTag, UVWStruct &uv, LONG ind);

// GeListNode compatiblity functions
Bool CDGetParameter(GeListNode *node, const DescID& id, GeData& t_data, LONG flags = CD_DESCFLAGS_GET_0);
Bool CDSetParameter(GeListNode *node, const DescID& id, const GeData& t_data, LONG flags = CD_DESCFLAGS_SET_0);
Bool CDGetDescription(GeListNode *node, Description *desc, LONG flags);

// C4DAtom compatiblity functions
C4DAtom* CDGetClone(C4DAtom* atom, LONG flags, AliasTrans *trn);

// BaseDocument compatiblity functions
void CDAddUndo(BaseDocument *doc, LONG type, void *data);
void CDAnimateObject(BaseDocument *doc, BaseList2D *op, const BaseTime &time, LONG flags);

// Misc. compatiblity functions
LONG CDGeGetCurrentOS(void);
Bool CDMinVector(Vector v, Real val);
DescID CDGetPSRTrackDescriptionID(LONG trkID);
void CDPriorityListAdd(PriorityList* list, GeListNode* op, LONG priority, LONG flags);
BasePlugin* CDFindPlugin(LONG id, LONG type);
Real CDGetNumerator(BaseTime *t);
void CDSetNumerator(BaseTime *t, Real r);


// R13 specific
Bool CDGetRange(BaseSelect *bs, LONG seg, LONG *a, LONG *b, LONG maxelem = CDMAXLONG);
Vector CDGetOptimalAngle(Vector oldRot, Vector newRot, BaseObject *op = NULL);
void CDGetActiveObjects(BaseDocument *doc, AtomArray &selection, LONG flags);

Vector CDMatrixToHPB(const Matrix &m, BaseObject *op = NULL);
Matrix CDHPBToMatrix(const Vector &hpb, BaseObject *op = NULL);



// R14 specific
Bool CDAddGlobalSymbol(Coffee *cof,String str,VALUE *val);
#if API_VERSION < 14000
VaType GetCoffeeType(LONG t);
#else
COFFEE_VTYPE GetCoffeeType(LONG t);
#endif


// R15+ specific
String CDLongToString(LONG l);
String CDRealToString(Real v, LONG vvk=-1, LONG nnk=-1, Bool e=false, UWORD xchar='0');
Real CDClamp(Real min, Real max, Real t);
Real CDBlend(Real a, Real b, Real mix);
Vector CDBlend(Vector a, Vector b, Real mix);


// R15 vector & matrix compatibility
Vector VNorm(Vector v);
Vector VCross(Vector a, Vector b);
Real VDot(Vector a, Vector b);
Bool VEqual(Vector a, Vector b, Real t=0.001);
Vector CDTransformVector(Vector v, Matrix m);
Matrix MInv(Matrix m);


//R15 File compatibility
Bool CDHFReadLong(HyperFile *hf, CDLong *l);
Bool CDHFReadFloat(HyperFile *hf, CDFloat *f);
Bool CDHFWriteLong(HyperFile *hf, CDLong l);
Bool CDHFWriteFloat(HyperFile *hf, CDFloat f);

Bool CDBFReadLong(BaseFile *bf, CDLong *l);
Bool CDBFReadFloat(BaseFile *bf, CDFloat *f);
Bool CDBFReadDouble(BaseFile *bf, CDDouble *d);
Bool CDBFWriteLong(BaseFile *bf, CDLong l);
Bool CDBFWriteFloat(BaseFile *bf, CDFloat f);
Bool CDBFWriteDouble(BaseFile *bf, CDDouble d);



// Plugin Register Functions
Bool CDRegisterTagPlugin(LONG id, const String &str, LONG info, DataAllocator *g, const String &description, const String &icon, LONG disklevel);
Bool CDRegisterObjectPlugin(LONG id, const String &str, LONG info, DataAllocator *g, const String &description, const String &icon, String icon_small, LONG disklevel);
Bool CDRegisterToolPlugin(LONG id, const String& str, LONG info, String icon, const String& help, ToolData* dat);
Bool CDRegisterCommandPlugin(LONG id, const String& str, LONG info, String icon, const String& help, CommandData* dat);
Bool CDRegisterMessagePlugin(LONG id, const String& str, LONG info, MessageData* dat);

#endif
