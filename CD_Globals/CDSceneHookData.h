//
//  CDSceneHookData.h
//	R9 to R12+ backward compatible scenehook data class
//  Created by Dan Libisch on 6/5/13.
//

#ifndef _CDSCENEHOOKDATA_H_
#define _CDSCENEHOOKDATA_H_

#include "c4d.h"
#include "c4d_scenehookdata.h"

#include "CDCompatibility.h"


class CDSceneHookData : public SceneHookData
{
public:
	CDSceneHookData();
	virtual ~CDSceneHookData();
	
	virtual LONG CDInitSceneHook(CDBaseSceneHook* node, BaseDocument* doc, BaseThread* bt);
	virtual void CDFreeSceneHook(CDBaseSceneHook* node, BaseDocument* doc);
	
	virtual Bool CDMouseInput(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Bool CDKeyboardInput(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Bool CDGetCursorInfo(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Real x, Real y, BaseContainer& bc);
	
	virtual Bool CDDraw(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags);
	virtual Bool CDAddToExecution(CDBaseSceneHook* node, PriorityList* list);
	virtual LONG CDExecute(CDBaseSceneHook* node, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags);	
	
#if API_VERSION < 12000
	virtual LONG InitSceneHook(PluginSceneHook* node, BaseDocument* doc, BaseThread* bt)
		{ return CDInitSceneHook((CDBaseSceneHook*)node, doc, bt); }
	virtual void FreeSceneHook(PluginSceneHook* node, BaseDocument* doc)
		{ return CDFreeSceneHook((CDBaseSceneHook*)node, doc); }
	
	virtual Bool MouseInput(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDMouseInput((CDBaseSceneHook*)node, doc, bd, win, msg); }
	virtual Bool KeyboardInput(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDKeyboardInput((CDBaseSceneHook*)node, doc, bd, win, msg); }
	virtual Bool GetCursorInfo(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, Real x, Real y, BaseContainer& bc)
		{ return CDGetCursorInfo((CDBaseSceneHook*)node, doc, bd, x, y, bc); }
	
	virtual Bool Draw(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
		{ return CDDraw((CDBaseSceneHook*)node, doc, bd, bh, bt, flags); }
	virtual Bool AddToExecution(PluginSceneHook* node, PriorityList* list)
		{ return CDAddToExecution((CDBaseSceneHook*)node, list); }
	virtual LONG Execute(PluginSceneHook* node, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
		{ return CDExecute((CDBaseSceneHook*)node, doc, bt, priority, flags); }
#else
	virtual EXECUTIONRESULT InitSceneHook(BaseSceneHook* node, BaseDocument* doc, BaseThread* bt)
		{ return (EXECUTIONRESULT)CDInitSceneHook((CDBaseSceneHook*)node, doc, bt); }
	virtual void FreeSceneHook(BaseSceneHook* node, BaseDocument* doc)
		{ return CDFreeSceneHook((CDBaseSceneHook*)node, doc); }
	
	virtual Bool MouseInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDMouseInput((CDBaseSceneHook*)node, doc, bd, win, msg); }
	virtual Bool KeyboardInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDKeyboardInput((CDBaseSceneHook*)node, doc, bd, win, msg); }
	virtual Bool GetCursorInfo(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Real x, Real y, BaseContainer& bc)
		{ return CDGetCursorInfo((CDBaseSceneHook*)node, doc, bd, x, y, bc); }
	
	virtual Bool Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
		{ return CDDraw((CDBaseSceneHook*)node, doc, bd, bh, bt, flags); }
	virtual Bool AddToExecution(BaseSceneHook* node, PriorityList* list)
		{ return CDAddToExecution((CDBaseSceneHook*)node, list); }
	virtual EXECUTIONRESULT Execute(BaseSceneHook* node, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
		{ return (EXECUTIONRESULT)CDExecute((CDBaseSceneHook*)node, doc, bt, priority, flags); }
#endif
	
};


#endif