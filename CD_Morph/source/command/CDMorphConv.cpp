//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDMorph.h"
#include "tCDMorphTag.h"

class CDMorphConvert : public CommandData
{
	public:
		Bool CreateTagFromObject(BaseDocument* doc, BaseObject *op, BaseObject *mop, BaseObject *dest, BaseTag *rTag, String str);

		virtual Bool Execute(BaseDocument* doc);

};

Bool CDMorphConvert::CreateTagFromObject(BaseDocument* doc, BaseObject *op, BaseObject *mop, BaseObject *dest, BaseTag *rTag, String str)
{
	Vector		*tpadr = GetPointArray(mop);
	Vector		*padr = GetPointArray(op);
	
	String mopName = mop->GetName();
	
	BaseSelect *bs = ToPoint(op)->GetPointS();
	if (bs) bs->DeselectAll();

	LONG i, pcnt=LMin(ToPoint(mop)->GetPointCount(),ToPoint(op)->GetPointCount());
	for (i=0; i<pcnt; i++)
	{
		if(!VEqual(padr[i],tpadr[i],0.001)) bs->Select(i);
	}
	
	BaseTag *cdmTag = NULL;
	cdmTag = BaseTag::Alloc(ID_CDMORPHTAGPLUGIN);
	if(dest == op) dest->InsertTag(cdmTag,rTag);
	else
	{
		BaseTag *msTag = dest->GetTag(ID_CDMORPHSLIDERPLUGIN);
		dest->InsertTag(cdmTag,msTag);
	}
	cdmTag->Message(MSG_MENUPREPARE);
	cdmTag->SetName(str);
	CDAddUndo(doc,CD_UNDO_NEW,cdmTag);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,cdmTag);
	BaseContainer *tagData = cdmTag->GetDataInstance();
	tagData->SetLink(MT_DEST_LINK,op);
	cdmTag->Message(MSG_MENUPREPARE);
	
	DescriptionCommand dc;
	dc.id = DescID(MT_SET_SELECTION);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	LONG bsPtCount = bs->GetCount();
	if (bsPtCount > 0)
	{
		LONG seg=0,a,b;
		while(CDGetRange(bs,seg++,&a,&b))
		{
			for (i=a; i<=b; ++i)
			{
				padr[i] = tpadr[i];
			}
		}
	}
	dc.id = DescID(MT_SET_OFFSET);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	return true;
}

Bool CDMorphConvert::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCnt = opSelLog->GetCount();
	if(opCnt > 0)
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0));
		if(op)
		{
			if(!IsValidPointObject(op)) return true;
			
			BaseTag *tag = op->GetTag(1019633);
			if(tag)
			{
				doc->StartUndo();
				
				BaseContainer *tData = tag->GetDataInstance();
				if(tData)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
					tData->SetLong(2000,1);
				}
				
				AutoAlloc<Description> desc;
				CDGetDescription(tag,desc,CD_DESCFLAGS_DESC_0);
				
				void* h = desc->BrowseInit();
				const BaseContainer* bc = 0;
				DescID id, groupid, scope;
				
				while (desc->GetNext(h, &bc, id, groupid))
				{
					LONG dID = id[0].id;
					if(dID > 3000)
					{
						GeData d, dSet;
						if(CDGetParameter(tag,DescLevel(dID),d))
						{
							if(d.GetType() == DA_REAL)
							{
								dSet = Real(0.0);
								CDSetParameter(tag,DescLevel(dID),dSet);
							}
						}
					}
				}
				desc->BrowseFree(h);
				
				BaseObject *clone = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,NULL);
				if(clone)
				{
					doc->InsertObject(clone,NULL,NULL,false);
					
					BaseTag *clTag = clone->GetTag(1019633);
					if(clTag)
					{
						BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
						if (!refTag)
						{
							refTag = BaseTag::Alloc(ID_CDMORPHREFPLUGIN);
							op->InsertTag(refTag,NULL);
							refTag->Message(MSG_MENUPREPARE);
							op->Message(MSG_UPDATE);
							CDAddUndo(doc,CD_UNDO_NEW,refTag);
						}
						
						AutoAlloc<Description> clDesc;
						CDGetDescription(clTag,clDesc,CD_DESCFLAGS_DESC_0);
						
						h = clDesc->BrowseInit();
						bc = 0;
						
						StatusSetSpin();
						
						LONG fourCnt = 0, dstCnt = 0;
						if(opCnt > 1) dstCnt = 1;
						
						while (clDesc->GetNext(h, &bc, id, groupid))
						{
							LONG dID = id[0].id;
							if(dID > 3000)
							{
								GeData d, dSet;
								if(CDGetParameter(clTag,DescLevel(dID),d))
								{
									if(d.GetType() == DA_REAL)
									{
										dSet = Real(1.0);
										CDSetParameter(clTag,DescLevel(dID),dSet);
										clone->Message(MSG_UPDATE);
										LONG limit = 3;
										
										BaseObject *dest = static_cast<BaseObject*>(opSelLog->GetIndex(dstCnt));
										if(dest)
										{
											if(dest->GetTag(ID_CDMORPHSLIDERPLUGIN))
											{
												BaseTag *msTag = dest->GetTag(ID_CDMORPHSLIDERPLUGIN);
												BaseContainer *msData = msTag->GetDataInstance();
												if(msData) limit = msData->GetLong(1040)-1;
											}
											
											if(opCnt > 2)
											{
												fourCnt++;
												if(fourCnt > limit)
												{
													if(dstCnt < opCnt-1) dstCnt++;
													fourCnt = 0;
												}
											}
											
											CreateTagFromObject(doc,op,clone,dest,refTag,bc->GetString(DESC_NAME));
											dSet = Real(0.0);
											CDSetParameter(clTag,DescLevel(dID),dSet);
										}
									}
								}
							}
						}
						clDesc->BrowseFree(h);
						StatusClear();
						
						BaseObject::Free(clone);
						
						CDAddUndo(doc,CD_UNDO_DELETE,tag);
						BaseTag::Free(tag);
						
						doc->EndUndo();
						EventAdd(EVENT_FORCEREDRAW);
					}
				}
			}
		}
	}

	return true;
}

class CDMorphConvertR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMorphConvertPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDMCONV); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMORPHCONVERT,name,PLUGINFLAG_HIDE,"CDMConvert.tif","CD Morph Convert",CDDataAllocator(CDMorphConvertR));
	else return CDRegisterCommandPlugin(ID_CDMORPHCONVERT,name,0,"CDMConvert.tif","CD Morph Convert",CDDataAllocator(CDMorphConvert));
}
