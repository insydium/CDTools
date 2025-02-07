#ifndef _CDSkinRefTag_H_
#define _CDSkinRefTag_H_

// c4d includes
#include "c4d.h"

// CDM Includes
#include "CDMStructures.h"
#include "CDTagData.h"
#include "CDArray.h"


enum // CD Skin Reference
{
	SKR_SET_REFERENCE			= 1000,
	SKR_RESTORE_REFERENCE		= 1001,
	SKR_S_POINT_COUNT			= 1002,
	SKR_DOUBLE_CHECK			= 1003,
	
	SKR_REF_MATRIX				= 1005,	
	SKR_REFERENCE_SCALE			= 1006,
	SKR_SCALE					= 1007,
	
	SKR_UPDATE_REFERENCE		= 1020,
	
	SKR_REF_EDITING				= 1030,
	
	SKR_POINTS_ADDED			= 1500,
	
	SKR_TAG_LINK				= 2000,
	
	SKR_CT_COUNT				= 3000,
	SKR_CT_LINK					= 3001,
	
	SKR_REF_MATRIX_FIX			= 10000
	
};


class CDSkinRefPlugin : public CDTagData
{
private:
	Bool	mScaling;
	Vector	startScale;
	
	void RemapReferencePoints(Vector *padr, TranslationMaps *map);
	void RecalculateSkinRef(BaseTag *tag, Matrix newM, Matrix oldM);
	void RemampCDSkinClusterTags(BaseDocument *doc, BaseObject *op, TranslationMaps *tMap);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	
public:
	Bool			doubleCheck;
	//CDMRefPoint		*skinRef, *deformRef;
	CDArray<CDMRefPoint>	skinRef, deformRef;
	
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	
	//CDMRefPoint*  GetSkinReference(void) { return skinRef; }
	//CDMRefPoint*  GetDeformReference(void) { return deformRef; }
	CDMRefPoint*  GetSkinReference(void) { return skinRef.Array(); }
	CDMRefPoint*  GetDeformReference(void) { return deformRef.Array(); }
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSkinRefPlugin); }
};



#endif
