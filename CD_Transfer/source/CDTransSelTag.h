//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#ifndef _CDTransSelTag_H_
#define _CDTransSelTag_H_

#include "CDTransfer.h"
#include "CDTagData.h"

#include "tCDTranSel.h"

enum
{
	//TRNSEL_TRANSFER_ON			= 1000,
	//TRNSEL_TARGET					= 1001,
	//TRNSEL_PURCHASE				= 1002,
	
	//TRNSEL_USE_BUTTON				= 1002,
	//TRNSEL_BUTTON_SELECT			= 1003
	
	TRNSEL_SEL_TRANS				= 2000,
	TRNSEL_TOGGLE_ON				= 2001,
	TRNSEL_BUTTON					= 2002
	
};

class CDTransferSelectedTag : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG GetButtonID(BaseContainer *tData, BaseList2D *trg);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDTransferSelectedTag); }
};


#endif
