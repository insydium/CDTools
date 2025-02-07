//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

class CDAddTipGoal : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddTipGoal::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = doc->GetActiveObject(); if(!op) return FALSE;
	Matrix opM = op->GetMg();
	String opName = op->GetName();
	Vector tipPos;
	
	Real aLen;
	if(op->GetDown())
	{
		aLen = Len(op->GetDown()->GetMg().off - opM.off);
		tipPos = opM.off + !opM.v3 * aLen;
	}
	else
	{
		aLen = op->GetRad().z;
		tipPos = (opM.off + op->GetMp()) + !opM.v3 * aLen;
	}
	
	Matrix tipMatrix = Matrix(tipPos,opM.v1,opM.v2,opM.v3);
	
	BaseObject* tipObject = NULL;
	tipObject = BaseObject::Alloc(Onull); if(!tipObject) return FALSE;
	tipObject->SetName(opName + GeLoadString(N_TIP_EFFECTOR));
	doc->InsertObject(tipObject,op, NULL,TRUE);
	tipObject->SetMg(tipMatrix);
	BaseTag *ikTag = NULL;
	ikTag = BaseTag::Alloc(Tikexpression);
	tipObject->InsertTag(ikTag,NULL);

	BaseObject* nullObject = NULL;
	nullObject = BaseObject::Alloc(Onull); if(!nullObject) return FALSE;
	nullObject->SetName(opName + GeLoadString(N_TIP_GOAL));
	doc->InsertObject(nullObject, NULL, NULL,TRUE);
	Matrix nullMatrix = nullObject->GetMg();
	nullMatrix.off = tipPos;
	nullObject->SetMg(nullMatrix);
	
	BaseContainer *data = nullObject->GetDataInstance();
	data->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
	ObjectColorProperties prop;
	prop.color = Vector(0,1,1);
	prop.usecolor = 2;
	nullObject->SetColorProperties(&prop);
	
	BaseContainer *tagData = ikTag->GetDataInstance();
	tagData->SetLink(IKEXPRESSIONTAG_LINK,nullObject);

	doc->SetActiveObject(nullObject);
	CDAddUndo(doc,CD_UNDO_NEW,nullObject);
	CDAddUndo(doc,CD_UNDO_NEW,tipObject);
	
	//DrawViews(DA_ONLY_ACTIVE_VIEW|DA_NO_THREAD|DA_NO_ANIMATION);
	EventAdd(EVENT_FORCEREDRAW);

	return TRUE;
}

class CDAddTipGoalR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			//MessageDialog(GeLoadString(MD_PLEASE_REGISTER));
			return TRUE;
		}
};

Bool RegisterAddTipGoal(void)
{
	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = FALSE;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = FALSE;
	
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
	if(kb != s) reg = FALSE;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = TRUE;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = TRUE;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = FALSE;
	}
#else
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO) reg = TRUE;
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = FALSE;
#endif
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_TIPGOAL); if(!name.Content()) return TRUE;
#if API_VERSION < 12000
	if(!reg) return RegisterCommandPlugin(ID_ADDTIPGOAL,name,PLUGINFLAG_HIDE,"CDtipgoal.tif","CD Add Tip Goal",gNew CDAddTipGoalR);
	else return RegisterCommandPlugin(ID_ADDTIPGOAL,name,0,"CDtipgoal.tif","CD Add Tip Goal",gNew CDAddTipGoal);
#else
	if(!reg) return RegisterCommandPlugin(ID_ADDTIPGOAL,name,PLUGINFLAG_HIDE,AutoBitmap("CDtipgoal.tif"),"CD Add Tip Goal",gNew CDAddTipGoalR);
	else return RegisterCommandPlugin(ID_ADDTIPGOAL,name,0,AutoBitmap("CDtipgoal.tif"),"CD Add Tip Goal",gNew CDAddTipGoal);
#endif
}
