/*
 *  CDSceneHookData.cpp
 *  CDTests
 *
 *  Created by Dan Libisch on 6/17/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDSceneHookData.h"

CDSceneHookData::CDSceneHookData()
{
}

CDSceneHookData::~CDSceneHookData()
{
}

LONG CDSceneHookData::CDInitSceneHook(CDBaseSceneHook* node, BaseDocument* doc, BaseThread* bt)
{
	GePrint("CDSceneHookData::InitSceneHook");
	
	//BaseContainer state;
	//GetInputEvent(BFM_COMMAND, state);
	//LONG cmdID = state.GetLong(BFM_CMD_ID);
	//GePrint("command ID = "+LongToString(cmdID));
	
	return CD_EXECUTION_RESULT_OK;
}//*/

void CDSceneHookData::CDFreeSceneHook(CDBaseSceneHook* node, BaseDocument* doc)
{
	GePrint("CDSceneHookData::FreeSceneHook");
	
}//*/


Bool CDSceneHookData::CDMouseInput(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	GePrint("CDSceneHookData::MouseInput");
	
	return false;
}//*/

Bool CDSceneHookData::CDKeyboardInput(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	GePrint("CDSceneHookData::KeyboardInput");
	
	return false;
}//*/

Bool CDSceneHookData::CDGetCursorInfo(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Real x, Real y, BaseContainer& bc)
{
	GePrint("CDSceneHookData::GetCursorInfo");
	
	return false;
}//*/

Bool CDSceneHookData::CDDraw(CDBaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
{
	GePrint("CDSceneHookData::Draw");
	//GePrint("CDSceneHookTest::Draw - Thread = "+LongToString(IdentifyThread(bt)));
	
	return true;
}

Bool CDSceneHookData::CDAddToExecution(CDBaseSceneHook* node, PriorityList* list)
{
	GePrint("CDSceneHookData::AddToExecution");
	//list->Add(node,CD_EXECUTION_PRIORITY_INITIAL, 0);
	//list->Add(node,CD_EXECUTION_PRIORITY_ANIMATION, 0);
	//list->Add(node,CD_EXECUTION_PRIORITY_EXPRESSION, 0);
	//list->Add(node,CD_EXECUTION_PRIORITY_DYNAMICS, 0);
	//list->Add(node,CD_EXECUTION_PRIORITY_GENERATOR, 0);
	
	return false;
}//*/

LONG CDSceneHookData::CDExecute(CDBaseSceneHook* node, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
{
	GePrint("CDSceneHookData::Execute");
	//GePrint("CDSceneHookData::Execute - Thread = "+LongToString(IdentifyThread(bt)));
	
	return CD_EXECUTION_RESULT_OK;
}//*/

