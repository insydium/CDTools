//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "tCDSymTag.h"
#include "CDSymmetry.h"
#include "CDSymmetryTag.h"
#include "CDArray.h"

class CDMirrorSelection : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc);
};

Bool CDMirrorSelection::Execute(BaseDocument *doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	if(!doc->IsEditMode()) return false;
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		BaseTag *tag = NULL;
		BaseObject *op = NULL;
		BaseContainer *tData = NULL;
		
		doc->StartUndo();
		
		for(i=0; i<opCnt; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				tag = op->GetTag(ID_CDSYMMETRYTAG);
				if(tag)
				{
					tData = tag->GetDataInstance();
					if(tData)
					{
						if(tData->GetBool(SYM_SYMMETRY_IS_SET))
						{
							CDSymmetryTag *symTag = static_cast<CDSymmetryTag*>(tag->GetNodeData());
							if(symTag)
							{
								LONG *sym = symTag->GetMirrorPoints();
								if(sym)
								{
									BaseSelect *bs = NULL;
									switch (doc->GetMode())
									{
										case Mpoints:
										{
											bs = ToPoint(op)->GetPointS();
											
											CDArray<LONG> sel;
											sel.Alloc(bs->GetCount());
											
											LONG seg=0,a,b, sCnt = 0;
											while(CDGetRange(bs,seg++,&a,&b))
											{
												for (i=a; i<=b; ++i)
												{
													sel[sCnt] = i;
													sCnt++;
												}
											}
												
											for(i=0; i<sCnt; i++)
											{
												if(sym[sel[i]] > -1) bs->Select(sym[sel[i]]);
											}
											sel.Free();
											break;
										}
										case Medges:
										{
											if(IsValidPolygonObject(op))
											{
												bs = ToPoly(op)->GetEdgeS();
												
												CDArray<LONG> sel;
												sel.Alloc(bs->GetCount());
												
												LONG seg=0,a,b, sCnt = 0;
												while(CDGetRange(bs,seg++,&a,&b))
												{
													for (i=a; i<=b; ++i)
													{
														sel[sCnt] = i;
														sCnt++;
													}
												}
												
												for(i=0; i<sCnt; i++)
												{
													bs->Select(GetEdgeSymmetry(op,sym,sel[i]));
												}
												sel.Free();
											}
											break;
										}
										case Mpolygons:
										{
											if(IsValidPolygonObject(op))
											{
												bs = ToPoly(op)->GetPolygonS();
												
												CDArray<LONG> sel;
												sel.Alloc(bs->GetCount());
												
												LONG seg=0,a,b, sCnt = 0;
												while(CDGetRange(bs,seg++,&a,&b))
												{
													for (i=a; i<=b; ++i)
													{
														sel[sCnt] = i;
														sCnt++;
													}
												}
												
												for(i=0; i<sCnt; i++)
												{
													bs->Select(GetPolySymmetry(op,sym,sel[i]));
												}
												sel.Free();
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		
		doc->EndUndo();
		
		CDDrawViews(CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_REDUCTION);
	}
	
	return true;
}

class CDMirrorSelectionR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMirrorSelection(void)
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
	String name=GeLoadString(IDS_CDMIRSEL); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMIRRORSELECTION,name,PLUGINFLAG_HIDE,"CDMirSelect.tif","CD Mirror Selection",CDDataAllocator(CDMirrorSelectionR));
	else return CDRegisterCommandPlugin(ID_CDMIRRORSELECTION,name,0,"CDMirSelect.tif","CD Mirror Selection",CDDataAllocator(CDMirrorSelection));
}
