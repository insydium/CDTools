//
//  CDClothCollider.h
//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch
//

#ifndef _CDCLOTHCOLLIDER_H_
#define _CDCLOTHCOLLIDER_H_

#include "stdlib.h"
//#include <vector>
//using std::vector;

#include "c4d.h"
//#include "c4d_symbols.h"
//#include "lib_collider.h"
#include "customgui_priority.h"

#include "tCDClothCollider.h"
#include "CDTagData.h"
#include "CDCloth.h"
#include "CDArray.h"

enum
{
	//CLD_COLLIDER_TYPE			= 1000,
		//CLD_PLANE				= 0,
		//CLD_SPHERE			= 1,
		//CLD_JOINT			= 2,
		//CLD_POLYGON			= 3,
	
	//CLD_RADIUS				= 1001,
	
	//CLD_PLANE_NORM			= 1002,
		//CLD_NORM_XP			= 0,
		//CLD_NORM_XN			= 1,
		//CLD_NORM_YP			= 2,
		//CLD_NORM_YN			= 3,
		//CLD_NORM_ZP			= 4,
		//CLD_NORM_ZN			= 5,
	
	//CLD_OFFSET				= 1003,
	
	//CLD_SELECTION_GROUP		= 1100,
	//CLD_SET_SELECTION			= 1101,
	//CLD_EDIT_SELECTION		= 1102,
	//CLD_RESTORE_SELECTION		= 1103,
	
	//CLD_PURCHASE				= 2100,
	
	CLD_SELECTION_SET			= 3000,
	CLD_SELECTION_COUNT			= 3001,
	
	CLD_POLYGON_OBJECT			= 4000
	
};


class CDClothCollider : public CDTagData
{
private:
	CDAABB			bounds;
	CDArray<LONG>	plyInd;
	CDArray<LONG>	spcA, spcB, spcC, spcD, spcE, spcF, spcG, spcH;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool BuildSpacePartitions(BaseObject *op);
	void FreeSpacePartitions(void);
	
	LONG GetNearestPointIndex(Vector pt, BaseObject *op);
	LONG GetNearestPolygonIndex(Vector pt, Vector *padr, LONG pCnt, BaseObject *op, CPolygon *vadr, LONG vCnt);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	
	CDAABB* GetBounds(void) { return &bounds; }
	Vector GetNearestSurfacePoint(BaseObject *op, Vector pt, Vector &n);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDClothCollider); }
	
};



#endif