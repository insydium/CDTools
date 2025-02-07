//	Cactus Dan's Align to Camera 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDAlignCam.h"
//#include "CDDebug.h"

class CDAlignToCamMessage : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDAlignToCamMessage::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	
	CDListData atcd; //
	//ListReceivedCoreMessages("CDAlignToCamMessage",id,bc);
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{						
			//GePrint("CDAlignToCamMessage::CoreMessage - EVMSG_CHANGE");
			
			PluginMessage(ID_CDALIGNTOCAMERA,&atcd);
			if(atcd.list)
			{
				LONG i, atcCnt = atcd.list->GetCount();
				for(i=0; i<atcCnt; i++)
				{
					BaseTag *tag = static_cast<BaseTag*>(atcd.list->GetIndex(i));
					if(tag)
					{
						if(tag->GetDocument() == doc)
						{
							tag->Message(ID_CDALIGNTOCAMERA);
						}
					}
				}
			}
			break;
		}
	}
	return true;
}

Bool RegisterCDAlignToCamMessagePlugin(void)
{
#if API_VERSION < 15000
    return RegisterMessagePlugin(ID_CDATCMESSAGE,"CD Align To Camera Message",0,gNew CDAlignToCamMessage);
#else
    return RegisterMessagePlugin(ID_CDATCMESSAGE,"CD Align To Camera Message",0,NewObjClear(CDAlignToCamMessage));
#endif
}
