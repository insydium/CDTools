//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDFBX.h"

class CDConvertToCDJointsDialog : public GeModalDialog
{
private:
	CDFbxOptionsUA ua;
	
public:	
	LONG opt;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDConvertToCDJointsDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDBTOJ));
		
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

Bool CDConvertToCDJointsDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCONVERTTOCDJOINTS);
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

Bool CDConvertToCDJointsDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_CDJS_OPTIONS_RB,opt);
	
	BaseContainer bc;
	bc.SetLong(IDC_CDJS_OPTIONS_RB,opt);
	SetWorldPluginData(ID_CDCONVERTTOCDJOINTS,bc,false);
	
	return true;
}

class CDConvertToCDJoints : public CommandData
{
private:
	CDConvertToCDJointsDialog dlg;
	
#if API_VERSION < 12000
	Bool ChildBonesToCDJoints(BaseDocument *doc, BaseObject *op, BaseObject *jnt, Bool tpef);
	Bool BoneToCDJoint(BaseDocument *doc);
#endif
	
#if API_VERSION >= 9800
	void TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk);
	void TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst);
	
	Bool ChildJointsToCDJoints(BaseDocument *doc, BaseObject *jnt);
	Bool JointToCDJoint(BaseDocument *doc);
#endif
	
public:
	virtual Bool Execute(BaseDocument *doc);
	
};

#if API_VERSION < 12000
Bool CDConvertToCDJoints::ChildBonesToCDJoints(BaseDocument *doc, BaseObject *op, BaseObject *jnt, Bool tpef)
{	BaseObject *prev=NULL, *joint=NULL, *prevCH=NULL, *ch=NULL, *jointTip=NULL;
	BaseTag *opTag = NULL, *tagClone = NULL;
	BaseContainer *jData=NULL, *tData=NULL, *opData = op->GetDataInstance();
	String opName;
	Matrix opMatrix;
	Real bnLen;
	Vector tipPos;
	Bool atTip=false, isBone=false, tipEffector=false;
	
	StatusSetSpin();
	while(op && jnt)
	{
		opName = op->GetName();
		opMatrix = op->GetMg();
		
		prevCH = jnt->GetDown();
		while(prevCH)
		{
			prev = prevCH;
			prevCH = prevCH->GetNext();
		}
		if(CDIsBone(op))
		{
			joint = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!joint) return false;
			joint->SetName(opName);
			
			doc->InsertObject(joint,jnt,prev,false);
			joint->Message(MSG_MENUPREPARE);
			
			joint->SetMg(opMatrix);
			CDAddUndo(doc,CD_UNDO_NEW,joint);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,joint);
			for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone=(BaseTag*)opTag->GetClone(0,NULL);
				if (tagClone)
				{
					joint->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			
			BaseObject *pr = op->GetUp();
			if(pr)
			{
				BaseContainer *prData = pr->GetDataInstance();
				if(prData)
				{
					if(prData->GetBool(BONEOBJECT_NULL))
					{
						jData = joint->GetDataInstance();
						jData->SetBool(1001,true);
					}
				}
			}
			
			if(opData)
			{
				bnLen = opData->GetReal(BONEOBJECT_LENGTH);
				tipPos = Vector(0,0,bnLen);
				
				if(!opData->GetBool(BONEOBJECT_NULL))
				{
					ch = op->GetDown();
					if(!ch)
					{
						jointTip = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!jointTip) return false;
						jointTip->SetName(opName + ".Tip");
						
						doc->InsertObject(jointTip,joint,NULL,false);
						jointTip->Message(MSG_MENUPREPARE);
						
						jointTip->SetMg(opMatrix);
						CDSetPos(jointTip,tipPos);
						CDAddUndo(doc,CD_UNDO_NEW,jointTip);
						
						tData = jointTip->GetDataInstance();
						tData->SetBool(1001,true);
					}
					else
					{
						atTip = false;
						while(ch)
						{
							if(Len(CDGetPos(ch) - tipPos) < 1) atTip = true;
							ch = ch->GetNext();
						}
						if(!atTip)
						{
							jointTip = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!jointTip) return false;
							jointTip->SetName(opName + ".Tip");
							
							doc->InsertObject(jointTip,joint,NULL,false);
							jointTip->Message(MSG_MENUPREPARE);
							
							jointTip->SetMg(opMatrix);
							CDSetPos(jointTip,tipPos);
							CDAddUndo(doc,CD_UNDO_NEW,jointTip);
							
							tData = jointTip->GetDataInstance();
							tData->SetBool(1001,true);
						}
						else
						{
							ch = op->GetDown();
							isBone = false;
							while(ch)
							{
								if(CDIsBone(ch) && (Len(CDGetPos(ch) - tipPos) < 1)) isBone = true;
								ch = ch->GetNext();
							}
							if(!isBone)
							{
								if(!opData->GetBool(BONEOBJECT_NULL))
								{
									jointTip = BaseObject::Alloc(ID_CDJOINTOBJECT); if (!jointTip) return false;
									jointTip->SetName(opName + ".Tip");
									
									doc->InsertObject(jointTip,joint,NULL,false);
									jointTip->Message(MSG_MENUPREPARE);
									
									jointTip->SetMg(opMatrix);
									CDSetPos(jointTip,tipPos);
									CDAddUndo(doc,CD_UNDO_NEW,jointTip);
									
									tData = jointTip->GetDataInstance();
									tData->SetBool(1001,true);
									ch = op->GetDown();
									if(!ch->GetDown()) tipEffector = true;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if(!tpef)
			{
				joint=(BaseObject*)op->GetClone(COPY_NO_HIERARCHY|COPY_NO_BRANCHES|COPY_NO_INTERNALS,NULL);
				if (joint)
				{
					doc->InsertObject(joint,jnt,prev,false);
					joint->Message(MSG_MENUPREPARE);
					
					joint->SetMg(opMatrix);
					CDAddUndo(doc,CD_UNDO_NEW,joint);
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					CDTransferGoals(op,joint);
					for (BaseTag* opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
					{
						tagClone=(BaseTag*)opTag->GetClone(0,NULL);
						if (tagClone)
						{
							joint->InsertTag(tagClone,NULL);
							CDAddUndo(doc,CD_UNDO_NEW,tagClone);
							CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
							CDTransferGoals(opTag,tagClone);
						}
					}
				}
			}
			else tpef = false;
		}
		prev = joint;
		ChildBonesToCDJoints(doc,op->GetDown(),joint,tipEffector);
		op = op->GetNext();
	}
	return true;
}

Bool CDConvertToCDJoints::BoneToCDJoint(BaseDocument *doc)
{
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	doc->GetActiveObjects(objects,false);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_BONE));
		return false;
	}
	LONG i, opCount = objects->GetCount();
	BaseObject *jnt = NULL, *jointTip = NULL;
	BaseTag *opTag=NULL, *tagClone=NULL;
	Matrix opM;
	String opName;
	Real bnLen;
	
	StatusSetSpin();
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && CDIsBone(op))
		{
			opM = op->GetMg();
			opName = op->GetName();
			
			jnt = BaseObject::Alloc(ID_CDJOINTOBJECT);
			if(!jnt)
			{
				doc->EndUndo();
				StatusClear();
				return false;
			}
			jnt->SetName(opName);
			doc->InsertObject(jnt,op->GetUp(),op,false);
			jnt->Message(MSG_MENUPREPARE);
			
			jnt->SetMg(opM);
			CDAddUndo(doc,CD_UNDO_NEW,jnt);
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,jnt);
			for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone=(BaseTag*)opTag->GetClone(0,NULL);
				if (tagClone)
				{
					jnt->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			if(op->GetDown())
			{
				if(!ChildBonesToCDJoints(doc,op->GetDown(),jnt,false))
				{
					doc->EndUndo();
					StatusClear();
					return true;
				}
			}
			else
			{
				BaseContainer *opData = op->GetDataInstance();
				if(opData)
				{
					if(!opData->GetBool(BONEOBJECT_NULL))
					{
						jointTip = BaseObject::Alloc(ID_CDJOINTOBJECT);
						if(!jnt)
						{
							doc->EndUndo();
							StatusClear();
							return false;
						}
						jointTip->SetName(opName + ".Tip");
						doc->InsertObject(jointTip,jnt,NULL,false);
						jointTip->Message(MSG_MENUPREPARE);
						
						bnLen = opData->GetReal(BONEOBJECT_LENGTH);
						jointTip->SetMg(opM);
						CDSetPos(jointTip,Vector(0,0,bnLen));
						CDAddUndo(doc,CD_UNDO_NEW,jointTip);
					}
				}
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
void CDConvertToCDJoints::TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk)
{
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_POSITION,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_SCALE,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_ROTATION,prvTrk);
}

void CDConvertToCDJoints::TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst)
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

Bool CDConvertToCDJoints::ChildJointsToCDJoints(BaseDocument *doc, BaseObject *op)
{
	if(!doc || !op) return false;
	
	while(op)
	{
		if(op->IsInstanceOf(Ojoint))
		{
			Matrix opM = op->GetMg();
			String opName = op->GetName();
			
			BaseObject *jnt = BaseObject::Alloc(ID_CDJOINTOBJECT);
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
				
				ChildJointsToCDJoints(doc, jnt->GetDown());
				op = jnt->GetNext();
			}
		}
		else
		{
			ChildJointsToCDJoints(doc,op->GetDown());
			op = op->GetNext();
		}
	}
	
	return true;
}

Bool CDConvertToCDJoints::JointToCDJoint(BaseDocument *doc)
{
	AutoAlloc<AtomArray> objects; if (!objects) return false;
    CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);	
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_JOINT));
		return false;
	}
	
	StatusSetSpin();
	doc->StartUndo();
	
	LONG i, opCount = objects->GetCount();
	for(i=0; i<opCount; i++)
	{
		BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->IsInstanceOf(Ojoint))
		{
			Matrix opM = op->GetMg();
			String opName = op->GetName();
			
			BaseObject *jnt = BaseObject::Alloc(ID_CDJOINTOBJECT);
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
				
				ChildJointsToCDJoints(doc, jnt->GetDown());
				
				CDAddUndo(doc,CD_UNDO_DELETE,op);
				BaseObject::Free(op);
			}
		}
	}
	doc->EndUndo();
	StatusClear();
	
	
	EventAdd(EVENT_FORCEREDRAW);
	return true;
}
#endif	

Bool CDConvertToCDJoints::Execute(BaseDocument *doc)
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
	BaseContainer *wpData = GetWorldPluginData(ID_CDCONVERTTOCDJOINTS);
	if(wpData) convert = wpData->GetLong(IDC_CDJS_OPTIONS_RB);
	
	switch(convert)
	{
		case 0:
			return JointToCDJoint(doc);
		case 1:
			return BoneToCDJoint(doc);
	}
#endif	
	
#if API_VERSION < 9800
	return BoneToCDJoint(doc);
#endif
	
#if API_VERSION >= 12000
	return JointToCDJoint(doc);
#endif
}

class CDConvertToCDJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDConvertToCDJoints(void)
{
	if(CDFindPlugin(1019259,CD_COMMAND_PLUGIN) && CDFindPlugin(ID_CDCONVERTTOCDJOINTS,CD_COMMAND_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDBTOJ); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCONVERTTOCDJOINTS,name,PLUGINFLAG_HIDE,"CDBones-Joints.TIF","CD Convert to CDJoints",CDDataAllocator(CDConvertToCDJointsR));
	else return CDRegisterCommandPlugin(ID_CDCONVERTTOCDJOINTS,name,0,"CDBones-Joints.TIF","CD Convert to CDJoints",CDDataAllocator(CDConvertToCDJoints));
}
