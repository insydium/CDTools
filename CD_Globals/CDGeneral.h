#ifndef _CDGeneral_H_
#define _CDGeneral_H_

#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"

// compatibility includes
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

// GraphView call function for retrieving the port ID of a description parameter
#define GvCall(op,fnc) (((GvOperatorData*)op)->*((OPERATORPLUGIN*)C4DOS.Bl->RetrieveTableX((NodeData*)op,1))->fnc)


enum
{
	CD_SERIAL_CINEMA4D		= 0,
	CD_SERIAL_BODYPAINT,
	CD_SERIAL_MULTILICENSE
};

enum
{
	CD_VERSION = 1000,
};

Real GetCDPluginVersion(LONG id);

// compatibility functions
Bool CDIsCommandChecked(LONG id);
Bool CDIsAxisMode(BaseDocument *doc);

void CDGeGetSerialInfo(LONG type, SerialInfo *si);
Bool CDReadPluginInfo(LONG pluginid, void *buffer, LONG size);
Bool CDWritePluginInfo(LONG pluginid, void *buffer, LONG size);
Bool CDCheckC4DVersion(String pName);

Bool CDDrawLayerXray(BaseDocument *doc, BaseObject *op);
Vector CDGetActiveViewColor();
Vector CDGetActiveChildViewColor();
Vector CDGetVertexStartColor();
Vector CDGetVertexEndColor();



// geometry functions
Bool IsValidPointObject(BaseObject *op);
Vector* GetPointArray(BaseObject *op);
Bool IsValidSplineObject(BaseObject *op);
Segment* GetSegmentArray(SplineObject *spl);
Tangent* GetTangentArray(BaseObject *op);

Vector CalcPointNormal(BaseObject *op, Vector *padr, LONG ind);
Vector GetBestFitPlaneNormal(Vector *padr, LONG cnt);
Real GetNearestSplinePoint(SplineObject *spl, Vector p, LONG seg, Bool unif);
void RecalculatePoints(BaseObject *op, Matrix newM, Matrix oldM);

Bool IsValidPolygonObject(BaseObject *op);
CPolygon* GetPolygonArray(BaseObject *op);

Real GetPolygonArea(Vector *padr, const CPolygon &v);
void GetEdgePointIndices(CPolygon *vadr, LONG edge, LONG &a, LONG &b);
void GetEdgePoints(Vector *padr, CPolygon *vadr, LONG ind, Vector &a, Vector &b);
Vector CalcEdgeCenter(Vector *padr, CPolygon *vadr, LONG ind);
LONG GetSharedPolyPointsCount(CPolygon pA, CPolygon pB);

Vector CalcPolygonCenter(Vector *padr, const CPolygon &v);
Vector CalcUVWCenter(UVWStruct u);

Vector GetBarycentricCoords(Vector a, Vector b, Vector c, Vector p);
Vector ClosestPointOnTriangle(Vector p, Vector a, Vector b, Vector c);
Vector ClosestPointOnPolygon(Vector *padr, CPolygon ply, Vector pt);

void StoreSelections(BaseObject *op, BaseSelect *ptS, BaseSelect *edgS, BaseSelect *plyS);
void RestoreSelections(BaseObject *op, BaseSelect *ptS, BaseSelect *edgS, BaseSelect *plyS);
Vector GetSelectionCenter(Vector *padr, CPolygon *vadr, BaseSelect *bs, LONG bsCnt);



// object functions
Bool ObjectHasSize(BaseObject *op);

Bool IsParentObject(BaseObject *ch, BaseObject *pr);
Bool IsParentSelected(BaseObject *op);
BaseObject* GetHNParentObject(BaseObject *op);
BaseObject* GetPolygonalObject(BaseObject *op);
BaseObject* GetObjectDeformer(BaseObject *op);

Bool IsObjectInDocument(BaseDocument *doc, BaseObject *docOp, BaseObject *op);
Bool IsHierarchyEqual(BaseObject *src, BaseObject *dst);
Bool IsUserDataEqual(BaseObject *src, BaseObject *dst, Bool hrchl);
LONG GetChildCount(BaseObject *op);

Vector GetGlobalPosition(BaseObject *op);
void SetGlobalPosition(BaseObject *op, Vector pos);

Vector GetGlobalScale(BaseObject *op);
void SetGlobalScale(BaseObject *op, Vector sca);
void ScaleAttributes(BaseList2D *lst, Vector sca);

Vector GetGlobalRotation(BaseObject *op);
void SetGlobalRotation(BaseObject *op, Vector rot);
Vector GetRotationalDifference(BaseObject *opA, BaseObject *opB, Bool local);

Bool ObjectsEqual(BaseObject *op, BaseObject *cln, Real t);
Bool ActiveModelToolScaling(BaseObject *op);



// transfer functions
void CDTransferGoals(BaseList2D *src, BaseList2D *dst);
void CDTransferSelections(BaseObject *src, BaseObject *dst);
void TransferAMTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk);
void TransferUDTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk);
void TransferVectorTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk, LONG trackID);
Bool TransferTags(BaseDocument *doc, BaseObject *src, BaseObject *dst);
Bool TransferUserData(BaseDocument *doc, BaseList2D *src, BaseList2D *dst);
Bool TransferChildren(BaseDocument *doc, BaseObject *src, BaseObject *dst);
void RepositionChildren(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, Bool undo);


// matrix/vector functions
Vector VectorSlerp(Vector v1, Vector v2, Real t);
Matrix MixM(Matrix a, Matrix b, Vector axis, Real theMix);
Bool MatrixEqual(Matrix a, Matrix b, Real e);
Matrix GetAffineMatrix(Matrix m);
Matrix GetNormalizedMatrix(Matrix m);
Vector GetMatrixScale(Matrix m);
Matrix ScaleMatrix(Matrix m, Vector sca);
Matrix GetMirrorMatrix(Matrix opM, Matrix prM, LONG axis);
Real LenSquare(Vector v1, Vector v2);


// animation functions
Bool CDHasVectorTrack(BaseList2D *op, LONG trackID);
LONG CDGetFirstVectorKeyFrame(BaseList2D *op, LONG fps, LONG trackID);
LONG CDGetPreviousVectorKeyFrame(BaseObject *op, LONG curFrm, LONG fps, LONG trackID);
LONG CDGetNextVectorKeyFrame(BaseObject *op, LONG curFrm, LONG fps, LONG trackID);
LONG CDGetLastVectorKeyFrame(BaseList2D *op, LONG fps, LONG trackID);

void CDCopyVectorTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, LONG trackID, BaseList2D *prvTrk);
void CDScaleVectorTrack(BaseDocument *doc, BaseList2D *op, Vector sca, LONG trackID);
void CDSetVectorKeys(BaseDocument *doc, BaseObject *op, BaseTime time, LONG trackID, LONG intrp = CD_SPLINE_INTERPOLATION);

void CDAnimateDocument(BaseDocument *doc, BaseThread *bt=NULL, Bool anim=true, Bool expr=true, Bool caches=true);

// general math functions
Real TruncatePercentFractions(Real x);
Real RoundToWhole(Real value);

// utility functions
Bool ForceDeformCacheRebuild(BaseDocument *doc, BaseObject *op);


#endif