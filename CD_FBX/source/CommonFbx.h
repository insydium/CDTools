//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch

#ifndef _CommonFbx_H_
#define _CommonFbx_H_

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

#include "CDCompatibility.h"

// fbx sdk includes
#ifdef __PC
	#if API_VERSION > 11999
		#pragma warning (push)
		#pragma warning(disable : 4263 4264)
		#include "fbxsdk.h"
		#pragma warning (pop)
	#else
		#include "fbxsdk.h"
	#endif
#else
	#include "fbxsdk.h"
#endif

enum
{
	APP_UNKNOWN		= 0,
	APP_MAYA,
	APP_3DSMAX
};

// common fbx sdk functions
void ConvertNurbsAndPatchRecursive(KFbxSdkManager* pSdkManager, KFbxNode* kNode, int lod);
void ConvertNurbsAndPatch(KFbxSdkManager* pSdkManager, KFbxScene* pScene, int lod);

KFbxXMatrix GetGlobalPosition(KFbxNode* kNode, KTime& pTime, KFbxXMatrix* pParentGlobalPosition = NULL);
KFbxXMatrix GetGlobalPosition(KFbxNode* kNode, KTime& pTime, KFbxPose* pPose, KFbxXMatrix* pParentGlobalPosition = NULL);
KFbxXMatrix GetPoseMatrix(KFbxPose* pPose, int pNodeIndex);
KFbxXMatrix GetGeometry(KFbxNode* kNode);

//float GetFrameRate(KTime::ETimeMode tMode);
KFbxXMatrix GetDefaultLocalPosition(KFbxNode* kNode);
void GetNodePSR(Vector *pos, Vector *sca, Vector *rot, KFbxNode *kNode);
KFbxVector4 GetOptimalRotation(KFbxVector4 oldRot, KFbxVector4 newRot);

KFbxVector4 VectorToKFbxVector4(Vector v);
Vector KFbxVector4ToVector(KFbxVector4 kV);

Matrix KFbxXMatrixToMatrix(KFbxXMatrix kM, LONG handed);
KFbxXMatrix MatrixToKFbxXMatrix(Matrix m, LONG handed, Real scale);

Matrix KFbxMatrixToMatrix(KFbxXMatrix kM, LONG handed);
KFbxMatrix MatrixToKFbxMatrix(Matrix m, LONG handed, Real scale);
KFbxXMatrix GetGeometricPivotMatrix(KFbxNode *kNode);

String CharToString(const char* chrs);
Bool IsUsableCharacter(const String s);
String FixNameCharacters(const String s);
char* StringToChar(String str);


// debug
void PrintKFbxXMatrix(KFbxXMatrix kM);
void PrintCoordinateSystem(KFbxAxisSystem &SceneAxisSystem);
void PrintPivotOffsets(KFbxNode *kNode, String name);
void PrintNodeAttributeType(KFbxNode *kNode, KFbxNodeAttribute::EAttributeType lAttributeType);

#endif