//
//  CDObjectData.h
//	R9 to R12+ backward compatible object data class
//  Created by Dan Libisch on 6/5/13.
//
#ifndef _CDOBJECTDATA_H_
#define _CDOBJECTDATA_H_


#include "c4d.h"

#include "CDCompatibility.h" // header for compatibility constants and functions

// object info flags (R12+)
#define CD_OBJECT_POINTOBJECT			(1<<11)
#define CD_OBJECT_POLYGONOBJECT			(1<<12)
#define CD_OBJECT_NO_PLA				(1<<13)
#define CD_OBJECT_DONTFREECACHE			(1<<14)
#define CD_OBJECT_CALL_ADDEXECUTION		(1<<15)

class CDObjectData : public ObjectData
{
public:
	CDObjectData();
	virtual ~CDObjectData();
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);

	virtual void CDGetDimension(BaseObject *op, Vector *mp, Vector *rad);
	virtual LONG CDDetectHandle(BaseObject *op, BaseDraw *bd, LONG x, LONG y, LONG qualifier);
	virtual Bool CDMoveHandle(BaseObject *op, BaseObject *undo, const Vector &mouse_pos, LONG hit_id, LONG qualifier, BaseDraw *bd);

	virtual BaseObject* CDGetVirtualObjects(BaseObject* op, HierarchyHelp* hh);
	
	virtual Bool CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool CDAddToExecution(BaseObject* op, PriorityList* list);
	virtual LONG CDExecute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags);

	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags);
	virtual Bool CDSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags);
	
#if API_VERSION < 12000
	virtual Bool CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
		{ return CDCopyTo(dest, snode, dnode, flags, trn); }
	
	virtual void GetDimension(PluginObject *op, Vector *mp, Vector *rad)
		{ CDGetDimension(op, mp, rad); }
	virtual LONG DetectHandle(PluginObject *op, BaseDraw *bd, LONG x, LONG y, LONG qualifier)
		{ return CDDetectHandle(op, bd, x, y, qualifier); }
	virtual Bool MoveHandle(PluginObject *op, PluginObject *undo, const Matrix &tm, LONG hit_id, LONG qualifier)
		{ return CDMoveHandle(op, undo, (Vector&)tm.off, hit_id, qualifier, NULL); }

	virtual BaseObject* GetVirtualObjects(PluginObject *op, HierarchyHelp *hh)
		{ return CDGetVirtualObjects(op, hh); }
	
	virtual Bool Draw(PluginObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh)
		{ return CDDraw(op, drawpass, bd, bh); }
	virtual Bool AddToExecution(PluginObject* op, PriorityList* list)
		{ return CDAddToExecution(op, list); }
	virtual LONG Execute(PluginObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
		{ return CDExecute(op, doc, bt, priority, flags); }
	
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, GeData &t_data, LONG flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(node, id, t_data, flags, itemdesc); }
	virtual Bool GetDDescription(GeListNode *node, Description *description, LONG &flags)
		{ return CDGetDDescription(node, description, flags); }
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags)
		{ return CDGetDParameter(node, id, t_data, flags); }
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags)
		{ return CDSetDParameter(node, id, t_data, flags); }
#else
	virtual Bool CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn)
		{ return CDCopyTo(dest, snode, dnode, (LONG)flags, trn); }
	
	virtual void GetDimension(BaseObject* op, Vector* mp, Vector* rad)
		{ CDGetDimension(op, mp, rad); }
	virtual LONG DetectHandle(BaseObject* op, BaseDraw* bd, LONG x, LONG y, QUALIFIER qualifier)
		{ return CDDetectHandle(op, bd, x, y, (LONG)qualifier); }
	#if API_VERSION < 13000
		virtual Bool MoveHandle(BaseObject* op, BaseObject* undo, const Matrix& tm, LONG hit_id, QUALIFIER qualifier)
			{ return CDMoveHandle(op, undo, (Vector&)tm.off, hit_id, (LONG)qualifier, NULL); }
	#else
		virtual Bool MoveHandle(BaseObject *op, BaseObject *undo, const Vector &mouse_pos, LONG hit_id, QUALIFIER qualifier, BaseDraw *bd)
            { return CDMoveHandle(op, undo, mouse_pos, hit_id, (LONG)qualifier, bd); }
	#endif
	
	virtual BaseObject* GetVirtualObjects(BaseObject* op, HierarchyHelp* hh)
		{ return CDGetVirtualObjects(op, hh); }

	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
		{ return (DRAWRESULT)CDDraw(op, (LONG)drawpass, bd, bh); }
	virtual Bool AddToExecution(BaseObject* op, PriorityList* list)
		{ return CDAddToExecution(op, list); }
	virtual EXECUTIONRESULT Execute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, EXECUTIONFLAGS flags)
		{ return (EXECUTIONRESULT)CDExecute(op, doc, bt, priority, (LONG)flags); }
	
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(node, id, t_data, 0, itemdesc); }
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
		{ return CDGetDDescription(node, description, (LONG&)flags); }
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags)
		{ return CDGetDParameter(node, id, t_data, (LONG&)flags); }
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
		{ return CDSetDParameter(node, id, t_data, (LONG&)flags); }
#endif
	
	//Parent class function call returns
	Bool CDSuperDrawReturn(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	Bool CDSuperGetDDescriptionReturn(GeListNode *node, Description *description, LONG &flags);
	Bool CDSuperGetDEnablingReturn(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	Bool CDSuperGetDParameterReturn(GeListNode* node, const DescID& id, GeData& t_data, LONG& flags);
	Bool CDSuperSetDParameterReturn(GeListNode* node, const DescID& id, const GeData& t_data, LONG& flags);
};



#endif