//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDJoint.h"
#include "CDToolData.h"

#include "oCDJoint.h"

class CDJTDialog: public SubDialog
{
	private:
		void DoEnable(void);

	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool InitDialog(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDJTDialog::InitDialog(void)
{
	BaseContainer *bc=GetToolData(GetActiveDocument(),ID_CDJOINTTOOLPLUGIN);
	if (!bc) return false;

	return true;
}

Bool CDJTDialog::CreateLayout(void)
{
	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupSpace(4,1);
			AddCheckbox(D_JT_AUTO_SIZE,BFH_LEFT,0,0,GeLoadString(D_JT_AUTO_SIZE));
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,4);
			AddStaticText(0,0,0,0,GeLoadString(D_JT_JOINT_SIZE),0);
			AddEditNumberArrows(D_JT_JOINT_SIZE,BFH_LEFT,70,0);
		}
		GroupEnd();
	}
	GroupEnd();
	
	return true;
}

Bool CDJTDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;

	BaseContainer *bc = GetWorldPluginData(ID_CDJOINTTOOLPLUGIN);
	if (!bc)
	{
		SetBool(D_JT_AUTO_SIZE, false);
		SetReal(D_JT_JOINT_SIZE,10,0.01,CDMAXREAL,0.1,FORMAT_REAL);
	}
	else
	{
		SetBool(D_JT_AUTO_SIZE, bc->GetBool(D_JT_AUTO_SIZE));
		SetReal(D_JT_JOINT_SIZE,bc->GetReal(D_JT_JOINT_SIZE),0.01,CDMAXREAL,0.1,FORMAT_REAL);
	}

	DoEnable();

	return InitDialog();
}

Bool CDJTDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool autoSize;
	Real jointSize;
	
	GetBool(D_JT_AUTO_SIZE,autoSize);
	GetReal(D_JT_JOINT_SIZE,jointSize);
	
	BaseContainer wpData;
	wpData.SetBool(D_JT_AUTO_SIZE,autoSize);
	wpData.SetReal(D_JT_JOINT_SIZE,jointSize);
	SetWorldPluginData(ID_CDJOINTTOOLPLUGIN,wpData,false);
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *bc=GetToolData(doc,ID_CDJOINTTOOLPLUGIN);
	if (!bc) return false;
	
	bc->SetBool(D_JT_AUTO_SIZE,autoSize);
	bc->SetReal(D_JT_JOINT_SIZE,jointSize);

	DoEnable();

	return true;
}

void CDJTDialog::DoEnable(void)
{
	Bool autoSize;
	
	GetBool(D_JT_AUTO_SIZE,autoSize);

	if(autoSize)  Enable(D_JT_JOINT_SIZE,false);
	else  Enable(D_JT_JOINT_SIZE,true);
}

class CDJointToolPlugin : public CDToolData
{
private:
	Real mx, my, zm;
	Bool mDrag, shift, ctrl, connected;
	
	Vector GetDrawPosition(BaseDraw *bd, BaseObject *op, BaseObject *ch);
	Matrix GetBoneDrawMatrix(Matrix m, Vector dir, Real r);
	void DrawJoint(BaseDraw *bd, Matrix m);
	void DrawBone(BaseDraw *bd, Matrix m, Vector end, Real rad);
	
public:
	virtual Bool InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt);
	virtual void FreeTool(BaseDocument* doc, BaseContainer& data);
	virtual Bool MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual Bool KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual LONG GetState(BaseDocument *doc);
	virtual Bool GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x, Real y, BaseContainer &bc);

	virtual LONG CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags);

	virtual SubDialog*	AllocSubDialog(BaseContainer* bc) { return CDDataAllocator(CDJTDialog); }
};

Bool CDJointToolPlugin::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
	return true;
}

void CDJointToolPlugin::FreeTool(BaseDocument* doc, BaseContainer& data)
{
	StatusClear();
}

LONG CDJointToolPlugin::GetState(BaseDocument *doc)
{
	if (doc->GetMode()==Mpaint) return 0;
	return CMD_ENABLED;
}

Vector CDJointToolPlugin::GetDrawPosition(BaseDraw *bd, BaseObject *op, BaseObject *ch)
{
	Vector mDir, mPos = Vector(mx,my,zm), opActivePos, pPos;
	Vector drawPosition = bd->SW(mPos);
	
	if(ch && ctrl && shift)
	{
		pPos = bd->WS(op->GetMg().off);
		pPos.z = zm;
		opActivePos = bd->SW(pPos);
		Real zPos = Len(drawPosition - opActivePos);
		drawPosition = op->GetMg().off + VNorm(op->GetMg().v3) * zPos;
	}
	else if(!ch && op && shift)
	{
		BaseObject *parent = op->GetUp();
		if(parent)
		{
			pPos = bd->WS(op->GetMg().off);
			pPos.z = zm;
			opActivePos = bd->SW(pPos);
			Real zPos = Len(drawPosition - opActivePos);
			drawPosition = op->GetMg().off + VNorm(parent->GetMg().v3) * zPos;
		}
		else
		{
			Vector dVec[8] = {
					Vector(1,0,zm),
					Vector(0.707,0.707,zm),
					Vector(0,1,zm),
					Vector(-0.707,0.707,zm),
					Vector(-1,0,zm),
					Vector(-0.707,-0.707,zm),
					Vector(0,-1,zm),
					Vector(0.707,-0.707,zm)};
			LONG i, ind=0;
			Vector opSPos = bd->WS(op->GetMg().off);
			opSPos.z = zm;
			mDir = VNorm(mPos - opSPos);
			for(i=0; i<8; i++)
			{
				Vector vDir = dVec[i];
				Real dotP = VDot(vDir, mDir);
				if(dotP > 0.924) ind = i;
			}
			Real zPos = Len(mPos - opSPos);
			Vector dPos = opSPos + dVec[ind] * zPos;
			dPos.z = zm;
			drawPosition = bd->SW(dPos);
		}
	}

	return drawPosition;
}

void CDJointToolPlugin::DrawJoint(BaseDraw *bd, Matrix m)
{
	Vector h;
	
	CDDrawCircle(bd,m);
	h=m.v2; m.v2=m.v3; m.v3=h;
	CDDrawCircle(bd,m);
	h=m.v1; m.v1=m.v3; m.v3=h;
	CDDrawCircle(bd,m);
}

void CDJointToolPlugin::DrawBone(BaseDraw *bd, Matrix m, Vector end, Real rad)
{
	Vector p1,p2,p3,p4;
	
	p1 = m * Vector(rad,0.0,0.0);
	p2 = m * Vector(0.0,rad,0.0);
	p3 = m * Vector(-rad,0.0,0.0);
	p4 = m * Vector(0.0,-rad,0.0);

	CDDrawLine(bd,p1,p2);
	CDDrawLine(bd,p2,p3);
	CDDrawLine(bd,p3,p4);
	CDDrawLine(bd,p4,p1);
	
	CDDrawLine(bd,end,p1);
	CDDrawLine(bd,end,p2);
	CDDrawLine(bd,end,p3);
	CDDrawLine(bd,end,p4);
}

Matrix CDJointToolPlugin::GetBoneDrawMatrix(Matrix m, Vector dir, Real r)
{
	Matrix bM, afM = GetAffineMatrix(m);
	
	bM.off = m.off + dir * r;
	bM.v3 = dir;
	Real upAngle = VDot(dir, Vector(0,1,0));
	if(upAngle > -0.9999 && upAngle < 0.9999)
	{
		bM.v1 = VNorm(VCross(Vector(0,1,0), bM.v3));
		bM.v2 = VNorm(VCross(bM.v3, bM.v1));
	}
	else
	{
		bM.v2 = VNorm(VCross(bM.v3, Vector(1,0,0)));
		bM.v1 = VNorm(VCross(bM.v2, bM.v3));
	}
	
	return m * MInv(afM) * bM;
}

LONG CDJointToolPlugin::CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags)
{
	Bool autoSize = false;
	Real jDspSize = 1.0, jSize = 10.0;
	BaseContainer *jdData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(jdData) jDspSize = jdData->GetReal(IDC_JOINT_SIZE);

	BaseContainer *jtData = GetWorldPluginData(ID_CDJOINTTOOLPLUGIN);
	if(jtData)
	{
		autoSize = jtData->GetBool(D_JT_AUTO_SIZE);
		if(!autoSize) jSize = jtData->GetReal(D_JT_JOINT_SIZE);
	}

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Vector endPos, startPos, endDir;
	
	BaseObject	*opActive = doc->GetActiveObject();
	if(opActive)
	{
		bd->LineZOffset(32);

		BaseObject	*opActiveChild = opActive->GetDown();
		if(opActiveChild)
		{
			if(opActiveChild->GetType() != ID_CDJOINTOBJECT)
			{
				opActiveChild = opActiveChild->GetNext();
				while(opActiveChild)
				{
					if(opActiveChild->GetType() == ID_CDJOINTOBJECT) break;
					else opActiveChild = opActiveChild->GetNext();
				}
			}
		}
		Real pRad, rad = jSize * jDspSize;
		if(opActive->GetType() == ID_CDJOINTOBJECT)
		{
			BaseContainer *pData = opActive->GetDataInstance(); 
			if(pData)
			{
				pRad = pData->GetReal(JNT_JOINT_RADIUS) * jDspSize;
				if(ctrl) rad = pRad;
			}
		}
		
		Matrix jntM, bM, drwM = Matrix();
		drwM.off = GetDrawPosition(bd, opActive, opActiveChild);
		if(!autoSize)
		{
			drwM.v1*=rad;
			drwM.v2*=rad;
			drwM.v3*=rad;
		}
		else
		{
			drwM.v1*=pRad;
			drwM.v2*=pRad;
			drwM.v3*=pRad;
		}
		
		bd->SetPen(Vector(1,0,0));
		if(mDrag)
		{
			DrawJoint(bd, drwM);
			
			if(opActive)
			{
				if(opActive->GetType() == ID_CDJOINTOBJECT)
				{
					jntM = opActive->GetMg();
					if(drwM.off != jntM.off)
					{
						endDir = VNorm(drwM.off - jntM.off);
						bM = GetBoneDrawMatrix(jntM, endDir, pRad);
						
						startPos = jntM.off;
						if(!autoSize)  endPos = startPos + VNorm(drwM.off - startPos) * (Len(drwM.off - startPos) - rad);
						else endPos = startPos + VNorm(drwM.off - startPos) * (Len(drwM.off - startPos) - pRad);
						
						DrawBone(bd,bM,endPos,pRad);
						
						if(opActiveChild && ctrl)
						{
							BaseContainer *chData = opActiveChild->GetDataInstance();
							
							if(chData->GetBool(JNT_CONNECTED)) connected = true;
							chData->SetBool(JNT_CONNECTED,false);
							
							endDir = VNorm(opActiveChild->GetMg().off - drwM.off);
							bM = GetBoneDrawMatrix(GetAffineMatrix(drwM), endDir, rad);
							
							startPos = drwM.off + endDir * rad;
							endPos = opActiveChild->GetMg().off - endDir * (chData->GetReal(JNT_JOINT_RADIUS) * jDspSize);
							
							DrawBone(bd,bM,endPos,pRad);
						}
					}
				}
			}
		}
	}
	else
	{
		bd->LineZOffset(32);
		Real rad = jSize * jDspSize;
		
		Matrix drwM = Matrix();
		drwM.off = bd->SW(Vector(mx,my,zm));
		drwM.v1*=rad;
		drwM.v2*=rad;
		drwM.v3*=rad;
		
		bd->SetPen(Vector(1,0,0));
		if(mDrag) DrawJoint(bd, drwM);
	}
	
	return CD_TOOLDRAW_HANDLES|CD_TOOLDRAW_AXIS;
}

Bool CDJointToolPlugin::KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	LONG key = msg.GetData(BFM_INPUT_CHANNEL).GetLong();
	String str = msg.GetData(BFM_INPUT_ASC).GetString();
	if(key == KEY_ESC)
	{
		doc->SetActiveObject(NULL,SELECTION_NEW);
		EventAdd(EVENT_FORCEREDRAW);
	}
	return false;
}

Bool CDJointToolPlugin::MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	Bool autoSize = false;
	Real jSize = 10.0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTTOOLPLUGIN);
	if(wpData)
	{
		autoSize = wpData->GetBool(D_JT_AUTO_SIZE);
		if(!autoSize) jSize = wpData->GetReal(D_JT_JOINT_SIZE);
	}
	
	mx = msg.GetReal(BFM_INPUT_X);
	my = msg.GetReal(BFM_INPUT_Y);
	LONG button;
	Matrix pM, opM, bM, drwM; 
	Vector endDir, opPos, h, sca;
	
	switch (msg.GetLong(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button=KEY_MRIGHT; break;
		default: return true;
	}
	
	BaseObject	*op=NULL, *opActive = doc->GetActiveObject(), *opActiveChild = NULL, *pred = NULL;
	if(opActive)  opActiveChild = opActive->GetDown();
	if(opActiveChild)
	{
		if(opActiveChild->GetType() != ID_CDJOINTOBJECT)
		{
			pred = opActiveChild;
			opActiveChild = opActiveChild->GetNext();
			while(opActiveChild)
			{
				if(opActiveChild->GetType() == ID_CDJOINTOBJECT) break;
				else
				{
					pred = opActiveChild;
					opActiveChild = opActiveChild->GetNext();
				}
			}
		}
	}
	
	Real dx, dy;
	Vector mDir, mPos, opActivePos, pPos;

	BaseObject *cam = bd->GetEditorCamera(); if(!cam) return true;
	zm=0;
	if (bd->GetProjection() == 0)
	{
		zm = Len(cam->GetMg().off);
		if(opActive) zm = Len(opActive->GetMg().off - cam->GetMg().off);
	}
	else
	{
		if(opActive)
		{
			pPos = bd->WS(opActive->GetMg().off);
			zm = pPos.z;
		}		
	}

	mDrag = false;
	ctrl = false;
	shift = false;
	connected = false;
	BaseContainer bc, device, *pData = NULL;
	
#if API_VERSION < 12000
	win->MouseDragStart(button,mx,my,MOUSEDRAG_DONTHIDEMOUSE|MOUSEDRAG_NOMOVE);
	while (win->MouseDrag(&dx,&dy,&device)==MOUSEDRAG_CONTINUE)
#else
	win->MouseDragStart(button,mx,my,MOUSEDRAGFLAGS_DONTHIDEMOUSE|MOUSEDRAGFLAGS_NOMOVE);
	while (win->MouseDrag(&dx,&dy,&device)==MOUSEDRAGRESULT_CONTINUE)
#endif
		
	{
		mx+=dx;
		my+=dy;
		mDrag = true;
		ctrl = (device.GetLong(BFM_INPUT_QUALIFIER) & QCTRL);
		shift = (device.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);
		
		if(autoSize)
		{
			if(opActive && opActive->GetType() == ID_CDJOINTOBJECT)
			{
				mPos = Vector(mx,my,zm);
				opPos = bd->SW(mPos);
				Real zPos = Len(opPos - opActive->GetMg().off);
				BaseContainer *jData = opActive->GetDataInstance();
				jSize = zPos * 0.1;
				jData->SetReal(JNT_JOINT_RADIUS,jSize);
				if(ctrl)
				{
					if(opActiveChild && opActiveChild->GetType() == ID_CDJOINTOBJECT)
					{
						BaseContainer *jchData = opActiveChild->GetDataInstance();
						jchData->SetReal(JNT_JOINT_RADIUS,jSize);
					}
				}
			}
		}
		else
		{
			if(ctrl)
			{
				if(opActiveChild && opActiveChild->GetType() == ID_CDJOINTOBJECT)
				{
					BaseContainer *jData = opActive->GetDataInstance();
					jSize = jData->GetReal(JNT_JOINT_RADIUS);
					BaseContainer *jchData = opActiveChild->GetDataInstance();
					jchData->SetReal(JNT_JOINT_RADIUS,jSize);
				}
			}
		}
		CDDrawViews(CD_DRAWFLAGS_INDRAG|CD_DRAWFLAGS_NO_REDUCTION|CD_DRAWFLAGS_NO_EXPRESSIONS|CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}
	mPos = Vector(mx,my,zm);
	opPos = bd->SW(mPos);
	if(opActiveChild && ctrl && shift)
	{
		pPos = bd->WS(opActive->GetMg().off);
		pPos.z = zm;
		opActivePos = bd->SW(pPos);
		Real zPos = Len(opPos - opActivePos);
		opPos = opActive->GetMg().off + VNorm(opActive->GetMg().v3) * zPos;
	}
	else if(!opActiveChild && opActive && shift)
	{
		BaseObject *parent = opActive->GetUp();
		if(parent)
		{
			pPos = bd->WS(opActive->GetMg().off);
			pPos.z = zm;
			opActivePos = bd->SW(pPos);
			Real zPos = Len(opPos - opActivePos);
			opPos = opActive->GetMg().off + VNorm(parent->GetMg().v3) * zPos;
		}
		else
		{
			Vector dVec[8] = {
					Vector(1,0,zm),
					Vector(0.707,0.707,zm),
					Vector(0,1,zm),
					Vector(-0.707,0.707,zm),
					Vector(-1,0,zm),
					Vector(-0.707,-0.707,zm),
					Vector(0,-1,zm),
					Vector(0.707,-0.707,zm)};
			LONG i, ind=0;
			Vector opSPos = bd->WS(opActive->GetMg().off);
			opSPos.z = zm;
			mDir = VNorm(mPos - opSPos);
			for(i=0; i<8; i++)
			{
				Vector vDir = dVec[i];
				Real dotP = VDot(vDir, mDir);
				if(dotP > 0.924) ind = i;
			}
			Real zPos = Len(mPos - opSPos);
			Vector dPos = opSPos + dVec[ind] * zPos;
			dPos.z = zm;
			opPos = bd->SW(dPos);
		}
	}

#if API_VERSION < 12000
	if(mDrag && win->MouseDragEnd()==MOUSEDRAG_FINISHED)
#else
	if(mDrag && win->MouseDragEnd()==MOUSEDRAGRESULT_FINISHED)
#endif
	{
		mDrag = false;
		if(!opActive)
		{
			op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
			
			doc->StartUndo();
			doc->InsertObject(op, NULL, NULL,true);
			CDAddUndo(doc,CD_UNDO_NEW,op);
			op->Message(MSG_MENUPREPARE);

			opM = Matrix();
			opM.off = opPos;
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			op->SetMg(opM);
			BaseContainer *data = op->GetDataInstance();
			data->SetBool(JNT_CONNECTED, false);
			data->SetReal(JNT_JOINT_RADIUS,jSize);
			doc->SetActiveObject(op);
			doc->EndUndo();
		}
		else
		{
			if(opActive->GetType() == ID_CDJOINTOBJECT)
			{
				pM = opActive->GetMg();
				Matrix scaM = MatrixScale(CDGetScale(opActive));
				if(opPos != pM.off)
				{
					if(!opActiveChild)
					{
						op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
						
						doc->StartUndo();
						CDAddUndo(doc,CD_UNDO_CHANGE,opActive);
						endDir = VNorm(opPos - pM.off);
						pM.v3 = endDir;
						Real upAngle = VDot(endDir, Vector(0,1,0));
						if(upAngle>-0.9999 && upAngle<0.9999)
						{
							pM.v1 = VNorm(VCross(Vector(0,1,0), pM.v3));
							pM.v2 = VNorm(VCross(pM.v3, pM.v1));
						}
						else
						{
							if(opPos.z - pM.off.z < 0)
							{
								pM.v2 = VNorm(VCross(pM.v3, Vector(-1,0,0)));
								pM.v1 = VNorm(VCross(pM.v2, pM.v3));
							}
							else
							{
								pM.v2 = VNorm(VCross(pM.v3, Vector(1,0,0)));
								pM.v1 = VNorm(VCross(pM.v2, pM.v3));
							}
						}
						opActive->SetMg(pM * scaM);
						pData = opActive->GetDataInstance();
						pData->SetMatrix(2000,pM * scaM);
						
						doc->InsertObject(op,opActive, NULL,true);
						CDAddUndo(doc,CD_UNDO_NEW,op);
						op->Message(MSG_MENUPREPARE);
						
						opM = pM;
						opM.off = opPos;
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						op->SetMg(opM);
						BaseContainer *data = op->GetDataInstance();
						data->SetBool(JNT_CONNECTED, true);
						data->SetReal(JNT_JOINT_RADIUS,jSize);
						doc->SetActiveObject(op);
						doc->EndUndo();
					}
					else
					{
						if(ctrl)
						{
							pData = opActive->GetDataInstance();
							if(connected)
							{
								BaseContainer *chData = opActiveChild->GetDataInstance();
								chData->SetBool(JNT_CONNECTED, true);
								chData->SetReal(JNT_JOINT_RADIUS,jSize);
								
								op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
								
								doc->StartUndo();
								doc->InsertObject(op,opActive, pred,true);
								CDAddUndo(doc,CD_UNDO_NEW,op);
								op->Message(MSG_MENUPREPARE);
								
								opM = Matrix();
								opM.off = opPos;
								CDAddUndo(doc,CD_UNDO_CHANGE,op);
								op->SetMg(opM);
								BaseContainer *data = op->GetDataInstance();
								data->SetBool(JNT_CONNECTED, true);
								data->SetReal(JNT_JOINT_RADIUS,jSize);
								doc->SetActiveObject(op);

								// move child joint
								Matrix chM = opActiveChild->GetMg();
								CDAddUndo(doc,CD_UNDO_CHANGE,opActiveChild);
								opActiveChild->Remove();
								doc->InsertObject(opActiveChild,op, NULL,false);
								opActiveChild->SetMg(chM);
								
								DescriptionCommand dc;
								dc.id = DescID(JNT_TOOL_ORIENT_JOINT);
								opActive->Message(MSG_DESCRIPTION_COMMAND,&dc);
								op->Message(MSG_DESCRIPTION_COMMAND,&dc);
								
								doc->EndUndo();
							}
							else
							{
								op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
								
								doc->StartUndo();
								doc->InsertObject(op,opActive, NULL,true);
								CDAddUndo(doc,CD_UNDO_NEW,op);
								op->Message(MSG_MENUPREPARE);
								
								opM = Matrix();
								opM.off = opPos;
								CDAddUndo(doc,CD_UNDO_CHANGE,op);
								op->SetMg(opM);
								BaseContainer *data = op->GetDataInstance();
								data->SetBool(JNT_CONNECTED, true);
								data->SetReal(JNT_JOINT_RADIUS,jSize);
								doc->SetActiveObject(op);
								doc->EndUndo();
							}
						}
						else
						{
							op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
							
							doc->StartUndo();
							doc->InsertObject(op,opActive, opActiveChild,true);
							CDAddUndo(doc,CD_UNDO_NEW,op);
							op->Message(MSG_MENUPREPARE);
							
							opM = Matrix();
							opM.off = opPos;
							CDAddUndo(doc,CD_UNDO_CHANGE,op);
							op->SetMg(opM);
							BaseContainer *data = op->GetDataInstance();
							data->SetBool(JNT_CONNECTED, true);
							data->SetBool(JNT_JOINT_RADIUS,jSize);
							doc->SetActiveObject(op);
							doc->EndUndo();
						}
					}
				}
			}
			else
			{
				op = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!op) return false;
				
				doc->StartUndo();
				doc->InsertObject(op,NULL, NULL,true);
				CDAddUndo(doc,CD_UNDO_NEW,op);
				op->Message(MSG_MENUPREPARE);
				
				opM  = Matrix();
				opM.off = opPos;
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				op->SetMg(opM);
				doc->SetActiveObject(op);
				doc->EndUndo();
			}
		}
		EventAdd();
		ctrl = false;
		shift = false;
		CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}

	return true;
}

Bool CDJointToolPlugin::GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x,Real y,BaseContainer &bc)
{
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(IDS_CDJOINTTOOL)+GeLoadString(IDS_HLP_CDJOINTTOOL));
	bc.SetLong(RESULT_CURSOR,MOUSE_CROSS);
	return true;
}

class CDJointToolPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDJointToolPlugin(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
	CHAR chr;
	
	aK = Mod(aK,25);
	bK = Mod(bK,3);
	if(Mod(aK,2) == 0) chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) | cK);
	else chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) & cK);
	b = chr;
	
	String s, hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(b);
	
	ch[1] = 0;
	for(i=0; i<2; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}
	s = hStr;
	if(kb != s) reg = false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = false;
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJOINTTOOL); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDJOINTTOOLPLUGIN,name,PLUGINFLAG_HIDE,"CDJointTool.tif","CD Joint Tool",CDDataAllocator(CDJointToolPluginR));
	else return CDRegisterToolPlugin(ID_CDJOINTTOOLPLUGIN,name,0,"CDJointTool.tif","CD Joint Tool",CDDataAllocator(CDJointToolPlugin));
}
