//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"

class CDTransAnimDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTransAnimDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDTRANSANIM));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_MODE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_MODE,BFH_CENTER,2,1);
					AddChild(CDTR_MODE, 0, GeLoadString(CDTR_SINGLE));
					AddChild(CDTR_MODE, 1, GeLoadString(CDTR_ANIM_HRCHY));

			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_TRANSFER),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_ANIM_POS,BFH_LEFT,0,0,GeLoadString(CDTR_ANIM_POS));
					AddCheckbox(CDTR_ANIM_SCA,BFH_LEFT,0,0,GeLoadString(CDTR_ANIM_SCA));
					AddCheckbox(CDTR_ANIM_ROT,BFH_LEFT,0,0,GeLoadString(CDTR_ANIM_ROT));
					AddCheckbox(CDTR_ANIM_AM,BFH_LEFT,0,0,GeLoadString(CDTR_ANIM_AM));
					AddCheckbox(CDTR_ANIM_UD,BFH_LEFT,0,0,GeLoadString(CDTR_ANIM_UD));
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

Bool CDTransAnimDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDTRANSFERANIM);
	if(!bc)
	{
		SetLong(CDTR_MODE,0);
		SetBool(CDTR_ANIM_POS,true);
		SetBool(CDTR_ANIM_SCA,true);
		SetBool(CDTR_ANIM_ROT,true);
		SetBool(CDTR_ANIM_AM,false);
		SetBool(CDTR_ANIM_UD,false);
	}
	else
	{
		SetLong(CDTR_MODE,bc->GetLong(CDTR_MODE));
		SetBool(CDTR_ANIM_POS,bc->GetBool(CDTR_ANIM_POS));
		SetBool(CDTR_ANIM_SCA,bc->GetBool(CDTR_ANIM_SCA));
		SetBool(CDTR_ANIM_ROT,bc->GetBool(CDTR_ANIM_ROT));
		SetBool(CDTR_ANIM_AM,bc->GetBool(CDTR_ANIM_AM));
		SetBool(CDTR_ANIM_UD,bc->GetBool(CDTR_ANIM_UD));
	}
		
	return true;
}

Bool CDTransAnimDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool pos, sca, rot, am, ud;
	LONG mir;
	
	GetLong(CDTR_MODE,mir);
	GetBool(CDTR_ANIM_POS,pos);
	GetBool(CDTR_ANIM_SCA,sca);
	GetBool(CDTR_ANIM_ROT,rot);
	GetBool(CDTR_ANIM_AM,am);
	GetBool(CDTR_ANIM_UD,ud);
	
	BaseContainer bc;
	bc.SetLong(CDTR_MODE,mir);
	bc.SetBool(CDTR_ANIM_POS,pos);
	bc.SetBool(CDTR_ANIM_SCA,sca);
	bc.SetBool(CDTR_ANIM_ROT,rot);
	bc.SetBool(CDTR_ANIM_AM,am);
	bc.SetBool(CDTR_ANIM_UD,ud);
	SetWorldPluginData(ID_CDTRANSFERANIM,bc,false);
	
	return true;
}

class CDTransferAnimation : public CommandData
{
private:
	CDTransAnimDialog dlg;
	BaseList2D *prvTrk;
	Bool enableUD;
	
	void TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data);
	void TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data);
	void TransferTagTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data);
	Bool DoAnimationTransfer(BaseDocument *doc, BaseContainer *data);
	
public:		
	virtual Bool Execute(BaseDocument* doc);
};

void CDTransferAnimation::TransferPSRTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data)
{
	if(data->GetBool(CDTR_ANIM_POS)) CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_POSITION,NULL);
	if(data->GetBool(CDTR_ANIM_SCA)) CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_SCALE,NULL);
	if(data->GetBool(CDTR_ANIM_ROT)) CDCopyVectorTracks(doc,src,dst,CD_ID_BASEOBJECT_ROTATION,NULL);
}

void CDTransferAnimation::TransferTagTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data)
{
	BaseTag *sTag = src->GetFirstTag();
	BaseTag *dTag = dst->GetFirstTag();
	
	while(sTag && dTag)
	{
		if(sTag->GetType() == dTag->GetType())
		{
			TransferAMTracks(doc, sTag, dTag, NULL);
		}
		sTag = sTag->GetNext();
		dTag = dTag->GetNext();
	}
}

void CDTransferAnimation::TransferAnimationTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseContainer *data)
{
	prvTrk = NULL;
	
	switch(data->GetLong(CDTR_MODE))
	{
		case 0:
		{
			if(src && dst)
			{
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,src);
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,dst);
				
				CDDeleteAllAnimationTracks(doc,dst);
				TransferPSRTracks(doc,src,dst,data);
				if(src->GetType() == dst->GetType())
				{
					if(data->GetBool(CDTR_ANIM_AM))
					{
						TransferAMTracks(doc,src,dst,prvTrk);
						TransferTagTracks(doc,src,dst,data);
					}
				}
				if(enableUD && data->GetBool(CDTR_ANIM_UD)) TransferUDTracks(doc,src,dst,prvTrk);
			}
			
			break;
		}
		case 1:
		{
			while(src && dst)
			{
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,src);
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,dst);
				
				CDDeleteAllAnimationTracks(doc,dst);
				TransferPSRTracks(doc,src,dst,data);
				if(src->GetType() == dst->GetType())
				{
					if(data->GetBool(CDTR_ANIM_AM))
					{
						TransferAMTracks(doc,src,dst,prvTrk);
						TransferTagTracks(doc,src,dst,data);
					}
				}
				if(enableUD && data->GetBool(CDTR_ANIM_UD)) TransferUDTracks(doc,src,dst,prvTrk);
				
				TransferAnimationTracks(doc,src->GetDown(),dst->GetDown(),data);
				src = src->GetNext();
				dst = dst->GetNext();
			}
			break;
		}
	}
}

Bool CDTransferAnimation::DoAnimationTransfer(BaseDocument *doc, BaseContainer *data)
{
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	
	LONG  selCount = opSelectionList->GetCount();
	if(selCount > 2)
	{
		MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
		doc->SetActiveObject(NULL);
	}
	else if(selCount < 2)
	{
		MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
	}
	else
	{
		BaseObject *op = static_cast<BaseObject*>(opSelectionList->GetIndex(0)); 
		if(!op) return false;
		BaseObject *trg = static_cast<BaseObject*>(opSelectionList->GetIndex(1));
		if(!trg) return false;
					
		Bool hrchl = false;
		if(data->GetLong(CDTR_MODE) == 1) hrchl = true;
		
		// check if hierarchy is the same
		if(hrchl)
		{
			if(!IsHierarchyEqual(op,trg))
			{
				if(!QuestionDialog(GeLoadString(MD_HIERARCHY_NOT_SAME))) return false;
			}
		}
		
		// check if User Data is the same
		enableUD = true;
		if(!IsUserDataEqual(op,trg,hrchl))
		{
			if(!QuestionDialog(GeLoadString(MD_USER_DATA_NOT_SAME))) enableUD = false;
		}
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();
		
		// Transfer animation
		TransferAnimationTracks(doc,op,trg,data);
		
		doc->SetActiveObject(trg);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

Bool CDTransferAnimation::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}

	BaseContainer *wpData = GetWorldPluginData(ID_CDTRANSFERANIM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetLong(CDTR_MODE,0);
		bc.SetBool(CDTR_ANIM_POS,true);
		bc.SetBool(CDTR_ANIM_SCA,true);
		bc.SetBool(CDTR_ANIM_ROT,true);
		bc.SetBool(CDTR_ANIM_AM,false);
		bc.SetBool(CDTR_ANIM_UD,false);
		SetWorldPluginData(ID_CDTRANSFERANIM,bc,false);
		wpData = GetWorldPluginData(ID_CDTRANSFERANIM);
	}

	return DoAnimationTransfer(doc,wpData);
}

class CDTransferAnimationR : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTransferAnimation(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDTRANSANIM); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTRANSFERANIM,name,PLUGINFLAG_HIDE,"CDTransAnim.tif","CD Transfer Animation",CDDataAllocator(CDTransferAnimationR));
	else return CDRegisterCommandPlugin(ID_CDTRANSFERANIM,name,0,"CDTransAnim.tif","CD Transfer Animation",CDDataAllocator(CDTransferAnimation));
}
