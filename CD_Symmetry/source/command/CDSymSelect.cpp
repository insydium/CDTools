//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2009 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
//#include "lib_collider.h"
#include "lib_sds.h"

#include "CDSymmetry.h"

class CDSymmetrySelect : public CommandData
{
	public:
		virtual LONG GetState(BaseDocument* doc);
		virtual Bool Execute(BaseDocument* doc);

};

LONG CDSymmetrySelect::GetState(BaseDocument* doc)
{
	BaseContainer *wpData = GetWorldPluginData(ID_CDSYMMETRYSELECT);
	if(wpData)
	{
		if(wpData->GetBool(IDS_CDSYMSELECT)) return CMD_ENABLED|CMD_VALUE;
	}
	
	return CMD_ENABLED;
}

Bool CDSymmetrySelect::Execute(BaseDocument* doc)
{
	BaseContainer *wpData = GetWorldPluginData(ID_CDSYMMETRYSELECT);
	if(wpData)
	{
		if(!wpData->GetBool(IDS_CDSYMSELECT)) wpData->SetBool(IDS_CDSYMSELECT,true);
		else wpData->SetBool(IDS_CDSYMSELECT,false);
	}
	else
	{
		BaseContainer bc;
		bc.SetBool(IDS_CDSYMSELECT,true); // default setting
		SetWorldPluginData(ID_CDSYMMETRYSELECT,bc,false);
	}
	
	CDDrawViews(CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_REDUCTION);

	return true;
}

class CDSymmetrySelectR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDSymmetrySelect(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDSYMSELECT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDSYMMETRYSELECT,name,PLUGINFLAG_HIDE,"CDSymSelect.tif","CD Symmetry Select",CDDataAllocator(CDSymmetrySelectR));
	else return CDRegisterCommandPlugin(ID_CDSYMMETRYSELECT,name,0,"CDSymSelect.tif","CD Symmetry Select",CDDataAllocator(CDSymmetrySelect));
}
