//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDClusterTag.h"
#include "CDSkinRefTag.h"
#include "tCDCluster.h"
#include "CDArray.h"


class CDMirrorSkinClusterDialog : public GeDialog
{
	private:
		CDJSOptionsUA ua;

	public:
		String oldName, newName;
		
		Bool DoMirror(Real t, LONG a);
		Bool MirrorTag(BaseDocument *doc, BaseObject *op, BaseTag *tag, Real t, LONG a);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};


Bool CDMirrorSkinClusterDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMIRRORSCTAG));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_MIRROR_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_MIRROR_AXIS,BFH_CENTER,3,1);
					AddChild(IDC_MIRROR_AXIS, 0, GeLoadString(M_X));
					AddChild(IDC_MIRROR_AXIS, 1, GeLoadString(M_Y));
					AddChild(IDC_MIRROR_AXIS, 2, GeLoadString(M_Z));
			}
			GroupEnd();

			GroupBegin(0,BFH_SCALEFIT,4,0,GeLoadString(D_MIRROR_C_RENAME),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_OLD_NAME),0);
				AddEditText(IDC_OLD_NAME,BFH_CENTER,70,0);
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_NEW_NAME),0);
				AddEditText(IDC_NEW_NAME,BFH_CENTER,70,0);
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,3,0,"",0);
			{
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_TOLERANCE),0);
				AddEditNumberArrows(IDC_P_TOLERANCE,BFH_LEFT);
				AddButton(GADGET_MIRROR,BFH_RIGHT,0,0,GeLoadString(D_MIRROR));
			}
			GroupEnd();
		}
		GroupEnd();
	}

	return res;
}

Bool CDMirrorSkinClusterDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDMIRRORSKINCLUSTER);
	if(!bc)
	{
		SetLong(IDC_MIRROR_AXIS,0);
		SetReal(IDC_P_TOLERANCE,0.1,0.001,100,0.001,FORMAT_REAL);
		SetString(IDC_OLD_NAME,oldName);
		SetString(IDC_NEW_NAME,newName);
	}
	else
	{
		SetLong(IDC_MIRROR_AXIS,bc->GetLong(IDC_MIRROR_AXIS));
		SetReal(IDC_P_TOLERANCE,bc->GetReal(IDC_P_TOLERANCE),0.001,100,0.001,FORMAT_REAL);
		SetString(IDC_OLD_NAME,bc->GetString(IDC_OLD_NAME));
		SetString(IDC_NEW_NAME,bc->GetString(IDC_NEW_NAME));
	}

	return true;
}

Bool CDMirrorSkinClusterDialog::Command(LONG id,const BaseContainer &msg)
{
	StopAllThreads(); // so the document can be safely modified
	
	Real t; LONG a;
	
	GetLong(IDC_MIRROR_AXIS,a);
	GetReal(IDC_P_TOLERANCE,t);
	GetString(IDC_OLD_NAME,oldName);
	GetString(IDC_NEW_NAME,newName);
	
	BaseContainer bc;
	bc.SetLong(IDC_MIRROR_AXIS,a);
	bc.SetReal(IDC_P_TOLERANCE,t);
	bc.SetString(IDC_OLD_NAME,oldName);
	bc.SetString(IDC_NEW_NAME,newName);
	SetWorldPluginData(ID_CDMIRRORSKINCLUSTER,bc,false);

	switch (id)
	{
		case GADGET_MIRROR:
			DoMirror(t, a);
			break;
	}

	return true;
}

Bool CDMirrorSkinClusterDialog::MirrorTag(BaseDocument *doc, BaseObject *op, BaseTag *tag, Real t, LONG a)
{
	BaseContainer	*tData = tag->GetDataInstance();
	BaseObject		*tagOp = tag->GetObject();
	BaseObject 		*dest = tData->GetObjectLink(CLST_DEST_LINK,doc); if(!dest) return false;
	if(!IsValidPointObject(dest)) return true;

	BaseTag *skRefTag = dest->GetTag(ID_CDSKINREFPLUGIN); if (!skRefTag) return true;
	CDMRefPoint *skinRef = CDGetSkinReference(skRefTag);

	LONG i, mi, pCnt =ToPoint(dest)->GetPointCount();
	
	CDArray<Real> tWeight;
	tWeight.Alloc(pCnt);
	
	if(!CDGetClusterWeight(tag, tData, tWeight.Array(), pCnt)) return false;
	
	CDArray<Real> mWeight;
	mWeight.Alloc(pCnt);
	mWeight.Fill(0.0);
	
	Bool useSymtag = false;
	LONG *symadr = NULL;
	BaseTag *symTag = dest->GetTag(ID_CDMIRRORPOINTSTAG);
	if(symTag)
	{
		BaseContainer *symData = symTag->GetDataInstance();
		if(symData)
		{
			if(symData->GetBool(10002))
			{
				symadr = CDSymGetMirrorPoints(symTag);
				if(symadr) useSymtag = true;
			}
		}
	}
	
	Bool symmetry = true;
	if(useSymtag)
	{
		for(i=0; i<pCnt; i++)
		{
			mWeight[i] = tWeight[symadr[i]];
		}
	}
	else
	{
		StatusSetText("Checking Mesh Symmetry");
		
		Vector pt, mpt; 
		Bool mirrored = false;
		for(i=0; i<pCnt; i++)
		{
			StatusSetBar(LONG(Real(i)/Real(pCnt)*100.0));
			if(tWeight[i] > 0)
			{
				pt.x = skinRef[i].x;
				pt.y = skinRef[i].y;
				pt.z = skinRef[i].z;
				switch (a)
				{
					case 0:
						pt.x *= -1;
						break;
					case 1:
						pt.y *= -1;
						break;
					case 2:
						pt.z *= -1;
						break;
					default:
					break;
				}
				mirrored = false;
				for(mi=0; mi<pCnt; mi++)
				{
					mpt.x = skinRef[mi].x;
					mpt.y = skinRef[mi].y;
					mpt.z = skinRef[mi].z;
					Real dif = Len(mpt - pt);
					if(dif<t)
					{
						mWeight[mi] = tWeight[i];
						mirrored = true;
					}
				}
				if(!mirrored)  symmetry = false;
			}
		}
		StatusClear();
	}

	if(!symmetry)
	{
		MessageDialog(GeLoadString(NOT_SYMMETRICAL_P));
	}
	
	LONG sPos;
	String tagName = tag->GetName();
	if(tagName.FindFirst(oldName,&sPos,0))
	{
		tagName.Delete(sPos,oldName.GetLength());
		tagName.Insert(sPos,newName);
	}
	
	BaseTag *skcTag = NULL;
	skcTag = BaseTag::Alloc(ID_CDCLUSTERTAGPLUGIN);
	skcTag->SetName(tagName);
	op->InsertTag(skcTag,NULL);
	skcTag->Message(MSG_MENUPREPARE);
	CDAddUndo(doc,CD_UNDO_NEW,skcTag);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,skcTag);
	BaseContainer *skcTData = skcTag->GetDataInstance();
	skcTData->SetLink(CLST_DEST_LINK,dest);
	skcTData->SetBool(CLST_MIX_PREVIOUS,tData->GetBool(CLST_MIX_PREVIOUS));
	if(tData->GetBool(CLST_BLEND_ROTATION))
	{
		skcTData->SetBool(CLST_BLEND_ROTATION,tData->GetBool(CLST_BLEND_ROTATION));
		skcTData->SetReal(CLST_ROT_MIX,tData->GetReal(CLST_ROT_MIX));
	}
	if(tData->GetBool(CLST_ATTACH_CHILD))
	{
		if(op->GetDown())
		{
			Vector jOffset, cOffset, pOffset;
			Matrix opM = tagOp->GetMg(), chM = tagOp->GetDown()->GetMg();
			Matrix mopM = op->GetMg(), mchM = op->GetDown()->GetMg();
			
			skcTData->SetBool(CLST_ATTACH_CHILD,tData->GetBool(CLST_ATTACH_CHILD));
			skcTData->SetBool(CLST_SHOW_GUIDE,tData->GetBool(CLST_SHOW_GUIDE));
			skcTData->SetReal(CLST_GUIDE_SIZE,tData->GetReal(CLST_GUIDE_SIZE));
			skcTData->SetBool(CLST_RESTRICT_TO_Z,tData->GetBool(CLST_RESTRICT_TO_Z));
			skcTData->SetReal(CLST_POS_MIX,tData->GetReal(CLST_POS_MIX));
			
			jOffset = opM * tData->GetVector(CLST_JOINT_OFFSET);
			cOffset = chM * tData->GetVector(CLST_CHILD_OFFSET);
			
			switch (a)
			{
				case 0:
					jOffset.x *= -1;
					cOffset.x *= -1;
					break;
				case 1:
					jOffset.y *= -1;
					cOffset.y *= -1;
					break;
				case 2:
					jOffset.z *= -1;
					cOffset.z *= -1;
					break;
				default:
				break;
			}
			skcTData->SetVector(CLST_JOINT_OFFSET, MInv(mopM) * jOffset);
			skcTData->SetVector(CLST_CHILD_OFFSET, MInv(mchM) * cOffset);
			if(tData->GetBool(CLST_ENABLE_PIVOT))
			{
				skcTData->SetBool(CLST_ENABLE_PIVOT,tData->GetBool(CLST_ENABLE_PIVOT));
				pOffset = opM * tData->GetVector(CLST_PIVOT_OFFSET);
				switch (a)
				{
					case 0:
						pOffset.x *= -1;
						break;
					case 1:
						pOffset.y *= -1;
						break;
					case 2:
						pOffset.z *= -1;
						break;
					default:
					break;
				}
				skcTData->SetVector(CLST_PIVOT_OFFSET, MInv(mopM) * pOffset);
			}
			skcTData->SetVector(CLST_STRENGTH,tData->GetVector(CLST_STRENGTH));
		}
	}
	skcTag->Message(MSG_CHANGE);

	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	DescriptionCommand dc;
	dc.id = DescID(CLST_BIND_POINTS);
	skcTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	CDSetClusterWeight(skcTag, skcTData, mWeight.Array(), pCnt);
	dest->Message(MSG_UPDATE);
	doc->SetActiveTag(skcTag);
	
	tWeight.Free();
	mWeight.Free();
	
	return true;
}

Bool CDMirrorSkinClusterDialog::DoMirror(Real t, LONG a)
{
	BaseDocument *doc = GetActiveDocument();
	
	AutoAlloc<AtomArray> tags; if (!tags) return false;
	doc->GetActiveTags(tags);
	LONG ti, tagCount = tags->GetCount();
	BaseTag *tagActive = static_cast<BaseTag*>(tags->GetIndex(0));
	if(!tagActive)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	Bool scTag = false;
	for(ti=0; ti<tagCount; ti++)
	{
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDCLUSTERTAGPLUGIN)
		{
			scTag = true;
		}
	}
	if(!scTag)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(0));
	if(!op)
	{
		MessageDialog(GeLoadString(A_SELECT_OBJECT));
		return false;	
	}
	
	doc->StartUndo();
	
	for(ti=0; ti<tagCount; ti++)
	{
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDCLUSTERTAGPLUGIN)
		{
			MirrorTag(doc,op,tagActive,t,a);
		}
	}
	
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDMirrorSkinCluster : public CommandData
{
	private:
		CDMirrorSkinClusterDialog dlg;

	public:

		virtual Bool Execute(BaseDocument *doc)
		{
		#if API_VERSION < 12000
			return dlg.Open(true,ID_CDMIRRORSKINCLUSTER,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDMIRRORSKINCLUSTER,-1,-1);
		#endif
		}

		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDMIRRORSKINCLUSTER,0,secret);
		}
};

class CDMirrorSkinClusterR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMirrorSkinCluster(void)
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
	String name=GeLoadString(IDS_CDMIRRORSCTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMIRRORSKINCLUSTER,name,PLUGINFLAG_HIDE,"CDMirrorSKC.tif","CD Mirror Skin Cluster",CDDataAllocator(CDMirrorSkinClusterR));
	else return CDRegisterCommandPlugin(ID_CDMIRRORSKINCLUSTER,name,0,"CDMirrorSKC.tif","CD Mirror Skin Cluster",CDDataAllocator(CDMirrorSkinCluster));
}
