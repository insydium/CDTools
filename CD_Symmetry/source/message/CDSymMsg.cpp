//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDSymmetry.h"

#include "oCDDynSym.h"

Bool selUpdate = false;

class CDSymmetryMessage : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDSymmetryMessage::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDSData cld, syd;
	
	Bool symSel = false;
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			symSel = true;

			PluginMessage(ID_CDSYMMETRYTAG,&syd);
			if(syd.list)
			{
				LONG i, syCnt = syd.list->GetCount();
				for(i=0; i<syCnt; i++)
				{
					BaseTag *syTag = static_cast<BaseTag*>(syd.list->GetIndex(i));
					if(syTag)
					{
						if(syTag->GetDocument() == doc)
						{
							BaseObject *op = syTag->GetObject();
							if(op && op->GetBit(BIT_ACTIVE))
							{
								syTag->Message(CD_MSG_UPDATE);
							}
						}
					}
				}
			}
			
			PluginMessage(ID_CDCENTERLOCKTAG,&cld);
			if(cld.list)
			{
				LONG i, clCnt = cld.list->GetCount();
				for(i=0; i<clCnt; i++)
				{
					BaseTag *clTag = static_cast<BaseTag*>(cld.list->GetIndex(i));
					if(clTag)
					{
						if(clTag->GetDocument() == doc)
						{
							BaseObject *op = clTag->GetObject();
							if(op)
							{
								BaseObject *pr = op->GetUp();
								if(!pr) BaseTag::Free(clTag);
								else
								{
									if(pr->GetType() != ID_CDDYNSYMMETRY) BaseTag::Free(clTag);
								}
							}
						}
					}
				}
			}
			break;
		}
		case CD_MSG_SELECTION_UPDATE:
		{
			symSel = true;
			selUpdate = true;
			break;
		}
		case CD_MSG_SYMMETRY_UPDATE:
		{
			Bool updateSym = true;
			switch(doc->GetAction())
			{
				case ID_MODELING_LOOP_TOOL:
					if(selUpdate)
					{
						updateSym = false;
						selUpdate = false;
					}
					break;
				case ID_MODELING_RING_TOOL:
					if(selUpdate)
					{
						updateSym = false;
						selUpdate = false;
					}
					break;
				case ID_MODELING_FILL_SELECTION_TOOL:
					if(selUpdate)
					{
						updateSym = false;
						selUpdate = false;
					}
					break;
			}
			if(updateSym)
			{
				BaseObject *op = doc->GetActiveObject();
				if(op)
				{
					if(IsValidPointObject(op) && !op->GetDown())
					{
						BaseObject *pr = op->GetUp();
						if(pr)
						{
							if(pr->GetType() == ID_CDDYNSYMMETRY)
							{
								BaseContainer *oData = pr->GetDataInstance();
								if(oData)
								{
									if(oData->GetBool(DS_AUTO_UPDATE))
									{
										if(pr->GetCache())
										{
											if(!ObjectsEqual(op,pr->GetCache(),oData->GetReal(DS_TOLERANCE)))
											{
												if(doc->DoUndo(true))
												{
													doc->StartUndo();
													CallCommand(ID_CDSYMMETRYUPDATE);
													doc->EndUndo();
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			break;
		}
	}
	
	if(doc->IsEditMode())
	{
		BaseContainer *wpData = GetWorldPluginData(ID_CDSYMMETRYSELECT);
		if(wpData)
		{
			if(wpData->GetBool(IDS_CDSYMSELECT) && symSel)
			{
				CallCommand(ID_CDMIRRORSELECTION);
			}
		}
	}
	
	return true;
}

Bool RegisterCDSymmetryMessagePlugin(void)
{
	return RegisterMessagePlugin(ID_CDSYMMESSAGE,"CD Symmetry Message",0,CDDataAllocator(CDSymmetryMessage));
}
