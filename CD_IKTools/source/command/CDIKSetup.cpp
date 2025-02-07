//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"
#include "CDArray.h"

enum
{
	GADGET_SETUP = 5000,
	GADGET_SEPARATOR,
	GADGET_NUMBER_BONES
};

class IKSetupDialog : public GeDialog
{
private:
	CDIKOptionsUA 			ua;

	void DoEnable(void);
	
	Bool SetupLinkageIK(int ikPole);
	Bool SetupPistonIK(int ikPole, int nbone);
	
	Bool SetupLimbIK(LONG ikPole, LONG sLine);
	Bool SetupQuadLegIK(LONG ikPole, LONG sLine);
	Bool SetupMechIK(LONG ikPole);
	Bool SetupFootIK(LONG ikPole);
	Bool SetupFinger(LONG ikPole);
	Bool SetupThumb(LONG ikPole);
	Bool SetupRotator(LONG sLine, LONG nbone);
	
	void FitSplineToJoints(BaseDocument *doc, BaseObject *jnt, SplineObject *spl, LONG jCnt, LONG trgCnt);
	Bool SetupSplineIK(LONG ikPole, LONG starg, LONG nbone);
	Bool SetupSpinal(LONG spAxis, LONG nbone);
	
public:
	IKSetupDialog(void);

	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

IKSetupDialog::IKSetupDialog(void)
{ 
}

Bool IKSetupDialog::CreateLayout(void)
{
	Bool res = GeDialog::CreateLayout();
	if(res)
	{
		SetTitle(GeLoadString(D_TITLE));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDIK_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDIK_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,3,0,"",0);
		{
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(10,10,10,10);
				
				GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_EXPR_TYPE),0);
				{
					GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
					GroupBorderSpace(8,8,8,8);
					
					AddRadioGroup(IDC_EXPRESSION_TYPE,0,1,0);
						AddChild(IDC_EXPRESSION_TYPE, 0, GeLoadString(D_LIMB));
						AddChild(IDC_EXPRESSION_TYPE, 1, GeLoadString(D_QUAD));
						AddChild(IDC_EXPRESSION_TYPE, 2, GeLoadString(D_MECH));
						AddChild(IDC_EXPRESSION_TYPE, 3, GeLoadString(D_FOOT));
						AddChild(IDC_EXPRESSION_TYPE, 4, GeLoadString(D_FINGER));
						AddChild(IDC_EXPRESSION_TYPE, 5, GeLoadString(D_THUMB));
						AddChild(IDC_EXPRESSION_TYPE, 6, GeLoadString(D_ROTATOR));
						AddChild(IDC_EXPRESSION_TYPE, 7, GeLoadString(D_SPLINE));
						AddChild(IDC_EXPRESSION_TYPE, 8, GeLoadString(D_SPINE));
						AddChild(IDC_EXPRESSION_TYPE, 9, GeLoadString(D_LINKAGE));
						AddChild(IDC_EXPRESSION_TYPE, 10, GeLoadString(D_PISTON));
				}
				GroupEnd();
			}
			GroupEnd();
		
			AddSeparatorV(30);

			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(10,10,10,10);
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_SOLVER_AXIS),0);
					AddComboBox(IDC_SOLVER_AXIS, BFH_SCALEFIT);
						AddChild(IDC_SOLVER_AXIS, 0, GeLoadString(D_X));
						AddChild(IDC_SOLVER_AXIS, 1, GeLoadString(D_Y));
						AddChild(IDC_SOLVER_AXIS, 2, GeLoadString(D_NX));
						AddChild(IDC_SOLVER_AXIS, 3, GeLoadString(D_NY));
				}
				GroupEnd();
				
				GroupSpace(0,8);
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_LN_TARGET),0);
					AddComboBox(IDC_SOLVER_LINE, BFH_SCALEFIT);
						AddChild(IDC_SOLVER_LINE, 0, GeLoadString(D_ROOT));
						AddChild(IDC_SOLVER_LINE, 1, GeLoadString(D_TIP));
				}
				GroupEnd();
				
				GroupSpace(0,8);
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_SPINAL_AXIS),0);
					AddComboBox(IDC_SPINAL_AXIS, BFH_SCALEFIT);
						AddChild(IDC_SPINAL_AXIS, 0, GeLoadString(D_X));
						AddChild(IDC_SPINAL_AXIS, 1, GeLoadString(D_Y));
						AddChild(IDC_SPINAL_AXIS, 2, GeLoadString(D_Z));
						AddChild(IDC_SPINAL_AXIS, 3, GeLoadString(D_NX));
						AddChild(IDC_SPINAL_AXIS, 4, GeLoadString(D_NY));
						AddChild(IDC_SPINAL_AXIS, 5, GeLoadString(D_NZ));
				}
				GroupEnd();
				
				GroupSpace(0,8);
				
				GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_SPL_TARGET),0);
				{
					GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
					GroupBorderSpace(8,8,8,8);
					
					AddRadioGroup(IDC_SPLINE_TARGETS,0,0,1);
						AddChild(IDC_SPLINE_TARGETS, 0, GeLoadString(D_SPL_TARG_A));
						AddChild(IDC_SPLINE_TARGETS, 1, GeLoadString(D_SPL_TARG_B));
						AddChild(IDC_SPLINE_TARGETS, 2, GeLoadString(D_SPL_TARG_C));
						AddChild(IDC_SPLINE_TARGETS, 3, GeLoadString(D_SPL_TARG_D));
				}
				GroupEnd();
				
				GroupSpace(0,8);
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_NUM_BONES),0);
					AddEditNumberArrows(GADGET_NUMBER_BONES,BFH_LEFT);
				}
				GroupEnd();

				AddSeparatorH(100);

				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					
					AddButton(GADGET_SETUP,BFH_CENTER,0,0,GeLoadString(D_SETUP_CHAIN));
				}
				GroupEnd();
			}
			GroupEnd();
		}
		GroupEnd();

	}

	
	return res;
}

Bool IKSetupDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;
	
	SetLong(IDC_EXPRESSION_TYPE,0);
	SetLong(IDC_SOLVER_AXIS,1);
	SetLong(IDC_SPINAL_AXIS,1);
	SetLong(IDC_SPLINE_TARGETS,0);
	SetLong(GADGET_NUMBER_BONES,2,2,50,1,FORMAT_LONG);

	DoEnable();

	return true;
}

Bool IKSetupDialog::SetupLimbIK(LONG ikPole, LONG sLine)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *ch = op->GetDown(); if(!ch) continue;

			LONG ikAxis, lnTarg;
			
			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			
			// Get the child's parameters
			Matrix chM = ch->GetMg();
			Vector chPos = chM.off;
			String chName = ch->GetName();
			
			// Get the grandchild's parameters
			Real bLen; Vector tipPos; Matrix tipM;
			BaseObject* tipOp = NULL;
			BaseObject *gch = ch->GetDown();
			if(!gch)
			{
				// If there is no grandchild, add a Tip Effector null object
				bLen = ch->GetRad().z*2;
				tipPos = chM.off + chM.v3 * bLen;
				tipM = Matrix(tipPos,chM.v1,chM.v2,chM.v3);
				tipOp = BaseObject::Alloc(Onull); if(!tipOp) continue;
				tipOp->SetName(chName + GeLoadString(N_TIP_EFFECTOR));
				// Set the tip effector's object's parameters
				BaseContainer *tipData = tipOp->GetDataInstance();
				tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
				tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
				ObjectColorProperties tProp;
				tProp.color = Vector(0,1,1);
				tProp.usecolor = 2;
				tipOp->SetColorProperties(&tProp);
				doc->InsertObject(tipOp,ch, NULL,true);
				tipOp->SetMg(tipM);
				gch = ch->GetDown();if(!gch) continue;
			}
			else
			{
				tipM = gch->GetMg();
				tipPos = tipM.off;
			}
			
			Vector sDir, normV;
			Vector goalV = tipPos - opPos;
			Real dot = VDot(VNorm(opM.v3), VNorm(goalV));
			if(dot > 0.996)
			{
				switch (ikPole)
				{
					case 0:
						sDir = opM.v1;
						break;
					case 1:
						sDir = opM.v2;
						break;
					case 2:
						sDir = -(opM.v1);
						break;
					case 3:
						sDir = -(opM.v2);
						break;
				}
			}
			else
			{
				normV = VNorm(VCross(goalV, opM.v3));
				sDir = VNorm(VCross(normV, goalV));
			}
			Real svLen = Len(chPos - opPos);
			svLen = svLen + (svLen/3);
			Vector sPos = opPos + sDir * svLen;

			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
			Matrix poleM = pole->GetMg();
			poleM.off = sPos;
			poleM.v1 = Vector(1,0,0);
			poleM.v2 = Vector(0,1,0);
			poleM.v3 = Vector(0,0,1);
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			// Add the Tip Goal null object
			BaseObject* goal = NULL;
			goal = BaseObject::Alloc(Onull); if(!goal) continue;
			goal->SetName(chName + GeLoadString(N_TIP_GOAL));
			doc->InsertObject(goal, NULL, NULL,true);
			Matrix goalM = goal->GetMg();
			goalM.off = gch->GetMg().off;
			goal->SetMg(goalM);
			
			// Set the Tip Goal object's parameters
			BaseContainer *gData = goal->GetDataInstance();
			gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			gData->SetReal(NULLOBJECT_RADIUS,20.0);
			gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			goal->SetColorProperties(&tProp);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			
			// Add the LimbIK Tag
			BaseTag *limbIKTag = NULL;
			limbIKTag = BaseTag::Alloc(ID_CDLIMBIKPLUGIN);
			op->InsertTag(limbIKTag,NULL);
			limbIKTag->Message(MSG_MENUPREPARE);
			limbIKTag->SetName(opName + GeLoadString(N_LIMB_IK));

			// Add the Solver and Tip Goal objects to the LimbIK tag links
			BaseContainer *tData = limbIKTag->GetDataInstance();
			switch (ikPole)
			{
				case 0:
					ikAxis = 10008;
					break;
				case 1:
					ikAxis = 10007;
					break;
				case 2:
					ikAxis = 10019;
					break;
				case 3:
					ikAxis = 10020;
					break;
			}
			switch (sLine)
			{
				case 0:
					lnTarg = 10004;
					break;
				case 1:
					lnTarg = 10005;
					break;
			}
			tData->SetLong(10006,ikAxis);
			tData->SetLong(10003,lnTarg);
			tData->SetLink(10009,pole);
			tData->SetLink(10010,goal);
			
			doc->SetActiveObject(goal);
			CDAddUndo(doc,CD_UNDO_NEW,limbIKTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);
			CDAddUndo(doc,CD_UNDO_NEW,tipOp);
			CDAddUndo(doc,CD_UNDO_NEW,goal);
			
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
			
			// Reset the the tip's position
			gch->SetMg(tipM);
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupQuadLegIK(LONG ikPole, LONG sLine)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *ch = op->GetDown(); if(!ch) continue;
			BaseObject *gch = ch->GetDown(); if(!gch) continue;

			LONG ikAxis, lnTarg;
			
			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			
			// Get the child's parameters
			Matrix chM = ch->GetMg();
			Vector chPos = chM.off;
			String chName = ch->GetName();
			
			// Get the grandchild's parameters
			Matrix gchM = gch->GetMg();
			Vector gchPos = gchM.off;
			String gchName = gch->GetName();
			
			// Get the tip effector's parameters
			Real bLen; Vector tipPos; Matrix tipM;
			BaseObject* tipOp = NULL;
			BaseObject *tipEff = gch->GetDown();
			if(!tipEff)
			{
				// If there is no grandchild, add a Tip Effector null object
				bLen = gch->GetRad().z*2;
				tipPos = gchM.off + gchM.v3 * bLen;
				tipM = Matrix(tipPos,gchM.v1,gchM.v2,gchM.v3);
				tipOp = BaseObject::Alloc(Onull); if(!tipOp) continue;
				tipOp->SetName(gchName + GeLoadString(N_TIP_EFFECTOR));
				doc->InsertObject(tipOp,gch, NULL,true);
				
				// Set the tip effector's object's parameters
				BaseContainer *tipData = tipOp->GetDataInstance();
				tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
				tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
				ObjectColorProperties tProp;
				tProp.color = Vector(0,1,1);
				tProp.usecolor = 2;
				tipOp->SetColorProperties(&tProp);
				tipOp->SetMg(tipM);
				tipEff = gch->GetDown();if(!tipEff) continue;
			}
			else
			{
				tipM = tipEff->GetMg();
				tipPos = tipM.off;
			}
			
			Vector sDir, normV;
			Vector goalV = tipPos - opPos;
			Real dot = VDot(VNorm(opM.v3), VNorm(goalV));
			if(dot > 0.996)
			{
				switch (ikPole)
				{
					case 0:
						sDir = opM.v1;
						break;
					case 1:
						sDir = opM.v2;
						break;
					case 2:
						sDir = -(opM.v1);
						break;
					case 3:
						sDir = -(opM.v2);
						break;
				}
			}
			else
			{
				normV = VNorm(VCross(goalV, opM.v3));
				sDir = VNorm(VCross(normV, goalV));
			}
			Real svLen = Len(chPos - opPos);
			svLen = svLen + (svLen/3);
			Vector sPos = opPos + sDir * svLen;

			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
			Matrix poleM = pole->GetMg();
			poleM.off = sPos;
			poleM.v1 = Vector(1,0,0);
			poleM.v2 = Vector(0,1,0);
			poleM.v3 = Vector(0,0,1);
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			// Add the Tip Goal null object
			BaseObject* goal = NULL;
			goal = BaseObject::Alloc(Onull); if(!goal) continue;
			goal->SetName(gchName + GeLoadString(N_TIP_GOAL));
			doc->InsertObject(goal, NULL, NULL,true);
			Matrix goalM = goal->GetMg();
			goalM.off = tipEff->GetMg().off;
			goal->SetMg(goalM);
			
			// Set the Tip Goal object's parameters
			BaseContainer *gData = goal->GetDataInstance();
			gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			gData->SetReal(NULLOBJECT_RADIUS,20.0);
			gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			goal->SetColorProperties(&tProp);
			
			// Set the distribution angle
			Vector distrVector = !(tipPos - gchPos);
			Real dAngle = (ACos(VDot(distrVector, sDir))) - pi05;
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			CDAddUndo(doc,CD_UNDO_CHANGE,gch);
			
			// Add the QuadlegIK Tag
			BaseTag *quadLegIKTag = NULL;
			quadLegIKTag = BaseTag::Alloc(ID_CDQUADLEGPLUGIN);
			op->InsertTag(quadLegIKTag,NULL);
			quadLegIKTag->Message(MSG_MENUPREPARE);
			quadLegIKTag->SetName(opName + GeLoadString(N_QUADLEG_IK));

			// Add the Solver and Tip Goal objects to the QuadLegIK tag links
			BaseContainer *tData = quadLegIKTag->GetDataInstance();
			switch (ikPole)
			{
				case 0:
					ikAxis = 10008;
					break;
				case 1:
					ikAxis = 10007;
					break;
				case 2:
					ikAxis = 10019;
					break;
				case 3:
					ikAxis = 10020;
					break;
			}
			switch (sLine)
			{
				case 0:
					lnTarg = 10004;
					break;
				case 1:
					lnTarg = 10005;
					break;
			}
			tData->SetLong(10006,ikAxis);
			tData->SetLong(10003,lnTarg);
			tData->SetReal(20003,-(dAngle));
			tData->SetLink(10009,pole);
			tData->SetLink(10010,goal);
			
			doc->SetActiveObject(goal);
			CDAddUndo(doc,CD_UNDO_NEW,quadLegIKTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);
			CDAddUndo(doc,CD_UNDO_NEW,tipOp);
			CDAddUndo(doc,CD_UNDO_NEW,goal);
			
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
			// Reset the the tip object's position
			tipEff->SetMg(tipM);
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupMechIK(LONG ikPole)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *ch = op->GetDown(); if(!ch) continue;
			BaseObject *gch = ch->GetDown(); if(!gch) continue;

			LONG ikAxis;
			
			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			
			// Get the child's parameters
			Matrix chM = ch->GetMg();
			Vector chPos = chM.off;
			String chName = ch->GetName();
			
			// Get the grandchild's parameters
			Matrix gchM = gch->GetMg();
			Vector gchPos = gchM.off;
			String gchName = gch->GetName();
			
			// Get the tip effector's parameters
			Real bLen; Vector tipPos; Matrix tipM;
			BaseObject* tipOp = NULL;
			BaseObject *tipEff = gch->GetDown();
			if(!tipEff)
			{
				// If there is no grandchild, add a Tip Effector null object
				bLen = gch->GetRad().z*2;
				tipPos = gchM.off + gchM.v3 * bLen;
				tipM = Matrix(tipPos,gchM.v1,gchM.v2,gchM.v3);
				tipOp = BaseObject::Alloc(Onull); if(!tipOp) continue;
				tipOp->SetName(gchName + GeLoadString(N_TIP_EFFECTOR));
				doc->InsertObject(tipOp,gch, NULL,true);
				
				// Set the tip effector's object's parameters
				BaseContainer *tipData = tipOp->GetDataInstance();
				tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
				tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
				ObjectColorProperties tProp;
				tProp.color = Vector(0,1,1);
				tProp.usecolor = 2;
				tipOp->SetColorProperties(&tProp);
				tipOp->SetMg(tipM);
				tipEff = gch->GetDown();if(!tipEff) continue;
			}
			else
			{
				tipM = tipEff->GetMg();
				tipPos = tipM.off;
			}
			
			// determine Solver direction
			Vector sDir, normV;
			Vector goalV = tipPos - chPos;
			Real dot = VDot(VNorm(chM.v3), VNorm(goalV));
			if(dot > 0.996)
			{
				switch (ikPole)
				{
					case 0:
						sDir = chM.v1;
						break;
					case 1:
						sDir = chM.v2;
						break;
					case 2:
						sDir = -(chM.v1);
						break;
					case 3:
						sDir = -(chM.v2);
						break;
				}
			}
			else
			{
				normV = VNorm(VCross(goalV, chM.v3));
				sDir = VNorm(VCross(normV, goalV));
			}
			
			Real svLen = Len(gchPos - chPos);
			svLen = svLen + (svLen/3);
			Vector sPos = opPos + sDir * svLen;
			
			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
			Matrix poleM = pole->GetMg();
			poleM.off = sPos;
			poleM.v1 = Vector(1,0,0);
			poleM.v2 = Vector(0,1,0);
			poleM.v3 = Vector(0,0,1);
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			// Add the Tip Goal null object
			BaseObject* goal = NULL;
			goal = BaseObject::Alloc(Onull); if(!goal) continue;
			goal->SetName(gchName + GeLoadString(N_TIP_GOAL));
			doc->InsertObject(goal, NULL, NULL,true);
			Matrix goalM = goal->GetMg();
			goalM.off = tipEff->GetMg().off;
			goal->SetMg(goalM);
			
			// Set the Tip Goal object's parameters
			BaseContainer *gData = goal->GetDataInstance();
			gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			gData->SetReal(NULLOBJECT_RADIUS,20.0);
			gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			goal->SetColorProperties(&tProp);
			
			// determine flip
			Bool flip = false;
			normV = VNorm(VCross(sDir, goalV));
			dot = VDot(normV, VNorm(opM.v3));
			if(dot > 0.0) flip = true;

			// Set the root angle
			goalV = tipPos - opPos;
			Real rtAngle = 0.0;
			Real A = Len(chM.off - opM.off);
			Real H = Len(goalV);
			Real B = Sqrt((H * H) - (A * A));

			if(H > 0.0)
			{
				Real theta = ACos(VDot(VNorm(goalV), VNorm(opM.v3)));
				Real gLen = Len(tipPos - chPos);
				
				if(gLen < B)
				{
					rtAngle = -(ACos(A/H) - theta);
				}
				else if(gLen > B)
				{
					rtAngle = theta - ACos(A/H);
				}
			}
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			CDAddUndo(doc,CD_UNDO_CHANGE,gch);
			
			// Add the MechIK Tag
			BaseTag *mechIKTag = BaseTag::Alloc(ID_CDMECHLIMBPLUGIN);
			op->InsertTag(mechIKTag,NULL);
			mechIKTag->Message(MSG_MENUPREPARE);
			mechIKTag->SetName(opName + GeLoadString(N_MECH_IK));

			// Add the Solver and Tip Goal objects to the Mech IK tag links
			BaseContainer *tData = mechIKTag->GetDataInstance();
			switch (ikPole)
			{
				case 0:
					ikAxis = 10008;
					break;
				case 1:
					ikAxis = 10007;
					break;
				case 2:
					ikAxis = 10019;
					break;
				case 3:
					ikAxis = 10020;
					break;
			}
			tData->SetLong(10006,ikAxis);
			tData->SetLink(10009,pole);
			tData->SetLink(10010,goal);
			tData->SetBool(10004,flip);
			tData->SetReal(10003,rtAngle);
			
			doc->SetActiveObject(goal);
			CDAddUndo(doc,CD_UNDO_NEW,mechIKTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);
			CDAddUndo(doc,CD_UNDO_NEW,tipOp);
			CDAddUndo(doc,CD_UNDO_NEW,goal);
			
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
			// Reset the the tip object's position
			tipEff->SetMg(tipM);
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
	
}

Bool IKSetupDialog::SetupFootIK(LONG ikPole)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *ch = op->GetDown(); if(!ch) continue;

			LONG ikAxis;
			
			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			
			// Get the child's parameters
			Matrix chM = ch->GetMg();
			Vector chPos = chM.off;
			String chName = ch->GetName();
			
			// Get the grandchild's parameters
			Real bLen; Vector tipPos; Matrix tipM;
			BaseObject* tipOp = NULL;
			BaseObject *gch = ch->GetDown();
			if(!gch)
			{
				// If there is no grandchild, add a Tip Effector null object
				bLen = ch->GetRad().z*2;
				tipPos = chM.off + chM.v3 * bLen;
				tipM = Matrix(tipPos,chM.v1,chM.v2,chM.v3);
				tipOp = BaseObject::Alloc(Onull); if(!tipOp) continue;
				tipOp->SetName(chName + GeLoadString(N_TIP_EFFECTOR));
				// Set the tip effector's object's parameters
				BaseContainer *tipData = tipOp->GetDataInstance();
				tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
				tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
				ObjectColorProperties tProp;
				tProp.color = Vector(0,1,1);
				tProp.usecolor = 2;
				tipOp->SetColorProperties(&tProp);
				doc->InsertObject(tipOp,ch, NULL,true);
				tipOp->SetMg(tipM);
				gch = ch->GetDown();if(!gch) return true;
			}
			else
			{
				tipPos = gch->GetMg().off;
			}
			
			// Calculate the Solver's Position
			Vector revFoot = VNorm(opPos - chPos);
			Vector revToe = VNorm(chPos - tipPos);
			Real footLen = Len(opPos - chPos);
			Real angleA = ACos(VDot(revFoot, revToe));
			Real sLen = footLen/Cos(angleA);
			Vector sPos = chPos + revToe * sLen;
			
			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, NULL, NULL,true);
			Matrix poleM = opM;
			poleM.off = sPos;
			poleM.v1 = Vector(1,0,0);
			poleM.v2 = Vector(0,1,0);
			poleM.v3 = Vector(0,0,1);
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);

			// Add the Foot Goal object
			BaseObject* footGoal = NULL;
			footGoal = BaseObject::Alloc(Onull); if(!footGoal) continue;
			footGoal->SetName(opName + GeLoadString(N_TIP_GOAL));
			doc->InsertObject(footGoal, pole, NULL,true);
			Matrix footGoalMatrix = chM;
			footGoalMatrix.v1 = Vector(1,0,0);
			footGoalMatrix.v2 = Vector(0,1,0);
			footGoalMatrix.v3 = Vector(0,0,1);
			footGoal->SetMg(footGoalMatrix);
			
			// Set the Foot Goal object's parameters
			BaseContainer *fData = footGoal->GetDataInstance();
			fData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
			ObjectColorProperties fProp;
			fProp.color = Vector(0,1,1);
			fProp.usecolor = 2;
			footGoal->SetColorProperties(&fProp);
			
			// Set the Tip Goal null object
			BaseObject* goal = NULL;
			
			tipPos = gch->GetMg().off;
			tipM = Matrix(tipPos,chM.v1,chM.v2,chM.v3);
			
			goal = BaseObject::Alloc(Onull); if(!goal) continue;
			goal->SetName(chName + GeLoadString(N_TIP_GOAL));
			doc->InsertObject(goal, pole, footGoal,true);
			tipM.v1 = Vector(1,0,0);
			tipM.v2 = Vector(0,1,0);
			tipM.v3 = Vector(0,0,1);
			goal->SetMg(tipM);
			
			// Set the Tip Goal object's parameters
			BaseContainer *gData = goal->GetDataInstance();
			gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			goal->SetColorProperties(&tProp);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);

			// Add the FootIK Tag
			BaseTag *footIKTag = NULL;
			footIKTag = BaseTag::Alloc(ID_CDFOOTIKPLUGIN);
			op->InsertTag(footIKTag,NULL);
			footIKTag->Message(MSG_MENUPREPARE);
			footIKTag->SetName(opName + GeLoadString(N_FOOT_IK));

			// Add the Solver and Tip Goal objects to the FootIK tag links
			BaseContainer *tData = footIKTag->GetDataInstance();
			switch (ikPole)
			{
				case 0:
					ikAxis = 10005;
					break;
				case 1:
					ikAxis = 10004;
					break;
				case 2:
					ikAxis = 10016;
					break;
				case 3:
					ikAxis = 10017;
					break;
			}
			tData->SetLong(10003,ikAxis);
			tData->SetLink(10006,pole);
			tData->SetLink(10007,footGoal);
			tData->SetLink(10008,goal);
			
			BaseObject* goalClone = NULL;
			BaseTag *pTag = NULL;
			BaseObject *theParent = op->GetUp();
			if(theParent)
			{
				theParent = theParent->GetUp();
				if(theParent) pTag = theParent->GetTag(ID_CDLIMBIKPLUGIN);
			}
			if(!pTag)
			{
				if(theParent)
				{
					theParent = theParent->GetUp();
					if(theParent)
					{
						pTag = theParent->GetTag(ID_CDQUADLEGPLUGIN);
						if(pTag)
						{
							BaseObject *pGoal = pTag->GetDataInstance()->GetObjectLink(10010,doc);
							if(pGoal)
							{
								AutoAlloc<AliasTrans> trans;
								if(trans)
								{
									if(trans->Init(doc))
									{
										goalClone = (BaseObject*)CDGetClone(pGoal,CD_COPYFLAGS_0,trans);
										if(goalClone)
										{
											trans->Translate(true);
											doc->InsertObject(goalClone,footGoal, NULL,false);
											CDAddUndo(doc,CD_UNDO_NEW,goalClone);
											goalClone->SetMg(pGoal->GetMg());
											
											//Transfer Goals
											CDTransferGoals(pGoal,goalClone);
											CDAddUndo(doc,CD_UNDO_DELETE,pGoal);
											BaseObject::Free(pGoal);
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				BaseObject *pGoal = pTag->GetDataInstance()->GetObjectLink(10010,doc);
				if(pGoal)
				{
					AutoAlloc<AliasTrans> trans;
					if(trans)
					{
						if(trans->Init(doc))
						{
							goalClone = (BaseObject*)CDGetClone(pGoal,CD_COPYFLAGS_0,trans);
							if(goalClone)
							{
								trans->Translate(true);
								doc->InsertObject(goalClone,footGoal, NULL,false);
								CDAddUndo(doc,CD_UNDO_NEW,goalClone);
								goalClone->SetMg(pGoal->GetMg());
								
								//Transfer Goals
								CDTransferGoals(pGoal,goalClone);
								CDAddUndo(doc,CD_UNDO_DELETE,pGoal);
								BaseObject::Free(pGoal);
							}
						}
					}
				}
			}
			
			doc->SetActiveObject(pole);
			CDAddUndo(doc,CD_UNDO_NEW,footIKTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);
			CDAddUndo(doc,CD_UNDO_NEW,footGoal);
			CDAddUndo(doc,CD_UNDO_NEW,goal);
				
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupFinger(LONG ikPole)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *chA = op->GetDown(); if(!chA) continue;
			BaseObject *chB = chA->GetDown(); if(!chB) continue;
			BaseObject *chC = chB->GetDown(); if(!chC) continue;

			LONG ikAxis;
			Real spreadAngle, bendAngle, curlAngle, dampAngle;

			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			Real bLen = op->GetRad().z*2;

			Matrix childAMatrix = chA->GetMg();
			Matrix childBMatrix = chB->GetMg();
			Matrix childCMatrix = chC->GetMg();
			Vector sPos, spreadVector;
			switch (ikPole)
			{
				case 0:
					spreadAngle = ((ACos(VDot(VNorm(childAMatrix.v2), VNorm(opM.v3)))) - pi05);
					spreadVector = VNorm(VCross(opM.v1, childAMatrix.v2));
					bendAngle = -((ACos(VDot(VNorm(childAMatrix.v1), spreadVector))) - pi05);
					curlAngle = -((ACos(VDot(VNorm(childBMatrix.v1), VNorm(childAMatrix.v3)))) - pi05);
					dampAngle = ACos(VDot(VNorm(childCMatrix.v3), VNorm(childBMatrix.v3)));
					sPos = opPos + opM.v1 * bLen/3;
					ikAxis = 10013;
					break;
				case 1:
					spreadAngle = -((ACos(VDot(VNorm(childAMatrix.v1), VNorm(opM.v3)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v1, opM.v2));
					bendAngle = -((ACos(VDot(VNorm(childAMatrix.v2), spreadVector))) - pi05);
					curlAngle = -((ACos(VDot(VNorm(childBMatrix.v2), VNorm(childAMatrix.v3)))) - pi05);
					dampAngle = ACos(VDot(VNorm(childCMatrix.v3), VNorm(childBMatrix.v3)));
					sPos = opPos + opM.v2 * bLen/3;
					ikAxis = 10012;
					break;
				case 2:
					spreadAngle = -((ACos(VDot(VNorm(childAMatrix.v2), VNorm(opM.v3)))) - pi05);
					spreadVector = VNorm(VCross(opM.v1, childAMatrix.v2));
					bendAngle = ((ACos(VDot(VNorm(childAMatrix.v1), spreadVector))) - pi05);
					curlAngle = ((ACos(VDot(VNorm(childBMatrix.v1), VNorm(childAMatrix.v3)))) - pi05);
					dampAngle = ACos(VDot(VNorm(childCMatrix.v3), VNorm(childBMatrix.v3)));
					sPos = opPos + (-(opM.v1)) * bLen/3;
					ikAxis = 10025;
					break;
				case 3:
					spreadAngle = ((ACos(VDot(VNorm(childAMatrix.v1), VNorm(opM.v3)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v1,opM.v2));
					bendAngle = ((ACos(VDot(VNorm(childAMatrix.v2), spreadVector))) - pi05);
					curlAngle = ((ACos(VDot(VNorm(childBMatrix.v2), VNorm(childAMatrix.v3)))) - pi05);
					dampAngle = ACos(VDot(VNorm(childCMatrix.v3), VNorm(childBMatrix.v3)));
					sPos = opPos + (-(opM.v2)) * bLen/3;
					ikAxis = 10026;
					break;
			}
			if(spreadAngle > Rad(15.0)) spreadAngle = Rad(15.0);
			if(spreadAngle < Rad(-15.0)) spreadAngle = Rad(-15.0);
			if(bendAngle > Rad(100.0)) bendAngle = Rad(100.0);
			if(bendAngle < Rad(-25.0)) bendAngle = Rad(-25.0);
			if(curlAngle > Rad(100.0)) curlAngle = Rad(100.0);
			if(curlAngle < Rad(-5.0)) curlAngle = Rad(-5.0);
			Real dampValue = 1 - (dampAngle / curlAngle);
			if(dampValue > 1.0) dampValue = 1.0;
			if(dampValue < 0.0) dampValue = 0.0;
			
			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, op->GetUp(), NULL,true);
			Matrix poleM = pole->GetMg();
			poleM.off = sPos;
			poleM.v1 = opM.v1;
			poleM.v2 = opM.v2;
			poleM.v3 = opM.v3;
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetReal(NULLOBJECT_RADIUS,bLen/20);
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,chA);
			CDAddUndo(doc,CD_UNDO_CHANGE,chB);
			CDAddUndo(doc,CD_UNDO_CHANGE,chC);

			// Add the Finger Tag
			BaseTag *fingerTag = NULL;
			fingerTag = BaseTag::Alloc(ID_CDFINGERPLUGIN);
			op->InsertTag(fingerTag,NULL);
			fingerTag->Message(MSG_MENUPREPARE);
			fingerTag->SetName(opName);

			// Add the Solver and Tip Goal objects to the Finger tag links
			BaseContainer *tData = fingerTag->GetDataInstance();
			tData->SetLong(10011,ikAxis);
			tData->SetReal(10023,spreadAngle);
			tData->SetReal(10020,bendAngle);
			tData->SetReal(10021,curlAngle);
			tData->SetReal(10022,dampValue);
			tData->SetLink(10014,pole);

			doc->SetActiveObject(pole);
			CDAddUndo(doc,CD_UNDO_NEW,fingerTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);

			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupThumb(LONG ikPole)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	LONG  i, opCount = objects->GetCount();
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *chA = op->GetDown(); if(!chA) continue;
			BaseObject *chB = chA->GetDown(); if(!chB) continue;
			BaseObject *chC = chB->GetDown(); if(!chC) continue;

			LONG ikAxis;
			Real gripAngle, twistAngle, spreadAngle, bendAngle, curlAngle;

			// Get the objects parameters
			Matrix opM = op->GetMg();
			Vector opPos = opM.off;
			String opName = op->GetName();
			Real bLen = op->GetRad().z*2;

			Matrix childAMatrix = chA->GetMg();
			Matrix childBMatrix = chB->GetMg();
			Matrix childCMatrix = chC->GetMg();
			Vector sPos, spreadVector, twistVector1, twistVector2, gripVector;
			switch (ikPole)
			{
				case 0:
					gripAngle = -((ACos(VDot(VNorm(opM.v1), VNorm(childAMatrix.v3)))) - pi05);
					twistVector1 = VNorm(VCross(childAMatrix.v3, opM.v1));
					twistVector2 = VNorm(VCross(twistVector1, childAMatrix.v3));
					twistAngle = ((ACos(VDot(twistVector2, VNorm(childAMatrix.v2)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v3, opM.v1));
					spreadAngle = -((ACos(VDot(VNorm(opM.v3), spreadVector))) - pi05);
					bendAngle = -((ACos(VDot(VNorm(childBMatrix.v1), VNorm(childAMatrix.v3)))) - pi05);
					curlAngle = -((ACos(VDot(VNorm(childCMatrix.v1), VNorm(childBMatrix.v3)))) - pi05);
					sPos = opPos + opM.v1 * 15;
					ikAxis = 10013;
					break;
				case 1:
					gripAngle = -((ACos(VDot(VNorm(opM.v2), VNorm(childAMatrix.v3)))) - pi05);
					twistVector1 = VNorm(VCross(childAMatrix.v3, opM.v2));
					twistVector2 = VNorm(VCross(twistVector1, childAMatrix.v3));
					twistAngle = -((ACos(VDot(twistVector2, VNorm(childAMatrix.v1)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v3, opM.v2));
					spreadAngle = -((ACos(VDot(VNorm(opM.v3), spreadVector))) - pi05);
					bendAngle = -((ACos(VDot(VNorm(childBMatrix.v2), VNorm(childAMatrix.v3)))) - pi05);
					curlAngle = -((ACos(VDot(VNorm(childCMatrix.v2), VNorm(childBMatrix.v3)))) - pi05);
					sPos = opPos + opM.v2 * 15;
					ikAxis = 10012;
					break;
				case 2:
					gripAngle = ((ACos(VDot(VNorm(opM.v1), VNorm(childAMatrix.v3)))) - pi05);
					twistVector1 = VNorm(VCross(childAMatrix.v3, opM.v1));
					twistVector2 = VNorm(VCross(twistVector1, childAMatrix.v3));
					twistAngle = ((ACos(VDot(twistVector2, VNorm(childAMatrix.v2)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v3, opM.v1));
					spreadAngle = ((ACos(VDot(VNorm(opM.v3), spreadVector))) - pi05);
					bendAngle = ((ACos(VDot(VNorm(childBMatrix.v1), VNorm(childAMatrix.v3)))) - pi05);
					curlAngle = ((ACos(VDot(VNorm(childCMatrix.v1), VNorm(childBMatrix.v3)))) - pi05);
					sPos = opPos + ( -(opM.v1)) * 15;
					ikAxis = 10021;
					break;
				case 3:
					gripAngle = ((ACos(VDot(VNorm(opM.v2), VNorm(childAMatrix.v3)))) - pi05);
					twistVector1 = VNorm(VCross(childAMatrix.v3, opM.v2));
					twistVector2 = VNorm(VCross(twistVector1, childAMatrix.v3));
					twistAngle = -((ACos(VDot(twistVector2, VNorm(childAMatrix.v1)))) - pi05);
					spreadVector = VNorm(VCross(childAMatrix.v3, opM.v2));
					spreadAngle = ((ACos(VDot(VNorm(opM.v3), spreadVector))) - pi05);
					bendAngle = ((ACos(VDot(VNorm(childBMatrix.v2), VNorm(childAMatrix.v3)))) - pi05);
					curlAngle = ((ACos(VDot(VNorm(childCMatrix.v2), VNorm(childBMatrix.v3)))) - pi05);
					sPos = opPos + ( -(opM.v2)) * 15;
					ikAxis = 10022;
					break;
			}
			if(gripAngle > Rad(30.0)) gripAngle = Rad(30.0);
			if(gripAngle < Rad(-30.0)) gripAngle = Rad(-30.0);
			if(twistAngle > Rad(30.0)) twistAngle = Rad(30.0);
			if(twistAngle < Rad(-30.0)) twistAngle = Rad(-30.0);
			if(spreadAngle > Rad(35.0)) spreadAngle = Rad(35.0);
			if(spreadAngle < Rad(-25.0)) spreadAngle = Rad(-25.0);
			if(bendAngle > Rad(60.0)) bendAngle = Rad(60.0);
			if(bendAngle < Rad(0.0)) bendAngle = Rad(0.0);
			if(curlAngle > Rad(90.0)) curlAngle = Rad(90.0);
			if(curlAngle < Rad(-25.0)) curlAngle = Rad(-25.0);
			
			// Add the Solver null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + GeLoadString(N_SOLVER));
			doc->InsertObject(pole, op->GetUp(), NULL,true);
			Matrix poleM = pole->GetMg();
			poleM.off = sPos;
			poleM.v1 = opM.v1;
			poleM.v2 = opM.v2;
			poleM.v3 = opM.v3;
			pole->SetMg(poleM);
			
			// Set the Solver object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetReal(NULLOBJECT_RADIUS,bLen/20);
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDAddUndo(doc,CD_UNDO_CHANGE,chA);
			CDAddUndo(doc,CD_UNDO_CHANGE,chB);
			CDAddUndo(doc,CD_UNDO_CHANGE,chC);

			// Add the Thumb Tag
			BaseTag *thumbTag = NULL;
			thumbTag = BaseTag::Alloc(ID_CDTHUMBPLUGIN);
			op->InsertTag(thumbTag,NULL);
			thumbTag->Message(MSG_MENUPREPARE);
			thumbTag->SetName(opName);

			// Add the Solver and Tip Goal objects to the Thumb tag links
			BaseContainer *tData = thumbTag->GetDataInstance();
			tData->SetLong(10011,ikAxis);
			tData->SetReal(10016,gripAngle);
			tData->SetReal(10017,twistAngle);
			tData->SetReal(10018,spreadAngle);
			tData->SetReal(10019,bendAngle);
			tData->SetReal(10020,curlAngle);
			tData->SetLink(10014,pole);

			doc->SetActiveObject(pole);
			CDAddUndo(doc,CD_UNDO_NEW,thumbTag);
			CDAddUndo(doc,CD_UNDO_NEW,pole);

			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupRotator(LONG sLine, LONG nbone)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	BaseObject *op = doc->GetActiveObject(); if(!op) return false;

	LONG i,lnTarg;
	
	// Get the objects parameters
	Matrix opM = op->GetMg();
	Vector opPos = opM.off;
	String opName = op->GetName();

	// Add the Rotator null object
	BaseObject *rotatorObject = BaseObject::Alloc(Onull); if(!rotatorObject) return false;
	
	doc->StartUndo();
	
	rotatorObject->SetName(opName + GeLoadString(N_ROTATOR));
	doc->InsertObject(rotatorObject, op->GetUp(), NULL,true);
	rotatorObject->SetMg(opM);
	
	// Set the Rotator object's parameters
	BaseContainer *rData = rotatorObject->GetDataInstance();
	rData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
	ObjectColorProperties rProp;
	rProp.color = Vector(0,1,1);
	rProp.usecolor = 2;
	rotatorObject->SetColorProperties(&rProp);

	CDAddUndo(doc,CD_UNDO_CHANGE,op);
	BaseObject *ch = op->GetDown();
	for(i=0; i<nbone; i++)
	{
		if(ch)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			ch = ch->GetDown();
		}
	}

	// Add the Rotator Tag
	BaseTag *rotatorTag = NULL;
	rotatorTag = BaseTag::Alloc(ID_CDROTATORPLUGIN);
	op->InsertTag(rotatorTag,NULL);
	rotatorTag->Message(MSG_MENUPREPARE);
	rotatorTag->SetName(opName + GeLoadString(N_ROTATOR));

	// Add the Rotator objects to the Rotator tag link
	BaseContainer *tData = rotatorTag->GetDataInstance();
	tData->SetLink(10030,rotatorObject);
	tData->SetLong(10018,nbone);
	switch (sLine)
	{
		case 0:
			lnTarg = 10016;
			break;
		case 1:
			lnTarg = 10017;
			break;
	}
	tData->SetLong(10015,lnTarg);

	doc->SetActiveObject(rotatorObject);
	CDAddUndo(doc,CD_UNDO_NEW,rotatorTag);
	CDAddUndo(doc,CD_UNDO_NEW,rotatorObject);
		
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;	
}

void IKSetupDialog::FitSplineToJoints(BaseDocument *doc, BaseObject *jnt, SplineObject *spl, LONG jCnt, LONG trgCnt)
{
	if(jnt && spl)
	{
		GePrint("Joint count = "+CDLongToString(jCnt));
		CDArray<Vector> csPadr;
		csPadr.Alloc(jCnt);
		
		BaseObject *ch = jnt;
		
		LONG i, pCnt = trgCnt+1;
		
		for(i=0; i<jCnt; i++)
		{
			csPadr[i] = ch->GetMg().off;
			ch = ch->GetDown();
		}
		
		SplineObject *cntrlSpline = FitCurve(csPadr.Array(), jCnt, 0, NULL);
		BaseContainer *splineData = cntrlSpline->GetDataInstance();
		splineData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_CUBIC);
		cntrlSpline->SetDefaultCoeff();

		doc->InsertObject(cntrlSpline, NULL, NULL, true);
		CDAddUndo(doc,CD_UNDO_NEW,cntrlSpline);
		
		Real csLen = CDGetSplineLength(cntrlSpline);
		CDArray<Vector> fPadr;
		fPadr.Alloc(pCnt);
		
		for(i=0; i<pCnt; i++)
		{
			Real percent = Real(i)/Real(trgCnt);
			fPadr[i] = CDGetSplinePoint(cntrlSpline,CDUniformToNatural(cntrlSpline,(csLen * percent)/csLen));
		}
		
		SplineObject *fitSpline = FitCurve(fPadr.Array(), pCnt, 0, NULL);
		splineData = fitSpline->GetDataInstance();
		splineData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
		fitSpline->SetDefaultCoeff();
		
		doc->InsertObject(fitSpline, NULL, NULL, true);
		CDAddUndo(doc,CD_UNDO_NEW,fitSpline);
		
		Real fsLen = CDGetSplineLength(fitSpline);
		Vector *fsPadr = GetPointArray(fitSpline);
		
		LONG tries = 0;
		while(fsLen < csLen && tries < 200)
		{
			for(i=1; i<trgCnt; i++)
			{
				Real n = GetNearestSplinePoint(fitSpline,fPadr[i],0,true);
				Vector sPt = CDGetSplinePoint(fitSpline,CDUniformToNatural(fitSpline,n));
				Vector dir = fPadr[i] - sPt;
				fPadr[i] = fPadr[i] + dir * 0.01;
			}
			for(i=1; i<trgCnt; i++)
			{
				fsPadr[i] = fPadr[i];
			}
			fsLen = CDGetSplineLength(fitSpline);
			tries++;
		}
		GePrint("tries = "+CDLongToString(tries));
		fitSpline->SetDefaultCoeff();
		
		Vector *sPadr = GetPointArray(spl);
		for(i=0; i<pCnt; i++)
		{
			sPadr[i] = fsPadr[i];
		}
		spl->SetDefaultCoeff();

		fPadr.Free();
		csPadr.Free();
		
		SplineObject::Free(cntrlSpline);
		SplineObject::Free(fitSpline);
	}
}

Bool IKSetupDialog::SetupSplineIK(LONG ikPole, LONG starg, LONG nbone)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	BaseObject *op = doc->GetActiveObject(); if(!op) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return true;
	
	LONG i, ikAxis;
	
	// Get the objects parameters
	Matrix opM = op->GetMg();
	Vector opPos = opM.off;
	String opName = op->GetName();

	Vector sPos;
	switch (ikPole)
	{
		case 0:
			sPos = opPos + opM.v1 * 50;
			ikAxis = 10014;
			break;
		case 1:
			sPos = opPos + opM.v2 * 50;
			ikAxis = 10013;
			break;
		case 2:
			sPos = opPos + ( -(opM.v1)) * 50;
			ikAxis = 10028;
			break;
		case 3:
			sPos = opPos + ( -(opM.v2)) * 50;
			ikAxis = 10029;
			break;
	}

	LONG cnt = 1;
	BaseObject *theBone = op;
	ch = op;
	while(cnt < nbone)
	{
		ch = ch->GetDown();
		if(!ch) break;
		theBone = ch;
		cnt++;
	}

	Real bnLen; Vector tipPos; Matrix tipM;
	BaseObject* tipOp = NULL;
	Matrix boneMatrix = theBone->GetMg();
	String boneName = theBone->GetName();
	BaseObject *tipEff = theBone->GetDown();
	if(!tipEff)
	{
		if(theBone->GetType() != ID_CDJOINTOBJECT)
		{
			// If there is no grandchild, add a Tip Effector null object
			bnLen = theBone->GetRad().z*2;
			tipPos = boneMatrix.off + boneMatrix.v3 * bnLen;
			tipM = Matrix(tipPos,boneMatrix.v1,boneMatrix.v2,boneMatrix.v3);
			tipOp = BaseObject::Alloc(Onull); if(!tipOp) return false;
			tipOp->SetName(boneName + GeLoadString(N_TIP_EFFECTOR));
			doc->InsertObject(tipOp,theBone, NULL,true);
			
			// Set the tip effector's object's parameters
			BaseContainer *tipData = tipOp->GetDataInstance();
			tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			tipOp->SetColorProperties(&tProp);
			tipOp->SetMg(tipM);
			tipEff = theBone->GetDown();if(!tipEff) return true;
		}
		else
		{
			tipEff = theBone;
			theBone = theBone->GetUp();
			cnt--;
		}
	}

	// Get the Tip position
	boneMatrix = theBone->GetMg();
	Vector goalAPosition = tipEff->GetMg().off;
	Vector splineVector = goalAPosition - opPos;
	
	// Add the Solver null object
	BaseObject* pole = NULL;
	pole = BaseObject::Alloc(Onull); if(!pole) return false;
	pole->SetName(opName + GeLoadString(N_SPLINE) + GeLoadString(N_SOLVER));
	doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
	Matrix poleM = pole->GetMg();
	poleM.off = sPos;
	poleM.v1 = Vector(1,0,0);
	poleM.v2 = Vector(0,1,0);
	poleM.v3 = Vector(0,0,1);
	pole->SetMg(poleM);
	
	// Set the Solver object's parameters
	BaseContainer *sData = pole->GetDataInstance();
	sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
	ObjectColorProperties sProp;
	sProp.color = Vector(0,1,1);
	sProp.usecolor = 2;
	pole->SetColorProperties(&sProp);
	 
	// Add the Spline object
	BaseObject* splineObject = NULL;
	splineObject = BaseObject::Alloc(Ospline); if(!splineObject) return false;
	splineObject->SetName(opName + GeLoadString(N_SPLINE));
	doc->InsertObject(splineObject, NULL, NULL,true);

	// Set the Spline object's parameters
	BaseContainer *splineData = splineObject->GetDataInstance();
	splineData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
	splineData->SetLong(SPLINEOBJECT_INTERPOLATION,SPLINEOBJECT_INTERPOLATION_NATURAL);
	splineData->SetLong(SPLINEOBJECT_SUB,16);
	
	//resize the spline to the number of valid target objects
	SplineObject *spl = splineObject->GetRealSpline();
	spl->ResizeObject(starg+2,spl->GetSegmentCount()); 
    
	// fit spline to joints
	FitSplineToJoints(doc, op, spl, cnt+1, starg+1);
	Matrix splM = spl->GetMg();
	Vector *sPadr = GetPointArray(spl);

	// Add the Tip Goal null object
	BaseObject* goalA = NULL;
	goalA = BaseObject::Alloc(Onull); if(!goalA) return false;
	goalA->SetName(opName + GeLoadString(N_SPLINE) + GeLoadString(N_TIP_GOAL));
	doc->InsertObject(goalA, NULL, NULL,true);
	Matrix goalAMatrix = goalA->GetMg();
	goalAMatrix.off = goalAPosition;
	goalAMatrix.v1 = Vector(1,0,0);
	goalAMatrix.v2 = Vector(0,1,0);
	goalAMatrix.v3 = Vector(0,0,1);
	goalA->SetMg(goalAMatrix);
	
	// Set the Tip Goal object's parameters
	BaseContainer *goalAData = goalA->GetDataInstance();
	goalAData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
	ObjectColorProperties goalAProp;
	goalAProp.color = Vector(0,1,1);
	goalAProp.usecolor = 2;
	goalA->SetColorProperties(&goalAProp);
	
	doc->StartUndo();
	
	ch = op->GetDown();
	for(i=0; i<nbone; i++)
	{
		if(ch)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			ch = ch->GetDown();
		}
	}
	
	// Add the SplineIK Tag
	BaseTag *splineIKTag = NULL;
	splineIKTag = BaseTag::Alloc(ID_CDSPLINEIKPLUGIN);
	op->InsertTag(splineIKTag,NULL);
	splineIKTag->Message(MSG_MENUPREPARE);
	splineIKTag->SetName(opName + GeLoadString(N_SPLINE_IK));
	
	// Add the objects to the SplineIK tag links and set the parameters
	BaseContainer *tData = splineIKTag->GetDataInstance();
	tData->SetBool(10030,false);
	tData->SetLong(10012,ikAxis);
	tData->SetLong(10010,cnt);
	tData->SetLink(10021,pole);
	tData->SetLink(10020,splineObject);
	tData->SetLink(40100,goalA);
	
	doc->SetActiveObject(goalA);
	CDAddUndo(doc,CD_UNDO_NEW,splineIKTag);
	CDAddUndo(doc,CD_UNDO_NEW,pole);
	CDAddUndo(doc,CD_UNDO_NEW,splineObject);
	CDAddUndo(doc,CD_UNDO_NEW,goalA);
	
	LONG lnkCnt = 1;

	// Add the Middle Goal null objects
	BaseObject* goalB = NULL; BaseObject* goalC = NULL; BaseObject* goalD = NULL;
	BaseContainer *goalBData = NULL; BaseContainer *goalCData = NULL; BaseContainer *goalDData = NULL;
	Matrix goalBMatrix, goalCMatrix, goalDMatrix;
	Vector goalBPosition, goalCPosition, goalDPosition;
	
	switch(starg)
	{
		case 1:
			goalBPosition = splM * sPadr[1];
			break;
		case 2:
			goalBPosition = splM * sPadr[2];
			goalCPosition = splM * sPadr[1];
			break;
		case 3:
			goalBPosition = splM * sPadr[3];
			goalCPosition = splM * sPadr[2];
			goalDPosition = splM * sPadr[1];
			break;
	}
	if(starg > 0)
	{
		// Add the Mid Goal null object
		goalB = BaseObject::Alloc(Onull); if(!goalB) return false;
		goalB->SetName(opName + GeLoadString(N_SPLINE) + GeLoadString(N_MID_GOAL));
		doc->InsertObject(goalB, NULL, NULL,true);
		goalBMatrix = goalB->GetMg();
		goalBMatrix.off = goalBPosition;
		goalBMatrix.v1 = Vector(1,0,0);
		goalBMatrix.v2 = Vector(0,1,0);
		goalBMatrix.v3 = Vector(0,0,1);
		goalB->SetMg(goalBMatrix);
		
		// Set the Mid Goal object's parameters
		goalBData = goalB->GetDataInstance();
		goalBData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
		goalB->SetColorProperties(&goalAProp);
		tData->SetLink(40101,goalB);
		lnkCnt++;
		CDAddUndo(doc,CD_UNDO_NEW,goalB);
	}
	if(starg > 1)
	{
		// Add the Mid.1 Goal null object
		goalC = BaseObject::Alloc(Onull); if(!goalC) return false;
		goalC->SetName(opName + GeLoadString(N_SPLINE) + GeLoadString(N_MID_GOAL) + ".1");
		doc->InsertObject(goalC, NULL, NULL,true);
		goalCMatrix = goalC->GetMg();
		goalCMatrix.off = goalCPosition;
		goalCMatrix.v1 = Vector(1,0,0);
		goalCMatrix.v2 = Vector(0,1,0);
		goalCMatrix.v3 = Vector(0,0,1);
		goalC->SetMg(goalCMatrix);
		
		// Set the Mid.1 Goal object's parameters
		goalCData = goalC->GetDataInstance();
		goalCData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
		goalC->SetColorProperties(&goalAProp);
		tData->SetLink(40102,goalC);
		lnkCnt++;
		CDAddUndo(doc,CD_UNDO_NEW,goalC);
	}
	if(starg > 2)
	{
		// Add the Mid.2 Goal null object
		goalD = BaseObject::Alloc(Onull); if(!goalD) return false;
		goalD->SetName(opName + GeLoadString(N_SPLINE) + GeLoadString(N_MID_GOAL) + ".2");
		doc->InsertObject(goalD, NULL, NULL,true);
		goalDMatrix = goalD->GetMg();
		goalDMatrix.off = goalDPosition;
		goalDMatrix.v1 = Vector(1,0,0);
		goalDMatrix.v2 = Vector(0,1,0);
		goalDMatrix.v3 = Vector(0,0,1);
		goalD->SetMg(goalDMatrix);
		
		// Set the Mid.2 Goal object's parameters
		goalDData = goalD->GetDataInstance();
		goalDData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
		goalD->SetColorProperties(&goalAProp);
		tData->SetLink(40103,goalD);
		lnkCnt++;
		CDAddUndo(doc,CD_UNDO_NEW,goalD);
	}
	tData->SetLong(10040,lnkCnt);
		
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupSpinal(LONG spAxis, LONG nbone)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	BaseObject *op = doc->GetActiveObject(); if(!op) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return true;
	
	LONG i, sAxis;
	
	// Get the objects parameters
	Matrix opM = op->GetMg();
	Vector opPos = opM.off;
		
	switch(spAxis)
	{
		case 0:
			sAxis = 1003;
			break;
		case 1:
			sAxis = 1004;
			break;
		case 2:
			sAxis = 1005;
			break;
		case 3:
			sAxis = 1006;
			break;
		case 4:
			sAxis = 1007;
			break;
		case 5:
			sAxis = 1008;
			break;
	}

	LONG cnt = 2;
	BaseObject *theBone = ch; 
	while (cnt < nbone)
	{
		ch = ch->GetDown();
		if(!ch) break;
		theBone = ch;
		cnt++;
	}

	Real bnLen;
	Vector tipPos;
	Matrix baseM, tipM;
	BaseObject* tipOp = NULL;
	Matrix boneMatrix = theBone->GetMg();
	String boneName = theBone->GetName();
	BaseObject *tipEff = theBone->GetDown();
	if(!tipEff)
	{
		if(theBone->GetType() != ID_CDJOINTOBJECT)
		{
			// If there is no grandchild, add a Tip Effector null object
			bnLen = theBone->GetRad().z*2;
			tipPos = boneMatrix.off + boneMatrix.v3 * bnLen;
			tipM = Matrix(tipPos,boneMatrix.v1,boneMatrix.v2,boneMatrix.v3);
			tipOp = BaseObject::Alloc(Onull); if(!tipOp) return false;
			tipOp->SetName(boneName + GeLoadString(N_TIP_EFFECTOR));
			doc->InsertObject(tipOp,theBone, NULL,true);
			
			// Set the tip effector's object's parameters
			BaseContainer *tipData = tipOp->GetDataInstance();
			tipData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			tipData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			tipOp->SetColorProperties(&tProp);
			tipOp->SetMg(tipM);
			tipEff = theBone->GetDown();if(!tipEff) return true;
		}
		else
		{
			tipEff = theBone;
			theBone = theBone->GetUp();
			cnt--;
		}
	}
	GePrint("cnt = "+CDLongToString(cnt));
	
	// Get the Tip position
	boneMatrix = theBone->GetMg();
	tipPos = tipEff->GetMg().off;
	
	baseM = Matrix();
	baseM.off = opPos;
	tipM = Matrix();
	tipM.off = tipPos;

	// Add Spline object
	BaseObject* splineObject = NULL;
	splineObject = BaseObject::Alloc(Ospline); if(!splineObject) return false;
	doc->InsertObject(splineObject, NULL, NULL,true);
	
	// Set the Spline object's parameters
	BaseContainer *splineData = splineObject->GetDataInstance();
	splineData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
	splineData->SetLong(SPLINEOBJECT_INTERPOLATION,SPLINEOBJECT_INTERPOLATION_NATURAL);
	splineData->SetLong(SPLINEOBJECT_SUB,16);
	
	//resize the spline to the number of valid target objects
	SplineObject *spl = splineObject->GetRealSpline();
	spl->ResizeObject(4,spl->GetSegmentCount()); 
    
	// fit spline to joints
	FitSplineToJoints(doc, op, spl, cnt+1, 3);
	Matrix splM = spl->GetMg();
	Vector *sPadr = GetPointArray(spl);
	
	Vector bDir, tDir;
	Real bLen, tLen;
	switch(spAxis)
	{
		case 0:
		{
			bDir = sPadr[1] - baseM.off;
			bLen = Len(bDir);
			
			baseM.v1 = VNorm(bDir);
			baseM.v2 = VNorm(VCross(baseM.v3, baseM.v1));
			baseM.v3 = VNorm(VCross(baseM.v1, baseM.v2));
			
			tDir = tipM.off - sPadr[2];
			tLen = Len(tDir);
			
			tipM.v1 = VNorm(tDir);
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
			tipM.v3 = VNorm(VCross(tipM.v1, tipM.v2));
			break;
		}
		case 1:
		{
			bDir = sPadr[1] - baseM.off;
			bLen = Len(bDir);
			
			baseM.v2 = VNorm(bDir);
			baseM.v3 = VNorm(VCross(baseM.v1, baseM.v2));
			baseM.v1 = VNorm(VCross(baseM.v2, baseM.v3));
			
			tDir = tipM.off - sPadr[2];
			tLen = Len(tDir);
			
			tipM.v2 = VNorm(tDir);
			tipM.v3 = VNorm(VCross(tipM.v1, tipM.v2));
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
			break;
		}
		case 2:
		{
			bDir = sPadr[1] - baseM.off;
			bLen = Len(bDir);
			
			baseM.v3 = VNorm(bDir);
			baseM.v2 = VNorm(VCross(baseM.v3, baseM.v1));
			baseM.v1 = VNorm(VCross(baseM.v2, baseM.v3));
			
			tDir = tipM.off - sPadr[2];
			tLen = Len(tDir);
			
			tipM.v3 = VNorm(tDir);
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
			break;
		}
		case 3:
		{
			bDir = baseM.off - sPadr[1];
			bLen = Len(bDir);
			
			baseM.v1 = VNorm(bDir);
			baseM.v2 = VNorm(VCross(baseM.v3, baseM.v1));
			baseM.v3 = VNorm(VCross(baseM.v1, baseM.v2));
			
			tDir = sPadr[2] - tipM.off;
			tLen = Len(tDir);
			
			tipM.v1 = VNorm(tDir);
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
			tipM.v3 = VNorm(VCross(tipM.v1, tipM.v2));
			break;
		}
		case 4:
		{
			bDir = baseM.off - sPadr[1];
			bLen = Len(bDir);
			
			baseM.v2 = VNorm(bDir);
			baseM.v3 = VNorm(VCross(baseM.v1, baseM.v2));
			baseM.v1 = VNorm(VCross(baseM.v2, baseM.v3));
			
			tDir = sPadr[2] - tipM.off;
			tLen = Len(tDir);
			
			tipM.v2 = VNorm(tDir);
			tipM.v3 = VNorm(VCross(tipM.v1, tipM.v2));
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
			break;
		}
		case 5:
		{
			bDir = baseM.off - sPadr[1];
			bLen = Len(bDir);
			
			baseM.v3 = VNorm(bDir);
			baseM.v2 = VNorm(VCross(baseM.v3, baseM.v1));
			baseM.v1 = VNorm(VCross(baseM.v2, baseM.v3));
			
			tDir = sPadr[2] - tipM.off;
			tLen = Len(tDir);
			
			tipM.v3 = VNorm(tDir);
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
			break;
		}
	}
	BaseObject::Free(splineObject);
	
	// Add the base controller object
	BaseObject* hipNull = NULL;
	hipNull = BaseObject::Alloc(Onull); if(!hipNull) return false;
	hipNull->SetName(GeLoadString(N_BASE_C));
	doc->InsertObject(hipNull, NULL, NULL,true);
	hipNull->SetMg(baseM);
	
	// Set the base controller object's parameters
	BaseContainer *hData = hipNull->GetDataInstance();
	hData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_CUBE);
	hData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
	ObjectColorProperties hProp;
	hProp.color = Vector(0,1,1);
	hProp.usecolor = 2;
	hipNull->SetColorProperties(&hProp);

	// Add the tip controller object
	BaseObject* chestNull = NULL;
	chestNull = BaseObject::Alloc(Onull); if(!chestNull) return false;
	chestNull->SetName(GeLoadString(N_TIP_C));
	doc->InsertObject(chestNull, NULL, NULL,true);
	chestNull->SetMg(tipM);
	
	// Set the tip controller object's parameters
	BaseContainer *chData = chestNull->GetDataInstance();
	chData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_CUBE);
	chData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
	ObjectColorProperties chProp;
	chProp.color = Vector(0,1,1);
	chProp.usecolor = 2;
	chestNull->SetColorProperties(&chProp);
	
	doc->StartUndo();
	
	ch = op->GetDown();
	for(i=0; i<nbone; i++)
	{
		if(ch)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			ch = ch->GetDown();
		}
	}

	// Add the Spinal Tag
	BaseTag *spinalTag = NULL;
	spinalTag = BaseTag::Alloc(ID_CDSPINALPLUGIN);
	op->InsertTag(spinalTag,NULL);
	spinalTag->Message(MSG_MENUPREPARE);
	spinalTag->SetName(op->GetName() + GeLoadString(N_SPINAL));
	
	// Add the objects to the Spinal tag links and set the parameters
	BaseContainer *tData = spinalTag->GetDataInstance();
	tData->SetBool(1001,false);
	tData->SetLong(1002,sAxis);
	tData->SetLong(1010,cnt);
	tData->SetLink(1015,hipNull);
	tData->SetReal(1016,bLen);
	tData->SetLink(1017,chestNull);
	tData->SetReal(1018,tLen);

	CDAddUndo(doc,CD_UNDO_NEW,hipNull);
	CDAddUndo(doc,CD_UNDO_NEW,chestNull);
	CDAddUndo(doc,CD_UNDO_NEW,spinalTag);
	
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupLinkageIK(int ikPole)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); if(!op) continue;
			BaseObject *ch = op->GetDown(); if(!ch) continue;
			BaseObject *gch = ch->GetDown(); if(!gch) continue;
			
			String opName = op->GetName();
			String chName = ch->GetName();
			
			Matrix opM = op->GetMg();
			Matrix gchM = gch->GetMg();
			
			Vector pDir;
			Vector goalV = gchM.off - opM.off;
			Real dot = VDot(VNorm(opM.v3), VNorm(goalV));
			if(dot > 0.996)
			{
				switch(ikPole)
				{
					case 0:
						pDir = VNorm(opM.v1);
						break;
					case 1:
						pDir = VNorm(opM.v2);
						break;
					case 2:
						pDir = VNorm(-opM.v1);
						break;
					case 3:
						pDir = VNorm(-opM.v2);
						break;
					default:
						pDir = VNorm(opM.v2);
						break;
				}
			}
			else
			{
				switch(ikPole)
				{
					case 0:
					case 1:
						pDir = VNorm(VCross(opM.v3, goalV));
						break;
					case 2:
					case 3:
						pDir = VNorm(VCross(goalV, opM.v3));
						break;
					default:
						pDir = VNorm(VCross(opM.v3, goalV));
						break;
				}
			}
			
			// Add the Pole null object
			BaseObject* pole = NULL;
			pole = BaseObject::Alloc(Onull); if(!pole) continue;
			pole->SetName(opName + ".Pole");
			doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
			Matrix poleM = pole->GetMg();
			poleM.off = opM.off + pDir * 50;
			poleM.v1 = Vector(1,0,0);
			poleM.v2 = Vector(0,1,0);
			poleM.v3 = Vector(0,0,1);
			pole->SetMg(poleM);
			
			// Set the Pole object's parameters
			BaseContainer *sData = pole->GetDataInstance();
			sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
			ObjectColorProperties sProp;
			sProp.color = Vector(0,1,1);
			sProp.usecolor = 2;
			pole->SetColorProperties(&sProp);
			
			// Add the Tip Goal null object
			BaseObject* goal = NULL;
			goal = BaseObject::Alloc(Onull); if(!goal) continue;
			goal->SetName(chName + ".Goal");
			doc->InsertObject(goal, NULL, NULL,true);
			Matrix goalM = goal->GetMg();
			goalM.off = gch->GetMg().off;
			goal->SetMg(goalM);
			
			// Set the Tip Goal object's parameters
			BaseContainer *gData = goal->GetDataInstance();
			gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
			gData->SetReal(NULLOBJECT_RADIUS, 20.0);
			gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
			ObjectColorProperties tProp;
			tProp.color = Vector(0,1,1);
			tProp.usecolor = 2;
			goal->SetColorProperties(&tProp);
			
			LONG ikAxis;
			switch(ikPole)
			{
				case 0:
					ikAxis = 10007;
					break;
				case 1:
					ikAxis = 10008;
					break;
				case 2:
					ikAxis = 10019;
					break;
				case 3:
					ikAxis = 10020;
					break;
				default:
					ikAxis = 10008;
					break;
			}
			
			BaseTag *lnkIKTag = NULL;
			lnkIKTag = BaseTag::Alloc(ID_CDLINKAGEIKPLUGIN); if(!lnkIKTag) continue;
			op->InsertTag(lnkIKTag,NULL);
			lnkIKTag->Message(MSG_MENUPREPARE);
			lnkIKTag->SetName(opName + GeLoadString(N_LINK_IK));
			
			BaseContainer *tData = lnkIKTag->GetDataInstance(); if(!tData) continue;
			tData->SetBool(10001, true);
			tData->SetLong(10006, ikAxis);
			tData->SetLink(10009, pole);
			tData->SetLink(10010, goal);

			doc->SetActiveObject(op);
			CDAddUndo(doc, CD_UNDO_NEW, lnkIKTag);
			CDAddUndo(doc, CD_UNDO_NEW, pole);
			CDAddUndo(doc, CD_UNDO_NEW, goal);
			
			CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
		}
		doc->EndUndo();
	}
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;	
}

Bool IKSetupDialog::SetupPistonIK(int ikPole, int nbone)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	BaseObject *op = doc->GetActiveObject(); if(!op) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return true;

	doc->StartUndo();

	Matrix opM = op->GetMg();
	String opName = op->GetName();
	
	LONG cnt = 0;
	BaseObject *jnt = op, *tip = ch;
	while(cnt < nbone)
	{
		jnt = jnt->GetDown();
		if(!jnt) break;
		tip = jnt;
		cnt++;
	}
	
	Matrix tipM = tip->GetMg();
	
	Vector pDir;
	switch(ikPole)
	{
		case 0:
			pDir = VNorm(opM.v1);
			break;
		case 1:
			pDir = VNorm(opM.v2);
			break;
		case 2:
			pDir = VNorm(-opM.v1);
			break;
		case 3:
			pDir = VNorm(-opM.v2);
			break;
		default:
			pDir = VNorm(opM.v2);
			break;
	}
	
	// Add the Pole null object
	BaseObject* pole = NULL;
	pole = BaseObject::Alloc(Onull); if(!pole) return false;
	pole->SetName(opName + ".Pole");
	doc->InsertObject(pole, op->GetUp(), op->GetPred(),true);
	Matrix poleM = pole->GetMg();
	poleM.off = opM.off + pDir * 50;
	poleM.v1 = Vector(1,0,0);
	poleM.v2 = Vector(0,1,0);
	poleM.v3 = Vector(0,0,1);
	pole->SetMg(poleM);
	
	// Set the Pole object's parameters
	BaseContainer *sData = pole->GetDataInstance();
	sData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_TRIANGLE);
	ObjectColorProperties sProp;
	sProp.color = Vector(0,1,1);
	sProp.usecolor = 2;
	pole->SetColorProperties(&sProp);
	
	// Add the Tip Goal null object
	BaseObject* goal = NULL;
	goal = BaseObject::Alloc(Onull); if(!goal) return false;
	goal->SetName(tip->GetName() + ".Goal");
	doc->InsertObject(goal, NULL, NULL,true);
	Matrix goalM = goal->GetMg();
	goalM.off = tipM.off;
	goal->SetMg(goalM);
	
	// Set the Tip Goal object's parameters
	BaseContainer *gData = goal->GetDataInstance();
	gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
	gData->SetReal(NULLOBJECT_RADIUS,20.0);
	gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
	ObjectColorProperties tProp;
	tProp.color = Vector(0,1,1);
	tProp.usecolor = 2;
	goal->SetColorProperties(&tProp);
	
	LONG ikAxis;
	switch(ikPole)
	{
		case 0:
			ikAxis = 10007;
			break;
		case 1:
			ikAxis = 10008;
			break;
		case 2:
			ikAxis = 10019;
			break;
		case 3:
			ikAxis = 10020;
			break;
		default:
			ikAxis = 10008;
			break;
	}
	
	BaseTag *pstIKTag = NULL;
	pstIKTag = BaseTag::Alloc(ID_CDPISTONIKPLUGIN); if(!pstIKTag) return false;
	op->InsertTag(pstIKTag,NULL);
	pstIKTag->Message(MSG_MENUPREPARE);
	pstIKTag->SetName(opName + GeLoadString(N_LINK_IK));
	
	BaseContainer *tData = pstIKTag->GetDataInstance(); if(!tData) return false;
	tData->SetLong(1600,cnt);
	tData->SetBool(10001,true);
	tData->SetLong(10006,ikAxis);
	tData->SetLink(10009,pole);
	tData->SetLink(10010,goal);
	
	doc->SetActiveObject(goal);
	CDAddUndo(doc,CD_UNDO_NEW,pstIKTag);
	CDAddUndo(doc,CD_UNDO_NEW,pole);
	CDAddUndo(doc,CD_UNDO_NEW,goal);
	
	LONG i;
	jnt = ch;
	Real tLen = Len(tipM.off - opM.off);
	if(tLen > 0.0)
	{
		for(i=0; i<cnt-1; i++)
		{
			Matrix jM = jnt->GetMg();
			Real jLen = Len(jM.off - opM.off);
			Real mix = jLen/tLen;
			tData->SetReal(30000+i, mix);
			jnt = jnt->GetDown();
		}
	}
	
	doc->EndUndo();
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

Bool IKSetupDialog::Command(LONG id,const BaseContainer &msg)
{
	LONG ikPole, spAxis, sLine, etype, starg; LONG nbone;
	
	GetLong(IDC_EXPRESSION_TYPE,etype);
	GetLong(IDC_SPLINE_TARGETS,starg);
	GetLong(GADGET_NUMBER_BONES,nbone);
	GetLong(IDC_SOLVER_AXIS,ikPole);
	GetLong(IDC_SOLVER_LINE,sLine);
	GetLong(IDC_SPINAL_AXIS,spAxis);

	switch (id)
	{
		case GADGET_SETUP:
			StopAllThreads(); // so the document can be safely modified
			switch (etype)
			{
				case 0:
					SetupLimbIK(ikPole, sLine);
					break;
				case 1:
					SetupQuadLegIK(ikPole, sLine);
					break;
				case 2:
					SetupMechIK(ikPole);
					break;
				case 3:
					SetupFootIK(ikPole);
					break;
				case 4:
					SetupFinger(ikPole);
					break;
				case 5:
					SetupThumb(ikPole);
					break;
				case 6:
					SetupRotator(sLine, nbone);
					break;
				case 7:
					SetupSplineIK(ikPole, starg, nbone);
					break;
				case 8:
					SetupSpinal(spAxis, nbone);
					break;
				case 9:
					SetupLinkageIK(ikPole);
					break;
				case 10:
					SetupPistonIK(ikPole, nbone);
					break;
			}
			break;
	}
	
	DoEnable();
	
	return true;
}

void IKSetupDialog::DoEnable(void)
{
	LONG etype;
	
	GetLong(IDC_EXPRESSION_TYPE,etype);
	switch (etype)
	{
		case 0:
			SetLong(GADGET_NUMBER_BONES,2,1,50,1,0);
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,true);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 1:
			SetLong(GADGET_NUMBER_BONES,2,1,50,1,0);
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,true);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 2:
			SetLong(GADGET_NUMBER_BONES,2,1,50,1,0);
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 3:
			SetLong(GADGET_NUMBER_BONES,2,1,50,1,0);
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 4:
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 5:
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 6:
			Enable(GADGET_NUMBER_BONES,true);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,false);
			Enable(IDC_SOLVER_LINE,true);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 7:
			Enable(GADGET_NUMBER_BONES,true);
			Enable(IDC_SPLINE_TARGETS,true);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 8:
			Enable(GADGET_NUMBER_BONES,true);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,false);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,true);
			break;
		case 9:
			SetLong(GADGET_NUMBER_BONES,2,1,50,1,0);
			Enable(GADGET_NUMBER_BONES,false);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
		case 10:
			Enable(GADGET_NUMBER_BONES,true);
			Enable(IDC_SPLINE_TARGETS,false);
			Enable(IDC_SOLVER_AXIS,true);
			Enable(IDC_SOLVER_LINE,false);
			Enable(IDC_SPINAL_AXIS,false);
			break;
			
			
	}
}

class CDIKSetupChain : public CommandData
{
private:
	IKSetupDialog dlg;
public:

	virtual Bool Execute(BaseDocument *doc)
	{
	#if API_VERSION < 12000
		 return dlg.Open(true,ID_CDIKSETUP,-1,-1);
	#else
		return dlg.Open(DLG_TYPE_ASYNC,ID_CDIKSETUP,-1,-1);
	#endif
	}

	virtual Bool RestoreLayout(void *secret)
	{
		return dlg.RestoreLayout(ID_CDIKSETUP,0,secret);
	}

};

class IKSetupDialogR : public GeDialog
{
	public:
		virtual Bool CreateLayout(void)
		{
			Bool res = GeDialog::CreateLayout();
			if(res)
			{
				SetTitle(GeLoadString(D_TITLE));
				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(10,5,10,5);
					
					AddStaticText(0,BFH_LEFT,0,0,"CD IK Tools v"+CDRealToString(GetCDPluginVersion(ID_CDIKTOOLS),-1,3),0);
				}
				GroupEnd();

			}
			return res;
		}
};

class CDIKSetupChainR : public CommandData
{
	private:
		IKSetupDialogR dlg;
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			 return true;
		}

		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDIKSETUP,0,secret);
		}

};

Bool RegisterCDIKSetupChain(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDIKSETUP); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDIKSETUP,name,PLUGINFLAG_HIDE,"CDiksetup.tif","CD IK Setup",CDDataAllocator(CDIKSetupChainR));
	else return CDRegisterCommandPlugin(ID_CDIKSETUP,name,0,"CDiksetup.tif","CD IK Setup",CDDataAllocator(CDIKSetupChain));
}

