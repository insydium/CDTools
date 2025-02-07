//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_scenehookdata.h"

#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDJoint.h"

#include "oCDJoint.h"

extern Vector colorH, colorCH;
extern Bool clusDialogOpen;
extern Real clusDialogRad;
extern Real clusDialogMax;
extern Real clusDialogFalloff;

class CDJSSceneHook : public SceneHookData
{
private:
	void DrawClusterAutoWtGuides(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh);
	void DrawJointsXRayMode(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh);
	
public:
	virtual Bool CDDraw(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags);
	
#if API_VERSION < 12000
	virtual Bool Draw(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
		{ return CDDraw((CDBaseSceneHook*)node, doc, bd, bh, bt, flags); }
#else
	virtual Bool Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, SCENEHOOKDRAW flags)
		{ return CDDraw((CDBaseSceneHook*)node, doc, bd, bh, bt, (LONG)flags); }
#endif

	static NodeData *Alloc(void) { return CDDataAllocator(CDJSSceneHook); }
};

static void DrawCircle(BaseDraw* bd, Matrix m)
{
	Vector segPts[36] = {
			Vector(1,0,0),
			Vector(0.985,0.174,0),
			Vector(0.94,0.342,0),
			Vector(0.866,0.5,0),
			Vector(0.766,0.643,0),
			Vector(0.643,0.766,0),
			Vector(0.5,0.866,0),
			Vector(0.342,0.94,0),
			Vector(0.174,0.985,0),
			Vector(0,1,0),
			Vector(-0.174,0.985,0),
			Vector(-0.342,0.94,0),
			Vector(-0.5,0.866,0),
			Vector(-0.643,0.766,0),
			Vector(-0.766,0.643,0),
			Vector(-0.866,0.5,0),
			Vector(-0.94,0.342,0),
			Vector(-0.985,0.174,0),
			Vector(-1,0,0),
			Vector(-0.985,-0.174,0),
			Vector(-0.94,-0.342,0),
			Vector(-0.866,-0.5,0),
			Vector(-0.766,-0.643,0),
			Vector(-0.643,-0.766,0),
			Vector(-0.5,-0.866,0),
			Vector(-0.342,-0.94,0),
			Vector(-0.174,-0.985,0),
			Vector(0,-1,0),
			Vector(0.174,-0.985,0),
			Vector(0.342,-0.94,0),
			Vector(0.5,-0.866,0),
			Vector(0.643,-0.766,0),
			Vector(0.766,-0.643,0),
			Vector(0.866,-0.5,0),
			Vector(0.94,-0.342,0),
			Vector(0.985,-0.174,0),
			};
	LONG i;
	for(i=1; i<36; i++)
	{
		CDDrawLine2D(bd,bd->WS(m*segPts[i-1]),bd->WS(m*segPts[i]));
	}
	CDDrawLine2D(bd,bd->WS(m*segPts[35]),bd->WS(m*segPts[0]));
	
}

static void DrawXRayJoint(BaseObject *op, BaseDraw* bd, BaseDrawHelp* bh, LONG transp)
{
	Real jDspSize = 1.0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(wpData) jDspSize = wpData->GetReal(IDC_JOINT_SIZE);

	BaseContainer *data = op->GetDataInstance();
	ObjectColorProperties prop;
	
	Matrix chNewMatrix, opNewMatrix;
	bd->SetMatrix_Matrix(op, opNewMatrix);
	bd->SetTransparency(transp);
	
	Real   rad = data->GetReal(JNT_JOINT_RADIUS) * jDspSize;
	Vector h, p1, p2, p3, p4, endDir, theColor;
	Matrix opM, chM, bM, jM = op->GetMg(), scaleM, targetM;

	BaseObject *ch = op->GetDown();
	while(ch)
	{
		if(ch->GetType() == ID_CDJOINTOBJECT)
		{
			BaseContainer *cData = ch->GetDataInstance();
			if(cData->GetBool(JNT_CONNECTED))
			{
				op->GetColorProperties(&prop);
				if(prop.usecolor > 0)  theColor = prop.color;
				else theColor = Vector(0,0,0.75);
				bd->SetPen(theColor);
				
				if(data->GetBool(JNT_HIGHLITE) && op->GetBit(BIT_ACTIVE)) bd->SetPen(colorH);
				else if(data->GetBool(JNT_HIGHLITE)) bd->SetPen(colorCH);

				Real opRad = data->GetReal(JNT_JOINT_RADIUS) * jDspSize;
				opM = op->GetMg();
				chM = ch->GetMg();
				endDir = VNorm(chM.off - opM.off);
				if(opM.off != chM.off)
				{
					bM.off = opM.off + endDir * opRad;
					bM.v3 = endDir;
					Real upAngle = VDot(endDir,opM.v2);
					if(upAngle>-0.9999 && upAngle<0.9999)
					{
						bM.v1 = VNorm(VCross(opM.v2, bM.v3));
						bM.v2 = VNorm(VCross(bM.v3, bM.v1));
					}
					else
					{
						bM.v2 = VNorm(VCross(bM.v3, opM.v1));
						bM.v1 = VNorm(VCross(bM.v2, bM.v3));
					}
					
					Matrix afM = GetAffineMatrix(opM);
					p1 = opM * MInv(afM) * bM * Vector(opRad,0.0,0.0);
					p2 = opM * MInv(afM) * bM * Vector(0.0,opRad,0.0);
					p3 = opM * MInv(afM) * bM * Vector(-opRad,0.0,0.0);
					p4 = opM * MInv(afM) * bM * Vector(0.0,-opRad,0.0);
					
					if(!data->GetBool(JNT_SHOW_PROXY) || data->GetBool(JNT_SHOW_ENVELOPE))
					{
						CDDrawLine2D(bd,bd->WS(p1),bd->WS(p2));
						CDDrawLine2D(bd,bd->WS(p2),bd->WS(p3));
						CDDrawLine2D(bd,bd->WS(p3),bd->WS(p4));
						CDDrawLine2D(bd,bd->WS(p4),bd->WS(p1));
						
						Vector tipPos = chM * ((VNorm(MInv(chM) * opM.off)) * (cData->GetReal(JNT_JOINT_RADIUS) * jDspSize));// 
						CDDrawLine2D(bd,bd->WS(tipPos),bd->WS(p1));
						CDDrawLine2D(bd,bd->WS(tipPos),bd->WS(p2));
						CDDrawLine2D(bd,bd->WS(tipPos),bd->WS(p3));
						CDDrawLine2D(bd,bd->WS(tipPos),bd->WS(p4));
					}
					else
					{
						if(op->GetDown() == ch && ch->GetType() == ID_CDJOINTOBJECT)
						{
							Matrix rootM = op->GetMg();
							LONG i;
							
							BaseObject *theChild = op->GetDown();
							Matrix tipM = rootM;
							Vector opPos = op->GetMg().off, cPos = theChild->GetMg().off;
							Real bnLen = Len(cPos - opPos);
							tipM.off = cPos;
							
							Vector rSk, tSk, rOff=data->GetVector(JNT_ROOT_OFFSET), tOff=data->GetVector(JNT_TIP_OFFSET);
							rSk.x = data->GetReal(JNT_SKEW_RX);
							rSk.y = data->GetReal(JNT_SKEW_RY);
							tSk.x = data->GetReal(JNT_SKEW_TX);
							tSk.y = data->GetReal(JNT_SKEW_TY);
							// Calculate the root skew position
							Vector skrPos = (rootM * Vector(rSk.x,rSk.y,0)) - cPos;
							rootM.off = cPos + VNorm(skrPos) * (bnLen - rOff.z);
							// Calculate the tip skew position
							Vector sktPos = (tipM * Vector(tSk.x,tSk.y,bnLen)) - opPos;
							tipM.off = opPos + VNorm(sktPos) * (bnLen + tOff.z);
							
							rootM.v3 = VNorm(tipM.off - rootM.off);
							rootM.v2 = VNorm(VCross(rootM.v3, rootM.v1));
							rootM.v1 = VNorm(VCross(rootM.v2, rootM.v3));
							tipM.v1 = rootM.v1;
							tipM.v2 = rootM.v2;
							tipM.v3 = rootM.v3;
							rootM.v1 *= rOff.x;
							rootM.v2 *= rOff.y;
							tipM.v1 *= tOff.x;
							tipM.v2 *= tOff.y;
							
							Vector ePts[5] = {
									Vector(1,1,0),
									Vector(-1,1,0),
									Vector(-1,-1,0),
									Vector(1,-1,0),
									Vector(1,1,0),
									};
							//Draw sides
							for(i=0; i<4; i++)
							{
								p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
								p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
								CDDrawLine2D(bd,bd->WS(p1),bd->WS(p2));
							}
							
							//Draw top
							for(i=0; i<4; i++)
							{
								p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
								p2 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i+1]);
								CDDrawLine2D(bd,bd->WS(p1),bd->WS(p2));
							}
							
							//Draw bottom
							for(i=0; i<4; i++)
							{
								p1 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
								p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i+1]);
								CDDrawLine2D(bd,bd->WS(p1),bd->WS(p2));
							}
						}
					}
				}
			}
		}
		ch = ch->GetNext();
	}
	
	op->GetColorProperties(&prop);
	if(prop.usecolor > 0)  theColor = prop.color;
	else theColor = Vector(0,0,0.75);
	bd->SetPen(theColor);
	
	if(data->GetBool(JNT_HIGHLITE) && op->GetBit(BIT_ACTIVE)) bd->SetPen(colorH);
	else if(data->GetBool(JNT_HIGHLITE)) bd->SetPen(colorCH);
	
	jM.v1*=rad;
	jM.v2*=rad;
	jM.v3*=rad;
	
	if(!data->GetBool(JNT_SHOW_PROXY) || data->GetBool(JNT_SHOW_ENVELOPE))
	{
		if(!op->GetDown())
		{
			if(!op->GetUp())
			{
				DrawCircle(bd,jM);
				h=jM.v2; jM.v2=jM.v3; jM.v3=h;
				DrawCircle(bd,jM);
				h=jM.v1; jM.v1=jM.v3; jM.v3=h;
				DrawCircle(bd,jM);
			}
			else
			{
				BaseObject *pr = op->GetUp();
				BaseContainer *prData = pr->GetDataInstance();
				if(prData)
				{
					if(!prData->GetBool(JNT_SHOW_PROXY) || prData->GetBool(JNT_SHOW_ENVELOPE))
					{
						DrawCircle(bd,jM);
						h=jM.v2; jM.v2=jM.v3; jM.v3=h;
						DrawCircle(bd,jM);
						h=jM.v1; jM.v1=jM.v3; jM.v3=h;
						DrawCircle(bd,jM);
					}
				}
			}
		}
		else
		{
			DrawCircle(bd,jM);
			h=jM.v2; jM.v2=jM.v3; jM.v3=h;
			DrawCircle(bd,jM);
			h=jM.v1; jM.v1=jM.v3; jM.v3=h;
			DrawCircle(bd,jM);
		}
	}
	
	bd->SetTransparency(0);
}

void CDJSSceneHook::DrawClusterAutoWtGuides(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh)
{
	AtomArray *opSelectionList = GetSelectionLog(doc);
	if(opSelectionList)
	{
		LONG  selCount = opSelectionList->GetCount();
		if(selCount == 2)
		{
			BaseObject *destObject = static_cast<BaseObject*>(opSelectionList->GetIndex(1));
			if(destObject)
			{
				Vector h;
				Matrix jM = Matrix();
				jM.off = destObject->GetMg().off;
				
				jM.v1*=clusDialogRad;
				jM.v2*=clusDialogRad;
				jM.v3*=clusDialogRad;
				
				bd->SetPen(Vector(1.0*clusDialogMax,0,0));
				CDDrawCircle(bd,jM);
				h=jM.v2; jM.v2=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);
				h=jM.v1; jM.v1=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);

				jM = Matrix();
				jM.off = destObject->GetMg().off;
				
				jM.v1*=clusDialogFalloff;
				jM.v2*=clusDialogFalloff;
				jM.v3*=clusDialogFalloff;
				
				bd->SetPen(Vector(1.0*clusDialogMax,1.0*clusDialogMax,0));
				CDDrawCircle(bd,jM);
				h=jM.v2; jM.v2=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);
				h=jM.v1; jM.v1=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);
			}
		}
	}
}

void CDJSSceneHook::DrawJointsXRayMode(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh)
{
	CDJSData jd;
	if(bd->GetParameterData(BASEDRAW_DISPLAYFILTER_OTHER) != 0)
	{
		PluginMessage(ID_CDJOINTOBJECT,&jd);
		if(jd.list)
		{
			LONG i, jCnt = jd.list->GetCount();
			BaseObject *op = NULL;
			for(i=0; i<jCnt; i++)
			{
				op = static_cast<BaseObject*>(jd.list->GetIndex(i));
				if(!op) continue;
				if(op->GetDocument() == doc)
				{
					Bool drawXray = true;
					if(GetC4DVersion() > 9999) drawXray = CDDrawLayerXray(doc,op);
					
					if(drawXray)
					{
						ObjectColorProperties prop;
						op->GetColorProperties(&prop);
						if(prop.xray)
						{
							if(op->GetEditorMode() == MODE_ON) DrawXRayJoint(op, bd, bh,-200);
							else if(op->GetEditorMode() == MODE_UNDEF)
							{
								Bool jDraw = true;
								BaseObject *parent = op->GetUp();
								while(parent && jDraw)
								{
									if(parent->GetEditorMode() == MODE_OFF) jDraw = false;
									if(parent->GetEditorMode() == MODE_ON) break;
									parent = parent->GetUp();
								}
								if(jDraw) DrawXRayJoint(op, bd, bh,-150);//-150
							}
						}
					}
				}
			}
		}
	}
	
}

Bool CDJSSceneHook::CDDraw(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
{
	GeData dispSData = bd->GetParameterData(BASEDRAW_DATA_SDISPLAYACTIVE);
	
	switch(flags)
	{
		case CD_SCENEHOOKDRAW_DRAW_PASS:
		{
			//Draw cluster autoweight guides
			if(clusDialogOpen) DrawClusterAutoWtGuides(doc,bd,bh);
			
			//Draw joints xray mode
			if(dispSData != BASEDRAW_SDISPLAY_NOSHADING) DrawJointsXRayMode(doc,bd,bh);
			
			break;
		}
	}
	
	return true;
}

Bool RegisterCDJSSceneHook(void)
{
	if(CDFindPlugin(ID_CDJSSCENEHOOK,CD_SCENEHOOK_PLUGIN)) return true;
	
	return RegisterSceneHookPlugin(ID_CDJSSCENEHOOK,"Scenehook",PLUGINFLAG_HIDE|PLUGINFLAG_HIDEPLUGINMENU,CDJSSceneHook::Alloc,CD_EXECUTION_RESULT_OK,0,NULL);
}
