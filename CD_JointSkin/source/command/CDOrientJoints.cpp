//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"

class CDOrientJoints : public CommandData
{
	public:
		Bool OrientChildJoints(BaseDocument* doc, BaseObject *op);
	
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDOrientJoints::OrientChildJoints(BaseDocument* doc, BaseObject *op)
{
	DescriptionCommand dc;
	
	while(op)
	{
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			dc.id = DescID(1014);
			op->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
		OrientChildJoints(doc,op->GetDown());
		op = op->GetNext();
	}
	return true;
}

Bool CDOrientJoints::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG i, opCount = objects->GetCount();
	DescriptionCommand dc;
	
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			dc.id = DescID(1014);
			op->Message(MSG_DESCRIPTION_COMMAND,&dc);
			if(op->GetDown())
			{
				if(!OrientChildJoints(doc,op->GetDown())) return true;
			}
		}
	}
	doc->EndUndo();

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDOrientJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDOrientJoints(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJORIENT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDORIENTJOINTS,name,PLUGINFLAG_HIDE,"CDOrientJoints.TIF","CD Orient Joints",CDDataAllocator(CDOrientJointsR));
	else return CDRegisterCommandPlugin(ID_CDORIENTJOINTS,name,0,"CDOrientJoints.TIF","CD Orient Joints",CDDataAllocator(CDOrientJoints));
}
