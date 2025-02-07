//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDFBX.h"

class CDConvertFromCDJointsDialog : public GeModalDialog
{
private:
	CDFbxOptionsUA ua;
	
public:	
	LONG opt;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDConvertFromCDJointsDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDJTOB));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(DLG_CDJS_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_CDJS_OPTIONS_RB,BFH_CENTER,2,1);
				AddChild(IDC_CDJS_OPTIONS_RB, 0, GeLoadString(IDS_JOINTS));
				AddChild(IDC_CDJS_OPTIONS_RB, 1, GeLoadString(IDS_BONES));
				
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDConvertFromCDJointsDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCONVERTFROMCDJOINTS);
	if(!bc)
	{
		SetLong(IDC_CDJS_OPTIONS_RB,0);
	}
	else
	{
		SetLong(IDC_CDJS_OPTIONS_RB,bc->GetBool(IDC_CDJS_OPTIONS_RB));
	}
	
	return true;
}

Bool CDConvertFromCDJointsDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_CDJS_OPTIONS_RB,opt);
	
	BaseContainer bc;
	bc.SetLong(IDC_CDJS_OPTIONS_RB,opt);
	SetWorldPluginData(ID_CDCONVERTFROMCDJOINTS,bc,false);
	
	return true;
}

class CDConvertFromCDJoints : public CommandData
{
private:
	CDConvertFromCDJointsDialog dlg;
	
#if API_VERSION < 12000
	Bool ChildCDJointsToBones(BaseDocument *doc, BaseObject *op, BaseObject *bn);
	Bool CDJointToBone(BaseDocument *doc);
#endif

#if API_VERSION >= 9800
	void TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk);
	void TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst);
	
	Bool ChildCDJointsToJoints(BaseDocument *doc, BaseObject *jnt);
	Bool CDJointToJoint(BaseDocument *doc);
#endif

public:
	virtual Bool Execute(BaseDocument *doc);
};

#if API_VERSION < 12000
Bool CDConvertFromCDJoints::ChildCDJointsToBones(BaseDocument* doc, BaseObject *op, BaseObject *bn)
{
	BaseObject *prev=NULL, *bone=NULL, *prevCH=NULL, *ch=NULL;
	BaseTag *opTag=NULL, *tagClone=NULL;
	BaseContainer *bnData=NULL; 
	String opName;
	Matrix opMatrix;
	Real bnLen;
	
	StatusSetSpin();
	while(op && bn)
	{
		opName = op->GetName();
		opMatrix = op->GetMg();

		prevCH = bn->GetDown();
		while(prevCH)
		{
			prev = prevCH;
			prevCH = prevCH->GetNext();
		}
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			bone = BaseObject::Alloc(Obone); if (!bone) return false;
			bone->SetName(opName);
			doc->InsertObject(bone,bn,prev,false);
			bone->SetMg(opMatrix);
			CDAddUndo(doc,CD_UNDO_NEW,bone);
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,bone);
			for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone = NULL;
				tagClone=(BaseTag*)opTag->GetClone(0,NULL);
				if (tagClone)
				{
					bone->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			bnData = bone->GetDataInstance(); if(!bnData) return false;
			bnData->SetBool(BONEOBJECT_NEWBONES,true);
			bnData->SetBool(BONEOBJECT_VERTEXMAPMODE,true);
			bnData->SetBool(ID_BASEOBJECT_GENERATOR_FLAG,false);
			
			ch = op->GetDown();
			if(!ch)
			{
				BaseObject *bnPr = bone->GetUp();
				if(bnPr && CDIsBone(bnPr))
				{
					BaseContainer *bnprData = bnPr->GetDataInstance();
					if(bnprData)
					{
						bnLen = bnprData->GetReal(BONEOBJECT_LENGTH);
						bnData->SetReal(BONEOBJECT_LENGTH,bnLen);
					}
				}
				else bnData->SetReal(BONEOBJECT_LENGTH,100.0);
				bnData->SetBool(BONEOBJECT_NULL,true);
			}
			else
			{
				bnLen = Len(ch->GetMg().off - op->GetMg().off);
				if(bnLen == 0)
				{
					BaseObject *bnPr = bone->GetUp();
					if(bnPr && CDIsBone(bnPr))
					{
						BaseContainer *bnprData = bnPr->GetDataInstance();
						if(bnprData)
						{
							bnLen = bnprData->GetReal(BONEOBJECT_LENGTH);
							bnData->SetReal(BONEOBJECT_LENGTH,bnLen);
						}
					}
					else bnData->SetReal(BONEOBJECT_LENGTH,100.0);
				}
				else
				{
					bnData->SetReal(BONEOBJECT_LENGTH,bnLen);
					bnData->SetBool(BONEOBJECT_NULL,false);
				}
			}
		}
		else
		{
			bone=(BaseObject*)op->GetClone(COPY_NO_HIERARCHY|COPY_NO_BRANCHES|COPY_NO_INTERNALS,NULL);
			if (bone)
			{
				doc->InsertObject(bone,bn,prev,false);
				bone->SetMg(opMatrix);
				CDAddUndo(doc,CD_UNDO_NEW,bone);
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				CDTransferGoals(op,bone);
				for (BaseTag* opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
				{
					tagClone = NULL;
					tagClone=(BaseTag*)opTag->GetClone(0,NULL);
					if (tagClone)
					{
						bone->InsertTag(tagClone,NULL);
						CDAddUndo(doc,CD_UNDO_NEW,tagClone);
						CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
						CDTransferGoals(opTag,tagClone);
					}
				}
			}
		}
		prev = bone;
		ChildCDJointsToBones(doc,op->GetDown(),bone);
		op = op->GetNext();
	}
	return true;
}

Bool CDConvertFromCDJoints::CDJointToBone(BaseDocument *doc)
{
	BaseObject *op=NULL, *bone=NULL, *ch=NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	doc->GetActiveObjects(objects,false);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_JOINT));
		return false;
	}
	LONG i, opCount = objects->GetCount();
	Matrix opMatrix;
	String opName;
	Real bnLen;
	BaseTag *tagClone=NULL;
	BaseContainer *bnData=NULL;
	
	StatusSetSpin();
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			opMatrix = op->GetMg();
			opName = op->GetName();
			
			bone = BaseObject::Alloc(Obone);
			if (!bone)
			{
				doc->EndUndo();
				StatusClear();
				return false;
			}
			bone->SetName(opName);
			doc->InsertObject(bone,op->GetUp(),op,false);
			bone->SetMg(opMatrix);
			CDAddUndo(doc,CD_UNDO_NEW,bone);
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,bone);
			for (BaseTag* opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone=(BaseTag*)opTag->GetClone(0,NULL);
				if (tagClone)
				{
					bone->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			
			bnData = bone->GetDataInstance();
			if(!bnData)
			{
				doc->EndUndo();
				StatusClear();
				return false;
			}
			bnData->SetBool(BONEOBJECT_NEWBONES,true);
			bnData->SetBool(BONEOBJECT_VERTEXMAPMODE,true);
			bnData->SetBool(ID_BASEOBJECT_GENERATOR_FLAG,false);
			
			ch = op->GetDown();
			if(ch)
			{
				bnLen = Len(ch->GetMg().off - op->GetMg().off);
				bnData->SetReal(BONEOBJECT_LENGTH,bnLen);
				bnData->SetBool(BONEOBJECT_NULL,false);
				if(!ChildCDJointsToBones(doc,op->GetDown(),bone))
				{
					doc->EndUndo();
					StatusClear();
					return true;
				}
			}
			else
			{
				BaseObject *bnPr = bone->GetUp();
				if(bnPr && CDIsBone(bnPr))
				{
					BaseContainer *bnprData = bnPr->GetDataInstance();
					if(bnprData)
					{
						bnLen = bnprData->GetReal(BONEOBJECT_LENGTH);
						bnData->SetReal(BONEOBJECT_LENGTH,bnLen);
					}
				}
				else bnData->SetReal(BONEOBJECT_LENGTH,200.0);
				bnData->SetBool(BONEOBJECT_NULL,true);
			}
		}
	}
	doc->EndUndo();
	StatusClear();

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}
#endif

#if API_VERSION >= 9800
void CDConvertFromCDJoints::TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk)
{
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_POSITION,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_SCALE,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_ROTATION,prvTrk);
}

void CDConvertFromCDJoints::TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	BaseList2D *prvTrk = NULL;
	
	if(src && dst)
	{
		if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,src);
		if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,dst);
		
		CDDeleteAllAnimationTracks(doc,dst);
		TransferPSRTracks(doc,src,dst,prvTrk);
		if(src->GetType() == dst->GetType())
		{
			TransferAMTracks(doc,src,dst,prvTrk);
		}
		TransferUDTracks(doc,src,dst,prvTrk);
	}
}

Bool CDConvertFromCDJoints::ChildCDJointsToJoints(BaseDocument *doc, BaseObject *op)
{
	if(!doc || !op) return false;
	
	while(op)
	{
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			Matrix opM = op->GetMg();
			String opName = op->GetName();
			
			BaseObject *jnt = BaseObject::Alloc(Ojoint);
			if(jnt)
			{
				jnt->SetName(opName);
				doc->InsertObject(jnt,op->GetUp(),op,false);
				jnt->Message(MSG_MENUPREPARE);
				
				jnt->SetMg(opM);
				CDAddUndo(doc,CD_UNDO_NEW,jnt);
				
				// Transfer Goals
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				CDTransferGoals(op,jnt);
				
				// Transfer User Data
				TransferUserData(doc,op,jnt);
				
				// Transfer tags
				TransferTags(doc,op,jnt);
				
				// Transfer animation tracks
				TransferAnimationTracks(doc,op,jnt);
				
				// Transfer children
				RepositionChildren(doc,op->GetDown(),jnt->GetMg(),op->GetMg(),true);
				TransferChildren(doc,op,jnt);
				
				CDAddUndo(doc,CD_UNDO_DELETE,op);
				BaseObject::Free(op);
				
				ChildCDJointsToJoints(doc, jnt->GetDown());
				op = jnt->GetNext();
			}
		}
		else
		{
			ChildCDJointsToJoints(doc,op->GetDown());
			op = op->GetNext();
		}
	}
	
	return true;
}

Bool CDConvertFromCDJoints::CDJointToJoint(BaseDocument *doc)
{
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_CDJOINT));
		return false;
	}
	
	StatusSetSpin();
	doc->StartUndo();
	
	LONG i, opCount = objects->GetCount();
	for(i=0; i<opCount; i++)
	{
		BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			Matrix opM = op->GetMg();
			String opName = op->GetName();
			
			BaseObject *jnt = BaseObject::Alloc(Ojoint);
			if(jnt)
			{
				jnt->SetName(opName);
				doc->InsertObject(jnt,op->GetUp(),op,false);
				jnt->Message(MSG_MENUPREPARE);
				
				jnt->SetMg(opM);
				CDAddUndo(doc,CD_UNDO_NEW,jnt);
				
				// Transfer Goals
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				CDTransferGoals(op,jnt);
				
				// Transfer User Data
				TransferUserData(doc,op,jnt);
				
				// Transfer tags
				TransferTags(doc,op,jnt);
				
				// Transfer animation tracks
				TransferAnimationTracks(doc,op,jnt);
				
				// Transfer children
				RepositionChildren(doc,op->GetDown(),jnt->GetMg(),op->GetMg(),true);
				TransferChildren(doc,op,jnt);
				
				CDAddUndo(doc,CD_UNDO_DELETE,op);
				BaseObject::Free(op);
				
				ChildCDJointsToJoints(doc, jnt->GetDown());
			}
		}
	}
	doc->EndUndo();
	StatusClear();
	
	EventAdd(EVENT_FORCEREDRAW);
	return true;
}

#endif

Bool CDConvertFromCDJoints::Execute(BaseDocument *doc)
{
	StopAllThreads(); // so the document can be safely modified
	
#if API_VERSION < 12000 && API_VERSION >= 9800
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	LONG convert = 0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDCONVERTFROMCDJOINTS);
	if(wpData) convert = wpData->GetLong(IDC_CDJS_OPTIONS_RB);
	
	switch(convert)
	{
		case 0:
			return CDJointToJoint(doc);
		case 1:
			return CDJointToBone(doc);
	}
#endif	
	
#if API_VERSION < 9800
	return CDJointToBone(doc);
#endif
	
#if API_VERSION >= 12000
	return CDJointToJoint(doc);
#endif
}

class CDConvertFromCDJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDConvertFromCDJoints(void)
{
	if(CDFindPlugin(1019259,CD_COMMAND_PLUGIN) && CDFindPlugin(ID_CDCONVERTFROMCDJOINTS,CD_COMMAND_PLUGIN)) return true;

	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDFBX_SERIAL_SIZE];
	String cdfnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE)) reg = false;
	
	cdfnr.SetCString(data,CDFBX_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdfnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdfnr.FindFirst("-",&pos);
	while(h)
	{
		cdfnr.Delete(pos,1);
		h = cdfnr.FindFirst("-",&pos);
	}
	cdfnr.ToUpper();
	kb = cdfnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDJTOB); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCONVERTFROMCDJOINTS,name,PLUGINFLAG_HIDE,"CDJoints-Bones.tif","CD Joints to Bones",CDDataAllocator(CDConvertFromCDJointsR));
	else return CDRegisterCommandPlugin(ID_CDCONVERTFROMCDJOINTS,name,0,"CDJoints-Bones.tif","CD Joints to Bones",CDDataAllocator(CDConvertFromCDJoints));
}
