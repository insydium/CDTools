//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"
#include "CDClusterTag.h"
#include "CDJoint.h"
#include "CDArray.h"

#include "oCDJoint.h"

class CDAddSkinDialog : public GeModalDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:
		Bool	useEnv, openPaint, normalPaint, includeChild, includeTip;
		LONG	maxJnt;
		Real	falloff;

		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddSkinDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle("CD Bind Skin");

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
			GroupBorder(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT|BFV_SCALEFIT,1,0,GeLoadString(IDC_INFLUENCE),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(10,10,10,10);

				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddCheckbox (IDC_AUTO_SEL_CHILDREN,BFH_LEFT,0,0,GeLoadString(IDC_AUTO_SEL_CHILDREN));
					AddCheckbox (IDC_INCLUDE_TIP,BFH_RIGHT,0,0,GeLoadString(IDC_INCLUDE_TIP));
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddStaticText(0,0,0,0,GeLoadString(IDC_JOINT_NUMBER),0);
					AddEditSlider(IDC_JOINT_NUMBER,BFH_SCALEFIT);
				GroupEnd();

				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddStaticText(0,0,0,0,GeLoadString(IDC_FALLOFF),0);
					AddEditSlider(IDC_FALLOFF,BFH_SCALEFIT);
				GroupEnd();

				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
					GroupSpace(4,1);
					AddCheckbox (IDC_ENVELOPE,BFH_LEFT,0,0,GeLoadString(IDC_ENVELOPE));
				GroupEnd();
			}
			GroupEnd();

			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				GroupSpace(4,1);
				AddCheckbox (IDC_OPENPAINTTOOL,BFH_LEFT,0,0,GeLoadString(IDC_OPENPAINTTOOL));
				AddCheckbox (IDC_NORMALIZE_P_WT,BFH_RIGHT,0,0,GeLoadString(IDC_NORMALIZE_P_WT));
			GroupEnd();

			AddDlgGroup(DLG_OK | DLG_CANCEL);
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDAddSkinDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;

	BaseContainer *bc = GetWorldPluginData(ID_ADDSKINCOMMAND);
	if(!bc)
	{
		SetBool(IDC_AUTO_SEL_CHILDREN,true);
		SetBool(IDC_NORMALIZE_P_WT,false);
		SetLong(IDC_JOINT_NUMBER,1,1,15,1);
		SetReal(IDC_FALLOFF,0.0,0.0,4.5,0.1,FORMAT_REAL);
		SetBool(IDC_ENVELOPE,false);
		SetBool(IDC_OPENPAINTTOOL,true);
		SetBool(IDC_INCLUDE_TIP,false);
	}
	else
	{
		SetBool(IDC_AUTO_SEL_CHILDREN,bc->GetBool(IDC_AUTO_SEL_CHILDREN));
		SetBool(IDC_NORMALIZE_P_WT,bc->GetBool(IDC_NORMALIZE_P_WT));
		SetLong(IDC_JOINT_NUMBER,bc->GetLong(IDC_JOINT_NUMBER),1,15,1);
		SetReal(IDC_FALLOFF,bc->GetReal(IDC_FALLOFF),0.0,4.5,0.1,FORMAT_REAL);
		SetBool(IDC_ENVELOPE,bc->GetBool(IDC_ENVELOPE));
		SetBool(IDC_OPENPAINTTOOL,bc->GetBool(IDC_OPENPAINTTOOL));
		SetBool(IDC_INCLUDE_TIP,bc->GetBool(IDC_INCLUDE_TIP));
	}

	return true;
}

Bool CDAddSkinDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_AUTO_SEL_CHILDREN,includeChild);
	GetBool(IDC_NORMALIZE_P_WT,normalPaint);
	GetLong(IDC_JOINT_NUMBER,maxJnt);
	GetReal(IDC_FALLOFF,falloff);
	GetBool(IDC_ENVELOPE,useEnv);
	GetBool(IDC_OPENPAINTTOOL,openPaint);
	GetBool(IDC_INCLUDE_TIP,includeTip);
	
	BaseContainer bc;
	bc.SetBool(IDC_NORMALIZE_P_WT,normalPaint);
	bc.SetBool(IDC_AUTO_SEL_CHILDREN,includeChild);
	bc.SetLong(IDC_JOINT_NUMBER,maxJnt);
	bc.SetReal(IDC_FALLOFF,falloff);
	bc.SetBool(IDC_ENVELOPE,useEnv);
	bc.SetBool(IDC_OPENPAINTTOOL,openPaint);
	bc.SetBool(IDC_INCLUDE_TIP,includeTip);
	SetWorldPluginData(ID_ADDSKINCOMMAND,bc,false);
	
	DoEnable();
	
	return true;
}

void CDAddSkinDialog::DoEnable(void)
{
	//GePrint("CDAddSkinDialog::DoEnable");
}

class CDAddSkin : public CommandData
{
	private:
		CDAddSkinDialog dlg;

		Bool ChildConnected(BaseObject *jnt);
		Bool SetSkeletonLinks(BaseDocument* doc, BaseObject *mesh, BaseObject *op, BaseTag *tag, LONG type);
		Bool AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *op);
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSkin::ChildConnected(BaseObject *jnt)
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

Bool CDAddSkin::SetSkeletonLinks(BaseDocument* doc, BaseObject *mesh, BaseObject *op, BaseTag *tag, LONG type)
{
	BaseContainer *tData = tag->GetDataInstance();
	LONG index;
	StatusSetSpin();

	if(op != mesh)
	{
		while(op)
		{
			index = tData->GetLong(SKN_J_COUNT);
			if(op->GetType() == type)
			{
				if(op->GetType() == ID_CDJOINTOBJECT)
				{
					if(!dlg.includeTip)
					{
						if(ChildConnected(op->GetDown()))
						{
							tData->SetLink(SKN_J_LINK+index,op);
							tData->SetLong(SKN_J_COUNT,index+1);
						}
					}
					else
					{
						tData->SetLink(SKN_J_LINK+index,op);
						tData->SetLong(SKN_J_COUNT,index+1);
					}
				}
				else
				{
					tData->SetLink(SKN_J_LINK+index,op);
					tData->SetLong(SKN_J_COUNT,index+1);
				}
			}
			SetSkeletonLinks(doc,mesh,op->GetDown(),tag,type);
			op = op->GetNext();
		}
	}
	return true;
}

Bool CDAddSkin::AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *op)
{
	BaseContainer *tData = tag->GetDataInstance(), *jointData=NULL;
	BaseObject *joint=NULL, *chJoint=NULL;
	
	LONG i, j, pCount = tData->GetLong(SKN_S_POINT_COUNT), jCount = tData->GetLong(SKN_J_COUNT);
	Matrix chMatrix, jMatrix, sectionM, opMatrix = op->GetMg();
	
	LONG max = dlg.maxJnt;
	Real falloff = dlg.falloff;
	Bool env = dlg.useEnv, tip = dlg.includeTip;
	
	Real weight, dist, dProd, tWeightA, tWeightB, adj;
	Vector rSk, tSk, rRad, tRad, d1, d2, closest;
	
	CDSkinVertex *skinWeight = CDGetSkinWeights(tag); if(!skinWeight) return false;
	Vector  *padr = GetPointArray(op), vertex;

	SplineObject *falloffspline = CDAllocateSplineObject(5,CD_SPLINETYPE_BSPLINE);
	Real f = falloff * 10;
	Vector *fsPts = GetPointArray(falloffspline);
	fsPts[0] = Vector(0.0,0.0,0.0);
	fsPts[1] = Vector(f,0.0,0.0);
	fsPts[2] = Vector(50.0,0.5,0.0);
	fsPts[3] = Vector(100.0-f,1.0,0.0);
	fsPts[4] = Vector(100.0,1.0,0.0);
	
	for(i=0; i<pCount; i++)
	{
		StatusSetBar(LONG(Real(i)/Real(pCount)*100.0));
		vertex = opMatrix * padr[i];
		CDArray<CDClusterWeight> tempWeight;
		tempWeight.Alloc(jCount);
		for(j=0; j<jCount; j++)
		{
			joint = tData->GetObjectLink(SKN_J_LINK+j,doc);
			jointData = joint->GetDataInstance();
			jMatrix = joint->GetMg();
			chJoint = joint->GetDown();
			Bool jenv = jointData->GetBool(JNT_SHOW_ENVELOPE);
			if(env && jenv)
			{
				if(!chJoint)
				{
					if(!tip) dist = Len(jMatrix.off - vertex)*100;
					else dist = Len(jMatrix.off - vertex);
				}
				else
				{
					chMatrix = chJoint->GetMg();
					rRad = jointData->GetVector(JNT_ROOT_OFFSET);
					tRad = jointData->GetVector(JNT_TIP_OFFSET);
					rSk.x = jointData->GetReal(JNT_SKEW_RX);
					rSk.y = jointData->GetReal(JNT_SKEW_RY);
					tSk.x = jointData->GetReal(JNT_SKEW_TX);
					tSk.y = jointData->GetReal(JNT_SKEW_TY);
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
					dProd = VDot(VNorm(d1), VNorm(d2));
					
					Real envFO = CDBlend(1.0,1.5,falloff/4.5);
					if (dProd < 0) // perpendicular is off the beginning of the line
					{
						dist = Len(jMatrix.off - vertex)*envFO;
					}
					else
					{
				    	adj = dProd * Len(d1);
				    	if(adj > Len(d2)) // perpendicular is off the end of the line
				    	{
				    		dist = Len(chMatrix.off - vertex)*envFO;
				    	}
				    	else // perpendicular is on the line somewhere
				    	{
							Real theMix = adj / Len(d2);
				    		sectionM = jMatrix;
				    		Vector vPos = jMatrix.off + VNorm(d2) * adj;
				    		sectionM.off = vPos;
				    		sectionM.v1 = CDBlend(jMatrix.v1,chMatrix.v1,theMix);
				    		sectionM.v2 = CDBlend(jMatrix.v2,chMatrix.v2,theMix);
				    		sectionM.v3 = jMatrix.v3;
				    		
				    		if(Len(vertex - vPos) > Len((sectionM * VNorm(MInv(sectionM) * vertex)) - vPos))
				    		{
				    			dist = Len(sectionM.off - vertex)*envFO;
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
					if(!tip) dist = Len(jMatrix.off - vertex)*100;
					else dist = Len(jMatrix.off - vertex);
				}
				else
				{
					chMatrix = chJoint->GetMg();
					d1 = vertex - jMatrix.off;
					d2 = chMatrix.off - jMatrix.off;
					dProd = VDot(VNorm(d1), VNorm(d2));
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
			// 1 / distance ^ 10
			weight = Real(1) / (dist * dist * dist * dist * dist * dist * dist * dist * dist * dist);
			tempWeight[j].ind = j;
			tempWeight[j].w = weight * (joint->GetRad().x + joint->GetRad().y)*0.5;
		}
		
		LONG size = sizeof(CDClusterWeight);
		qsort(tempWeight.Array(),jCount, size,CompareWeights);
		
		if(max > jCount) max = jCount;
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
			skinWeight[i].jw[tempWeight[j].ind] = fWeight;
		}
		tempWeight.Free();
	}
	
	SplineObject::Free(falloffspline);
	
	return true;
}

Bool CDAddSkin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	
	LONG  selCount = opSelectionList->GetCount();
	if(selCount > 1)
	{
		BaseObject *mesh = static_cast<BaseObject*>(opSelectionList->GetIndex(0)); 
		if(!mesh) return false;
		if (!IsValidPointObject(mesh)) return false;
		if (mesh->GetTag(ID_CDSKINPLUGIN)) return false;
		
		if(!dlg.Open()) return false;
		
		AutoAlloc<AtomArray> objects;
		if(!objects) return false;

		if(dlg.includeChild) CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
		else CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		
		objects->Remove(mesh);
		
		LONG  opCount = objects->GetCount();
		if(opCount > 0)
		{
			if(opCount == 1 && static_cast<BaseObject*>(objects->GetIndex(0)) == mesh) return false;
			
			StatusSetSpin();
			doc->StartUndo();
			
			// add skin ref tag
			BaseTag *skinRef = mesh->GetTag(ID_CDSKINREFPLUGIN);
			if(skinRef)
			{
				CDAddUndo(doc,CD_UNDO_DELETE,skinRef);
				BaseTag::Free(skinRef);
			}
			skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
			mesh->InsertTag(skinRef,NULL);
			CDAddUndo(doc,CD_UNDO_NEW,skinRef);
			
			mesh->Message(MSG_UPDATE);
			DescriptionCommand skrdc;
			skrdc.id = DescID(SKR_SET_REFERENCE);
			skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
			
			// add skin tag
			BaseTag *cdsTag=NULL;
			cdsTag = BaseTag::Alloc(ID_CDSKINPLUGIN);

			BaseTag *refTag = mesh->GetTag(ID_CDMORPHREFPLUGIN);
			if(refTag) mesh->InsertTag(cdsTag,refTag);
			else mesh->InsertTag(cdsTag,skinRef);
			CDAddUndo(doc,CD_UNDO_NEW,cdsTag);
			
			BaseContainer *tData = cdsTag->GetDataInstance();
			tData->SetLink(SKN_DEST_ID,mesh);
			tData->SetLong(SKN_J_COUNT,0);
			tData->SetLong(SKN_J_LINK_ID,0);
			
			// set the joint skeleton links
			LONG i, index, jIndex = SKN_J_LINK;
			BaseObject* jointObject = NULL;
			for(i=0; i<opCount; i++)
			{
				jointObject = static_cast<BaseObject*>(objects->GetIndex(i));
				if(jointObject != mesh)
				{
					index = tData->GetLong(SKN_J_COUNT);
					tData->SetLink(jIndex+index,jointObject);
					tData->SetLong(SKN_J_COUNT,index+1);
					if(dlg.includeChild)
					{
						if(!SetSkeletonLinks(doc,mesh,jointObject->GetDown(),cdsTag,jointObject->GetType()))
						{
							StatusClear();
						}
					}
				}
			}
			BaseObject *skeletonRoot = static_cast<BaseObject*>(objects->GetIndex(0));
			doc->SetActiveObject(skeletonRoot);

			// Initialize skin tag's weight array
			cdsTag->Message(MSG_MENUPREPARE);

			// Auto compute weights
			if(!AutoComputeWeights(doc,cdsTag,mesh))
			{
				StatusClear();
				return false;
			}
			if(dlg.normalPaint)
			{
				tData->SetBool(SKN_NORMALIZED_WEIGHT,true);
			}
			
			// Set accumulated weights
			DescriptionCommand dc;
			dc.id = DescID(SKN_ACCUMULATE);
			cdsTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
			
			//Bind skin
			dc.id = DescID(SKN_AUTO_BIND_SKIN);
			cdsTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
			cdsTag->Message(MSG_UPDATE);
			
			doc->SetActiveTag(cdsTag);

			doc->EndUndo();
			
			StatusClear();

			if(dlg.openPaint)
			{
				doc->SetAction(ID_SKINWEIGHTTOOL);
				
				BaseContainer *bc = GetWorldPluginData(ID_SKINWEIGHTTOOL);
				if(bc) bc->SetBool(D_WP_NORMALIZED,dlg.normalPaint);
				
				SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog layout
			}
			
		}

		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddSkinR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSkin(void)
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
	String name=GeLoadString(IDS_SKINCOM); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ADDSKINCOMMAND,name,PLUGINFLAG_HIDE,"CDAddSkin.TIF","CD Bind Skin",CDDataAllocator(CDAddSkinR));
	else return CDRegisterCommandPlugin(ID_ADDSKINCOMMAND,name,0,"CDAddSkin.TIF","CD Bind Skin",CDDataAllocator(CDAddSkin));
}
