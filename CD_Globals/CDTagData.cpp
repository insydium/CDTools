//
//  CDTagData.cpp
//	R9 to R12+ backward compatible tag data class
//  Created by Dan Libisch on 6/5/13.
//
 
#include "CDTagData.h"

CDTagData::CDTagData()
{
}

CDTagData::~CDTagData()
{
}

Bool CDTagData::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	return true;
}

Bool CDTagData::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	return true;
}

Bool CDTagData::CDAddToExecution(BaseTag* tag, PriorityList* list)
{
	return false; // returns false by default, if overridden return true
} 

LONG CDTagData::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	return CD_EXECUTION_RESULT_OK;
}

Bool CDTagData::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	return true;
}

Bool CDTagData::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	/*
	 CDGetDDescription must return with the following code
	 if creating descriptions programatically:
	 
	 flags |= CD_DESCFLAGS_DESC_LOADED; // "CD" description flag equivalents set
	 return CDSuperGetDDescriptionReturn(node,description,flags);
	 #endif
	 */
	
	return true;
}

Bool CDTagData::CDGetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
{
	return true;
}

Bool CDTagData::CDSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
{
	return true;
}


//Parent class function returns
Bool CDTagData::CDSuperGetDDescriptionReturn(GeListNode *node, Description *description, LONG &flags)
{
#if API_VERSION < 12000
	return TagData::GetDDescription(node, description, flags);
#else
	return TagData::GetDDescription(node, description, (DESCFLAGS_DESC&)flags);
#endif
}

Bool CDTagData::CDSuperGetDEnablingReturn(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
#if API_VERSION < 12000
	GeData data = t_data;
	return TagData::GetDEnabling(node, id, data, flags, itemdesc);
#else
	return TagData::GetDEnabling(node, id, t_data, (DESCFLAGS_ENABLE)flags, itemdesc);
#endif
}

Bool CDTagData::CDSuperGetDParameterReturn(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
{
#if API_VERSION < 12000
	return TagData::GetDParameter(node, id, t_data, flags);
#else
	return TagData::GetDParameter(node, id, t_data, (DESCFLAGS_GET&)flags);
#endif
}

Bool CDTagData::CDSuperSetDParameterReturn(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
{
#if API_VERSION < 12000
	return TagData::SetDParameter(node, id, t_data, flags);
#else
	return TagData::SetDParameter(node, id, t_data, (DESCFLAGS_SET&)flags);
#endif
}


