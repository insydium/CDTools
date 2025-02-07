//	Cactus Dan's Align to Camera 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "lib_modeling.h"
#include "customgui_priority.h"

#include "tAlignToCam.h"
#include "CDAlignCam.h"
#include "CDGeneral.h"
#include "CDCompatibility.h"
#include "CDTagData.h"

//#include "CDDebug.h"

enum
{
	CUR_PROJECTION		= 10000
};

class CDAlignToCamera : public CDTagData
{
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);

    static NodeData* Alloc(void) { return CDDataAllocator(CDAlignToCamera); }
};

Bool CDAlignToCamera::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(CUR_PROJECTION,0);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,true);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDListData atcd;
	PluginMessage(ID_CDALIGNTOCAMERA,&atcd);
	if(atcd.list) atcd.list->Append(node);
	
	return true;
}

void CDAlignToCamera::Free(GeListNode *node)
{	
	CDListData atcd;
	PluginMessage(ID_CDALIGNTOCAMERA,&atcd);
	if(atcd.list) atcd.list->Remove(node);
}

Bool CDAlignToCamera::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseDocument *doc = tag->GetDocument(); if(!doc) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	//ListReceivedMessages("CDAlignToCamera",type,data);
	
	BaseDraw *bd = doc->GetActiveBaseDraw();
	switch(type)
	{
		case ID_CDALIGNTOCAMERA:
		{
			if(bd)
			{
				LONG oldPrj = tData->GetLong(CUR_PROJECTION), newPrj = bd->GetProjection();
				if(oldPrj != newPrj)
				{
					tData->SetLong(CUR_PROJECTION,newPrj);
					
					EventAdd(EVENT_FORCEREDRAW);
				}
			}
			break;
		}
	}
	
	return true;
}

LONG CDAlignToCamera::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseDraw   *bd = NULL;
	if(!CheckIsRunning(CHECKISRUNNING_EXTERNALRENDERING))
	{
		bd = doc->GetActiveBaseDraw(); if (!bd) return false;
	}
	else bd = doc->GetRenderBaseDraw(); if (!bd) return false;
	
	BaseObject *cp = bd->GetSceneCamera(doc); 
	if(!cp) cp = bd->GetEditorCamera();
	if(!cp) return false;
	
	Matrix opM = op->GetMg(), cpM;
	switch(bd->GetProjection())
	{
		case 0:
		{
			cpM = cp->GetMg();
			break;
		}
		default:
		{
			cpM = bd->GetMg();
			break;
		}
	}
	
	if(!(op->GetInfo() & OBJECT_INPUT))
	{
		Vector opSca = CDGetScale(op);
		
		opM.v3 = VNorm(cpM.v3);
		opM.v1 = VNorm(VCross(cpM.v2, opM.v3));
		opM.v2 = VNorm(VCross(opM.v3, opM.v1));
		
		op->SetMg(opM);
		CDSetScale(op,opSca);
		op->Message(MSG_UPDATE);
	}
	else
	{
		BaseObject *vop = op->GetCache();
		if(vop)
		{
			if(vop->GetDown())
			{
				BaseObject *ch = vop->GetDown();
				
				while(ch)
				{
					Vector chSca = CDGetScale(ch);
					Matrix chM = ch->GetMg();			
					
					chM.v3 = VNorm(cpM.v3);
					chM.v1 = VNorm(VCross(cpM.v2, chM.v3));
					chM.v2 = VNorm(VCross(chM.v3, chM.v1));
					
					ch->SetMg(chM);
					CDSetScale(ch,chSca);
					ch->Message(MSG_UPDATE);
					
					ch = ch->GetNext();
				}
			}
			else
			{
				Vector opSca = CDGetScale(op);
				
				opM.v3 = VNorm(cpM.v3);
				opM.v1 = VNorm(VCross(cpM.v2, opM.v3));
				opM.v2 = VNorm(VCross(opM.v3, opM.v1));
				
				op->SetMg(opM);
				CDSetScale(op,opSca);
				op->Message(MSG_UPDATE);
			}
		}
	}

	return CD_EXECUTION_RESULT_OK;
}

Bool RegisterCDAlignToCamera(void)
{
	LONG info = TAG_EXPRESSION|TAG_VISIBLE;
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDALIGNCAM); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDALIGNTOCAMERA,name,info,CDAlignToCamera::Alloc,"tAlignToCam","tAlignToCam.tif",0);
}
