#ifndef _CDSkinTag_H_
#define _CDSkinTag_H_

// c4d includes
#include "c4d.h"
#include "lib_modeling.h"

// CDJS Includes
#include "CDJSStructures.h"
#include "CDMStructures.h"
#include "CDTagData.h"

enum // CD Skin
{
	//SKN_BIND_SKIN					= 1000,
	//SKN_UNBIND_SKIN				= 1001,
	SKN_SKIN_MATRIX				= 1002,
	SKN_S_POINT_COUNT			= 1003,
	SKN_DEST_ID					= 1004,
	
	SKN_INIT_WEIGHT				= 1005,
	SKN_W_SET					= 1006,
	SKN_J_LINK_ID				= 1007,
	SKN_ACCUMULATE				= 1008,
	SKN_SET_T_WEIGHT			= 1009,
	SKN_GET_T_WEIGHT			= 1010,
	
	SKN_ADD_JOINT				= 1011,
	SKN_DELETE_JOINT			= 1012,
	SKN_J_COUNT_CHANGE			= 1013,
	SKN_JOINT_DELETED			= 1014,
	
	SKN_MIRROR_WEIGHT			= 1015,
	SKN_MIRROR_AXIS				= 1016,
	SKN_P_TOLERANCE				= 1017,
	SKN_J_TOLERANCE				= 1018,
	SKN_INIT_MJ_INDEX			= 1019,
	SKN_INIT_MP_INDEX			= 1020,
	SKN_P_MIRRORED				= 1021,
	SKN_MJ_ID					= 1022,
	SKN_SET_PAINT_WEIGHT		= 1023,
	SKN_MIRROR_DIRECTION		= 1024,
	
	//SKN_GO_TO_BIND_POSE			= 1025,
	SKN_J_MIRRORED				= 1026,
	
	//SKN_ID_SKIN_REFERENCE			= 1027,
	//SKN_EDIT_REF					= 1028,
	//SKN_RESET_REF					= 1029,
	
	SKN_REF_EDITING				= 1030,
	SKN_AUTO_BIND_SKIN			= 1031,
	SKN_FREE_MIRROR_IND			= 1032,
	
	SKN_SQUASH_N_STRETCH_ON		= 1033,
	
	SKN_NORMALIZED_WEIGHT		= 1040,
	SKN_NORMALIZE_ALL			= 1041,
	
	//SKN_PURCHASE				= 1050,
	//SKN_CHECK_REF				= 1051,
	
	SKN_J_COUNT					= 2000,
	SKN_J_LINK					= 2001,
	
	SKN_BOUND					= 5000,
	SKN_J_MATRIX				= 5001,
	
	SKN_JL_MATRIX				= 8001,
};

class CDSkinPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseObject *op, BaseContainer *tData);
	
	void InitSkinWeight(BaseContainer *tData, LONG pCnt, LONG jCnt);
	void InitJointMirrorIndex(BaseDocument *doc, BaseObject *op, BaseContainer *tData, LONG *jIndM, LONG jCnt);
	void InitPointMirrorIndex(BaseObject *op, BaseContainer *tData, CDMRefPoint *skinRef, LONG *pIndM, LONG pCnt);
	void InitJointStorage(LONG jCnt);
	
	void AccumulateWeights(BaseDocument *doc, BaseContainer *tData, LONG pCnt, LONG jCnt);
	void NormalizeAllWeights(BaseDocument *doc, BaseContainer *tData, LONG pCnt, LONG jCnt, Real *minWt);
	void RemapSkinWeights(BaseContainer *tData, TranslationMaps *map);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	Bool UpdateJointStorage(BaseDocument *doc, BaseContainer *tData);
	
public:
	Bool			jCountChange;
	//LONG			*pIndM, *jIndM;
	//CDSkinVertex	*skinWeight;
	//CDJTransM		*jM;
	//Matrix			*J;
	CDArray<LONG>			pIndM, jIndM;
	CDArray<CDSkinVertex>	skinWeight;
	CDArray<CDJTransM>		jM;
	CDArray<Matrix>			J;
	
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	Bool GetWeight(BaseContainer *tData, Real *tWeight, LONG pCnt);
	Bool SetWeight(BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt);
	Bool GetJointColors(BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt);
	
	//CDSkinVertex* GetSkinWeight(void) { return skinWeight; }
	CDSkinVertex* GetSkinWeight(void) { return skinWeight.Array(); }
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSkinPlugin); }

};



#endif
