//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTAimDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTAimDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDAIMTRANS));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_A_GROUP),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_A_GROUP,BFH_CENTER,2,1);
					AddChild(CDTR_A_GROUP, 0, GeLoadString(CDTR_TO_GROUP));
					AddChild(CDTR_A_GROUP, 1, GeLoadString(CDTR_TO_LAST));

			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_AIM),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_AXIS,BFH_CENTER,3,1);
					AddChild(CDTR_AXIS, 0, GeLoadString(CDTR_MIR_X));
					AddChild(CDTR_AXIS, 1, GeLoadString(CDTR_MIR_Y));
					AddChild(CDTR_AXIS, 2, GeLoadString(CDTR_MIR_Z));

				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(HIERARCHAL_ALIGN,BFH_LEFT,0,0,GeLoadString(HIERARCHAL_ALIGN));
				}
				GroupEnd();
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_KEYS),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_AUTO_KEY,BFH_LEFT,0,0,GeLoadString(CDTR_AUTO_KEY));
					AddCheckbox(CDTR_KEEP_SELECTION,BFH_LEFT,0,0,GeLoadString(CDTR_KEEP_SELECTION));
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

Bool CDTAimDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDAIMTRANSFERCOM);
	if(!bc)
	{
		SetLong(CDTR_A_GROUP,0);
		SetLong(CDTR_AXIS,2);
		SetBool(CDTR_AUTO_KEY,false);
		SetBool(CDTR_KEEP_SELECTION,false);
		SetBool(HIERARCHAL_ALIGN,false);
	}
	else
	{
		SetLong(CDTR_A_GROUP,bc->GetLong(CDTR_A_GROUP));
		SetLong(CDTR_AXIS,bc->GetLong(CDTR_AXIS));
		SetBool(CDTR_AUTO_KEY,bc->GetBool(CDTR_AUTO_KEY));
		SetBool(CDTR_KEEP_SELECTION,bc->GetBool(CDTR_KEEP_SELECTION));
		SetBool(HIERARCHAL_ALIGN,bc->GetBool(HIERARCHAL_ALIGN));
	}
	
	DoEnable();
		
	return true;
}

Bool CDTAimDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool sel, akey, hrchy;
	LONG axis, grp;
	
	GetLong(CDTR_A_GROUP,grp);
	GetLong(CDTR_AXIS,axis);
	GetBool(CDTR_AUTO_KEY,akey);
	GetBool(CDTR_KEEP_SELECTION,sel);
	GetBool(HIERARCHAL_ALIGN,hrchy);

	BaseContainer bc;
	bc.SetLong(CDTR_A_GROUP,grp);
	bc.SetLong(CDTR_AXIS,axis);
	bc.SetBool(CDTR_AUTO_KEY,akey);
	bc.SetBool(CDTR_KEEP_SELECTION,sel);
	bc.SetBool(HIERARCHAL_ALIGN,hrchy);
	SetWorldPluginData(ID_CDAIMTRANSFERCOM,bc,false);
	
	DoEnable();

	return true;
}

void CDTAimDialog::DoEnable(void)
{
	BaseDocument *doc = GetActiveDocument();
	AtomArray *opSelLog = GetSelectionLog(doc);
	BaseContainer *wpData = GetWorldPluginData(ID_CDAIMTRANSFERCOM);
	if(opSelLog)
	{
		Bool children = false;
		LONG i, opCnt = opSelLog->GetCount();
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(i));
			if(op)
			{
				if(op->GetDown()) children = true;
			}
		}
		if(children) Enable(HIERARCHAL_ALIGN,true);
		else
		{
			SetBool(HIERARCHAL_ALIGN,false);
			if(wpData) wpData->SetBool(HIERARCHAL_ALIGN,false);
			Enable(HIERARCHAL_ALIGN,false);
		}
		
		if(opCnt > 2)
		{
			Enable(CDTR_A_GROUP,true);
			if(CDTR_A_GROUP == 0) Enable(CDTR_AUTO_KEY,true);
			else
			{
				SetBool(CDTR_AUTO_KEY,false);
				if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
				Enable(CDTR_AUTO_KEY,false);
			}
		}
		else
		{
			Enable(CDTR_A_GROUP,false);
			if(opCnt > 1) Enable(CDTR_AUTO_KEY,true);
			else
			{
				SetBool(CDTR_AUTO_KEY,false);
				if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
				Enable(CDTR_AUTO_KEY,false);
			}
		}
	}
	if(wpData)
	{
		if(!wpData->GetBool(CDTR_AUTO_KEY))
		{
			SetBool(CDTR_KEEP_SELECTION,false);
			Enable(CDTR_KEEP_SELECTION,false);
		}
		else Enable(CDTR_KEEP_SELECTION,true);
	}
}

class CDTAimCommand : public CommandData
{
	private:
		CDTAimDialog dlg;

	public:
		Bool DoAlign(BaseDocument *doc, BaseContainer *data, Bool shift, Bool optn);
		Bool DoAlignChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data);
		Matrix GetTransferMatrix(BaseContainer *data, Matrix src, Matrix dst);

		virtual Bool Execute(BaseDocument* doc);

};

Matrix CDTAimCommand::GetTransferMatrix(BaseContainer *data, Matrix src, Matrix dst)
{
	Matrix transM = src;
	if(dst.off != src.off)
	{
		switch(data->GetLong(CDTR_AXIS))
		{
			case 0:
			{
				transM.v1 = VNorm(dst.off - src.off);
				if(Abs(VDot(VNorm(transM.v1), VNorm(transM.v3))) > 0.707)
				{
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
				}
				else
				{
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
				}
				break;
			}
			case 1:
			{
				transM.v2 = VNorm(dst.off - src.off);
				if(Abs(VDot(VNorm(transM.v2), VNorm(transM.v3))) > 0.707)
				{
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
				}
				else
				{
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
					transM.v3 = VCross(VNorm(transM.v1), VNorm(transM.v2)) * Len(src.v3);
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
				}
				break;
			}
			case 2:
			{
				transM.v3 = VNorm(dst.off - src.off);
				if(Abs(VDot(VNorm(transM.v3), VNorm(transM.v2))) > 0.707)
				{
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
				}
				else
				{
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
					transM.v2 = VCross(VNorm(transM.v3), VNorm(transM.v1)) * Len(src.v2);
					transM.v1 = VCross(VNorm(transM.v2), VNorm(transM.v3)) * Len(src.v1);
				}
				break;
			}
		}
	}
	
	return transM;
}

Bool CDTAimCommand::DoAlignChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data)
{
	while(op)
	{
		BaseObject *ch = op->GetDown();
		if(ch)
		{
			Vector theScale = CDGetScale(op);
			Matrix chM = ch->GetMg(), opM = op->GetMg(), transM;
			
			transM = GetTransferMatrix(data,opM,chM);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			if(CDIsAxisMode(doc))
			{
				if(IsValidPointObject(op))
				{
					if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
					{
						RecalculateReference(doc,op,transM,opM);
					}
					else RecalculatePoints(op,transM,opM);
				}
				RepositionChildren(doc,ch,transM,opM,true);
				op->SetMg(transM);
			}
			else
			{
				RepositionChildren(doc,ch,transM,opM,true);
				op->SetMg(transM);
			}
			
			DoAlignChildren(doc,ch,data);
		}
		op = op->GetNext();
	}
	
	return true;
}

Bool CDTAimCommand::DoAlign(BaseDocument *doc, BaseContainer *data, Bool shift, Bool optn)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	BaseObject *undoOp = NULL;
	
	LONG  i, opCnt = opSelLog->GetCount();
	if(opCnt == 0) return false;
	
	if(data->GetBool(HIERARCHAL_ALIGN))
	{
		doc->StartUndo();
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(i)); 
			if(op)
			{
				BaseObject *ch = op->GetDown();
				if(ch)
				{
					Vector theScale = CDGetScale(op);
					Matrix chM = ch->GetMg(), opM = op->GetMg(), transM;
					
					transM = GetTransferMatrix(data,opM,chM);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					if(CDIsAxisMode(doc))
					{
						if(IsValidPointObject(op))
						{
							if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
							{
								RecalculateReference(doc,op,transM,opM);
							}
							else RecalculatePoints(op,transM,opM);
						}
						
						RepositionChildren(doc,ch,transM,opM,true);
						op->SetMg(transM);
					}
					else
					{
						RepositionChildren(doc,ch,transM,opM,true);
						op->SetMg(transM);
					}
					
					DoAlignChildren(doc,ch,data);
				}
			}
		}
		doc->EndUndo();
		EventAdd(EVENT_FORCEREDRAW);
	}
	else
	{
		if(opCnt > 1)
		{
			BaseObject *target = NULL;
			BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
			if(!op) return false;
			Vector theScale = CDGetScale(op);
			
			Matrix targM, opM = op->GetMg(), transM;
			if(doc->IsEditMode())
			{
				if(opCnt < 3)
				{
					target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
					if(!target) return false;
					
					if(IsValidPointObject(target))
					{
						targM = GetEditModeTransferMatrix(doc,target);
					}
					else
					{
						targM = target->GetMg();
					}
				}
				else
				{
					MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
					doc->SetActiveObject(NULL);
					return false;
				}
			}
			else
			{
				if(opCnt > 2)
				{
					CDAABB bounds;
					bounds.Empty();
					
					switch(data->GetLong(CDTR_A_GROUP))
					{
						case 0:
						{
							LONG i;
							for(i=1; i<opCnt; i++)
							{
								target = static_cast<BaseObject*>(opSelLog->GetIndex(i)); 
								if(target)
								{
									targM = target->GetMg();
									
									bounds.AddPoint(targM * (target->GetMp() + target->GetRad()));
									bounds.AddPoint(targM * (target->GetMp() - target->GetRad()));
								}
							}
							targM = Matrix();
							targM.off = bounds.GetCenterPoint();
							break;
						}
						case 1:
						{
							target = static_cast<BaseObject*>(opSelLog->GetIndex(opCnt-1)); 
							if(!target) return false;
							targM = target->GetMg();
							break;
						}
					}
				}
				else
				{
					target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
					if(!target) return false;
					targM = target->GetMg();
				}
			}

			if(!data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(NULL);
			doc->StartUndo();
			
			transM = GetTransferMatrix(data,opM,targM);
			
			switch(data->GetLong(CDTR_A_GROUP))
			{
				case 0:
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					if(CDIsAxisMode(doc) || (doc->IsEditMode() && optn))
					{
						if(IsValidPointObject(op))
						{
							if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
							{
								RecalculateReference(doc,op,transM,opM);
							}
							else RecalculatePoints(op,transM,opM);
						}
						
						if(op->GetDown()) RepositionChildren(doc,op->GetDown(),transM,opM,true);
						op->SetMg(transM);
					}
					else
					{
						if(shift) RepositionChildren(doc,op->GetDown(),transM,opM,true);
						if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
						{
							undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
							op->SetMg(transM);
							doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
						}
						else op->SetMg(transM);
					}
					break;
				}
				case 1:
				{
					LONG i;
					for(i=0; i<opCnt-1; i++)
					{
						op = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(op)
						{
							opM = op->GetMg();
							transM = GetTransferMatrix(data,opM,targM);
							
							CDAddUndo(doc,CD_UNDO_CHANGE,op);
							if(CDIsAxisMode(doc) || (doc->IsEditMode() && optn))
							{
								if(IsValidPointObject(op))
								{
									if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
									{
										RecalculateReference(doc,op,transM,opM);
									}
									else RecalculatePoints(op,transM,opM);
								}
								
								if(op->GetDown()) RepositionChildren(doc,op->GetDown(),transM,opM,true);
								op->SetMg(transM);
							}
							else
							{
								if(shift) RepositionChildren(doc,op->GetDown(),transM,opM,true);
								if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
								{
									undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
									op->SetMg(transM);
									doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
								}
								else op->SetMg(transM);
							}
						}
					}
					break;
				}
			}
			
			if(!data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(op);
			
			doc->SetMode(Mobject);
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
		else
		{
			if(doc->IsEditMode())
			{
				BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
				if(!op) return false;
				if(IsValidPointObject(op))
				{
					Matrix targM, opM = op->GetMg(), transM;
					
					doc->StartUndo();
			
					targM = GetEditModeTransferMatrix(doc,op);
					
					transM = GetTransferMatrix(data,opM,targM);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
					{
						RecalculateReference(doc,op,transM,opM);
					}
					else RecalculatePoints(op,transM,opM);
					
					if(op->GetDown()) RepositionChildren(doc,op->GetDown(),transM,opM,true);
					op->SetMg(transM);
					
					doc->SetActiveObject(op);
					doc->EndUndo();
					
					EventAdd(EVENT_FORCEREDRAW);
				}
				else
				{
					MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
					return false;
				}
				doc->SetMode(Mobject);
			}
			else
			{
				MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
				return false;
			}
		}
	}
	
	return true;
}

Bool CDTAimCommand::Execute(BaseDocument* doc)
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
	Bool optn = (state.GetLong(BFM_INPUT_QUALIFIER) & QALT);

	BaseContainer *wpData = GetWorldPluginData(ID_CDAIMTRANSFERCOM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetLong(CDTR_AXIS,2);
		bc.SetBool(CDTR_AUTO_KEY,false);
		bc.SetBool(CDTR_KEEP_SELECTION,false);
		bc.SetBool(HIERARCHAL_ALIGN,false);
		SetWorldPluginData(ID_CDAIMTRANSFERCOM,bc,false);
		wpData = GetWorldPluginData(ID_CDAIMTRANSFERCOM);
	}
	
	return DoAlign(doc, wpData, shift, optn);
}

class CDTAimCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTAlignCommand(void)
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
	String name=GeLoadString(IDS_CDAIMTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDAIMTRANSFERCOM,name,PLUGINFLAG_HIDE,"CDTransAim.tif","CD Transfer Align",CDDataAllocator(CDTAimCommandR));
	else return CDRegisterCommandPlugin(ID_CDAIMTRANSFERCOM,name,0,"CDTransAim.tif","CD Transfer Align"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTAimCommand));
}
