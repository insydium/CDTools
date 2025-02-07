#ifndef _CDSymmetryTag_H_
#define _CDSymmetryTag_H_

// c4d includes
#include "c4d.h"
#include "lib_modeling.h"

#include "CDTagData.h"
#include "CDArray.h"

enum
{	
	//SYM_USE_SYMMETRY				= 1000,
	//SYM_SET_SYMMETRY				= 1001,
	//SYM_RELEASE_SYMMETRY			= 1002,
	//SYM_TOLERANCE				= 1003,
	SYM_OPEN_TOOL				= 1004,
	
	//SYM_SYMMETRY_AXIS				= 2000,
	//SYM_MX					= 2001,
	//SYM_MY					= 2002,
	//SYM_MZ					= 2003,
	
	//SYM_RESTRICT_SYM			= 2010,
	//SYM_RESTRICT_AXIS			= 2011,
	//SYM_POSITIVE			= 2012,
	//SYM_NEGATIVE			= 2013,
	
	//SYM_LOCK_CENTER				= 2020,
	
	//SYM_SHOW_GUIDE				= 2030,
	//SYM_GUIDE_SIZE				= 2031,
	//SYM_GUIDE_COLOR				= 2032,
	
	//SYM_PURCHASE				= 2100,
	
	SYM_POINTS_ADDED			= 3000,
	
	SYM_POINT_COUNT				= 10000,
	SYM_OP_ID					= 10001,
	SYM_SYMMETRY_IS_SET			= 10002,
	SYM_OLD_PT_COUNT			= 10003
	
};


class CDSymmetryTag : public CDTagData
{
private:
	//LONG	ipA, ipB, ipC, ipD, inA, inB, inC, inD;
	//LONG	*pA, *pB, *pC, *pD, *nA, *nB, *nC, *nD;
	CDArray<LONG>	pA, pB, pC, pD, nA, nB, nC, nD;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseObject *op, BaseContainer *tData);
	
	Bool SetNewPointsSymmetry(BaseTag *tag, BaseObject *op, BaseContainer *tData, TranslationMaps *tMap);
	Bool SetAddedPointsSymmetry(BaseTag *tag, BaseObject *op, BaseContainer *tData, LONG oCnt, LONG nCnt);
	
	void BuildSpacePartitions(BaseContainer *tData, Vector *padr, Vector mp, LONG pCnt, LONG qSize);
	void CheckMirrorAssignment(Vector *padr, Vector pt, Vector ptMir, LONG pInd, LONG mInd);
	LONG GetMirrorIndex(Vector *padr, Vector pt, LONG *neg, LONG n, LONG pInd, LONG pCnt, Real t);
	Bool SetSymmetry(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void RemapMirrorPoints(TranslationMaps *tMap);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	void MirrorSplineTangents(BaseContainer *tData, BaseObject *op, LONG src, LONG dst);
	
public:
	//LONG	*mpt;
	CDArray<LONG>	mpt;
	
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
	
	//LONG* GetMirrorPoints(void) { return mpt; }
	LONG* GetMirrorPoints(void) { return mpt.Array(); }
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSymmetryTag); }
};

#endif

