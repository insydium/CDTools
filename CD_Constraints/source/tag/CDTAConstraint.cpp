//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDTAConstraint.h"
#include "CDConstraint.h"

enum
{
	//TAC_PURCHASE				= 1000,
	
	//TAC_SHOW_LINES			= 1000,
	//TAC_LINE_COLOR			= 1001,
	
	//TAC_LENGTH				= 1002,
	//TAC_HEIGHT				= 1003,
	//TAC_SPEED					= 1004,

	//TAC_RESET_POSITION		= 1005,
	//TAC_OVERSTEP				= 1006,
		
	//TAC_TARGET				= 1010,
	//TAC_STRENGTH				= 1011,
	
	//TAC_HEIGHT_SPLINE			= 1500,
	
	//TAC_ID_TARGET				= 2000,
	//TAC_USE_START_FRAME		= 2001,
	//TAC_SET_START_FRAME		= 2002,
	//TAC_RELEASE_START_FRAME	= 2003,
	
	//TAC_ID_STRIDE				= 3000,

	TAC_OLD_POS					= 5000,
	TAC_NEW_POS					= 5001,
	
	TAC_START_SET				= 10000,
	TAC_START_FRAME				= 10001,
	TAC_START_POS				= 10002
};

class CDTAConstraint : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

public:
	Real t, newTime, oldTime;
	LONG oldFrame;
	Vector gTracking;
	
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDTAConstraint); }
};

Bool CDTAConstraint::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(TAC_SHOW_LINES,true);
	tData->SetVector(TAC_LINE_COLOR, Vector(1,0,0));
	
	tData->SetReal(TAC_STRENGTH, 1.0);
	tData->SetReal(TAC_SPEED, 0.5);
	tData->SetReal(TAC_LENGTH, 300.0);
	tData->SetReal(TAC_HEIGHT, 100.0);
	tData->SetReal(TAC_OVERSTEP, 0.0);
	
	tData->SetVector(TAC_OLD_POS, Vector(0,0,0));
	tData->SetVector(TAC_NEW_POS, Vector(0,0,0));
	
	tData->SetBool(TAC_START_SET,false);
		
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	GeData spd(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);

	SplineData* sp = (SplineData*)spd.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
	if(sp)
	{
		sp->MakeCubicSpline(-1);
		sp->InsertKnot( 0.0, 0.0 );
		sp->InsertKnot( 0.5, 1.0 );
		sp->InsertKnot( 1.0, 0.0 );
	#if API_VERSION < 13000
		sp->SetRound(1.0);
	#endif
	}
	tData->SetData(TAC_HEIGHT_SPLINE, spd);
	

	return true;
}

Bool CDTAConstraint::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	BaseObject *goal = tData->GetObjectLink(TAC_TARGET,doc); if (!goal) return true;
	
	Vector opPosition = op->GetMg().off;
	Vector goalPosition = goal->GetMg().off;
	
	if (!tData->GetBool(TAC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(TAC_LINE_COLOR));
	CDDrawLine(bd,opPosition, goalPosition);
	
	return true;
}

void CDTAConstraint::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdcnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
			
			tData->SetLink(TAC_TARGET+T_LST,tData->GetLink(TAC_TARGET,doc));
			tData->SetReal(TAC_LENGTH+T_LST,tData->GetReal(TAC_LENGTH));
			tData->SetReal(TAC_HEIGHT+T_LST,tData->GetReal(TAC_HEIGHT));
			tData->SetReal(TAC_SPEED+T_LST,tData->GetReal(TAC_SPEED));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDTAConstraint::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	BaseDocument *doc = tag->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	BaseObject *goal = NULL;
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==TAC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==TAC_RESET_POSITION)
			{
				goal = tData->GetObjectLink(TAC_TARGET,doc);
				if(goal)
				{
					Matrix opM = op->GetMg();
					Vector theScale = CDGetScale(op);
					t = 1.0;
					tData->SetVector(TAC_OLD_POS, goal->GetMg().off);
					tData->SetVector(TAC_NEW_POS, goal->GetMg().off);
					opM.off = goal->GetMg().off;
					op->SetMg(opM);
					CDSetScale(op,theScale);
					//op->Message(MSG_UPDATE);
				}
			}
			else if(dc->id[0].id==TAC_SET_START_FRAME)
			{
				tData->SetLong(TAC_START_FRAME,doc->GetTime().GetFrame(doc->GetFps()));
				tData->SetVector(TAC_START_POS,CDGetPos(op));
				tData->SetBool(TAC_START_SET,true);
			}
			else if(dc->id[0].id==TAC_RELEASE_START_FRAME)
			{
				tData->SetBool(TAC_START_SET,false);
				tData->SetBool(TAC_USE_START_FRAME,false);
			}
			break;
		}
	}

	return true;
}

Bool CDTAConstraint::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(TAC_TARGET,tData->GetLink(TAC_TARGET+T_LST,doc));
		tData->SetReal(TAC_LENGTH,tData->GetReal(TAC_LENGTH+T_LST));
		tData->SetReal(TAC_HEIGHT,tData->GetReal(TAC_HEIGHT+T_LST));
		tData->SetReal(TAC_SPEED,tData->GetReal(TAC_SPEED+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDTAConstraint::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *goal = tData->GetObjectLink(TAC_TARGET,doc); if (!goal) return false;
	
	if(tData->GetBool(TAC_USE_START_FRAME) && !tData->GetBool(TAC_START_SET)) return true;
	
	if(tData->GetBool(TAC_START_SET))
	{
		if(doc->GetTime().GetFrame(doc->GetFps()) == tData->GetLong(TAC_START_FRAME))
		{
			CDSetPos(op,tData->GetVector(TAC_START_POS));
			tData->SetVector(TAC_OLD_POS,op->GetMg().off);
			tData->SetVector(TAC_NEW_POS, op->GetMg().off);
			return true;
		}
	}
	
	Matrix opM = op->GetMg(), goalM = goal->GetMg();
	Real length = tData->GetReal(TAC_LENGTH), height = tData->GetReal(TAC_HEIGHT), speed = tData->GetReal(TAC_SPEED);
	Vector opPos, oldPos = tData->GetVector(TAC_OLD_POS), newPos = tData->GetVector(TAC_NEW_POS), theScale = CDGetScale(op);
	Vector oldRot, newRot, rotSet;
	
	SplineData *heightSpline = (SplineData*)tData->GetCustomDataType(TAC_HEIGHT_SPLINE,CUSTOMDATATYPE_SPLINE);
	
	Real overstep = 1.0 + tData->GetReal(TAC_OVERSTEP);
	
	Real gLen = Len(goalM.off - newPos);
	if(gLen > length)
	{
		t = 0.0;
		oldPos = newPos;
		
		Real dist = length * overstep;
		Vector glDir = goalM.off - oldPos;
		Vector strDir = VNorm(goalM.off - gTracking) * (dist - Len(goalM.off - oldPos));
		newPos = oldPos + glDir + strDir;
		
		tData->SetVector(TAC_OLD_POS, oldPos);
		tData->SetVector(TAC_NEW_POS, newPos);
	}
		
	LONG fps;
	LONG curFrame = doc->GetTime().GetFrame(doc->GetFps());
	if(!CheckIsRunning(CHECKISRUNNING_EXTERNALRENDERING))
	{
		fps = doc->GetFps();
		LONG newTime = GeGetTimer();
		if(newTime < oldTime+(1000/fps) && curFrame == oldFrame)
		{
			return true;
		}
	}
	else
	{
		BaseContainer bcRender = doc->GetActiveRenderData()->GetData();
		fps = bcRender.GetLong(RDATA_FRAMERATE);
	}
	
	Real relativeSpeed = Len(goalM.off - gTracking);
	Real normalSpeed = length * speed;
	Real f = 1;
	if(normalSpeed > 0.0)
	{
		f = (relativeSpeed/ normalSpeed) * overstep;
		if(f < 0.5) f = 0.5;
	}
	if(!VEqual(oldPos,newPos,0.001))
	{
		t += speed * f;
		if(t > 1.0) t = 1.0;
		opPos = CDBlend(oldPos,newPos,t);
		Vector yHeight = heightSpline->GetPoint(t);
		if(f > 1.0) f = 1.0;
		opPos.y += height * yHeight.y * f;
	}
	else opPos = oldPos;
	
	Real strength = tData->GetReal(TAC_STRENGTH);
	opM.off = CDBlend(opM.off, opPos, strength);
	oldRot = CDGetRot(op);
	op->SetMg(opM);
	newRot = CDGetRot(op);
	rotSet = CDGetOptimalAngle(oldRot, newRot, op);
	CDSetRot(op,rotSet);
	CDSetScale(op,theScale);
	
	oldFrame = curFrame;
	oldTime = GeGetTimer();
	gTracking = goalM.off;
	
	return CD_EXECUTION_RESULT_OK;
}


Bool CDTAConstraint::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(TAC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDTAConstraint::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case TAC_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TAC_HEIGHT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TAC_SPEED:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TAC_HEIGHT_SPLINE:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TAC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TAC_USE_START_FRAME:
			if(!tData->GetBool(TAC_START_SET)) return true;
			else return false;
		case TAC_SET_START_FRAME:
			if(!tData->GetBool(TAC_START_SET) && tData->GetBool(TAC_USE_START_FRAME)) return true;
			else return false;
		case TAC_RELEASE_START_FRAME:
			return tData->GetBool(TAC_START_SET);
	}
	return true;
}

Bool RegisterCDTAConstraintPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDTACONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDTACONSTRAINTPLUGIN,name,info,CDTAConstraint::Alloc,"tCDTAConstraint","CDTAConstraint.tif",0);
}
