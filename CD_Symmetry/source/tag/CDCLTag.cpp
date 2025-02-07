//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2009 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "tCDCL.h"
#include "CDSymmetry.h"
#include "CDTagData.h"
#include "CDArray.h"

enum
{	
	LKC_POINT_COUNT					= 1000,
	LKC_SYMMETRY_AXIS				= 1001,
	LKC_TOLERANCE					= 1002,
	LKC_LOCK_ON						= 1003
};

class CDLockCenterPoints : public CDTagData
{
private:
	CDArray<LONG>	cpt;
	
	void UpdateCenterPoints(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDLockCenterPoints); }
};

Bool CDLockCenterPoints::Init(GeListNode *node)
{	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDSData cld;
	PluginMessage(ID_CDCENTERLOCKTAG,&cld);
	if(cld.list) cld.list->Append(node);
	
	return true;
}

void CDLockCenterPoints::Free(GeListNode *node)
{	
	cpt.Free();
	
	CDSData cld;
	PluginMessage(ID_CDCENTERLOCKTAG,&cld);
	if(cld.list) cld.list->Remove(node);
}

Bool CDLockCenterPoints::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG cnt = tData->GetLong(LKC_POINT_COUNT);

	if (cnt > 0)
	{
		//Level 0
		if (level >= 0)
		{
			LONG i;
			if(!cpt.Alloc(cnt)) return false;
			for (i=0; i<cnt; i++)
			{
				CDHFReadLong(hf, &cpt[i]);
			}
		}
	}
	
	
	return true;
}

Bool CDLockCenterPoints::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG cnt = tData->GetLong(LKC_POINT_COUNT);

	if (cnt > 0)
	{
		//Level 0
		LONG i;
		for (i=0; i<cnt; i++)
		{
			CDHFWriteLong(hf, cpt[i]);
		}
	}
	
	return true;
}

Bool CDLockCenterPoints::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG cnt = tData->GetLong(LKC_POINT_COUNT);

	if(cnt > 0)
	{
		cpt.Copy(((CDLockCenterPoints*)dest)->cpt);
	}
	
	return true;
}

void CDLockCenterPoints::UpdateCenterPoints(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Vector  *padr = GetPointArray(op);
	if(padr)
	{
		Real t = tData->GetReal(LKC_TOLERANCE);
		LONG i, pCnt = ToPoint(op)->GetPointCount();//cnt = 0, 
		
		cpt.Free();
		cpt.Init();
		
		for(i=0; i<pCnt; i++)
		{
			Vector pt = padr[i];
			switch(tData->GetLong(LKC_SYMMETRY_AXIS))
			{
				case 0:
				{
					if(pt.x < t && pt.x > -t) cpt.Append(i);
					break;
				}
				case 1:
				{
					if(pt.y < t && pt.y > -t) cpt.Append(i);
					break;
				}
				case 2:
				{
					if(pt.z < t && pt.z > -t) cpt.Append(i);
					break;
				}
			}
		}
		
		tData->SetLong(LKC_POINT_COUNT,cpt.Size());
	}
}

Bool CDLockCenterPoints::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	switch (type)
	{
		case CD_MSG_UPDATE:
		{
			UpdateCenterPoints(tag,doc,op,tData);
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			UpdateCenterPoints(tag,doc,op,tData);
			break;
		}
		case MSG_TRANSLATE_POINTS:
		{
			UpdateCenterPoints(tag,doc,op,tData);
			break;
		}
	}
	
	return true;
}

LONG CDLockCenterPoints::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!IsValidPolygonObject(op)) return false;
	
	BaseObject *pr = op->GetUp();
	if(!pr) return false;
	if(pr->GetType() != ID_CDDYNSYMMETRY) return false;
	
	if(cpt.IsEmpty()) return false;
	Vector  *padr = GetPointArray(op); if(!padr) return false;
	
	LONG pCnt = ToPoint(op)->GetPointCount();
	LONG i, cnt = tData->GetLong(LKC_POINT_COUNT);
	
	for(i=0; i<cpt.Size(); i++)
	{
		LONG ind = cpt[i];
		if(ind < pCnt)
		{
			Vector pt = padr[ind];
			switch(tData->GetLong(LKC_SYMMETRY_AXIS))
			{
				case 0:
				{
					pt.x = 0.0;
					break;
				}
				case 1:
				{
					pt.y = 0.0;
					break;
				}
				case 2:
				{
					pt.z = 0.0;
					break;
				}
			}
			padr[ind] = pt;
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool RegisterCDLockCenterPoints(void)
{
	// decide by name if the plugin shall be registered - just for user convenience
	return CDRegisterTagPlugin(ID_CDCENTERLOCKTAG,"CD CL Tag",TAG_EXPRESSION|PLUGINFLAG_HIDE,CDLockCenterPoints::Alloc,"tCDCL","CDCL.tif",0);
}
