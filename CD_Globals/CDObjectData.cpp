//
//  CDObjectData.cpp
//	R9 to R12+ backward compatible object data class
//  Created by Dan Libisch on 6/5/13.
//
#include "CDObjectData.h"


CDObjectData::CDObjectData()
{
}

CDObjectData::~CDObjectData()
{
}

Bool CDObjectData::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	return true;
}

void CDObjectData::CDGetDimension(BaseObject *op, Vector *mp, Vector *rad)
{
}

LONG CDObjectData::CDDetectHandle(BaseObject *op, BaseDraw *bd, LONG x, LONG y, LONG qualifier)
{
	return NOTOK;
}

Bool CDObjectData::CDMoveHandle(BaseObject *op, BaseObject *undo, const Vector &mouse_pos, LONG hit_id, LONG qualifier, BaseDraw *bd)
{
	return true;
}

BaseObject* CDObjectData::CDGetVirtualObjects(BaseObject* op, HierarchyHelp* hh)
{
	return NULL;
}

Bool CDObjectData::CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	// CDDraw must return with the following code:
	return CDSuperDrawReturn(op, drawpass, bd, bh);
}

Bool CDObjectData::CDAddToExecution(BaseObject* op, PriorityList* list)
{
	return false;  // returns false by default, if overridden return true
}

LONG CDObjectData::CDExecute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
{
	return CD_EXECUTION_RESULT_OK;
}

Bool CDObjectData::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	return true;
}

Bool CDObjectData::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	/*
	 CDGetDDescription must return with the following code
	 if creating descriptions programatically:
	 
	 flags |= CD_DESCFLAGS_DESC_LOADED; // "CD" description flag equivalents set
	 return CDSuperGetDDescriptionReturn(node, description, flags);
	 */
	
	return true;
}

Bool CDObjectData::CDGetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
{
	return true;
}

Bool CDObjectData::CDSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
{
	return true;
}


//Parent class function call returns
Bool CDObjectData::CDSuperDrawReturn(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
#if API_VERSION < 12000
	#if API_VERSION < 9800
		return ObjectData::Draw((PluginObject*)op, drawpass, bd, bh);
	#else
		return ObjectData::Draw(op, drawpass, bd, bh);
	#endif
#else
	return CD_DRAWRESULT_OK;
#endif
}

Bool CDObjectData::CDSuperGetDDescriptionReturn(GeListNode *node, Description *description, LONG &flags)
{
#if API_VERSION < 12000
	return ObjectData::GetDDescription(node,description,flags);
#else
	return ObjectData::GetDDescription(node,description,(DESCFLAGS_DESC&)flags);
#endif
}

Bool CDObjectData::CDSuperGetDEnablingReturn(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
#if API_VERSION < 12000
	GeData data = t_data;
	return ObjectData::GetDEnabling(node, id, data, flags, itemdesc);
#else
	return ObjectData::GetDEnabling(node, id, t_data, (DESCFLAGS_ENABLE)flags, itemdesc);
#endif
}

Bool CDObjectData::CDSuperGetDParameterReturn(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
{
#if API_VERSION < 12000
	return ObjectData::GetDParameter(node, id, t_data, flags);
#else
	return ObjectData::GetDParameter(node, id, t_data, (DESCFLAGS_GET&)flags);
#endif
}

Bool CDObjectData::CDSuperSetDParameterReturn(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
{
#if API_VERSION < 12000
	return ObjectData::SetDParameter(node, id, t_data, flags);
#else
	return ObjectData::SetDParameter(node, id, t_data, (DESCFLAGS_SET&)flags);
#endif
}
