#ifndef _CDMorphTag_H_
#define _CDMorphTag_H_

// c4d includes
#include "c4d.h"

// CDM Includes
#include "CDMStructures.h"
#include "CDTagData.h"
#include "CDArray.h"

enum
{
	//MT_SHOW_GUIDE				= 1000,
	//MT_GUIDE_COLOR			= 1001,
	MT_POINT_COUNT				= 1002,
	MT_REFERENCE_NEW			= 1003,
	MT_REF_MATRIX				= 1006,
	MT_M_SCALING				= 1007,
	
	MT_OP_DEST					= 1009,
	//MT_DEST_LINK				= 1010,
	//MT_SET_SELECTION			= 1011,
	//MT_SET_OFFSET				= 1012,
	//MT_EDIT_OFFSET			= 1013,
	//MT_RESTORE_SELECTION		= 1014,
	MT_SELECTION_EXISTS			= 1015,
	MT_OFFSET_EXISTS			= 1016,
	
	//MT_ID_CONTROLLER			= 1017,
	//MT_MIX_SLIDER				= 1018,
	
	//MT_USE_BONE_ROTATION		= 1020,
	
	//MT_ROTATION_AXIS			= 1021,
	//MT_ROTATION_H				= 1022,
	//MT_ROTATION_P				= 1023,
	//MT_ROTATION_B				= 1024,
	
	//MT_MIN_VALUE				= 1025,
	//MT_MAX_VALUE				= 1026,
	//MT_CLAMP_MIN				= 1027,
	//MT_CLAMP_MAX				= 1028,
	
	//MT_EDIT_SELECTION			= 1031,
	//MT_HIDE_UNSELECTED		= 1032,
	MT_SELECTION_EDITING		= 1033,
	MT_OFFSET_EDITING			= 1034,
	//MT_UNHIDE_ALL				= 1035,
	//MT_ERASE_OFFSET			= 1036,
	//MT_SELECT_POINTS			= 1037,
	
	//MT_USE_MIDPOINT			= 1040,
	//MT_SET_MIDPOINT			= 1041,
	//MT_EDIT_MIDPOINT			= 1042,
	//MT_ERASE_MIDPOINT			= 1043,
	MT_MIDPOINT_EXISTS			= 1044,
	MT_MIDPOINT_EDITING			= 1045,
	MT_MIDPOINT_COUNT			= 1046,
	//MT_USE_SLIDERS			= 1047,
	//MT_MID_OFFSET				= 1048,
	//MT_MID_CONTROLS			= 1049,
	//MT_MID_WIDTH				= 1050,
	//MT_PURCHASE				= 1100,
	//MT_SELECTED_ONLY			= 1101,
	
	MT_BS_POINT_COUNT			= 2000,
	MT_LEVEL					= 2001,
	
	MT_D_SEL_COUNT				= 3000,
	MT_COMMON_NORM				= 3001,
	MT_COMMON_PARENT			= 3002,
	MT_COMMON_WIDTH				= 3003
};

class CDMorphTagPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Vector GetSkinnedPoint(BaseDocument *doc, BaseObject *op, Vector pt, BaseTag *skTag, LONG mInd);
	Vector SetSkinnedPoint(BaseDocument *doc, BaseObject *op, Vector pt, BaseTag *skTag, LONG mInd);
	Vector CalcSphericalMidPoint(BaseContainer *tData, Vector rPt, Vector mPt, CDMSpPoint sPt);
	
	BaseTag* GetReferenceTag(BaseObject *op, Bool update);
	CDMRefPoint* GetReference(BaseObject *op);
	Real GetJointRotationMix(BaseContainer *tData, BaseObject *op);
	BaseObject* GetMesh(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool MCEqual(CDMSpPoint sp);
	void InitMidpointNormals(BaseObject *op, CDMRefPoint *refadr, BaseContainer *tData);
	Vector CalcCenter(Vector a, Vector b, Vector c);
	Vector MorphSlerp(Vector a, Vector b, CDMSpPoint sp, Real t);
	
public:
	//CDMPoint		*msadr;
	//CDMSpPoint		*sphPt;
	CDArray<CDMPoint>		msadr;
	CDArray<CDMSpPoint>		sphPt;
	
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
	
	void RemapMorphPoints(BaseTag *tag, BaseContainer *data, LONG *remap, LONG cnt);
	void RecalculateMorphPoints(BaseTag *tag, BaseContainer *tData, Matrix newM, Matrix oldM);
	
	//CDMPoint* GetMorphPoints(void) { return msadr; }
	//CDMSpPoint* GetSphericalPoints(void) { return sphPt; }
	CDMPoint* GetMorphPoints(void) { return msadr.Array(); }
	CDMSpPoint* GetSphericalPoints(void) { return sphPt.Array(); }
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMorphTagPlugin); }
};


#endif

