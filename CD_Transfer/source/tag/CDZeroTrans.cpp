//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch


#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDTransfer.h"
#include "CDTagData.h"

#include "tCDZeroTrans.h"

enum
{
	//ZT_ZERO_COORDS		= 1000,
	//ZT_TRANS_POS			= 1001,
	//ZT_TRANS_SCA			= 1002,
	//ZT_TRANS_ROT			= 1003,
	
	//ZT_LOCK_ZERO_TRANS	= 1004,
	//ZT_RETURN_TO_ZERO		= 1005,
	//ZT_PURCHASE			= 1006,

	ZT_OJECT_MATRIX			= 2000,
	ZT_LOCAL_MATRIX			= 2001,
	ZT_LOCAL_MSET			= 2002,
	ZT_OLD_ROT				= 2003,
	ZT_ROT_DIFFERENCE		= 2004,
	
	ZT_POS_VECTOR			= 2010,
	ZT_SCA_VECTOR			= 2011,
	ZT_ROT_VECTOR			= 2012,
	
	ZT_ADD_KEY				= 2020,
	ZT_PARAMETER_CHANGE		= 2021,			

	ZT_AUTO_POS				= 2023,
	ZT_AUTO_SCA				= 2024,
	ZT_AUTO_ROT				= 2025,
	ZT_PREV_FRAME			= 2026
};

// from CD Joints & Skin CD Freeze Transformation
enum
{
	CDJS_FREEZE				= 10104,
	CDJS_FRZ_CHILD,
	CDJS_FRZ_PRIM,
	CDJS_POS,
	CDJS_SCA,
	CDJS_ROT
};

class CDZeroTransfromation : public CDTagData
{
private:
	LONG xPrty;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool AutoKey(BaseDocument *doc, BaseTag *tClone, BaseContainer *tcData, BaseTag *tag, BaseContainer *tData);

public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);

	virtual Bool CDAddToExecution(BaseTag* tag, PriorityList* list);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDZeroTransfromation); }
};

Bool CDZeroTransfromation::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(ZT_ADD_KEY,false);
	tData->SetBool(ZT_PARAMETER_CHANGE,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	tData->SetVector(ZT_ROT_DIFFERENCE,Vector(0,0,0));

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDTRData ztD;
	PluginMessage(ID_CDZEROTRANSTAG,&ztD);
	if(ztD.list) ztD.list->Append(node);

	return true;
}

void CDZeroTransfromation::Free(GeListNode *node)
{
	CDTRData ztD;
	PluginMessage(ID_CDZEROTRANSTAG,&ztD);
	if(ztD.list) ztD.list->Remove(node);
}

void CDZeroTransfromation::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdtnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
	CHAR chr;
	
	aK = Mod(aK,25);
	bK = Mod(bK,3);
	if(Mod(aK,2) == 0) chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) | cK);
	else chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) & cK);
	b = chr;
	
	String s, hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(b);
	
	ch[1] = 0;
	for(i=0; i<2; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}
	s = hStr;
	if(kb != s) reg = false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = false;
	}
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	tData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!tData->GetBool(T_SET))
		{
			tData->SetLink(T_OID,op);
			
			tData->SetBool(ZT_LOCK_ZERO_TRANS+T_LST,tData->GetBool(ZT_LOCK_ZERO_TRANS));
			
			tData->SetBool(T_SET,true);
		}
	}
}

static Bool CreateKeys(BaseDocument *doc, BaseList2D *op, const BaseTime &time, LONG index, Vector value, Vector prev)
{
	Bool setPrevKey = (time != doc->GetMinTime());
	
	DescID dscID;
	
	// X value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.x);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.x);
	
	// Y value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.y);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.y);
	
	// Z value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.z);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.z);
	
	return true;
}

Bool CDZeroTransfromation::AutoKey(BaseDocument *doc, BaseTag *tClone, BaseContainer *tcData, BaseTag *tag, BaseContainer *tData)
{
	if(CDIsAxisMode(doc)) return false;
	
	switch(doc->GetAction())
	{
		case ID_MODELING_SCALE:
		{
			if(!VEqual(tcData->GetVector(ZT_TRANS_SCA),tData->GetVector(ZT_TRANS_SCA),0.01) && !CDIsCommandChecked(IDM_A_SIZE))
			{
				CreateKeys(doc,tag,doc->GetTime(),ZT_TRANS_SCA,tData->GetVector(ZT_TRANS_SCA),tcData->GetVector(ZT_TRANS_SCA));
			}
			break;
		}
		case ID_MODELING_ROTATE:
		{
			if(!VEqual(tcData->GetVector(ZT_TRANS_ROT),tData->GetVector(ZT_TRANS_ROT),0.001) && !CDIsCommandChecked(IDM_A_DIR))
			{
				CreateKeys(doc,tag,doc->GetTime(),ZT_TRANS_ROT,tData->GetVector(ZT_TRANS_ROT),tcData->GetVector(ZT_TRANS_ROT));
			}
			break;
		}
		default:
		{
			if(!VEqual(tcData->GetVector(ZT_TRANS_POS),tData->GetVector(ZT_TRANS_POS),0.001) && !CDIsCommandChecked(IDM_A_POS))
			{
				CreateKeys(doc,tag,doc->GetTime(),ZT_TRANS_POS,tData->GetVector(ZT_TRANS_POS),tcData->GetVector(ZT_TRANS_POS));
			}
			break;
		}
	}
	
	return true;
}

Bool CDZeroTransfromation::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
	}
	BaseDocument *doc = tag->GetDocument(); if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	Matrix opM = op->GetMg(), chM, prM = Matrix(), trgM;
	switch (type)
	{
		case MSG_DESCRIPTION_INITUNDO:
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == ZT_TRANS_POS)
			{
				tData->SetBool(ZT_PARAMETER_CHANGE,true);
				if(tData->GetBool(ZT_ADD_KEY)) tData->SetBool(ZT_ADD_KEY,false);
			}
			else if(descID[0].id  == ZT_TRANS_SCA)
			{
				tData->SetBool(ZT_PARAMETER_CHANGE,true);
				if(tData->GetBool(ZT_ADD_KEY)) tData->SetBool(ZT_ADD_KEY,false);
			}
			else if(descID[0].id  == ZT_TRANS_ROT)
			{
				tData->SetBool(ZT_PARAMETER_CHANGE,true);
				if(tData->GetBool(ZT_ADD_KEY)) tData->SetBool(ZT_ADD_KEY,false);
			}
			break;
		}
		case CD_MSG_UPDATE:
		{
			if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
			{
				if(CDIsCommandChecked(IDM_A_PARAMETER))
				{
					if(tData->GetBool(ZT_ADD_KEY))
					{
						AutoAlloc<AliasTrans> trans;
						BaseTag *tClone = (BaseTag*)CDGetClone(tag,CD_COPYFLAGS_0,trans);
						if(tClone)
						{
							BaseContainer *tcData = tClone->GetDataInstance();
							if(tcData)
							{
								tcData->SetVector(ZT_TRANS_POS,tData->GetVector(ZT_AUTO_POS));
								tcData->SetVector(ZT_TRANS_SCA,tData->GetVector(ZT_AUTO_SCA));
								tcData->SetVector(ZT_TRANS_ROT,tData->GetVector(ZT_AUTO_ROT));
								
								AutoKey(doc,tClone,tcData,tag,tData);
							}
							BaseTag::Free(tClone);
						}
					}
				}
			}
			
			tData->SetVector(ZT_AUTO_POS,tData->GetVector(ZT_TRANS_POS));
			tData->SetVector(ZT_AUTO_SCA,tData->GetVector(ZT_TRANS_SCA));
			tData->SetVector(ZT_AUTO_ROT,tData->GetVector(ZT_TRANS_ROT));
			
			tData->SetBool(ZT_PARAMETER_CHANGE,false);
			tData->SetBool(ZT_ADD_KEY,false);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==ZT_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==ZT_ZERO_COORDS)
			{
				if(tData->GetBool(T_REG))
				{
					if(!tData->GetBool(ZT_LOCK_ZERO_TRANS))
					{
						if(CDGetScale(op) != Vector(1,1,1))
						{
							if(CDFindPlugin(ID_CDFREEZETRANSFORMATION,CD_COMMAND_PLUGIN))
							{
								doc->SetActiveObject(op);
								
								BaseContainer bc;
								if(CDFindPlugin(ID_ABOUTCDJOINTSKIN,CD_COMMAND_PLUGIN))
								{
									bc.SetBool(CDJS_FRZ_CHILD,false);
									bc.SetBool(CDJS_FRZ_PRIM,false);
									bc.SetBool(CDJS_POS,false);
									bc.SetBool(CDJS_SCA,true);
									bc.SetBool(CDJS_ROT,false);
								}
								else
								{
									bc.SetBool(CDTR_FRZ_CHILD,false);
									bc.SetBool(CDTR_FRZ_PRIM,false);
									bc.SetBool(CDTR_POS,false);
									bc.SetBool(CDTR_SCA,true);
									bc.SetBool(CDTR_ROT,false);
								}
								SetWorldPluginData(ID_CDFREEZETRANSFORMATION,bc,false);
								CallCommand(ID_CDFREEZETRANSFORMATION);
								
								opM = op->GetMg();
								
								doc->SetActiveTag(tag);
							}
							
						}
						
						BaseObject *pr = op->GetUp();
						if(pr)
						{
							prM = pr->GetMg();
						}
						trgM = MInv(prM) * opM;

						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						tData->SetMatrix(ZT_LOCAL_MATRIX,trgM);
						tData->SetBool(ZT_LOCAL_MSET,true);
						
						tData->SetVector(ZT_TRANS_POS,Vector(0,0,0));
						tData->SetVector(ZT_POS_VECTOR,Vector(0,0,0));
						
						tData->SetVector(ZT_TRANS_SCA,Vector(1,1,1));
						tData->SetVector(ZT_SCA_VECTOR,Vector(1,1,1));
						
						tData->SetVector(ZT_TRANS_ROT,Vector(0,0,0));
						tData->SetVector(ZT_ROT_VECTOR,Vector(0,0,0));
						tData->SetVector(ZT_OLD_ROT,Vector(0,0,0));
						tData->SetVector(ZT_ROT_DIFFERENCE,Vector(0,0,0) - CDGetRot(op));
						
					}
				}
			}
			else if(dc->id[0].id==ZT_RETURN_TO_ZERO)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				tData->SetVector(ZT_TRANS_POS,Vector(0,0,0));
				tData->SetVector(ZT_TRANS_SCA,Vector(1,1,1));
				tData->SetVector(ZT_TRANS_ROT,Vector(0,0,0));
				tData->SetVector(ZT_OLD_ROT,Vector(0,0,0));
			}
			break;
		}
	}
	return true;
}

Bool CDZeroTransfromation::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool enable = true;
	
	if(!tData->GetBool(T_REG))
	{
		if(!tData->GetBool(T_SET)) enable = false;
		
		Bool tagMoved = false;
		if(op != tData->GetObjectLink(T_OID,doc))
		{
			BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
			if(tOp)
			{
				if(tOp->GetDocument())
				{
					tagMoved = true;
					tData->SetBool(T_MOV,true);
				}
			}
			if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_OID,op);
		}
		else tData->SetBool(T_MOV,false);
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;

		tData->SetBool(ZT_LOCK_ZERO_TRANS,tData->GetBool(ZT_LOCK_ZERO_TRANS+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDZeroTransfromation::CDAddToExecution(BaseTag* tag, PriorityList* list)
{
	LONG pMode = CYCLE_EXPRESSION, pVal = 0;
	
	GeData d;
	if(CDGetParameter(tag,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pMode = pd->GetPriorityValue(PRIORITYVALUE_MODE).GetLong();
			pVal = pd->GetPriorityValue(PRIORITYVALUE_PRIORITY).GetLong();
		}
	}
	
	xPrty = CD_EXECUTION_PRIORITY_EXPRESSION;
	switch(pMode)
	{
		case CYCLE_INITIAL:
			xPrty = CD_EXECUTION_PRIORITY_INITIAL;
			break;
		case CYCLE_ANIMATION:
			xPrty = CD_EXECUTION_PRIORITY_ANIMATION;
			break;
		case CYCLE_EXPRESSION:
			xPrty = CD_EXECUTION_PRIORITY_EXPRESSION;
			break;
		case CYCLE_DYNAMICS:
			xPrty = CD_EXECUTION_PRIORITY_DYNAMICS;
			break;
		case CYCLE_GENERATORS:
			xPrty = CD_EXECUTION_PRIORITY_GENERATOR;
			break;
	}
	
	CDPriorityListAdd(list, tag, xPrty, CD_EXECUTION_0);
	if(pVal != xPrty) CDPriorityListAdd(list, tag, xPrty + pVal, CD_EXECUTION_0);
		
	return true;
}

LONG CDZeroTransfromation::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;

	Matrix opM = op->GetMg(), transM = Matrix(), trgM, prM = Matrix();
	Vector newRot, localRot, theScale, theColor, oldRot = tData->GetVector(ZT_OLD_ROT);
	Bool opT = false;
	
#if API_VERSION > 12999
	ROTATIONORDER order = op->GetRotationOrder();
#endif
	
	BaseObject *pr = op->GetUp();
	if(pr)
	{
		prM = pr->GetMg();
		transM = prM;
	}
	if(tData->GetBool(ZT_LOCAL_MSET))
	{
		transM = prM * tData->GetMatrix(ZT_LOCAL_MATRIX);
	}
	trgM = MInv(transM) * opM;
	
	//Check ifchanged
	if(!VEqual(tData->GetVector(ZT_TRANS_POS),tData->GetVector(ZT_POS_VECTOR),0.001)) opT = true;
	if(!VEqual(tData->GetVector(ZT_TRANS_SCA),tData->GetVector(ZT_SCA_VECTOR),0.001)) opT = true;
	if(!VEqual(tData->GetVector(ZT_TRANS_ROT),tData->GetVector(ZT_ROT_VECTOR),0.001)) opT = true;

	if(opT)
	{
		newRot = tData->GetVector(ZT_TRANS_ROT);
		trgM = CDHPBToMatrix(newRot, op);
		trgM.off = tData->GetVector(ZT_TRANS_POS);
		opM = transM * trgM;
		theScale = CDGetScale(op);
		op->SetMg(opM);
		CDSetScale(op,theScale);
		theScale = tData->GetVector(ZT_TRANS_SCA);
		CDSetScale(op,theScale);
	}
	
	LONG curFrame = doc->GetTime().GetFrame(doc->GetFps());
	LONG prvFrame = tData->GetLong(ZT_PREV_FRAME);
	
	//Set the Local position readout
	tData->SetVector(ZT_TRANS_POS,trgM.off);
	
	//Set the Local scale readout
	theScale.x = Len(trgM.v1);
	theScale.y = Len(trgM.v2);
	theScale.z = Len(trgM.v3);
	tData->SetVector(ZT_TRANS_SCA,theScale);
	
	//Set the Local rotation readout
	newRot = CDMatrixToHPB(trgM, op);
	
	localRot = CDGetOptimalAngle(oldRot, newRot, op);
	
	tData->SetVector(ZT_TRANS_ROT,localRot);
	
	// check for auto key
	if(priority < xPrty + 500)
	{
		Bool addKey = false;
		if(curFrame == prvFrame && op->GetBit(BIT_ACTIVE))
		{
			if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
			{
				if(CDIsDirty(op,CD_DIRTY_MATRIX) && !tData->GetBool(ZT_PARAMETER_CHANGE))
				{
					BaseList2D *bsl = doc->GetUndoPtr();
					if(bsl)
					{
						if(bsl->IsInstanceOf(Obase))
						{
							if(!VEqual(tData->GetVector(ZT_TRANS_POS),tData->GetVector(ZT_AUTO_POS),0.001))
							{
								addKey = true;
							}
							if(!VEqual(tData->GetVector(ZT_TRANS_SCA),tData->GetVector(ZT_AUTO_SCA),0.001))
							{
								addKey = true;
							}
							if(!VEqual(tData->GetVector(ZT_TRANS_ROT),tData->GetVector(ZT_AUTO_ROT),0.001))
							{
								addKey = true;
							}
						}
					}
				}
			}
		}
		tData->SetBool(ZT_ADD_KEY,addKey);
	}
	
	tData->SetVector(ZT_POS_VECTOR,trgM.off);
	tData->SetVector(ZT_SCA_VECTOR,theScale);
	tData->SetVector(ZT_ROT_VECTOR,localRot);

	tData->SetVector(ZT_OLD_ROT,localRot);
	tData->SetLong(ZT_PREV_FRAME,curFrame);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDZeroTransfromation::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(ZT_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDZeroTransfromation::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case ZT_ZERO_COORDS:
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(ZT_LOCK_ZERO_TRANS)) return false;
			else return true;
		case ZT_RETURN_TO_ZERO:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case ZT_LOCK_ZERO_TRANS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	
	return true;	
}

Bool RegisterCDZeroTransfromationPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
	CHAR chr;
	
	aK = Mod(aK,25);
	bK = Mod(bK,3);
	if(Mod(aK,2) == 0) chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) | cK);
	else chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) & cK);
	b = chr;
	
	String s, hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(b);
	
	ch[1] = 0;
	for(i=0; i<2; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}
	s = hStr;
	if(kb != s) reg = false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = false;
	}
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDZTRANS); if(!name.Content()) return true;
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;

	return CDRegisterTagPlugin(ID_CDZEROTRANSTAG,name,info,CDZeroTransfromation::Alloc,"tCDZeroTrans","CDZeroTrans.tif",0);
}

