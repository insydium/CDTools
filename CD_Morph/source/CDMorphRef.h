#ifndef _CDMorphRef_H_
#define _CDMorphRef_H_

// c4d includes
#include "c4d.h"

// CDM Includes
#include "CDMStructures.h"
#include "CDTagData.h"
#include "CDArray.h"


enum
{
	MR_REFERENCE_LINK			= 1000,
	MR_REFERENCE_SET			= 1001,
	
	MR_POINT_COUNT				= 1002,
	MR_REFERENCE_NEW			= 1003,
	MR_REFERENCE_SCALE			= 1004,
	MR_SCALE					= 1005,
	MR_REF_MATRIX				= 1006,
	MR_M_SCALING				= 1007,
	
	MR_REMAP_COUNT				= 1009,
	
	MR_UPDATE_REFERENCE		= 1020,
	MR_CDMR_LEVEL				= 1021,
	
	MR_INIT_REF				= 2000,
	//MR_EDIT_MESH				= 2001, // in tCDMorphRef.h
	//MR_RESET_REF				= 2002, // in tCDMorphRef.h
	MR_EDITING					= 2003,
	MR_DELTA_EDITING			= 2004,
	
	MR_POINTS_ADDED			= 3000,
	
	MR_FFD_REF_SIZE			= 3100,	// Vector
	MR_FFD_REF_XSUB			= 3101,	// LONG
	MR_FFD_REF_YSUB			= 3102,	// LONG
	MR_FFD_REF_ZSUB			= 3103	// LONG
};


class CDMorphRefPlugin : public CDTagData
{
private:
	Bool	mScaling, editMesh;
	Vector	startScale;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseObject *op, BaseContainer *tData);
	
	void RecalculateMorphRef(BaseDocument *doc, BaseObject *op, BaseTag *tag, Matrix newM, Matrix oldM);
	void RemapReferencePoints(Vector *padr, TranslationMaps *map);
	void RemampCDMorphTags(BaseDocument *doc, BaseObject *op, TranslationMaps *tMap);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	Bool InitReference(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	Bool SetReference(BaseContainer *tData, BaseObject *op);
	Bool RestoreReference(BaseContainer *tData, BaseObject *op);
	
public:
	//CDMRefPoint *rpadr, *mpadr;
	CDArray<CDMRefPoint>	rpadr, mpadr;
	
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	//CDMRefPoint* GetReferenceArray(void) { return rpadr; }
	//CDMRefPoint* GetMorphArray(void) { return mpadr; }
	CDMRefPoint* GetReferenceArray(void) { return rpadr.Array(); }
	CDMRefPoint* GetMorphArray(void) { return mpadr.Array(); }
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMorphRefPlugin); }
};


#endif
