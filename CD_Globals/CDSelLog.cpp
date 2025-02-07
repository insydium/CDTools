//	Cactus Dan's Selection Logger
//	Copyright 2008 by Cactus Dan Libisch

#include "CDSelLog.h"
#include "CDCompatibility.h"

AtomArray *opSelectionList = NULL;
Bool selectionChanged;

Bool CDSLMessage::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{			
			BaseList2D *sh = doc->FindSceneHook(ID_CDSLSCENEHOOK);
			if(sh)
			{
				CDSLSceneHook *sHook = static_cast<CDSLSceneHook*>(sh->GetNodeData());
				if(sHook) sHook->UpdateSelectionList(doc);
			}
			break;
		}
	}
	return true;
}

void InitSelectionLog(void)
{
	opSelectionList = AtomArray::Alloc();
}

void FreeSelectionLog(void)
{
	if(opSelectionList)  AtomArray::Free(opSelectionList);
}

AtomArray* GetSelectionLog(BaseDocument *doc)
{
	AtomArray *list = NULL;
	
	BaseList2D *sh = doc->FindSceneHook(ID_CDSLSCENEHOOK);
	if(sh)
	{
		CDSLSceneHook *sHook = static_cast<CDSLSceneHook*>(sh->GetNodeData());
		if(sHook) list = sHook->GetSelectionList();
	}
	
	return list;
}

Bool SelectionLogChanged(BaseDocument *doc)
{
	Bool changed = false;
	
	BaseList2D *sh = doc->FindSceneHook(ID_CDSLSCENEHOOK);
	if(sh)
	{
		CDSLSceneHook *sHook = static_cast<CDSLSceneHook*>(sh->GetNodeData());
		if(sHook) changed = *sHook->GetChanged();
	}
	
	return changed;
}

void CDSLSceneHook::UpdateSelectionList(BaseDocument *doc)
{
	BaseObject *selOp = NULL, *actOp = NULL;
	LONG si, ai, activeCnt, selCnt;
	selectionChanged = false;
	
	if(opSelectionList)
	{
		AutoAlloc<AtomArray> opActiveList;
		if(opActiveList)
		{
			CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
			activeCnt = opActiveList->GetCount();
			switch (activeCnt)
			{
				case 0:
				{
					if(opSelectionList->GetCount() > 0) selectionChanged = true;
					opSelectionList->Flush();
					break;
				}
				case 1:
				{
					selCnt = opSelectionList->GetCount();
					actOp = static_cast<BaseObject*>(opActiveList->GetIndex(0));
					if(activeCnt != selCnt) selectionChanged = true;
					else
					{
						selOp = static_cast<BaseObject*>(opSelectionList->GetIndex(0));
						if(selOp != actOp) selectionChanged = true;
					}
					opSelectionList->Flush();
					opSelectionList->Append(actOp);
					break;
				}
				default:
				{
					//Check the selection list for valid active objects
					AutoAlloc<AtomArray> opRemoveList;
					if(opRemoveList)
					{
						selCnt = opSelectionList->GetCount();
						for(si=0; si<selCnt; si++)
						{
							selOp = static_cast<BaseObject*>(opSelectionList->GetIndex(si));
							if(opActiveList->Find(selOp) == NOTOK) opRemoveList->Append(selOp);
						}
						if(opRemoveList->GetCount() > 0)
						{
							selectionChanged = true;
							for(si=0; si<opRemoveList->GetCount(); si++)
							{
								selOp = static_cast<BaseObject*>(opRemoveList->GetIndex(si));
								opSelectionList->Remove(selOp);
							}
						}
					}
					selCnt = opSelectionList->GetCount();
					if(activeCnt > selCnt)
					{
						selectionChanged = true;
						//Add active opActiveList objects that are not in the selection list
						if(selCnt > 0)
						{
							for(si=0; si<selCnt; si++)
							{
								selOp = static_cast<BaseObject*>(opSelectionList->GetIndex(si));
								opActiveList->Remove(selOp);
							}
							activeCnt = opActiveList->GetCount();
							for(ai=0; ai<activeCnt; ai++)
							{
								actOp = static_cast<BaseObject*>(opActiveList->GetIndex(ai));
								opSelectionList->Append(actOp);
							}
						}
						else
						{
							for(ai=0; ai<activeCnt; ai++)
							{
								actOp = static_cast<BaseObject*>(opActiveList->GetIndex(ai));
								opSelectionList->Append(actOp);
							}
						}
					}
					break;
				}
			}
		}
		sList = opSelectionList;
		chngd = &selectionChanged;
	}
}


Bool CDSLSceneHook::CDMouseInput(BaseList2D* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	if(doc->IsEditMode())
	{
		Bool addEvnt = false;
		switch(doc->GetAction())
		{
			case ID_MODELING_LOOP_TOOL:
				addEvnt = true;
				break;
			case ID_MODELING_RING_TOOL:
				addEvnt = true;
				break;
			case ID_MODELING_FILL_SELECTION_TOOL:
				addEvnt = true;
				break;
		}
		
		if(addEvnt)
		{
			LONG button;
			switch (msg.GetLong(BFM_INPUT_CHANNEL))
			{
				case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT;
				{
					SpecialEventAdd(CD_MSG_SELECTION_UPDATE);
					break;
				}
			}
		}
	}
	
	return false;
}


// Register functions for message and scene hook
Bool RegisterCDSLMessage(void)
{
	return RegisterMessagePlugin(ID_CDSLMESSAGE, "CDSLMessage", 0, CDDataAllocator(CDSLMessage));
}

Bool RegisterCDSLSceneHook(void)
{
	return RegisterSceneHookPlugin(ID_CDSLSCENEHOOK, "CDSLSceneHook", PLUGINFLAG_HIDE|PLUGINFLAG_HIDEPLUGINMENU, CDSLSceneHook::Alloc, CD_EXECUTION_RESULT_OK, 0, NULL);
}

Bool RegisterSelectionLog(void)
{
	if(CDFindPlugin(ID_CDSLSCENEHOOK,CD_SCENEHOOK_PLUGIN)) return false;
	else
	{
		if (!RegisterCDSLSceneHook()) return false;
		if (!RegisterCDSLMessage()) return false;
	}
	
	return true;
}

