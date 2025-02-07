//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDClusterTag.h"
#include "CDSkinTag.h"
#include "CDPaintSkin.h"

#include "tCDSkin.h"
#include "oCDJoint.h"

class CDAddJointsDialog : public GeModalDialog
{
private:
	CDJSOptionsUA ua;
	
public:	
	Bool	autoWt, autoBnd;

	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddJointsDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDJADD));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDJS_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDJS_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_WP_DYN_TITLE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddCheckbox (IDC_CDADDJ_AUTO_WT,BFH_LEFT,0,0,GeLoadString(IDC_CDADDJ_AUTO_WT));
				AddCheckbox (IDC_CDADDJ_AUTO_BIND,BFH_LEFT,0,0,GeLoadString(IDC_CDADDJ_AUTO_BIND));

			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddJointsDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDJOINTS);
	if(!bc)
	{
		SetBool(IDC_CDADDJ_AUTO_WT,false);
		SetBool(IDC_CDADDJ_AUTO_BIND,false);
	}
	else
	{
		SetBool(IDC_CDADDJ_AUTO_WT,bc->GetBool(IDC_CDADDJ_AUTO_WT));
		SetBool(IDC_CDADDJ_AUTO_BIND,bc->GetBool(IDC_CDADDJ_AUTO_BIND));
	}

	return true;
}

Bool CDAddJointsDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_CDADDJ_AUTO_WT,autoWt);
	GetBool(IDC_CDADDJ_AUTO_BIND,autoBnd);
	
	BaseContainer bc;
	bc.SetBool(IDC_CDADDJ_AUTO_WT,autoWt);
	bc.SetBool(IDC_CDADDJ_AUTO_BIND,autoBnd);
	SetWorldPluginData(ID_CDADDJOINTS,bc,false);
	
	return true;
}

class CDAddJoints : public CommandData
{
private:
	CDAddJointsDialog dlg;
	AtomArray *jointList;
	LONG jlstCnt;
	
	Bool ChildConnected(BaseObject *jnt);
	Bool AddChildJoints(BaseDocument* doc, BaseObject *mesh, BaseObject *op, BaseTag *tag, LONG type);
	Bool AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *op, LONG max, Bool env, Real falloff, Bool norm, LONG start);
	
public:		
	virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddJoints::ChildConnected(BaseObject *jnt)
{
	Bool connected = false;
	
	while(jnt)
	{
		BaseContainer *jData = jnt->GetDataInstance();
		if(jData)
		{
			if(jData->GetBool(JNT_CONNECTED)) connected = true;
		}
		jnt = jnt->GetNext();
	}

	return connected;
}

Bool CDAddJoints::AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *op, LONG max, Bool env, Real falloff, Bool norm, LONG start)
{
	BaseContainer *tData = tag->GetDataInstance(), *jointData=NULL;
	BaseObject *joint=NULL, *chJoint=NULL;
	
	LONG i, j, pCnt = tData->GetLong(SKN_S_POINT_COUNT), jCnt = tData->GetLong(SKN_J_COUNT);
	Matrix chMatrix, jMatrix, sectionM, opMatrix = op->GetMg();
	
	Real weight, dist, dProd, tWeightA, tWeightB, adj;
	Vector rSk, tSk, rRad, tRad, d1, d2, closest;
	
	CDArray<CDSkinVertex> autoWt;
	autoWt.Alloc(pCnt);
	for(i=0; i<pCnt; i++)
	{
		autoWt[i].jw.Alloc(jCnt);
		autoWt[i].jw.Fill(0.0);
		autoWt[i].jPlus.Alloc(jCnt);
		
	}
	
	Vector  *padr = GetPointArray(op), vertex;

	SplineObject *falloffspline = CDAllocateSplineObject(5,CD_SPLINETYPE_BSPLINE);
	if(!falloffspline)
	{
		autoWt.Free();
		return false;
	}
	Real f = falloff * 10;
	Vector *fsPts = GetPointArray(falloffspline);
	fsPts[0] = Vector(0.0,0.0,0.0);
	fsPts[1] = Vector(f,0.0,0.0);
	fsPts[2] = Vector(50.0,0.5,0.0);
	fsPts[3] = Vector(100.0-f,1.0,0.0);
	fsPts[4] = Vector(100.0,1.0,0.0);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	for(i=0; i<pCnt; i++)
	{
		StatusSetBar(LONG(Real(i)/Real(pCnt)*100.0));
		vertex = opMatrix * padr[i];
		
		// NOTE: see if it will work only cycling through the already assigned joints for the point
		CDArray<CDClusterWeight> tempWeight;
		tempWeight.Alloc(jCnt);
		
		for(j=0; j<jCnt; j++)
		{
			joint = tData->GetObjectLink(SKN_J_LINK+j,doc);
			jointData = joint->GetDataInstance();
			jMatrix = joint->GetMg();
			chJoint = joint->GetDown();
			Bool jenv = jointData->GetBool(3001);
			if(env && jenv)
			{
				if(!chJoint)
				{
					dist = Len(jMatrix.off - vertex);
				}
				else
				{
					chMatrix = chJoint->GetMg();
					rRad = jointData->GetVector(3002);
					tRad = jointData->GetVector(3004);
					rSk.x = jointData->GetReal(3006);
					rSk.y = jointData->GetReal(3007);
					tSk.x = jointData->GetReal(3008);
					tSk.y = jointData->GetReal(3009);
					jMatrix.off = jMatrix * Vector(rSk.x,rSk.y,rRad.z);
					chMatrix.off = chMatrix * Vector(tSk.x,tSk.y,tRad.z);
					jMatrix.v3 = VNorm(chMatrix.off - jMatrix.off);
					jMatrix.v2 = VNorm(VCross(jMatrix.v3, jMatrix.v1));
					jMatrix.v1 = VNorm(VCross(jMatrix.v2, jMatrix.v3));
					chMatrix.v1 = jMatrix.v1;
					chMatrix.v2 = jMatrix.v2;
					chMatrix.v3 = jMatrix.v3;
					jMatrix.v1 *= rRad.x;
					jMatrix.v2 *= rRad.y;
					chMatrix.v1 *= tRad.x;
					chMatrix.v2 *= tRad.y;
					
					d1 = vertex - jMatrix.off;
					d2 = chMatrix.off - jMatrix.off;
					dProd = VDot(VNorm(d1),VNorm(d2));
					if (dProd < 0) // perpendicular is off the beginning of the line
					{
						dist = Len(jMatrix.off - vertex);
					}
					else
					{
				    	adj = dProd * Len(d1);
				    	if(adj > Len(d2)) // perpendicular is off the end of the line
				    	{
				    		dist = Len(chMatrix.off - vertex);
				    	}
				    	else // perpendicular is on the line somewhere
				    	{
				    		Real theMix = adj / Len(d2);
				    		sectionM = jMatrix;
				    		closest = jMatrix.off + VNorm(d2) * adj;
				    		sectionM.off = closest;
				    		sectionM.v1 = CDBlend(jMatrix.v1,chMatrix.v1,theMix);
				    		sectionM.v2 = CDBlend(jMatrix.v2,chMatrix.v2,theMix);
				    		sectionM.v3 = jMatrix.v3;
				    		
				    		if(Len(vertex - closest) > Len((sectionM * VNorm(MInv(sectionM) * vertex)) - closest))
				    		{
				    			dist = Len(sectionM.off - vertex)*100;
				    		}
				    		else
				    		{
					    		dist = 1;
				    		}
				    	}
					}
				}
			}
			else
			{
				if(!chJoint)
				{
					dist = Len(jMatrix.off - vertex);
				}
				else
				{
					chMatrix = chJoint->GetMg();
					d1 = vertex - jMatrix.off;
					d2 = chMatrix.off - jMatrix.off;
					dProd = VDot(VNorm(d1),VNorm(d2));
				    if (dProd < 0) // perpendicular is off the beginning of the line
				    {
						closest = jMatrix.off;
				    }
				    else
				    {
				    	adj = dProd * Len(d1);
				    	if(adj > Len(d2)) // perpendicular is off the end of the line
				    	{
				    		closest = chMatrix.off;
				    	}
				    	else // perpendicular is on the line somewhere
				    	{
				    		closest = jMatrix.off + VNorm(d2) * adj;
				    	}
				    }
				    dist = Len(closest - vertex);
				}
			}
			
			weight = Real(1) / (dist * dist * dist * dist * dist * dist * dist * dist * dist * dist);
			tempWeight[j].ind = j;
			tempWeight[j].w = weight * (joint->GetRad().x + joint->GetRad().y)/2;
		}
		
		LONG size = sizeof(CDClusterWeight);
		qsort(tempWeight.Array(),jCnt, size,CompareWeights);
		if(max > jCnt) max = jCnt;
		tWeightA = 0.0;
		for(j=0; j<max; j++)
		{
			tWeightA += tempWeight[j].w;
		}
		tWeightB = 0.0;
		Real fWeight = 0;
		for(j=0; j<max; j++)
		{
			fWeight = 0;
			if(tWeightA > 0)
			{
				Vector softPt = falloffspline->GetSplinePoint(CDUniformToNatural(falloffspline,tempWeight[j].w / tWeightA));
				fWeight = softPt.y;
				if(fWeight < 0.01) fWeight = 0.0;
			}
			tempWeight[j].w = fWeight;
			tWeightB += tempWeight[j].w;
		}
		
		Real totalFinWt = 0.0;
		for(j=0; j<max; j++)
		{
			fWeight = 0;
			if(tWeightB > 0)
			{
				fWeight = TruncatePercentFractions(tempWeight[j].w / tWeightB);
				totalFinWt += fWeight;
				if(j == max-1)
				{
					Real remainder = 1 - totalFinWt;
					if(remainder > 0) fWeight += remainder;
				}
			}
			autoWt[i].jw[tempWeight[j].ind] = fWeight;
		}
		tempWeight.Free();
	}
	
	for(j=start; j<jCnt; j++)
	{
		tData->SetLong(SKN_J_LINK_ID,j);
		CDArray<Real> jtWt;
		if(jtWt.Alloc(pCnt))
		{
			for(i=0; i<pCnt; i++)
			{
				jtWt[i] = autoWt[i].jw[j];
			}
			if(norm) tData->SetBool(SKN_NORMALIZED_WEIGHT,true);
			CDSetJointWeight(tag, doc, tData, jtWt.Array(), pCnt);
			jtWt.Free();
		}
	}

	autoWt.Free();
	
	SplineObject::Free(falloffspline);
		
	return true;
}
Bool CDAddJoints::AddChildJoints(BaseDocument* doc, BaseObject *mesh, BaseObject *op, BaseTag *tag, LONG type)
{
	BaseContainer *tData = tag->GetDataInstance();
	LONG j, jIndex = SKN_J_LINK;
	
	while(op)
	{
		if(op != mesh)
		{
			if(op->GetType() == type)
			{
				Bool addToList = true;
				for(j=0; j<jlstCnt; j++)
				{
					if(op == static_cast<BaseObject*>(jointList->GetIndex(j))) addToList = false;
				}
				if(addToList)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					LONG jAddCnt = tData->GetLong(SKN_J_COUNT_CHANGE);
					tData->SetLink(jIndex+jAddCnt,op);
					jAddCnt++;
					tData->SetLong(SKN_J_COUNT_CHANGE,jAddCnt);
				}
			}
			AddChildJoints(doc,mesh,op->GetDown(),tag,type);
			op = op->GetNext();
		}
	}
	return true;
}

Bool CDAddJoints::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseTag* tag = doc->GetActiveTag();
	if(!tag)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	else if(tag && tag->GetType() != ID_CDSKINPLUGIN)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	
	BaseObject *mesh = tag->GetObject();
	if(mesh)
	{
		if(!IsValidPointObject(mesh))
		{
			MessageDialog(GeLoadString(D_AD_JOINT_ERR));
			return false;
		}
	}
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	
	LONG opCnt = objects->GetCount();
	if(opCnt < 1)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	else if(opCnt == 1 && mesh && static_cast<BaseObject*>(objects->GetIndex(0)) == mesh)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	
	BaseContainer *tData = tag->GetDataInstance();
	if(tData->GetBool(SKN_BOUND))
	{
		MessageDialog(GeLoadString(D_UNBIND_FIRST));
		return false;
	}
	
	LONG jAddCnt = tData->GetLong(SKN_J_COUNT);
	LONG jAddStrt = jAddCnt;
	LONG i, j, jIndex = SKN_J_LINK;
	jointList = AtomArray::Alloc();
	BaseObject *joint = NULL;
	jlstCnt = 0;
	for(i=0; i<jAddCnt; i++)
	{
		joint = tData->GetObjectLink(jIndex+i,doc);
		if(joint)
		{
			jointList->Append(joint);
			jlstCnt++;
		}
	}
	
	BaseContainer *tbc=GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
	if(tbc) tbc->SetLong(WP_SKIN_JOINT_COUNT,jAddCnt);
	
	Bool addChild = true, showDialog = false;
	BaseObject *op = NULL;
	for(i=0; i<opCnt; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op->GetDown()) showDialog = true;
	}
	if(showDialog)
	{
		if(!QuestionDialog(GeLoadString(D_AD_CHILDREN))) addChild = false;
		else  addChild = true;
	}
	
	StatusSetSpin();
	doc->StartUndo();
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	tData->SetLong(SKN_J_COUNT_CHANGE,jAddCnt);
	
	for(i=0; i<opCnt; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op != mesh)
		{
			Bool addToList = true;
			for(j=0; j<jlstCnt; j++)
			{
				if(op == static_cast<BaseObject*>(jointList->GetIndex(j))) addToList = false;
			}
			if(addToList)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				jAddCnt = tData->GetLong(SKN_J_COUNT_CHANGE);
				
				tData->SetLink(jIndex+jAddCnt,op);
				jAddCnt++;
				tData->SetLong(SKN_J_COUNT_CHANGE,jAddCnt);
			}
			if(addChild)
			{
				if(op->GetDown())
				{
					if(!AddChildJoints(doc,mesh,op->GetDown(),tag,op->GetType())) return true;
				}
			}
			
		}
	}
	doc->SetActiveObject(static_cast<BaseObject*>(objects->GetIndex(0)));

	DescriptionCommand dc;
	dc.id = DescID(SKN_ADD_JOINT);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	LONG max = 3;
	Bool env = false, norm = false;
	Real falloff = 2.5;
	
	BaseContainer *skwData = GetWorldPluginData(ID_ADDSKINCOMMAND);
	if(skwData)
	{
		max = skwData->GetLong(IDC_JOINT_NUMBER);
		env = skwData->GetBool(IDC_ENVELOPE);
		norm = skwData->GetBool(IDC_NORMALIZE_P_WT);
		falloff = skwData->GetReal(IDC_FALLOFF);
	}
	
	Bool autoWt = false, bind = false;
	BaseContainer *wpData = GetWorldPluginData(ID_CDADDJOINTS);
	if(wpData)
	{
		autoWt = wpData->GetBool(IDC_CDADDJ_AUTO_WT);
		bind = wpData->GetBool(IDC_CDADDJ_AUTO_BIND);
	}
	
	if(autoWt) AutoComputeWeights(doc,tag,mesh,max,env,falloff,norm,jAddStrt);
	if(bind)
	{
		dc.id = DescID(SKN_BIND_SKIN);
		tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
	
	op = static_cast<BaseObject*>(objects->GetIndex(0));
	doc->SetActiveObject(op);
	doc->SetActiveTag(tag);
	
	StatusClear();
	
	doc->EndUndo();
	EventAdd();

	AtomArray::Free(jointList);
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDAddJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddJoints(void)
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
	String name=GeLoadString(IDS_CDJADD); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDJOINTS,name,PLUGINFLAG_HIDE,"CDAddJoints.TIF","CD Add Joints",CDDataAllocator(CDAddJointsR));
	else return CDRegisterCommandPlugin(ID_CDADDJOINTS,name,0,"CDAddJoints.TIF","CD Add Joints",CDDataAllocator(CDAddJoints));
}
