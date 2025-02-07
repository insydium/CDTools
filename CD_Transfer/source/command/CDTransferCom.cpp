//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTransferDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTransferDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(CDTR_TITLE));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_GROUP),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_GROUP,BFH_CENTER,2,1);
					AddChild(CDTR_GROUP, 0, GeLoadString(CDTR_TO_GROUP));
					AddChild(CDTR_GROUP, 1, GeLoadString(CDTR_TO_LAST));

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
				
				GroupBegin(0,BFH_LEFT,4,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDTR_NORMAL_AXIS),0);
					
					AddRadioGroup(CDTR_NORMAL_AXIS,BFH_CENTER,3,1);
						AddChild(CDTR_NORMAL_AXIS, 0, GeLoadString(CDTR_POS_X));
						AddChild(CDTR_NORMAL_AXIS, 1, GeLoadString(CDTR_POS_Y));
						AddChild(CDTR_NORMAL_AXIS, 2, GeLoadString(CDTR_POS_Z));
						
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_COMPONENTS_ONLY,BFH_LEFT,0,0,GeLoadString(CDTR_COMPONENTS_ONLY));
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

Bool CDTransferDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDTRANSFERCOM);
	if(!bc)
	{
		SetLong(CDTR_GROUP,0);
		SetBool(CDTR_POS,true);
		SetBool(CDTR_POS_X,true);
		SetBool(CDTR_POS_Y,true);
		SetBool(CDTR_POS_Z,true);
		SetBool(CDTR_SCA,true);
		SetBool(CDTR_ROT,true);
		SetBool(CDTR_COMPONENTS_ONLY,false);
		SetBool(CDTR_AUTO_KEY,false);
		SetBool(CDTR_KEEP_SELECTION,false);
		SetLong(CDTR_NORMAL_AXIS,2);
	}
	else
	{
		SetLong(CDTR_GROUP,bc->GetLong(CDTR_GROUP));
		SetBool(CDTR_POS,bc->GetBool(CDTR_POS));
		SetBool(CDTR_POS_X,bc->GetBool(CDTR_POS_X));
		SetBool(CDTR_POS_Y,bc->GetBool(CDTR_POS_Y));
		SetBool(CDTR_POS_Z,bc->GetBool(CDTR_POS_Z));
		SetBool(CDTR_SCA,bc->GetBool(CDTR_SCA));
		SetBool(CDTR_ROT,bc->GetBool(CDTR_ROT));
		SetBool(CDTR_COMPONENTS_ONLY,bc->GetBool(CDTR_COMPONENTS_ONLY));
		SetBool(CDTR_AUTO_KEY,bc->GetBool(CDTR_AUTO_KEY));
		SetBool(CDTR_KEEP_SELECTION,bc->GetBool(CDTR_KEEP_SELECTION));
		SetLong(CDTR_NORMAL_AXIS,bc->GetLong(CDTR_NORMAL_AXIS));
	}
		
	DoEnable();
	
	return true;
}

Bool CDTransferDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool pos, sca, rot, px, py, pz, sel, akey, cmpts;
	LONG mir, grp;
	
	GetLong(CDTR_GROUP,grp);
	GetBool(CDTR_POS,pos);
	GetBool(CDTR_POS_X,px);
	GetBool(CDTR_POS_Y,py);
	GetBool(CDTR_POS_Z,pz);
	GetBool(CDTR_SCA,sca);
	GetBool(CDTR_ROT,rot);
	GetBool(CDTR_COMPONENTS_ONLY,cmpts);
	GetBool(CDTR_AUTO_KEY,akey);
	GetBool(CDTR_KEEP_SELECTION,sel);
	GetLong(CDTR_NORMAL_AXIS,mir);

	BaseContainer bc;
	bc.SetLong(CDTR_GROUP,grp);
	bc.SetBool(CDTR_POS,pos);
	bc.SetBool(CDTR_POS_X,px);
	bc.SetBool(CDTR_POS_Y,py);
	bc.SetBool(CDTR_POS_Z,pz);
	bc.SetBool(CDTR_SCA,sca);
	bc.SetBool(CDTR_ROT,rot);
	bc.SetBool(CDTR_COMPONENTS_ONLY,cmpts);
	bc.SetBool(CDTR_AUTO_KEY,akey);
	bc.SetBool(CDTR_KEEP_SELECTION,sel);
	bc.SetLong(CDTR_NORMAL_AXIS,mir);
	SetWorldPluginData(ID_CDTRANSFERCOM,bc,false);
	
	DoEnable();
	
	return true;
}

void CDTransferDialog::DoEnable(void)
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
	
	BaseDocument *doc = GetActiveDocument();
	AtomArray *opSelLog = GetSelectionLog(doc);
	BaseContainer *wpData = GetWorldPluginData(ID_CDTRANSFERCOM);
	if(opSelLog && wpData)
	{
		LONG  opCnt = opSelLog->GetCount();
		if(opCnt > 2)
		{
			Enable(CDTR_GROUP,true);
			if(CDTR_GROUP == 0) Enable(CDTR_AUTO_KEY,true);
			else
			{
				SetBool(CDTR_AUTO_KEY,false);
				if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
				Enable(CDTR_AUTO_KEY,false);
			}
		}
		else
		{
			Enable(CDTR_GROUP,false);
			if(opCnt > 1) Enable(CDTR_AUTO_KEY,true);
			else
			{
				SetBool(CDTR_AUTO_KEY,false);
				if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
				Enable(CDTR_AUTO_KEY,false);
			}
		}
		
		Bool enCmpt = true;
		if(!doc->IsEditMode()) enCmpt = false;
		else
		{
			if(opCnt != 2) enCmpt = false;
			else
			{
				BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0));
				BaseObject *trg = static_cast<BaseObject*>(opSelLog->GetIndex(1));
				if(!IsValidPointObject(op) || !IsValidPointObject(trg)) enCmpt = false;
			}
		}
		if(enCmpt) Enable(CDTR_COMPONENTS_ONLY,true);
		else
		{
			SetBool(CDTR_COMPONENTS_ONLY,false);
			if(wpData) wpData->SetBool(CDTR_COMPONENTS_ONLY,false);
			Enable(CDTR_COMPONENTS_ONLY,false);
		}
	}
	
	if(wpData)
	{
		if(!wpData->GetBool(CDTR_AUTO_KEY))
		{
			SetBool(CDTR_KEEP_SELECTION,false);
			wpData->SetBool(CDTR_KEEP_SELECTION,false);
			Enable(CDTR_KEEP_SELECTION,false);
		}
		else Enable(CDTR_KEEP_SELECTION,true);
	}
}

class CDTransferCommand : public CommandData
{
	private:
		CDTransferDialog dlg;

	public:
		Bool DoTransfer(BaseDocument *doc, BaseContainer *data, Bool shift, Bool optn);
		Matrix GetTransferMatrix(BaseDocument *doc, BaseContainer *data, Matrix src, Matrix dst, LONG opCnt);

		virtual Bool Execute(BaseDocument* doc);
};

Matrix CDTransferCommand::GetTransferMatrix(BaseDocument *doc, BaseContainer *data, Matrix src, Matrix dst, LONG opCnt)
{
	Matrix transM = src;
	
	if(data->GetBool(CDTR_POS))
	{
		if(data->GetBool(CDTR_POS_X)) transM.off.x = dst.off.x;
		if(data->GetBool(CDTR_POS_Y)) transM.off.y = dst.off.y;
		if(data->GetBool(CDTR_POS_Z)) transM.off.z = dst.off.z;
	}
	if(data->GetBool(CDTR_ROT) && opCnt < 3)
	{
		transM.v1 = VNorm(dst.v1) * Len(src.v1);
		transM.v2 = VNorm(dst.v2) * Len(src.v2);
		transM.v3 = VNorm(dst.v3) * Len(src.v3);
	}
	if(data->GetBool(CDTR_SCA) && opCnt < 3 && !doc->IsEditMode())
	{
		transM.v1 = VNorm(transM.v1) * Len(dst.v1);
		transM.v2 = VNorm(transM.v2) * Len(dst.v2);
		transM.v3 = VNorm(transM.v3) * Len(dst.v3);
	}
	
	return transM;
}

Bool CDTransferCommand::DoTransfer(BaseDocument *doc, BaseContainer *data, Bool shift, Bool optn)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	BaseObject *undoOp = NULL;
	
	LONG  opCnt = opSelLog->GetCount();
	if(opCnt == 0) return false;
	
	Bool oMode = false;
	if(opCnt > 1)
	{
		BaseObject *trg = NULL;
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!op) return false;
		
		Matrix targM, opM = op->GetMg(), transM, opTargM;
		if(doc->IsEditMode())
		{
			if(opCnt < 3)
			{
				trg = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
				if(!trg) return false;
				
				if(IsValidPointObject(trg))
				{
					targM = GetEditModeTransferMatrix(doc,trg);
					
					switch (data->GetLong(CDTR_NORMAL_AXIS))
					{
						case 0:
						{
							targM = targM * MatrixRotY(1.570796327);
							break;
						}
						case 1:
						{
							targM = targM * MatrixRotX(-1.570796327);
							targM = targM * MatrixRotY(3.141592654);
							break;
						}
					}
					if(IsValidPointObject(op))
					{
						if(ElementsSelected(doc,op))
						{
							opTargM = GetEditModeTransferMatrix(doc,op);
							if(!MatrixEqual(opTargM,op->GetMg(),0.001))
							{
								switch (data->GetLong(CDTR_NORMAL_AXIS))
								{
									case 0:
									{
										opTargM = opTargM * MatrixRotY(-1.570796327);
										break;
									}
									case 1:
									{
										opTargM = opTargM * MatrixRotX(1.570796327);
										break;
									}
									case 2:
									{
										opTargM = opTargM * MatrixRotY(3.141592654);
										break;
									}
								}
								if(!data->GetBool(CDTR_COMPONENTS_ONLY))
								{
									opTargM = MInv(opTargM) * opM;
									targM = targM * opTargM;
								}
							}
						}
					}
				}
				else
				{
					targM = trg->GetMg();
				}
			}
			else
			{
				MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
				doc->SetActiveObject(NULL);
				return false;
			}
			oMode = true;
		}
		else
		{
			if(opCnt > 2)
			{
				CDAABB bounds;
				bounds.Empty();
				
				switch(data->GetLong(CDTR_GROUP))
				{
					case 0:
					{
						LONG i;
						for(i=1; i<opCnt; i++)
						{
							trg = static_cast<BaseObject*>(opSelLog->GetIndex(i)); 
							if(trg)
							{
								targM = trg->GetMg();
								
								bounds.AddPoint(targM * (trg->GetMp() + trg->GetRad()));
								bounds.AddPoint(targM * (trg->GetMp() - trg->GetRad()));
							}
						}
						targM = Matrix();
						targM.off = bounds.GetCenterPoint();
						break;
					}
					case 1:
					{
						trg = static_cast<BaseObject*>(opSelLog->GetIndex(opCnt-1)); 
						if(!trg) return false;
						targM = trg->GetMg();
						break;
					}
				}
			}
			else
			{
				trg = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
				if(!trg) return false;
				targM = trg->GetMg();
			}
		}

		if(!data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(NULL);
		doc->StartUndo();
		
		transM = GetTransferMatrix(doc,data,opM,targM,opCnt);
		
		switch(data->GetLong(CDTR_GROUP))
		{
			case 0:
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				if((CDIsAxisMode(doc)) || (doc->IsEditMode() && optn))
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
					if(data->GetBool(CDTR_COMPONENTS_ONLY) && doc->IsEditMode())
					{
						RecalculateComponents(doc,op,transM,opTargM);
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
						
						if(opCnt == 2)
						{
							Vector tRot = GetGlobalRotation(trg);
							SetGlobalRotation(op,tRot);
						}
					}
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
						}
						else if(shift) RepositionChildren(doc,op->GetDown(),transM,opM,true);
						if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
						{
							undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
							op->SetMg(transM);
							doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
						}
						else op->SetMg(transM);
					}
				}
				break;
			}
		}
		if(!data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(op);
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
				
				transM = GetTransferMatrix(doc,data,opM,targM,opCnt);
				
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
			oMode = true;
		}
		else
		{
			MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
			return false;
		}
	}
	if(oMode) doc->SetMode(Mobject);
	
	return true;
}

Bool CDTransferCommand::Execute(BaseDocument* doc)
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

	BaseContainer *wpData = GetWorldPluginData(ID_CDTRANSFERCOM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetBool(CDTR_POS,true);
		bc.SetBool(CDTR_POS_X,true);
		bc.SetBool(CDTR_POS_Y,true);
		bc.SetBool(CDTR_POS_Z,true);
		bc.SetBool(CDTR_SCA,true);
		bc.SetBool(CDTR_ROT,true);
		bc.SetBool(CDTR_AUTO_KEY,false);
		bc.SetBool(CDTR_KEEP_SELECTION,false);
		bc.SetLong(CDTR_NORMAL_AXIS,0);
		SetWorldPluginData(ID_CDTRANSFERCOM,bc,false);
		wpData = GetWorldPluginData(ID_CDTRANSFERCOM);
	}
	
	Bool optnOff = true;
	if(wpData->GetBool(CDTR_ROT)) optnOff = false;
	if(wpData->GetBool(CDTR_SCA)) optnOff = false;
	if(wpData->GetBool(CDTR_POS))
	{
		if(wpData->GetBool(CDTR_POS_X)) optnOff = false;
		if(wpData->GetBool(CDTR_POS_Y)) optnOff = false;
		if(wpData->GetBool(CDTR_POS_Z)) optnOff = false;
	}
	if(optnOff)
	{
		MessageDialog(GeLoadString(MD_ALL_OPTIONS_OFF));
		return false;	
	}

	return DoTransfer(doc, wpData, shift, optn);
}

class CDTransferCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTransferCommand(void)
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
	String name=GeLoadString(IDS_CDTRANSFER); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTRANSFERCOM,name,PLUGINFLAG_HIDE,"CDTransfer.tif","CD Transfer",CDDataAllocator(CDTransferCommandR));
	else return CDRegisterCommandPlugin(ID_CDTRANSFERCOM,name,0,"CDTransfer.tif","CD Transfer"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTransferCommand));
}
