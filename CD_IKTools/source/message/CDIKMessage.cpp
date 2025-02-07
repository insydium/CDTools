//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

static void GetTags(BaseDocument *doc, BaseObject *op, AtomArray *tagList)
{
	BaseTag *tag=NULL;
	
	while(op)
	{
		for(tag = op->GetFirstTag(); tag; tag = tag->GetNext())
		{
			if(tag)
			{
				if(tag->GetType() == ID_CDFINGERPLUGIN || tag->GetType() == ID_CDTHUMBPLUGIN)
				{
					tagList->Append(tag);
				}
			}
		}
		GetTags(doc,op->GetDown(),tagList);
		
		op = op->GetNext();
	}
}

static void UpdateFingerThumbTags(BaseDocument *doc)
{
	AutoAlloc<AtomArray> tagList;
	if(tagList)
	{
		BaseObject *op = doc->GetFirstObject();
		if(op)
		{
			GetTags(doc,op,tagList);
			
			LONG i, tCnt = tagList->GetCount();
			if(tCnt > 0)
			{
				for(i=0; i<tCnt; i++)
				{
					BaseTag *tag = static_cast<BaseTag*>(tagList->GetIndex(i));
					if(tag)
					{
						BaseContainer *tData = tag->GetDataInstance();
						if(tData)
						{
							BaseObject *dest = tData->GetObjectLink(IK_BONE_LINK,doc);
							if(dest && dest != tag->GetObject())
							{
								tag->Remove();
								dest->InsertTag(tag,NULL);
							}
						}
					}
				}
			}
		}
	}
}

class CDIKMessagePlugin : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDIKMessagePlugin::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDIKData spnl, spik;
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			PluginMessage(ID_CDSPINALPLUGIN,&spnl);
			if(spnl.list)
			{
				LONG i, spnlCnt = spnl.list->GetCount();
				for(i=0; i<spnlCnt; i++)
				{
					BaseTag *spnTag = static_cast<BaseTag*>(spnl.list->GetIndex(i));
					if(spnTag)
					{
						if(spnTag->GetDocument() == doc)
						{
							spnTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}
			PluginMessage(ID_CDSPLINEIKPLUGIN,&spik);
			if(spik.list)
			{
				LONG i, spkCnt = spik.list->GetCount();
				for(i=0; i<spkCnt; i++)
				{
					BaseTag *spkTag = static_cast<BaseTag*>(spik.list->GetIndex(i));
					if(spkTag)
					{
						if(spkTag->GetDocument() == doc)
						{
							spkTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}
			break;
		}
		case CD_MSG_FINGER_THUMB_UPDATE:
		{
			UpdateFingerThumbTags(doc);
			break;
		}
		case ID_CDIKHANDLETOOL:
		{
			doc->SetAction(ID_MODELING_LIVESELECTION);
			EventAdd(EVENT_FORCEREDRAW);
			break;
		}
	}
	
	return true;
}

Bool RegisterCDIKMessagePlugin(void)
{
	return RegisterMessagePlugin(ID_CDIKMESSAGEPLUGIN,"CDC Message",0,CDDataAllocator(CDIKMessagePlugin));
}
