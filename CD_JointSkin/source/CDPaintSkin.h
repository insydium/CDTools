//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch
#ifndef _CDPAINTSKIN_H_
#define _CDPAINTSKIN_H_

#include "c4d.h"

#include "CDToolData.h"


enum
{
	WP_PREV_TAG_LINK				= 3000,
	WP_SKIN_JOINT_COUNT				= 3001,
	WP_PREV_MESH_LINK				= 3002,
	
	WP_CURRENT_TAG_LINK				= 4000,
	
	WP_SET_STRENGTH					= 5000,
	WP_ADD_STRENGTH					= 5001,
	WP_BLEND_STRENGTH				= 5002,
	WP_REDRAW_LIST					= 5003
};


class CDPSWDialog: public SubDialog
{
private:
	SimpleListView			listview;
	AutoAlloc<BaseSelect>	selection;
	LONG					scrlPosY;
	
	void CreateDynamicStrength(void);
	void CreateDynamicGroup(void);
	void DoEnable(void);
	
	Bool JointInList(BaseDocument *doc, BaseObject *jnt, BaseContainer *tData);
	
public:
	Bool showList;
	
	void ReLayout(void);
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool CoreMessage(LONG id, const BaseContainer &msg);
	virtual Bool InitDialog(void);
	virtual Bool Command(LONG id, const BaseContainer &msg);
};

class CDPaintSkinWeight : public CDToolData
{
private:
	Real			mx, my, mWtSmpl;// , *tWeight, *paintWeight
	Bool			mDrag, wHilight;// , *mPass
	BaseDraw		*displayBD;
	GeData 			dispFilterHN;
	GeData 			dispFilterSDS;
	GeData 			dispFilterDF;
	LONG			weightDisplayIndex;
	String			mWt, mWtT;
	Vector			vw1, vw2;
	
#if API_VERSION < 12000
	Vector			*ptColor;
#else
	SVector			*ptColor;
#endif
	
	Vector GetProjectionRayDelta(LONG projection);
	void UpdatePaintData(BaseDocument *doc, BaseContainer &data);
	
	Bool DrawText(LONG xpos, LONG ypos, BaseDraw *bd, Bool total);
	Bool DoToolDrawing(BaseDocument *doc, BaseContainer &data, BaseDraw *bd);
	
	Bool SetDisplayFilters(BaseDraw* bd);
	Bool StoreDisplayFilters(BaseDraw* bd);
	Bool RestoreDisplayFilters(BaseDraw* bd);
	
public:
	CDPSWDialog		*dlg;
	
	virtual Bool InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt);
	virtual void FreeTool(BaseDocument* doc, BaseContainer& data);
	virtual Bool Message(BaseDocument *doc, BaseContainer &data, LONG type, void *t_data);
	
	virtual Bool MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual LONG GetState(BaseDocument *doc);
	virtual Bool GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x, Real y, BaseContainer &bc);
	
	virtual void FreeDisplayControl(void);
	virtual Bool DisplayControl(BaseDocument* doc, BaseObject* op, BaseObject* chainstart, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds);
	
	virtual LONG CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags);
	virtual Bool CDInitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, const AtomArray* active);
	
	virtual SubDialog*	AllocSubDialog(BaseContainer* bc) { dlg = CDDataAllocator(CDPSWDialog); return dlg; }
};

#endif