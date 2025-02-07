//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDAConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	AC_TARGET_COUNT			= 1040
};

class CDAddAConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		LONG	aType, align;
		Bool	addEmpty;

		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddAConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDA));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDC_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDC_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDS_TARGET_TYPE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_TARGET,BFH_CENTER,2,1);
					AddChild(IDC_TARGET, 0, GeLoadString(A_TARGET));
					AddChild(IDC_TARGET, 1, GeLoadString(A_UPVECTOR));

				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_ALIGN_AXIS),0);
					AddComboBox(IDS_ALIGN_AXIS, BFH_SCALEFIT);
						AddChild(IDS_ALIGN_AXIS, 0, GeLoadString(IDS_ALIGN_OFF));
						AddChild(IDS_ALIGN_AXIS, 1, GeLoadString(IDS_ALIGN_X));
						AddChild(IDS_ALIGN_AXIS, 2, GeLoadString(IDS_ALIGN_Y));
				}
					
				AddCheckbox(IDC_USE_EMPTY_LINK,BFH_LEFT,0,0,GeLoadString(IDC_USE_EMPTY_LINK));
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddAConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	SetLong(IDC_TARGET,0);
	SetLong(IDS_ALIGN_AXIS,0);
	SetBool(IDC_USE_EMPTY_LINK,true);

	DoEnable();
	
	return true;
}

Bool CDAddAConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_TARGET,aType);
	GetLong(IDS_ALIGN_AXIS,align);
	GetBool(IDC_USE_EMPTY_LINK,addEmpty);
	
	DoEnable();
	
	return true;
}

void CDAddAConstraintDialog::DoEnable(void)
{
	LONG etype;
	
	GetLong(IDC_TARGET,etype);
	switch(etype)
	{
		case 0:
			Enable(IDS_ALIGN_AXIS,false);
			break;
		case 1:
			Enable(IDS_ALIGN_AXIS,true);
			break;
	}
}

class CDAddAConstraint : public CommandData
{
	private:
		CDAddAConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddAConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified

	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		BaseObject *target = NULL, *upVector = NULL;
		
		if(opCount > 3)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		if(opCount == 2)
		{
			target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
			if(!dlg.Open()) return false;
		}
		else if(opCount == 3)
		{
			target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
			upVector = static_cast<BaseObject*>(opSelLog->GetIndex(2)); 
		}

		doc->SetActiveObject(NULL);
		doc->StartUndo();

		LONG i, cnt;
		BaseTag *aTag = NULL;
		if(!destObject->GetTag(ID_CDACONSTRAINTPLUGIN))
		{
			aTag = BaseTag::Alloc(ID_CDACONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(aTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,aTag);
			aTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			aTag = destObject->GetTag(ID_CDACONSTRAINTPLUGIN);
		}
		BaseContainer *tData = aTag->GetDataInstance();
		if(tData)
		{
			cnt = tData->GetLong(AC_TARGET_COUNT);
			
			if(!target)
			{
				doc->SetActiveTag(aTag);
				doc->EndUndo();
				EventAdd(EVENT_FORCEREDRAW);
				return true;
			}
			
			if(cnt < MAX_ADD)
			{			
				CDAddUndo(doc,CD_UNDO_CHANGE,destObject);
				
				if(opCount == 2)
				{
					switch (dlg.aType)
					{
						case 0:
						{
							if(cnt == 1 && tData->GetObjectLink(AC_TARGET,doc) == NULL && tData->GetObjectLink(AC_UP_VECTOR,doc) == NULL)
							{
								CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
								tData->SetLink(AC_TARGET,target);
							}
							else
							{
								if(!dlg.addEmpty) 
								{
									DescriptionCommand dc;
									dc.id = DescID(AC_ADD_AIM);
									aTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
									
									CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
									tData->SetLink(AC_TARGET+cnt,target);
									
									cnt = tData->GetLong(AC_TARGET_COUNT);
								}
								else
								{
									Bool foundEmptyT = false;
									for(i=0; i<cnt; i++)
									{
										if(!foundEmptyT && tData->GetObjectLink(AC_TARGET+i,doc) == NULL)
										{
											CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
											tData->SetLink(AC_TARGET+i,target);
											foundEmptyT = true;
										}
									}
									if(!foundEmptyT)
									{
										DescriptionCommand dc;
										dc.id = DescID(AC_ADD_AIM);
										aTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
										
										CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
										tData->SetLink(AC_TARGET+cnt,target);
										
										cnt = tData->GetLong(AC_TARGET_COUNT);
									}
								}
							}
							break;
						}
						case 1:
						{
							if(cnt == 1 && tData->GetObjectLink(AC_TARGET,doc) == NULL && tData->GetObjectLink(AC_UP_VECTOR,doc) == NULL)
							{
								CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
								tData->SetLink(AC_UP_VECTOR,target);
								switch (dlg.align)
								{
									case 0:
										break;
									case 1:
										tData->SetLong(AC_ALIGN_AXIS,AC_ALIGN_X);
										tData->SetLong(AC_POLE_AXIS,AC_POLE_X);
										break;
									case 2:
										tData->SetLong(AC_ALIGN_AXIS,AC_ALIGN_Y);
										tData->SetLong(AC_POLE_AXIS,AC_POLE_Y);
										break;
								}
							}
							else
							{
								if(!dlg.addEmpty)
								{
									DescriptionCommand dc;
									dc.id = DescID(AC_ADD_AIM);
									aTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
									
									CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
									tData->SetLink(AC_UP_VECTOR+cnt,target);
									switch (dlg.align)
									{
										case 0:
											break;
										case 1:
											tData->SetLong(AC_ALIGN_AXIS+cnt,AC_ALIGN_X+cnt);
											break;
										case 2:
											tData->SetLong(AC_ALIGN_AXIS+cnt,AC_ALIGN_Y+cnt);
											break;
									}
									cnt = tData->GetLong(AC_TARGET_COUNT);
								}
								else
								{
									Bool foundEmptyU = false;
									for(i=0; i<cnt; i++)
									{
										if(!foundEmptyU && tData->GetObjectLink(AC_UP_VECTOR+i,doc) == NULL)
										{
											CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
											tData->SetLink(AC_UP_VECTOR+i,target);
											switch (dlg.align)
											{
												case 0:
													break;
												case 1:
													tData->SetLong(AC_ALIGN_AXIS+i,AC_ALIGN_X+i);
													tData->SetLong(AC_POLE_AXIS,AC_POLE_X);
													break;
												case 2:
													tData->SetLong(AC_ALIGN_AXIS+i,AC_ALIGN_Y+i);
													tData->SetLong(AC_POLE_AXIS,AC_POLE_Y);
													break;
											}
											foundEmptyU = true;
										}
									}
									if(!foundEmptyU)
									{
										DescriptionCommand dc;
										dc.id = DescID(AC_ADD_AIM);
										aTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
										
										CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
										tData->SetLink(AC_UP_VECTOR+cnt,target);
										switch (dlg.align)
										{
											case 0:
												break;
											case 1:
												tData->SetLong(AC_ALIGN_AXIS+cnt,AC_ALIGN_X+cnt);
												tData->SetLong(AC_POLE_AXIS,AC_POLE_X);
												break;
											case 2:
												tData->SetLong(AC_ALIGN_AXIS+cnt,AC_ALIGN_Y+cnt);
												tData->SetLong(AC_POLE_AXIS,AC_POLE_Y);
												break;
										}
										cnt = tData->GetLong(AC_TARGET_COUNT);
									}
								}
							}
							break;
						}
					}
				}
				else
				{
					if(target && upVector)
					{
						if(cnt == 1 && tData->GetObjectLink(AC_TARGET,doc) == NULL && tData->GetObjectLink(AC_UP_VECTOR,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
							tData->SetLink(AC_TARGET,target);
							tData->SetLink(AC_UP_VECTOR,upVector);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(AC_ADD_AIM);
							aTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,aTag);
							tData->SetLink(AC_TARGET+cnt,target);
							tData->SetLink(AC_UP_VECTOR+cnt,upVector);
							
							cnt = tData->GetLong(AC_TARGET_COUNT);
						}
					}
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(aTag);
			aTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddAConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddAConstraint(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDADDA); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDACONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddAConst.tif","CD Add Aim Constraint",CDDataAllocator(CDAddAConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDACONSTRAINT,name,0,"CDAddAConst.tif","CD Add Aim Constraint",CDDataAllocator(CDAddAConstraint));
}
