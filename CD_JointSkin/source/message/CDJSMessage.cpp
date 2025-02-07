//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"

class CDJSMessagePlugin : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool TasksRunning(void)
{
	if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING)) return true;
	if(CheckIsRunning(CHECKISRUNNING_VIEWDRAWING)) return true;
	if(CheckIsRunning(CHECKISRUNNING_EDITORRENDERING)) return true;
	if(CheckIsRunning(CHECKISRUNNING_EXTERNALRENDERING)) return true;
	if(CheckIsRunning(CHECKISRUNNING_PAINTERUPDATING)) return true;
	if(CheckIsRunning(CHECKISRUNNING_MATERIALPREVIEWRUNNING)) return true;
	
	return false;
}

Bool CDJSMessagePlugin::CoreMessage(LONG id, const BaseContainer &bc)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	CDJSData scd, skd, srd, jd, bpd, rgd; //
	
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			// Check for AutoKey
			if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
			{
				PluginMessage(ID_CDJOINTOBJECT,&jd);
				if(jd.list)
				{
					LONG i, jCnt = jd.list->GetCount();
					for(i=0; i<jCnt; i++)
					{
						BaseObject *jnt = static_cast<BaseObject*>(jd.list->GetIndex(i));
						if(jnt)
						{
							if(jnt->GetDocument() == doc)
							{
								jnt->Message(CD_MSG_UPDATE);
							}
						}
					}
				}
			}
			
			// Check if skin tags need updating
			PluginMessage(ID_CDSKINPLUGIN,&skd);
			if(skd.list)
			{
				LONG i, skCnt = skd.list->GetCount();
				for(i=0; i<skCnt; i++)
				{
					BaseTag *skTag = static_cast<BaseTag*>(skd.list->GetIndex(i));
					if(skTag)
					{
						if(skTag->GetDocument() == doc)
						{
							skTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}
			
			// Check if CD Bind Pose tag needs updating
			PluginMessage(ID_CDBINDPOSETAG,&bpd);
			if(bpd.list)
			{
				LONG i, bpCnt = bpd.list->GetCount();
				for(i=0; i<bpCnt; i++)
				{
					BaseTag *bpTag = static_cast<BaseTag*>(bpd.list->GetIndex(i));
					if(bpTag)
					{
						if(bpTag->GetDocument() == doc)
						{
							bpTag->Message(CD_MSG_UPDATE);
						}
					}
				}
			}
			if(!TasksRunning())
			{
				// Check if CD Skin Weight tool needs updating
				if(doc) 
				{
					if(doc->GetAction() == ID_SKINWEIGHTTOOL)
					{
						BasePlugin *tool = CDFindPlugin(ID_SKINWEIGHTTOOL, CD_TOOL_PLUGIN);
						if(tool)
						{
							tool->Message(CD_MSG_UPDATE);
						}
					}
				}

				PluginMessage(ID_CDJOINTSANDSKIN,&scd);	
			}
			
			break;
		}
		case ID_CDSKINREFPLUGIN:
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
		case ID_CDJOINTMIRRORPLUGIN:
		{
			// Check for mirroring unbound skin
			MessageDialog(GeLoadString(MD_MIRROR_SKINNED_MESH));
			if(doc)
			{
				doc->DoUndo();
				doc->SetActiveObject(NULL);
			}
			break;
		}
	}
	return true;
}

Bool RegisterCDJSMessagePlugin(void)
{
	if(CDFindPlugin(ID_JSMESSAGEPLUGIN,CD_MESSAGE_PLUGIN)) return true;
	
	return RegisterMessagePlugin(ID_JSMESSAGEPLUGIN,"CDJS Message",0,CDDataAllocator(CDJSMessagePlugin));
}
