//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

// Starts the plugin registration

#include "c4d.h"
#include "CDConstraint.h"

// forward declarations
// tag
Bool RegisterCDPConstraintPlugin(void);
Bool RegisterCDRConstraintPlugin(void);
Bool RegisterCDSConstraintPlugin(void);
Bool RegisterCDPRConstraintPlugin(void);
Bool RegisterCDDConstraintPlugin(void);
Bool RegisterCDMConstraintPlugin(void);
Bool RegisterCDAConstraintPlugin(void);
Bool RegisterCDCConstraintPlugin(void);
Bool RegisterCDPSRConstraintPlugin(void);
Bool RegisterCDLConstraintPlugin(void);
Bool RegisterCDSFConstraintPlugin(void);
Bool RegisterCDSPRConstraintPlugin(void);
Bool RegisterCDPTConstraintPlugin(void);
Bool RegisterCDTAConstraintPlugin(void);
Bool RegisterCDNConstraintPlugin(void);
Bool RegisterCDSPLConstraintPlugin(void);
Bool RegisterCDSpaceSwitcherPlugin(void);

// command
Bool RegisterAboutCDConstraints(void);
Bool RegisterCDAutoRedraw(void);
Bool RegisterCDAddRConstraint(void);
Bool RegisterCDAddPConstraint(void);
Bool RegisterCDAddSConstraint(void);
Bool RegisterCDAddCConstraint(void);
Bool RegisterCDAddPRConstraint(void);
Bool RegisterCDAddDConstraint(void);
Bool RegisterCDAddMConstraint(void);
Bool RegisterCDAddSFConstraint(void);
Bool RegisterCDAddPSRConstraint(void);
Bool RegisterCDAddAConstraint(void);
Bool RegisterCDAddPTConstraint(void);
Bool RegisterCDAddSPRConstraint(void);
Bool RegisterCDAddTAConstraint(void);
Bool RegisterCDAddLConstraint(void);
Bool RegisterCDAddNConstraint(void);
Bool RegisterCDAddSPLConstraint(void);
Bool RegisterCDAddSWConstraint(void);

// message
Bool RegisterCDCMessagePlugin(void);

// Selection logger
Bool RegisterSelectionLog(void);

static Real version = 1.544;

static AtomArray *cdswTagList = NULL;

Bool PluginStart(void)
{
	String pName = "CD Constraints";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDCONSTRAINT,bc,false);
	
	// Allocate tag list
	cdswTagList = AtomArray::Alloc();
	
	Bool reg = RegisterCDC();

	// Register plugins
	if (!RegisterCDAutoRedraw()) return false;
	
	// message plugins
	if (!RegisterCDCMessagePlugin()) return false;
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}

	// tag / expression plugins
	if (!RegisterCDPConstraintPlugin()) return false;
	if (!RegisterCDRConstraintPlugin()) return false;
	if (!RegisterCDSConstraintPlugin()) return false;
	if (!RegisterCDPRConstraintPlugin()) return false;
	if (!RegisterCDDConstraintPlugin()) return false;
	if (!RegisterCDMConstraintPlugin()) return false;
	if (!RegisterCDAConstraintPlugin()) return false;
	if (!RegisterCDCConstraintPlugin()) return false;
	if (!RegisterCDPSRConstraintPlugin()) return false;
	if (!RegisterCDLConstraintPlugin()) return false;
	if (!RegisterCDSFConstraintPlugin()) return false;
	if (!RegisterCDSPRConstraintPlugin()) return false;
	if (!RegisterCDPTConstraintPlugin()) return false;
	if (!RegisterCDTAConstraintPlugin()) return false;
	if (!RegisterCDNConstraintPlugin()) return false;
	if (!RegisterCDSPLConstraintPlugin()) return false;
	if (!RegisterCDSpaceSwitcherPlugin()) return false;
	
	// menu command plugins
	if (!RegisterAboutCDConstraints()) return false;
	if (!RegisterCDAddRConstraint()) return false;
	if (!RegisterCDAddPConstraint()) return false;
	if (!RegisterCDAddSConstraint()) return false;
	if (!RegisterCDAddCConstraint()) return false;
	if (!RegisterCDAddPRConstraint()) return false;
	if (!RegisterCDAddDConstraint()) return false;
	if (!RegisterCDAddMConstraint()) return false;
	if (!RegisterCDAddSFConstraint()) return false;
	if (!RegisterCDAddPSRConstraint()) return false;
	if (!RegisterCDAddAConstraint()) return false;
	if (!RegisterCDAddPTConstraint()) return false;
	if (!RegisterCDAddSPRConstraint()) return false;
	if (!RegisterCDAddTAConstraint()) return false;
	if (!RegisterCDAddLConstraint()) return false;
	if (!RegisterCDAddNConstraint()) return false;
	if (!RegisterCDAddSPLConstraint()) return false;
	if (!RegisterCDAddSWConstraint()) return false;
	
	String license = "";
	if(!reg) license = " - Runtime";
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) license = " - Demo";
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) license = " - Demo";
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) license = " - Runtime";
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) license = " - Demo";
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) license = " - Runtime";
#endif
	GePrint(pName+" v"+CDRealToString(version,-1,3)+license);
	return true;
}

void PluginEnd(void)
{
	AtomArray::Free(cdswTagList);
	
	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	CDCNData *swd; //
	
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init())
			{
				// don't start plugin without resource
				GePrint("CD Constraints failed to load resources");
				return false; 
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+98);
			return true;
			
		case ID_CDSWCONSTRAINTPLUGIN:
			swd = (CDCNData *)data;
			swd->list = cdswTagList;
			return true;

	}

	return false;
}

// user area
void CDCAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDConstraints.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDCOptionsUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDCOptions.tif"));

	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}


void CDCOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDCOptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 14;
	*a = 8;
	*b = -46;
	*c = 49;
}

int CompareFrames(const void *f1, const void *f2)
{
  	//GePrint("CompareFrames");
  	CDSWTrackKey	*sw1,*sw2;
  	
  	sw1 = (CDSWTrackKey*)f1;
  	sw2 = (CDSWTrackKey*)f2;
  	if(sw1->frm < sw2->frm) return -1;
	else if (sw1->frm > sw2->frm) return 1;
	else return 0;
}

BaseTag* GetPreviousConstraintTag(BaseObject *op)
{
	BaseTag *prev = NULL;
	
	BaseTag *tag = op->GetFirstTag();
	while(tag)
	{
		switch(tag->GetType())
		{
			case ID_CDPCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDRCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDSCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDACONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDMCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDNCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDDCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDPRCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDCCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDPSRCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDLCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDSFCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDSPRCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDPTCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDTACONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDSPLCONSTRAINTPLUGIN:
				prev = tag;
				break;
		}
		tag = tag->GetNext();
	}
	
	return prev;
}

