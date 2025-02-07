//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"
#include "CDMorph.h"

extern Bool cdsymPlug;

// from CD Morph Reference
enum
{
	M_SCALING				= 1007
};

class CDMorphMessage : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDMorphMessage::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDMData mrd, mxd;
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			// Check if CD Morph Mixer tag needs updating
			PluginMessage(ID_CDMORPHMIXPLUGIN,&mxd);
			if(mxd.list)
			{
				LONG i, mxCnt = mxd.list->GetCount();
				for(i=0; i<mxCnt; i++)
				{
					BaseTag *mxTag = static_cast<BaseTag*>(mxd.list->GetIndex(i));
					if(mxTag)
					{
						if(mxTag->GetDocument() == doc)
						{
							mxTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}

			if(!cdsymPlug)
			{
				CDMData syd;
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
			}
					
			break;
		}
		case CD_MSG_SCALE_CHANGE:
		{
			if(doc->DoUndo(true))
			{
				doc->StartUndo();
				doc->DoRedo();
				PluginMessage(ID_CDMORPHREFPLUGIN,&mrd);
				if(mrd.list)
				{
					LONG i, mrCnt = mrd.list->GetCount();
					for(i=0; i<mrCnt; i++)
					{
						BaseTag *mrTag = static_cast<BaseTag*>(mrd.list->GetIndex(i));
						if(mrTag)
						{
							if(mrTag->GetDocument())
							{
								BaseContainer *mrData = mrTag->GetDataInstance();
								if(mrData)
								{
									if(mrData->GetBool(M_SCALING))
									{
										mrTag->Message(CD_MSG_SCALE_CHANGE);
									}
								}
							}
						}
					}
				}
				doc->EndUndo();
			}
			
			break;
		}
		case ID_CDMORPHREFPLUGIN:
		{
			// Check for scaling with Model tool
			MessageDialog(GeLoadString(MD_MODEL_TOOL_SCALE));
			if(doc)
			{
				doc->DoUndo();
				doc->SetActiveObject(NULL);
				doc->SetAction(ID_MODELING_LIVESELECTION);
			}
			break;
		}
	}
	return true;
}

Bool RegisterCDMorphMessagePlugin(void)
{
	if(CDFindPlugin(ID_CDMRMESSAGEPLUGIN,CD_MESSAGE_PLUGIN)) return true;
	
	return RegisterMessagePlugin(ID_CDMRMESSAGEPLUGIN,"CDM Message",0,CDDataAllocator(CDMorphMessage));
}
