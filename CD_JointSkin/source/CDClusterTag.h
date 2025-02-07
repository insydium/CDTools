#ifndef _CDClusterTag_H_
#define _CDClusterTag_H_

#include "lib_modeling.h"

#include "CDTagData.h"
#include "CDJSStructures.h"
#include "CDGeneral.h"
#include "CDCompatibility.h"
#include "CDArray.h"

enum // CD Skin Cluster
{
	//CLST_DEST_LINK				= 1000,
	//CLST_BIND_POINTS				= 1001,
	//CLST_UNBIND_POINTS			= 1002,
	//CLST_MIX_PREVIOUS			= 1003,
	//CLST_BLEND_ROTATION			= 1004,
	//CLST_ROT_MIX					= 1005,
	//CLST_ATTACH_CHILD			= 1006,
	//CLST_SHOW_GUIDE				= 1007,
	//CLST_JOINT_OFFSET			= 1008,
	//CLST_CHILD_OFFSET			= 1009,
	
	//CLST_RESTRICT_TO_Z			= 1010,
	//CLST_STRENGTH				= 1011,
	//CLST_LINE_COLOR				= 1012,
	//CLST_GUIDE_SIZE				= 1013,
	//CLST_POS_MIX					= 1014,
	//CLST_ATTACH_EXTERNAL			= 1015,
	//CLST_ATTACH_LINK				= 1016,
	//CLST_LOCAL_TRANS				= 1017,
	
	CLST_CHECK_REF				= 1020,
	
	CLST_REF_MATRIX			= 2000,
	CLST_DEST_MATRIX		= 2001,
	CLST_DEST_ID			= 2002,
	CLST_DEST_SET			= 2003,
	
	CLST_BOUND					= 2004,
	CLST_T_POINT_COUNT			= 2005,
	CLST_T_POINT_SET			= 2006,
	
	CLST_C_POINT_COUNT			= 2007,
	CLST_C_REF_MATRIX			= 2008,
	CLST_B_REF_MATRIX			= 2009,
	CLST_R_POINT_COUNT			= 2010,
	//CLST_ENABLE_PIVOT			= 2011,
	//CLST_PIVOT_OFFSET			= 2012,
	//CLST_ID_CLUSTERBLEND			= 2013,
	CLST_GPL_MATRIX				= 2014,
	
	//CLST_PURCHASE				= 2100,
	
	CLST_OPTIMIZE		= 3000,
};


class CDClusterTagPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
public:
	//CDClusterWeight	*cWeight;
	CDArray<CDClusterWeight>	cWeight;
	
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	Bool GetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool SetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool RemapClusterWeights(BaseContainer *tData, TranslationMaps *map);	
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDClusterTagPlugin); }
};


#endif

