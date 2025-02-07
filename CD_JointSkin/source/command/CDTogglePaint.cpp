//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "oCDJoint.h"
#include "CDJointSkin.h"
#include "CDPaintSkin.h"

class CDTogglePaintMode : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDTogglePaintMode::Execute(BaseDocument* doc)
{
	if(doc->GetAction() == ID_SKINWEIGHTTOOL)
	{
		BaseContainer *wpd = GetWorldPluginData(ID_SKINWEIGHTTOOL);
		if(wpd)
		{
			LONG mode;
			Real s;
			switch(wpd->GetLong(D_WP_MODE))
			{
				case D_WP_MODE_SET:
					mode = D_WP_MODE_ADD;
					s = wpd->GetReal(WP_ADD_STRENGTH);
					break;
				default:
					mode = D_WP_MODE_SET;
					s = wpd->GetReal(WP_SET_STRENGTH);
					break;
			}
			
			BaseContainer wpData;
			wpData.SetLong(D_WP_MODE,mode);
			wpData.SetReal(D_WP_STRENGTH,s);
			SetWorldPluginData(ID_SKINWEIGHTTOOL,wpData,true);
			
			BaseContainer *bc = GetToolData(doc,ID_SKINWEIGHTTOOL);
			if(bc)
			{
				bc->SetLong(D_WP_MODE,mode);
				bc->SetReal(D_WP_STRENGTH,s);
			}
			
			SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog layout
		}
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

class CDTogglePaintModeR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTogglePaintMode(void)
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
	String name=GeLoadString(IDS_CDTOGLPNT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTOGGLEPAINTMODE,name,PLUGINFLAG_HIDE,"CDTogglePnt.tif","CD Toggle Paint Mode",CDDataAllocator(CDTogglePaintModeR));
	else return CDRegisterCommandPlugin(ID_CDTOGGLEPAINTMODE,name,0,"CDTogglePnt.tif","CD Toggle Paint Mode",CDDataAllocator(CDTogglePaintMode));
}
