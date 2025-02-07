//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDConstraint.h"
//#include "CDDebug.h"

class CDCMessagePlugin : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDCMessagePlugin::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDCNData swd; 
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			// Check if sw tags need updating
			PluginMessage(ID_CDSWCONSTRAINTPLUGIN,&swd);
			if(swd.list)
			{
				LONG i, swCnt = swd.list->GetCount();
				for(i=0; i<swCnt; i++)
				{
					BaseTag *swTag = static_cast<BaseTag*>(swd.list->GetIndex(i));
					if(swTag)
					{
						if(swTag->GetDocument() == doc)
						{
							BaseObject *tagOp = swTag->GetObject();
							if(tagOp)
							{
								if(IsObjectInDocument(doc,doc->GetFirstObject(),tagOp))
								{
									//GePrint("object in doc");
									swTag->Message(CD_MSG_UPDATE);
								}
							}
						}
					}
				}
			}
			break;
		}
	}
	
	return true;
}

Bool RegisterCDCMessagePlugin(void)
{
	return RegisterMessagePlugin(ID_CDCMESSAGEPLUGIN,"CDC Message",0,CDDataAllocator(CDCMessagePlugin));
}
