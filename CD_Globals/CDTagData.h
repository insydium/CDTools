//
//  CDTagData.h
//	R9 to R12+ backward compatible tag data class
//  Created by Dan Libisch on 6/5/13.
//

#ifndef _CDTAGDATA_H_
#define _CDTAGDATA_H_

#include "c4d.h"

#include "CDCompatibility.h" // header for compatibility constants and functions

class CDTagData : public TagData
{
public:
	CDTagData();
	virtual ~CDTagData();
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool CDAddToExecution(BaseTag* tag, PriorityList* list);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags);
	virtual Bool CDSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags);
	
#if API_VERSION < 12000
	virtual Bool CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
		{ return CDCopyTo(dest, snode, dnode, flags, trn); }
	virtual Bool Draw(PluginTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
		{ return CDDraw(tag, op, bd, bh); }
	virtual Bool AddToExecution(PluginTag* tag, PriorityList* list)
		{ return CDAddToExecution(tag, list); }
	virtual LONG Execute(PluginTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
		{ return CDExecute(tag,doc,op,bt,priority,flags); }
	
	virtual Bool GetDDescription(GeListNode *node, Description *description, LONG &flags)
		{ return CDGetDDescription(node, description, flags); }
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, GeData &t_data, LONG flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(node, id, t_data, flags, itemdesc); }
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
		{ return CDGetDParameter(node, id, t_data, flags); }
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
		{ return CDSetDParameter(node, id, t_data, flags); }
#else
	virtual Bool CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn)
		{ return CDCopyTo(dest, snode, dnode, (LONG)flags, trn); }
	virtual Bool Draw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
		{ return CDDraw(tag, op, bd, bh); }
	virtual Bool AddToExecution(BaseTag* tag, PriorityList* list)
		{ return CDAddToExecution(tag, list); }
	virtual EXECUTIONRESULT Execute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, EXECUTIONFLAGS flags)
		{ return (EXECUTIONRESULT)CDExecute(tag,doc,op,bt,priority,(LONG)flags); }
	
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
		{ return CDGetDDescription(node, description, (LONG&)flags); }
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(node, id, t_data, 0, itemdesc); }
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags)
		{ return CDGetDParameter(node, id, t_data, (LONG&)flags); }
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
		{ return CDSetDParameter(node, id, t_data, (LONG&)flags); }
#endif
	
	//Parent class function call returns
	Bool CDSuperGetDDescriptionReturn(GeListNode *node, Description *description, LONG &flags);
	Bool CDSuperGetDEnablingReturn(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	Bool CDSuperGetDParameterReturn(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags);
	Bool CDSuperSetDParameterReturn(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags);
	
};


#endif