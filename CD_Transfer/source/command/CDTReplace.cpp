//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDReplaceDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDReplaceDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDREPLTRANS));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDTR_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDTR_OPTIONS_IMAGE);
		}
		GroupEnd();

		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_REPLACE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_MIRROR,BFH_CENTER,2,1);
					AddChild(CDTR_MIRROR, 0, GeLoadString(CDTR_SINGLE));
					AddChild(CDTR_MIRROR, 1, GeLoadString(CDTR_MULTIPLE));

			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_TRANSFER),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,4,0,"",0);
				{
					AddCheckbox(CDTR_POS,BFH_LEFT,0,0,GeLoadString(CDTR_POS));
					AddCheckbox(CDTR_POS_X,BFH_LEFT,0,0,GeLoadString(CDTR_POS_X));
					AddCheckbox(CDTR_POS_Y,BFH_LEFT,0,0,GeLoadString(CDTR_POS_Y));
					AddCheckbox(CDTR_POS_Z,BFH_LEFT,0,0,GeLoadString(CDTR_POS_Z));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_SCA,BFH_LEFT,0,0,GeLoadString(CDTR_SCA));
					AddCheckbox(CDTR_ROT,BFH_LEFT,0,0,GeLoadString(CDTR_ROT));
				}
				GroupEnd();

				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					AddCheckbox(CDTR_NAME,BFH_LEFT,0,0,GeLoadString(CDTR_NAME));
				}
				GroupEnd();
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDReplaceDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDREPLACETRANSCOM);
	if(!bc)
	{
		SetBool(CDTR_POS,true);
		SetBool(CDTR_POS_X,true);
		SetBool(CDTR_POS_Y,true);
		SetBool(CDTR_POS_Z,true);
		SetBool(CDTR_SCA,true);
		SetBool(CDTR_ROT,true);
		SetBool(CDTR_NAME,true);
		SetLong(CDTR_MIRROR,0);
	}
	else
	{
		SetBool(CDTR_POS,bc->GetBool(CDTR_POS));
		SetBool(CDTR_POS_X,bc->GetBool(CDTR_POS_X));
		SetBool(CDTR_POS_Y,bc->GetBool(CDTR_POS_Y));
		SetBool(CDTR_POS_Z,bc->GetBool(CDTR_POS_Z));
		SetBool(CDTR_SCA,bc->GetBool(CDTR_SCA));
		SetBool(CDTR_ROT,bc->GetBool(CDTR_ROT));
		SetBool(CDTR_NAME,bc->GetBool(CDTR_NAME));
		SetLong(CDTR_MIRROR,bc->GetLong(CDTR_MIRROR));
	}
		
	DoEnable();
	
	return true;
}

Bool CDReplaceDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool pos, sca, rot, px, py, pz, name;
	LONG mir;
	
	GetBool(CDTR_POS,pos);
	GetBool(CDTR_POS_X,px);
	GetBool(CDTR_POS_Y,py);
	GetBool(CDTR_POS_Z,pz);
	GetBool(CDTR_SCA,sca);
	GetBool(CDTR_ROT,rot);
	GetBool(CDTR_NAME,name);
	GetLong(CDTR_MIRROR,mir);
	
	BaseContainer bc;
	bc.SetBool(CDTR_POS,pos);
	bc.SetBool(CDTR_POS_X,px);
	bc.SetBool(CDTR_POS_Y,py);
	bc.SetBool(CDTR_POS_Z,pz);
	bc.SetBool(CDTR_SCA,sca);
	bc.SetBool(CDTR_ROT,rot);
	bc.SetBool(CDTR_NAME,name);
	bc.SetLong(CDTR_MIRROR,mir);
	SetWorldPluginData(ID_CDREPLACETRANSCOM,bc,false);
	
	DoEnable();
	
	return true;
}

void CDReplaceDialog::DoEnable(void)
{
	Bool pos;
	
	GetBool(CDTR_POS,pos);
	if(pos)
	{
		Enable(CDTR_POS_X,true);
		Enable(CDTR_POS_Y,true);
		Enable(CDTR_POS_Z,true);
	}
	else
	{
		Enable(CDTR_POS_X,false);
		Enable(CDTR_POS_Y,false);
		Enable(CDTR_POS_Z,false);
	}
}

class CDReplaceCommand : public CommandData
{
	private:
		CDReplaceDialog dlg;
		BaseList2D *prvTrk;

		void TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst);
		void TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst);

	public:
		Bool DoReplace(BaseDocument *doc, BaseContainer *data, Bool shift);
		virtual Bool Execute(BaseDocument* doc);

};

void CDReplaceCommand::TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_POSITION,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_SCALE,prvTrk);
	CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_ROTATION,prvTrk);
}

void CDReplaceCommand::TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	prvTrk = NULL;
	
	if(src && dst)
	{
		if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,src);
		if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,dst);
		
		CDDeleteAllAnimationTracks(doc,dst);
		TransferPSRTracks(doc,src,dst);
		if(src->GetType() == dst->GetType())
		{
			TransferAMTracks(doc,src,dst,prvTrk);
		}
		TransferUDTracks(doc,src,dst,prvTrk);
	}
}

Bool CDReplaceCommand::DoReplace(BaseDocument *doc, BaseContainer *data, Bool shift)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG i, opCnt = opSelLog->GetCount();
	if(opCnt > 2 && data->GetLong(CDTR_MIRROR) == 0)
	{
		MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
		doc->SetActiveObject(NULL);
	}
	else if(opCnt < 2)
	{
		MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
	}
	else
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!op) return false;
		
		BaseObject *trg = NULL, *opClone = NULL;
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();
		
		for(i=1; i<opCnt; i++)
		{
			trg = static_cast<BaseObject*>(opSelLog->GetIndex(i));
			if(trg)
			{
				// Check for parent/child flip
				if(IsParentObject(trg,op))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,trg);
					trg->Remove();
					doc->InsertObject(trg,op->GetUp(),op,false);
				}
				else if(IsParentObject(op,trg))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					op->Remove();
				}
				
				AutoAlloc<AliasTrans> trans;
				if(trans)
				{
					if(trans->Init(doc))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						if(CDIsAxisMode(doc))
						{
							if(IsValidPointObject(op))
							{
								if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
								{
									RecalculateReference(doc,op,trg->GetMg(),op->GetMg());
								}
								else RecalculatePoints(op,trg->GetMg(),op->GetMg());
							}
							
							if(op->GetDown()) RepositionChildren(doc,op->GetDown(),trg->GetMg(),op->GetMg(),true);
						}
						else if(shift) RepositionChildren(doc,op->GetDown(),trg->GetMg(),op->GetMg(),true);
						
						opClone = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,trans);
						if(opClone)
						{
							trans->Translate(true);
							doc->InsertObject(opClone,trg->GetUp(),trg,false);
							CDAddUndo(doc,CD_UNDO_NEW,opClone);
							
							if(data->GetBool(CDTR_POS)) CDSetPos(opClone,CDGetPos(trg));
							if(data->GetBool(CDTR_ROT)) CDSetRot(opClone,CDGetRot(trg));
							if(data->GetBool(CDTR_SCA)) CDSetScale(opClone,CDGetScale(trg));
							if(data->GetBool(CDTR_NAME)) opClone->SetName(trg->GetName());
							
							// Transfer Goals
							CDAddUndo(doc,CD_UNDO_CHANGE,trg);
							CDTransferGoals(trg,opClone);
							
							// Transfer User Data
							TransferUserData(doc,trg,opClone);
							
							// Transfer tags
							TransferTags(doc,trg,opClone);
							
							// Transfer animation tracks
							TransferAnimationTracks(doc,trg,opClone);
							
							// Transfer children
							RepositionChildren(doc,trg->GetDown(),opClone->GetMg(),trg->GetMg(),true);
							TransferChildren(doc,trg,opClone);
							
							CDAddUndo(doc,CD_UNDO_DELETE,trg);
							BaseObject::Free(trg);
							
						}
					}
				}
			}
		}
		doc->SetActiveObject(opClone);
		CDAddUndo(doc,CD_UNDO_DELETE,op);
		BaseObject::Free(op);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

Bool CDReplaceCommand::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	}
	Bool shift = (state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);

	BaseContainer *wpData = GetWorldPluginData(ID_CDREPLACETRANSCOM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetBool(CDTR_POS,true);
		bc.SetBool(CDTR_POS_X,true);
		bc.SetBool(CDTR_POS_Y,true);
		bc.SetBool(CDTR_POS_Z,true);
		bc.SetBool(CDTR_SCA,true);
		bc.SetBool(CDTR_ROT,true);
		bc.SetBool(CDTR_NAME,true);
		bc.SetLong(CDTR_MIRROR,0);
		SetWorldPluginData(ID_CDREPLACETRANSCOM,bc,false);
		wpData = GetWorldPluginData(ID_CDREPLACETRANSCOM);
	}

	return DoReplace(doc, wpData, shift);
}

class CDReplaceCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDReplaceCommand(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDREPLTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDREPLACETRANSCOM,name,PLUGINFLAG_HIDE,"CDTReplace.tif","CD Transfer Replace",CDDataAllocator(CDReplaceCommandR));
	else return CDRegisterCommandPlugin(ID_CDREPLACETRANSCOM,name,0,"CDTReplace.tif","CD Transfer Replace"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDReplaceCommand));
}
