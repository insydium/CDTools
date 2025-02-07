//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"

class CDTransferMessage : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDTransferMessage::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDTRData trData, ztData, taData;
	CDTrnSel trnSel;
	trnSel.trn = false;
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{			
			PluginMessage(ID_CDZEROTRANSTAG,&ztData);
			if(ztData.list)
			{
				LONG i, ztCnt = ztData.list->GetCount();
				for(i=0; i<ztCnt; i++)
				{
					BaseTag *ztTag = static_cast<BaseTag*>(ztData.list->GetIndex(i));
					if(ztTag)
					{
						if(ztTag->GetDocument() == doc)
						{
							ztTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}
			
			PluginMessage(ID_CDTRANSELECTEDTAG,&trData);
			if(trData.list)
			{
				LONG i, trnsCnt = trData.list->GetCount();
				for(i=0; i<trnsCnt; i++)
				{
					BaseTag *trnsTag = static_cast<BaseTag*>(trData.list->GetIndex(i));
					if(trnsTag)
					{
						if(trnsTag->GetDocument() == doc)
						{
							BaseObject *op = trnsTag->GetObject();
							if(op)
							{
								if(op->GetBit(BIT_ACTIVE))
								{
									trnsTag->Message(CD_MSG_UPDATE,&trnSel);
								}
							}
						}
					}
				}
			}
			if(trnSel.trn)
			{
				if(doc->DoUndo()) CallCommand(ID_CDTRANSELCOM);
			}
			break;
		}
	}
	return true;
}

Bool RegisterCDTransferMessagePlugin(void)
{
	return RegisterMessagePlugin(ID_CDTRANSMESSAGE,"CD Transfer Message",0,CDDataAllocator(CDTransferMessage));
}
