#ifndef _CDJoint_H_
#define _CDJoint_H_

// c4d includes
#include "c4d.h"

// CDJS Includes
#include "CDJSStructures.h"
#include "CDObjectData.h"


enum // CD Joint
{
	//JNT_JOINT_RADIUS			= 1000,
	//JNT_CONNECTED				= 1001,
	JNT_ORIENTATION				= 1002,
	//JNT_ORIENT_JOINT			= 1004,
	
	//JNT_ID_TRANSFORM			= 1005,
	//JNT_ZERO_COORDS			= 1006,
	//JNT_TRANS_POS				= 1007,
	//JNT_TRANS_SCA				= 1008,
	//JNT_TRANS_ROT				= 1009,
	
	JNT_POS_VECTOR				= 1010,
	JNT_SCA_VECTOR				= 1011,
	JNT_ROT_VECTOR				= 1012,
	JNT_HIGHLITE				= 1013,
	JNT_TOOL_ORIENT_JOINT		= 1014,
	//JNT_SHOW_XRAY				= 1015,
	//JNT_SHOW_LOCAL_AXIS		= 1016,
	//JNT_ROT_DIFFERENCE		= 1017,
	JNT_OLD_ROT					= 1018,
	JNT_SET_ROT					= 1019,
	
	//JNT_LOCK_ZERO_TRANS		= 1100,
	//JNT_RETURN_TO_ZERO		= 1101,
	
	JNT_JOINT_M					= 1500,
	
	JNT_JOINT_MATRIX			= 2000,
	JNT_LOCAL_MATRIX			= 2001,
	JNT_LOCAL_MSET				= 2002,
	JNT_OLD_TRANS_ROT			= 2003,
	
	JNT_ADD_KEY					= 2020,
	JNT_PARAMETER_CHANGE		= 2021,
	
	JNT_AUTO_POS				= 2023,
	JNT_AUTO_SCA				= 2024,
	JNT_AUTO_ROT				= 2025,
	JNT_PREV_FRAME				= 2026,
	
	//JNT_PURCHASE				= 2100,
	//JNT_PRIORITY				= 2200,
	//JNT_INITIAL				= 2201,
	//JNT_ANIMATION				= 2202,
	//JNT_EXPRESSION			= 2203,
	//JNT_DYNAMICS				= 2204,
	//JNT_GENERATORS			= 2205,
	//JNT_PRTY_VALUE			= 2206,	
	//JNT_PRTY_CAM				= 2206,
	
	JNT_STRETCH_VALUE			= 2500,
	JNT_SQUASH_VALUE			= 2501,
	JNT_USE_SPLINE				= 2502,
	JNT_S_AND_S_SPLINE			= 2503,
	JNT_SPL_START				= 2504,
	JNT_SPL_END					= 2505,
	JNT_J_LENGTH				= 2506,
	JNT_J_BASE					= 2507,
	JNT_SKIN_VOLUME				= 2508,
	
	//JNT_ID_ENVELOPE			= 3000,
	//JNT_SHOW_ENVELOPE			= 3001,
	//JNT_ROOT_OFFSET			= 3002,
	//JNT_MID_OFFSET			= 3003,
	//JNT_TIP_OFFSET			= 3004,
	//JNT_SHOW_PROXY			= 3005,
	//JNT_SKEW_RX				= 3006,
	//JNT_SKEW_RY				= 3007,
	//JNT_SKEW_TX				= 3008,
	//JNT_SKEW_TY				= 3009,
};

class CDJoint : public CDObjectData
{
private:
	Bool execute;
	LONG xPrty;
	
	void CheckOpReg(BaseContainer *oData);
	
	Bool AutoKey(BaseDocument *doc, BaseObject *opClone, BaseContainer *ocData, BaseObject *op, BaseContainer *oData);
	Bool CreateKeys(BaseDocument *doc, BaseList2D *op, const BaseTime &time, LONG index, Vector value, Vector prev);
	
	BaseObject* GetCharacterObjectParent(BaseObject *op);
	Bool IsAdjustMode(BaseObject *caOp);
	
	Vector GetRTHandle(BaseObject *op, LONG id);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode* node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);	

	virtual void CDGetDimension(BaseObject *op, Vector *mp, Vector *rad);
	virtual LONG CDDetectHandle(BaseObject *op, BaseDraw *bd, LONG x, LONG y, LONG qualifier);
	virtual Bool CDMoveHandle(BaseObject *op, BaseObject *undo, const Vector &mouse_pos, LONG hit_id, LONG qualifier, BaseDraw *bd);

	virtual Bool CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool CDAddToExecution(BaseObject* op, PriorityList* list);
	virtual LONG CDExecute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags);

	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDJoint); }
};



#endif

