//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"

#include "tCDSkin.h"

class CDMirrorSkinWeightDialog : public GeModalDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:
		LONG	axis, direction;
		Real	ptolerance, jtolerance;
		
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDMirrorSkinWeightDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMIRRORSW));
		
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
					
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_MIRROR_DIRECTION),0);
					AddComboBox(IDC_MIRROR_DIRECTION,BFH_CENTER);
						AddChild(IDC_MIRROR_DIRECTION, 0, GeLoadString(M_POS));
						AddChild(IDC_MIRROR_DIRECTION, 1, GeLoadString(M_NEG));
				}
			}
			GroupEnd();
			GroupBegin(0,BFH_SCALEFIT,4,0,GeLoadString(D_TOLERANCE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_P_TOLERANCE),0);
				AddEditNumberArrows(IDC_P_TOLERANCE,BFH_LEFT);
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_J_TOLERANCE),0);
				AddEditNumberArrows(IDC_J_TOLERANCE,BFH_LEFT);
			}
			GroupEnd();
		}
		GroupEnd();

		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDMirrorSkinWeightDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDMIRRORSKINWEIGHT);
	if(!bc)
	{
		SetLong(IDC_MIRROR_AXIS,0);
		SetLong(IDC_MIRROR_DIRECTION,0);
		SetReal(IDC_P_TOLERANCE,0.005,0.001,100,0.001,FORMAT_REAL);
		SetReal(IDC_J_TOLERANCE,2.00,0.01,100,0.01,FORMAT_REAL);
	}
	else
	{
		SetLong(IDC_MIRROR_AXIS,bc->GetLong(IDC_MIRROR_AXIS));
		SetLong(IDC_MIRROR_DIRECTION,bc->GetLong(IDC_MIRROR_DIRECTION));
		SetReal(IDC_P_TOLERANCE,bc->GetReal(IDC_P_TOLERANCE),0.001,100,0.001,FORMAT_REAL);
		SetReal(IDC_J_TOLERANCE,bc->GetReal(IDC_J_TOLERANCE),0.01,100,0.01,FORMAT_REAL);
	}


	return true;
}

Bool CDMirrorSkinWeightDialog::Command(LONG id,const BaseContainer &msg)
{	
	GetLong(IDC_MIRROR_AXIS,axis);
	GetLong(IDC_MIRROR_DIRECTION,direction);
	GetReal(IDC_P_TOLERANCE,ptolerance);
	GetReal(IDC_J_TOLERANCE,jtolerance);

	BaseContainer bc;
	bc.SetLong(IDC_MIRROR_AXIS,axis);
	bc.SetLong(IDC_MIRROR_DIRECTION,direction);
	bc.SetReal(IDC_P_TOLERANCE,ptolerance);
	bc.SetReal(IDC_J_TOLERANCE,jtolerance);
	SetWorldPluginData(ID_CDMIRRORSKINWEIGHT,bc,false);

	return true;
}

class CDMirrorSkinWeight : public CommandData
{
	private:
		CDMirrorSkinWeightDialog dlg;
		Bool MirrorChildWeights(BaseDocument* doc, BaseObject *op, BaseTag *tag, LONG type);
		
	public:		
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDMirrorSkinWeight::MirrorChildWeights(BaseDocument* doc, BaseObject *op, BaseTag *tag, LONG type)
{
	BaseContainer *tagData = tag->GetDataInstance();
	LONG j,jCount, jIndex = SKN_J_LINK;
	DescriptionCommand dc;
	StatusSetSpin();
	
	while(op)
	{
		if(op->GetType() == type)
		{
			jCount = tagData->GetLong(SKN_J_COUNT);
			for(j=0; j<jCount; j++)
			{
				if(op == tagData->GetObjectLink(jIndex+j,doc))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					tagData->SetLong(SKN_J_LINK_ID,j);
					
					dc.id = DescID(SKN_MIRROR_WEIGHT);
					tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
				}
			}
		}
		MirrorChildWeights(doc,op->GetDown(),tag,type);
		op = op->GetNext();
	}
	return true;
}

Bool CDMirrorSkinWeight::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	DescriptionCommand dc;
	
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
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG i, j, opCount = objects->GetCount(), jIndex = SKN_J_LINK;
	BaseContainer *tagData = tag->GetDataInstance();

	if(opCount < 1)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	else if(opCount == 1)
	{
		BaseObject *jop = static_cast<BaseObject*>(objects->GetIndex(0));
		if(mesh && jop == mesh)
		{
			MessageDialog(GeLoadString(D_AD_JOINT_ERR));
			return false;
		}
		else
		{
			LONG jCount = tagData->GetLong(SKN_J_COUNT);
			Bool jopInList = false;
			for(j=0; j<jCount; j++)
			{
				if(jop == tagData->GetObjectLink(jIndex+j,doc))  jopInList = true;
			}
			if(!jopInList)
			{
				MessageDialog(GeLoadString(D_J_NOT_IN_LIST));
				return false;
			}
		}
	}
	
	if(tagData->GetBool(SKN_BOUND))
	{
		dc.id = DescID(SKN_GO_TO_BIND_POSE);
		tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
	
	if(!dlg.Open()) return false;
	
	Bool includeChild = true, showDialog = false;;
	BaseObject *op = NULL;
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op->GetDown()) showDialog = true;
	}
	if(showDialog)
	{
		if(!QuestionDialog(GeLoadString(D_AD_CHILDREN))) includeChild = false;
		else  includeChild = true;
	}
	
	objects->Flush();

	if(!includeChild) CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	else CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);

	
	opCount = objects->GetCount();

	LONG jCount = tagData->GetLong(SKN_J_COUNT);
	tagData->SetLong(SKN_MIRROR_AXIS,dlg.axis);
	tagData->SetLong(SKN_MIRROR_DIRECTION,dlg.direction);
	tagData->SetReal(SKN_P_TOLERANCE,dlg.ptolerance);
	tagData->SetReal(SKN_J_TOLERANCE,dlg.jtolerance);
	tagData->SetLink(SKN_MJ_ID,static_cast<BaseObject*>(objects->GetIndex(0)));
	
	dc.id = DescID(SKN_INIT_MJ_INDEX);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	dc.id = DescID(SKN_INIT_MP_INDEX);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);

	if(!tagData->GetBool(SKN_P_MIRRORED) && tagData->GetBool(SKN_J_MIRRORED))
	{
		MessageDialog(GeLoadString(NOT_SYMMETRICAL_P));
	}
	else if(tagData->GetBool(SKN_P_MIRRORED) && !tagData->GetBool(SKN_J_MIRRORED))
	{
		MessageDialog(GeLoadString(NOT_SYMMETRICAL_J));
	}
	else if(!tagData->GetBool(SKN_P_MIRRORED) && !tagData->GetBool(SKN_J_MIRRORED))
	{
		MessageDialog(GeLoadString(NOT_SYMMETRICAL));
	}
	
	StatusSetSpin();
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		for(j=0; j<jCount; j++)
		{
			if(op == tagData->GetObjectLink(jIndex+j,doc))
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				tagData->SetLong(SKN_J_LINK_ID,j);
				
				dc.id = DescID(SKN_MIRROR_WEIGHT);
				tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
				if(includeChild)
				{
					if(op->GetDown())
					{
						if(!MirrorChildWeights(doc,op->GetDown(),tag,op->GetType())) return true;
					}
				}
			}
		}
	}
	doc->SetActiveObject(tagData->GetObjectLink(SKN_MJ_ID,doc));

	dc.id = DescID(SKN_ACCUMULATE);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	dc.id = DescID(SKN_FREE_MIRROR_IND);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	doc->SetActiveTag(tag);
	
	doc->EndUndo();

	EventAdd(EVENT_FORCEREDRAW);
	StatusClear();

	return true;
}

class CDMirrorSkinWeightR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMirrorSkinWeight(void)
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
	String name=GeLoadString(IDS_CDMIRRORSW); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMIRRORSKINWEIGHT,name,PLUGINFLAG_HIDE,"CDMirrorSW.tif","CD Mirror Skin Weight",CDDataAllocator(CDMirrorSkinWeightR));
	else return CDRegisterCommandPlugin(ID_CDMIRRORSKINWEIGHT,name,0,"CDMirrorSW.tif","CD Mirror Skin Weight",CDDataAllocator(CDMirrorSkinWeight));
}
