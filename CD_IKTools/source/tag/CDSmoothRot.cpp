//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSmoothRot.h"
#include "CDIKtools.h"
//#include "CDDebug.h"

enum
{
	//SMR_PURCHASE					= 1000,
	
	//SMR_USE_SMOOTHROT				= 1001,
};

class CDSmoothRotationPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Vector GetKeyRotation(BaseDocument *doc, BaseObject *op, LONG prvFrm, LONG fps);
	Vector GetRotationalDifference(BaseObject *op, Matrix aM, Matrix bM, Vector aR, Vector bR);
	Real GetRotationMix(Vector a, Vector b, Vector c);
	void SetTrackZeroAngle(BaseObject *op, LONG trackID);
	
	Bool LoopsBefore(BaseObject *op, LONG trackID);
	Bool LoopsAfter(BaseObject *op, LONG trackID);
	Bool LoopsBeforeOscillate(BaseObject *op, LONG trackID);
	Bool LoopsAfterOscillate(BaseObject *op, LONG trackID);
	void RecalculateLoopedFrame(BaseObject *op, LONG fps, LONG frstFrm, LONG lastFrm, LONG curFrm, LONG &prvFrm, LONG &nxtFrm);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSmoothRotationPlugin); }
};

Bool CDSmoothRotationPlugin::Init(GeListNode *node)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SMR_USE_SMOOTHROT,true);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

Bool CDSmoothRotationPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDSmoothRotationPlugin::Message");
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = node->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			
			if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION))
			{
				SetTrackZeroAngle(op,CD_ID_BASEOBJECT_ROTATION);
			}
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SMR_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}

	return true;
}

void CDSmoothRotationPlugin::SetTrackZeroAngle(BaseObject *op, LONG trackID)
{
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)); // track H
	if(CDHasAnimationTrack(op,dscID)) CDSetTrackZeroAngle(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)); // track P
	if(CDHasAnimationTrack(op,dscID)) CDSetTrackZeroAngle(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)); // track B
	if(CDHasAnimationTrack(op,dscID)) CDSetTrackZeroAngle(op,dscID);
}

Vector CDSmoothRotationPlugin::GetKeyRotation(BaseDocument *doc, BaseObject *op, LONG frm, LONG fps)
{
	BaseTime time = BaseTime(frm,fps);
	
	CDAnimateObject(doc,op,time,CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
	Vector rot = CDGetRot(op);
	
	return rot;
}

Real CDSmoothRotationPlugin::GetRotationMix(Vector a, Vector b, Vector c)
{
	Real xMix, yMix, zMix;
	
	if(a.x > c.x) xMix = (a.x-b.x)/(a.x-c.x);
	else xMix = (b.x-a.x)/(c.x-a.x);

	if(a.y > c.y) yMix = (a.y-b.y)/(a.y-c.y);
	else yMix = (b.y-a.y)/(c.y-a.y);

	if(a.z > c.z) zMix = (a.z-b.z)/(a.z-c.z);
	else zMix = (b.z-a.z)/(c.z-a.z);
	
	return (xMix + yMix + zMix)/3;
}

Vector CDSmoothRotationPlugin::GetRotationalDifference(BaseObject *op, Matrix aM, Matrix bM, Vector aR, Vector bR)
{
	Vector difRot = bR - aR;
	Matrix localM = MInv(aM) * bM;

	return CDGetOptimalAngle(difRot,CDMatrixToHPB(localM,op),op);
}

Bool CDSmoothRotationPlugin::LoopsBefore(BaseObject *op, LONG trackID)
{
	if(!op) return false;
	
#if API_VERSION < 9800
	return false;
#else
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)); // track H
	Bool h = CDHasLoopBefore(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)); // track P
	Bool p = CDHasLoopBefore(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)); // track B
	Bool b = CDHasLoopBefore(op,dscID);
	
	return h && p && b;
#endif
}

Bool CDSmoothRotationPlugin::LoopsAfter(BaseObject *op, LONG trackID)
{
	if(!op) return false;
	
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)); // track H
	Bool h = CDHasLoopAfter(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)); // track P
	Bool p = CDHasLoopAfter(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)); // track B
	Bool b = CDHasLoopAfter(op,dscID);
	
	return h && p && b;
}

Bool CDSmoothRotationPlugin::LoopsBeforeOscillate(BaseObject *op, LONG trackID)
{
	if(!op) return false;
	
#if API_VERSION < 9800
	return false;
#else
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)); // track H
	Bool h = CDIsLoopBeforeOscillate(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)); // track P
	Bool p = CDIsLoopBeforeOscillate(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)); // track B
	Bool b = CDIsLoopBeforeOscillate(op,dscID);
	
	return h && p && b;
#endif
}

Bool CDSmoothRotationPlugin::LoopsAfterOscillate(BaseObject *op, LONG trackID)
{
	if(!op) return false;
	
#if API_VERSION < 9800
	return false;
#else
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)); // track H
	Bool h = CDIsLoopAfterOscillate(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)); // track P
	Bool p = CDIsLoopAfterOscillate(op,dscID);
	
	dscID = DescID(DescLevel(trackID,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)); // track B
	Bool b = CDIsLoopAfterOscillate(op,dscID);
	
	return h && p && b;
#endif
}

void CDSmoothRotationPlugin::RecalculateLoopedFrame(BaseObject *op, LONG fps, LONG frstFrm, LONG lastFrm, LONG curFrm, LONG &prvFrm, LONG &nxtFrm)
{
	LONG trkLen = lastFrm - frstFrm;
	
#if API_VERSION < 9800
	LONG loops = (curFrm-frstFrm)/trkLen;
	LONG frm = curFrm - (trkLen * loops);
	
	prvFrm = CDGetPreviousVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) + (trkLen * loops);
	if(CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) > lastFrm) nxtFrm = lastFrm + (trkLen * loops);
	else nxtFrm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) + (trkLen * loops);
#else
	
	if(curFrm < frstFrm)
	{
		LONG loops = (lastFrm-curFrm)/trkLen;
		LONG frm = curFrm + (trkLen * loops);
		
		if(LoopsAfterOscillate(op,CD_ID_BASEOBJECT_ROTATION))
		{
			if(loops % 2 > 0) frm = frstFrm + (lastFrm - frm);
		}
		
		prvFrm = CDGetPreviousVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) - (trkLen * loops);
		if(CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) > lastFrm) nxtFrm = lastFrm - (trkLen * loops);
		else nxtFrm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) - (trkLen * loops);
	}
	else
	{
		LONG loops = (curFrm-frstFrm)/trkLen;
		LONG frm = curFrm - (trkLen * loops);
		
		if(LoopsAfterOscillate(op,CD_ID_BASEOBJECT_ROTATION))
		{
			if(loops % 2 > 0) frm = frstFrm + (lastFrm - frm);
		}

		prvFrm = CDGetPreviousVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) + (trkLen * loops);
		if(CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) > lastFrm) nxtFrm = lastFrm + (trkLen * loops);
		else nxtFrm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION) + (trkLen * loops);
	}
#endif
}

LONG CDSmoothRotationPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	if(!CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION)) return false;
	
	if(!tData->GetBool(SMR_USE_SMOOTHROT)) return false;
	
#if API_VERSION > 9799
	if(flags & CD_EXECUTION_INDRAG)
	{
		if(op->GetBit(BIT_ACTIVE) && doc->GetAction() == 200000090)
		{
			BaseContainer state;
			GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
			if(!(state.GetLong(BFM_INPUT_QUALIFIER) & QALT)) return false;
		}
	}
#endif
	
	LONG fps = doc->GetFps();
	LONG curFrm = doc->GetTime().GetFrame(fps);
	
	LONG frstFrm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
	LONG lastFrm = CDGetLastVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
	
	LONG prvFrm = CDGetPreviousVectorKeyFrame(op,curFrm,fps,CD_ID_BASEOBJECT_ROTATION);
	LONG nxtFrm = CDGetNextVectorKeyFrame(op,curFrm,fps,CD_ID_BASEOBJECT_ROTATION);
	
	if(nxtFrm > lastFrm)
	{
		if(!LoopsAfter(op,CD_ID_BASEOBJECT_ROTATION)) return false;
		else
		{
			if(lastFrm - frstFrm == 0) return false;
			RecalculateLoopedFrame(op,fps,frstFrm,lastFrm,curFrm,prvFrm,nxtFrm);
		}
	}
	if(curFrm < frstFrm)
	{
		if(!LoopsBefore(op,CD_ID_BASEOBJECT_ROTATION)) return false;
		else
		{
			if(lastFrm - frstFrm == 0) return false;
			RecalculateLoopedFrame(op,fps,frstFrm,lastFrm,curFrm,prvFrm,nxtFrm);
		}
	}
	
	if(nxtFrm-prvFrm == 0 || prvFrm == curFrm) return false;
	
	Matrix newM, opM = op->GetMl();
	Vector opPos = CDGetPos(op), opSca = CDGetScale(op);
	
	Vector prvRot = GetKeyRotation(doc,op,prvFrm,fps);
	Vector curRot = GetKeyRotation(doc,op,curFrm,fps);
	Vector nxtRot = GetKeyRotation(doc,op,nxtFrm,fps);

	Matrix prvM = CDHPBToMatrix(prvRot, op);
	Matrix nxtM = CDHPBToMatrix(nxtRot, op);
	
	Vector difRot = GetRotationalDifference(op,prvM,nxtM,prvRot,nxtRot);
	if(VEqual(Vector(0,0,0),difRot,0.001)) return false;
	
	Real mix = GetRotationMix(prvRot,curRot,nxtRot);
	
	CDQuaternion aQ, bQ, mixQ, q25, q50, q75;
	aQ.SetHPB(prvRot);
	bQ.SetHPB(nxtRot);
	q50 = !(aQ + bQ);
	q25 = !(q50 + aQ);
	q75 = !(q50 + bQ);
	mixQ = CDQSlerpBezier(aQ,q25,q50,q75,bQ,mix);
	newM = mixQ.GetMatrix();
	
	op->SetMl(newM);
	CDSetPos(op,opPos);
	CDSetScale(op,opSca);
	
	return CD_EXECUTION_RESULT_OK;
}

void CDSmoothRotationPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdknr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
			tData->SetLink(T_PID,op->GetUp());
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSmoothRotationPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		else
		{
			if(op->GetUp() != tData->GetObjectLink(T_PID,doc))
			{
				BaseObject *pOp = tData->GetObjectLink(T_PID,doc);
				if(pOp)
				{
					if(pOp->GetDocument())
					{
						tagMoved = true;
						tData->SetBool(T_MOV,true);
					}
				}
				if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_PID,op->GetUp());
			}
			else tData->SetBool(T_MOV,false);
		}
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSmoothRotationPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SMR_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSmoothRotationPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case SMR_USE_SMOOTHROT:	
			if(!CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDSmoothRotationPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDSMOOTHROT); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSMOOTROTATIONPLUGIN,name,info,CDSmoothRotationPlugin::Alloc,"tCDSmoothRot","CDSmoothRot.tif",1);
}
