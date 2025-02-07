//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch
	
#ifndef _CDFBX_H_
#define _CDFBX_H_

// standard includes
#include "stdlib.h"
#include <stdio.h>
#include <math.h>

// c4d includes
#include "ge_matrix.h"
#include "ge_math.h"
#include "c4d_tools.h"
#include "lib_prefs.h"
#include "lib_modeling.h"

// CD includes
#include "CDQuaternion.h"
#include "CDGeneral.h"
#include "CDaabb.h"
#include "CDMessages.h"
#include "CDSelLog.h"
#include "CDReg.h"
#include "CDUserArea.h"

// CDJS includes
#include "CDJSStructures.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"
#include "CDClusterTag.h"
#include "CDJSFuncLib.h"

// CDM includes
#include "CDMStructures.h"
#include "CDMorphRef.h"
#include "CDMorphTag.h"
#include "CDMorphFuncLib.h"

// compatibility includes
#include "CDCompatibility.h"
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

// Serial number constants
#define C4D_SERIAL_SIZE				12
#define CDFBX_SERIAL_SIZE			27

// CD FBX ID's
#define ID_CDFBXPLUGIN				1026515

// Command ID's
#define ID_CDFBXIMPORTER			1025115
#define ID_CDFBXEXPORTER			1025116
#define ID_ABOUTCDFBX				1026516

// Tag ID's
#define ID_CDRETARGETTAG			1027091
#define ID_CDDISPLAYNORMALS			1027405
#define ID_CDPLANERPOSITION			1027574

// External ID's
#define ID_CDJOINTSANDSKIN			1019258
#define ID_ABOUTCDJOINTSKIN 		1019259
#define ID_CDJOINTOBJECT			1019254
#define ID_CDCLUSTERTAGPLUGIN		1018751
#define ID_CDSKINPLUGIN				1019251
#define ID_CDSKINREFPLUGIN			1019291
#define ID_CDBINDPOSETAG			1019257
#define ID_JSMESSAGEPLUGIN			1019424
#define ID_CDCONVERTTOCDJOINTS		1019481
#define ID_CDCONVERTFROMCDJOINTS	1019564
#define ID_CDCONVERTTOCDSKIN		1019562
#define ID_CDCONVERTFROMCDSKIN		1019563

#define ID_ABOUTCDMORPH 			1017238
#define ID_CDMORPHTAGPLUGIN			1017237
#define ID_CDMORPHREFPLUGIN			1017273
#define ID_CDSYMMETRYTAG			1020413

#define ID_ABOUTCDTRANSFER	 		1022171
#define ID_CDTRANSFERAXIS			1025261
#define ID_CDZEROTRANSTAG			1022137

#define ID_CDPCONSTRAINTPLUGIN		1017945
#define ID_CDRCONSTRAINTPLUGIN		1017946
#define ID_CDSCONSTRAINTPLUGIN		1017947
#define ID_CDACONSTRAINTPLUGIN		1017948
#define ID_CDMCONSTRAINTPLUGIN		1017949
#define ID_CDNCONSTRAINTPLUGIN		1017950
#define ID_CDDCONSTRAINTPLUGIN		1017954
#define ID_CDPRCONSTRAINTPLUGIN		1017955
#define ID_CDCCONSTRAINTPLUGIN		1017979
#define ID_CDPSRCONSTRAINTPLUGIN	1018001
#define ID_CDLCONSTRAINTPLUGIN		1018014
#define ID_CDSFCONSTRAINTPLUGIN		1020598
#define ID_CDSPRCONSTRAINTPLUGIN	1020816
#define ID_CDPTCONSTRAINTPLUGIN		1020866
#define ID_CDTACONSTRAINTPLUGIN		1021527
#define ID_CDSPLCONSTRAINTPLUGIN	1022258
#define ID_CDSWCONSTRAINTPLUGIN		1023909

#define ID_CDLIMBIKPLUGIN			1015855
#define ID_CDFOOTIKPLUGIN			1015856
#define ID_CDQUADLEGPLUGIN			1018184
#define ID_CDIKHANDLEPLUGIN			1027519
#define ID_CDDUALTARGETPLUGIN		1015908

// c4d internal ID's
#define ID_PLA_TRACK			100004812
#define ID_CLOTH_TAG			100004020
#define ID_MOCCA_CONSTRAINT		1019364

enum // key recording states
{
	IDM_A_POS	= 12417,
	IDM_A_SIZE	= 12418,
	IDM_A_DIR	= 12419,
	
	IDM_A_PLA	= 12421,
	IDM_A_PARAMETER	= 12422,
	
	IDM_AUTOKEYS	= 12425,
};

enum // CD Skin
{
	SKIN_MATRIX				= 1002,
	S_POINT_COUNT			= 1003,
	DEST_ID					= 1004,
	INIT_WEIGHT				= 1005,
	
	J_LINK_ID				= 1007,
	ACCUMULATE				= 1008,
	SET_T_WEIGHT			= 1009,
	
	GO_TO_BIND_POSE			= 1025,

	AUTO_BIND_SKIN			= 1031,
	
	NORMALIZED_WEIGHT		= 1040,
	
	J_COUNT					= 2000,
	J_LINK					= 2001,
	
	BOUND					= 5000,
	J_MATRIX				= 5001,
	
	JL_MATRIX				= 8001,
	
	S_DUMMY
	
};

enum // CD Skin Reference
{
	CDSKR_SET_REFERENCE			= 1000,
	CDSKR_RESTORE_REFERENCE		= 1001
};

enum// CD Joint
{
	CDJ_JOINT_RADIUS			= 1000,
	CDJ_CONNECTED				= 1001,
	CDJ_ZERO_COORDS				= 1006,
	CDJ_TRANS_ROT				= 1009
};

enum// CD Zero Tranformation
{
	ZT_TRANS_ROT			= 1003
};
enum // CD Bind Pose
{
	BP_POSE_RESTORE			= 2400
};

enum // CD Morph
{
	M_DEST_LINK				= 1010,
	M_SET_SELECTION				= 1011,
	M_SET_OFFSET					= 1012,
	M_MIX_SLIDER					= 1018,
	
	M_USE_BONE_ROTATION			= 1020,
	
	M_ROTATION_AXIS				= 1021,
	M_ROTATION_H			= 1022,
	M_ROTATION_P			= 1023,
	M_ROTATION_B			= 1024,
	
	M_MIN_VALUE					= 1025,
	M_MAX_VALUE					= 1026,
	M_CLAMP_MIN					= 1027,
	M_CLAMP_MAX					= 1028
};

enum // CD Position Constraint
{
	P_AB_MIX				= 1012,

	P_AXIS_X				= 1013,
	P_OFFSET_X				= 1014,
	P_AXIS_Y				= 1015,
	P_OFFSET_Y				= 1016,
	P_AXIS_Z				= 1017,
	P_OFFSET_Z				= 1018,

	P_STRENGTH				= 1019,

	P_USE_AB_MIX			= 1023,
	
	P_TARGET_COUNT			= 1040,
	P_TARGET				= 4000,
	P_POS_MIX				= 5000
};

enum // CD Rotation Constraint
{
	R_AB_MIX				= 1012,

	R_STRENGTH				= 1013,
	
	R_AXIS_X				= 1016,
	R_OFFSET_X				= 1017,
	R_AXIS_Y				= 1018,
	R_OFFSET_Y				= 1019,
	R_AXIS_Z				= 1020,
	R_OFFSET_Z				= 1021,
	
	R_USE_AB_MIX			= 1023,
	
	R_TARGET_COUNT			= 1036,
	
	R_INTERPOLATION			= 1040,
		R_AVERAGE			= 1041,
		R_SHORTEST			= 1042,					
	
	R_TARGET				= 4000,
	R_ROT_MIX				= 5000
};

enum // CD Scale Constraint
{
	S_AB_MIX				= 1012,

	S_AXIS_X				= 1013,
	S_OFFSET_X				= 1014,
	S_AXIS_Y				= 1015,
	S_OFFSET_Y				= 1016,
	S_AXIS_Z				= 1017,
	S_OFFSET_Z				= 1018,
	
	S_STRENGTH				= 1019,
	
	S_USE_AB_MIX			= 1023,

	S_TARGET_COUNT			= 1040,
	S_TARGET				= 4000,
	S_SCA_MIX				= 5000
};

enum // CD PSR Constraint
{
	PSR_USE_P			= 1012,
	PSR_USE_R			= 1013,
	PSR_USE_S			= 1014,
	
	PSR_AB_MIX				= 1017,
	
	PSR_STRENGTH			= 1018,
	
	PSR_USE_AB_MIX			= 1023,
	
	PSR_COUNT			= 1030,
	
	PSR_TARGET_COUNT			= 1034,
	
	PSR_INTERPOLATION		= 1040,
		PSR_AVERAGE			= 1041,
		PSR_SHORTEST		= 1042,					
	
	PSR_TARGET				= 4000,
	
	PSR_PSR_MIX				= 5000,
	
	PSR_P_AXIS_X				= 50011,
	PSR_P_OFFSET_X			= 50012,
	PSR_P_AXIS_Y				= 50013,
	PSR_P_OFFSET_Y			= 50014,
	PSR_P_AXIS_Z				= 50015,
	PSR_P_OFFSET_Z			= 50016,
	
	PSR_R_AXIS_X				= 50019,
	PSR_R_OFFSET_X			= 50020,
	PSR_R_AXIS_Y				= 50021,
	PSR_R_OFFSET_Y			= 50022,
	PSR_R_AXIS_Z				= 50023,
	PSR_R_OFFSET_Z			= 50024,
	
	PSR_S_AXIS_X				= 50028,
	PSR_S_OFFSET_X			= 50029,
	PSR_S_AXIS_Y				= 50030,
	PSR_S_OFFSET_Y			= 50031,
	PSR_S_AXIS_Z				= 50032,
	PSR_S_OFFSET_Z			= 50033,
};

enum // CD Aim Constraint
{
	A_STRENGTH		= 1012,
	
	A_POLE_AXIS		= 1015,
	A_POLE_X		= 1016,
	A_POLE_Y		= 1017,
	A_POLE_NX		= 1018,
	A_POLE_NY		= 1019,
	
	A_AB_MIX				= 1020,
	A_USE_AB_MIX			= 1023,

	A_POLE_OFF			= 1024,
	
	A_TARGET_COUNT			= 1040,
	
	A_TARGET				= 4000,
	A_UP_VECTOR			= 5000,
	A_AIM_MIX				= 6000,
	A_UP_MIX				= 7000,
	
	A_ALIGN_AXIS			= 16000,
	A_ALIGN_OFF		= 16100,
	A_ALIGN_X			= 16200,
	A_ALIGN_Y			= 16300,
	A_ALIGN_Z			= 16400,
	A_ALIGN_NX		= 16500,
	A_ALIGN_NY		= 16600,
	A_ALIGN_NZ		= 16700,
	
	A_Z_DIRECTION			= 16800
	
};

enum // CD Parent Constraint
{
	PR_TARGET				= 1010,
	PR_TARGET_SCALE			= 1015,
	PR_STRENGTH				= 1017
};

enum // CD Limb IK
{
	LMBIK_TWIST					= 1501,
	
	LMBIK_POLE_AXIS			= 10006,
	LMBIK_POLE_Y			= 10007,
	LMBIK_POLE_X			= 10008,
	
	LMBIK_SOLVER_LINK		= 10009,
	
	LMBIK_GOAL_LINK			= 10010,
	
	LMBIK_POLE_NX			= 10019,
	LMBIK_POLE_NY			= 10020
	
};

enum // CD Foot IK
{
	FTIK_POLE_AXIS		= 10003,
		FTIK_POLE_Y		= 10004,
		FTIK_POLE_X		= 10005,
	
	FTIK_SOLVER_LINK		= 10006,
	
	FTIK_GOAL_A_LINK		= 10007,
	FTIK_GOAL_B_LINK		= 10008,
	
	FTIK_POLE_NX			= 10016,
	FTIK_POLE_NY			= 10017
	
};

enum // CD Quadleg IK
{
	QLIK_TWIST				= 1501,
	
	QLIK_POLE_AXIS			= 10006,
		QLIK_POLE_Y			= 10007,
		QLIK_POLE_X			= 10008,
	
	QLIK_SOLVER_LINK		= 10009,
	
	QLIK_GOAL_LINK			= 10010,
	
	QLIK_POLE_NX			= 10019,
	QLIK_POLE_NY			= 10020
};

enum // CD IK Handle
{	
	HIK_POLE_AXIS				= 1017,
		HIK_POLE_Y				= 1018,
		HIK_POLE_X				= 1019,
		HIK_POLE_NX				= 1020,
		HIK_POLE_NY				= 1021,
	
	HIK_BONES_IN_GROUP				= 1022,
	HIK_TWIST						= 1026,
	
	HIK_SOLVER					= 2001,
		HIKRP					= 2002,
		HIKSC					= 2003,
		HIKHD					= 2004,
	
	HIK_POLE_LINK				= 2010,
	HIK_GOAL_LINK				= 2011,
	
	HIKSC_POLE_VECTOR			= 2014
};



enum
{
	POLE_X,
	POLE_Y,
	POLE_NX,
	POLE_NY
};



// user area classes
class CDFbxAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

class CDFbxOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);

String CharToString(const char* chr);
Bool IsUsableCharacter(const String s);
String FixNameCharacters(const String s);
char* ConvertPathname(Filename *fName);
Bool MaterialChannelEnabled(BaseMaterial *mat, const DescID& id);

Vector CDMCalcPointNormal(BaseObject *op, CDMRefPoint *refadr, LONG ind);
Vector CDMCalcFaceNormal(CDMRefPoint *refadr, const CPolygon &ply);
Vector CDMCalcPolygonCenter(CDMRefPoint *refadr, const CPolygon &ply);
Matrix GetRPMatrix(LONG pole, Vector o, Vector g, Vector p);

void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM);
//void TransferVectorTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk, LONG trackID);

// batch paths storage class for coffee scripts
class CDFbxBatchPaths
{
private:
	String batchExportPath, batchImportPath;

public:
	CDFbxBatchPaths();
	~CDFbxBatchPaths();
	
	Bool SetExportPath(void);
	String GetExportPath(void);
	Bool SetImportPath(void);
	String GetImportPath(void);
};

Bool SetBatchOutputPath(void);
String GetBatchOutputPath(void);
Bool SetBatchInputPath(void);
String GetBatchInputPath(void);

// coffee extension functions
Bool CDFbxAddSymbols(Coffee *cofM);

void CDFbxClearExportData(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxSetExportData(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxSetOutputPath(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxExportFile(Coffee* cof, VALUE *&sp, LONG argc);

void CDFbxClearImportData(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxSetImportData(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxSetInputPath(Coffee* cof, VALUE *&sp, LONG argc);
void CDFbxImportFile(Coffee* cof, VALUE *&sp, LONG argc);



// sn functions
Bool RegisterCDFBX(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif

