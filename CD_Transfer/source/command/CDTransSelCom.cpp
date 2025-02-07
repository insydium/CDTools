//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "stdlib.h"

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"
#include "CDTransSelTag.h"

#include "tCDTranSel.h"

class CDTransferSelectedCommand : public CommandData
{
	private:
		LONG GetButtonID(BaseContainer *tData, BaseList2D *dest);
		
	public:
		virtual Bool Execute(BaseDocument* doc);
};

LONG CDTransferSelectedCommand::GetButtonID(BaseContainer *tData, BaseList2D *trg)
{
	LONG buttonID;
	
	AutoAlloc<Description> desc;
	
	CDGetDescription(trg,desc,CD_DESCFLAGS_DESC_0);
	void* h = desc->BrowseInit();
	const BaseContainer *descBC = NULL;
	DescID id, groupid, srcID;
	
	Bool skip = true;
	LONG i=0;
	while(desc->GetNext(h, &descBC, id, groupid))
	{
		if(id == DescID(ID_BASELIST_NAME)) skip = false;
		if(skip) continue;
		
		if(id[0].dtype == DTYPE_BUTTON)
		{
			i++;
			if(tData->GetLong(TRNSEL_BUTTON_SELECT) == TRNSEL_BUTTON+i)
			{
				buttonID = id[0].id;
				break;
			}
		}
	}
	desc->BrowseFree(h);
	
	return buttonID;
}

Bool CDTransferSelectedCommand::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	CDTRData trnsD;
	Bool selTrans = false, addSel = false;
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT) addSel = true;
				
	PluginMessage(ID_CDTRANSELECTEDTAG,&trnsD);
	if(trnsD.list)
	{
		BaseObject *op = NULL;
		BaseTag *trnsTag = NULL;
		LONG i, trnsCnt = trnsD.list->GetCount();
		for(i=0; i<trnsCnt; i++)
		{
			trnsTag = static_cast<BaseTag*>(trnsD.list->GetIndex(i));
			if(trnsTag)
			{
				if(trnsTag->GetDocument() == doc)
				{
					op = trnsTag->GetObject();
					if(op)
					{
						BaseContainer *tData = trnsTag->GetDataInstance();
						if(!tData->GetBool(T_MOV))
						{
							BaseList2D *dest = tData->GetLink(TRNSEL_TARGET,doc);
							if(dest && tData->GetBool(TRNSEL_TRANSFER_ON) && tData->GetBool(TRNSEL_SEL_TRANS))
							{
								if(!addSel)
								{
									doc->SetActiveObject(NULL);
									doc->SetActiveTag(NULL);
								}
								
								CDAddUndo(doc,CD_UNDO_CHANGE,dest);
								
								if(tData->GetBool(TRNSEL_USE_BUTTON))
								{
									LONG button = GetButtonID(tData, dest);
									DescriptionCommand dc;
									dc.id = DescID(button);
									dest->Message(MSG_DESCRIPTION_COMMAND,&dc);
									EventAdd(EVENT_FORCEREDRAW);
								}
								else
								{
									if(dest->IsInstanceOf(Obase))
									{
										if(dest->GetType() != Oselection)
										{
											doc->SetActiveObject(static_cast<BaseObject*>(dest),SELECTION_ADD);
										}
									}
									else if(dest->IsInstanceOf(Tbase))
									{
										doc->SetActiveTag(static_cast<BaseTag*>(dest),SELECTION_ADD);
									}
								}
								
								selTrans = true;
								tData->SetBool(TRNSEL_SEL_TRANS,false);
							}
						}
					}
				}
			}
		}
	}
	if(selTrans) EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

Bool RegisterCDTransferSelectedCommand(void)
{
	// decide by name if the plugin shall be registered - just for user convenience  PLUGINFLAG_HIDE|
	String name=GeLoadString(IDS_CDTSCOMMAND); if (!name.Content()) return true;
	return CDRegisterCommandPlugin(ID_CDTRANSELCOM,name,PLUGINFLAG_HIDEPLUGINMENU,"","CD Transfer Selected Command",CDDataAllocator(CDTransferSelectedCommand));
}
