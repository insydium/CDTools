#ifndef _CDSelLog_H_
#define _CDSelLog_H_

#include "c4d.h"
#include "c4d_scenehookdata.h"

#include "CDMessages.h"
#include "CDCompatibility.h"

// CDSL ID's
#define ID_CDSLSCENEHOOK			1022082
#define ID_CDSLMESSAGE				1022081

AtomArray* GetSelectionLog(BaseDocument *doc);
Bool SelectionLogChanged(BaseDocument *doc);

void InitSelectionLog(void);
void FreeSelectionLog(void);

class CDSLMessage : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);
};

class CDSLSceneHook : public SceneHookData
{
public:
	AtomArray *sList;
	Bool *chngd;
	
	void UpdateSelectionList(BaseDocument *doc);
	AtomArray* GetSelectionList(void) { return sList; }
	Bool* GetChanged(void) { return chngd; }

	Bool CDMouseInput(BaseList2D* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);

#if API_VERSION < 12000
	virtual Bool MouseInput(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDMouseInput(node, doc, bd, win, msg); }
#else
	virtual Bool MouseInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
		{ return CDMouseInput(node, doc, bd, win, msg); }
#endif
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSLSceneHook); }
};

#endif
