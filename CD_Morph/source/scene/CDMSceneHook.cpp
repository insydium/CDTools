//	Cactus Dan's Morph
//	Copyright 2010 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_scenehookdata.h"

#include "CDMorph.h"
#include "CDMorphTag.h"

#include "tCDMorphTag.h"

extern Bool mSplitDialogOpen;

class CDMSceneHook : public SceneHookData
{
private:
	Bool DrawMSplitGuides(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh);
	Vector GetPointColor(Real t, Real f, Vector v1, Vector v2);
	
public:
	Bool CDDraw(BaseList2D* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags);
	
#if API_VERSION < 12000
	virtual Bool Draw(PluginSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
		{ return CDDraw(node, doc, bd, bh, bt, flags); }
#else
	virtual Bool Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, SCENEHOOKDRAW flags)
		{ return CDDraw(node, doc, bd, bh, bt, (LONG)flags); }
#endif
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMSceneHook); }
};

Vector CDMSceneHook::GetPointColor(Real t, Real f, Vector v1, Vector v2)
{
	Vector color;
	
	if(f < 0.01) f = 0.01;
	
	if(t > f) color = Vector(0,0,0);
	else
	{
		if(t < 0.01) color = v2;
		else color = CDBlend(v1,v2,1-(t/f));
	}
	
	return color;
}

Bool CDMSceneHook::DrawMSplitGuides(BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh)
{
	BaseContainer *wpData = GetWorldPluginData(ID_CDMORPHSPLIT); if(!wpData) return true;
	
	Vector v1 = CDGetVertexStartColor();
	Vector v2 = CDGetVertexEndColor();
	
	BaseTag *tag = doc->GetActiveTag(); if(!tag) return true;
	if(tag->GetType() == ID_CDMORPHTAGPLUGIN)
	{
		BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
		BaseObject *op = tData->GetObjectLink(MT_DEST_LINK,doc); if(!op) return true;
		Vector *padr = GetPointArray(op); if(!padr) return true;
		
		Matrix opM = op->GetMg();
		
		CDMPoint *mpadr = CDGetMorphPoints(tag); if(!mpadr) return true;
		
		LONG i, pInd, cnt = tData->GetLong(MT_BS_POINT_COUNT), pCnt = tData->GetLong(MT_POINT_COUNT);
		
		AutoAlloc<ViewportSelect> vps; if(!vps) return true;
		
		LONG left, top, right, bottom, width, height;
		bd->GetFrame(&left, &top, &right, &bottom);
		width = right - left + 1;
		height = bottom - top + 1;
		
	#if API_VERSION < 12000
		if(!vps->Init(width,height,bd,op,Mpoints,true,VIEWPORT_USE_HN|VIEWPORT_USE_DEFORMERS)) return true;
	#else
		if(!vps->Init(width,height,bd,op,Mpoints,true,VIEWPORTSELECTFLAGS_USE_HN|VIEWPORTSELECTFLAGS_USE_DEFORMERS)) return true;
	#endif

		CDAABB bnds;
		bnds.Empty();
		
		for(i=0; i<cnt; i++)
		{
			pInd = mpadr[i].ind;
			if(pInd < pCnt)
			{
				Vector pt = padr[pInd];
				bnds.AddPoint(pt);
			}
		}
		
		
		Real f = wpData->GetReal(IDC_FALLOFF);
		Vector min = bnds.min, max = bnds.max, cen = bnds.GetCenterPoint();
		for(i=0; i<cnt; i++)
		{
			pInd = mpadr[i].ind;
			if(pInd < pCnt)
			{
				Real t;
				Vector pt = padr[pInd];
				switch(wpData->GetLong(IDC_SPLIT_AXIS))
				{
					case 0:
					{	
						if(pt.x < cen.x) t = (pt.x-cen.x)/(min.x-cen.x);
						else  t = (pt.x-cen.x)/(max.x-cen.x);
						break;
					}
					case 1:
					{
						if(pt.y < cen.y) t = (pt.y-min.y)/(cen.y-min.y);
						else  t = (pt.y-max.y)/(cen.y-max.y);
						break;
					}
					case 2:
					{
						if(pt.z < cen.z) t = (pt.z-min.z)/(cen.z-min.z);
						else  t = (pt.z-max.z)/(cen.z-max.z);
						break;
					}
				}
				
				Vector color = GetPointColor(t,f,v1,v2);
				
				bd->SetPen(color);
				
				Vector scrPt = bd->WS(opM*pt);
				LONG x = scrPt.x;
				LONG y = scrPt.y;
				scrPt.z = 0.0;
				
				ViewportPixel *vp = vps->GetNearestPoint(op,x,y,10.0);
				if(vp)
				{
					if(vp->i == pInd) CDDrawHandle2D(bd,scrPt,CD_DRAWHANDLE_MIDDLE);
				}
				
			}
		}
	}
	
	return true;
}

Bool CDMSceneHook::CDDraw(BaseList2D* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, LONG flags)
{
	GeData dispSData = bd->GetParameterData(BASEDRAW_DATA_SDISPLAYACTIVE);
	
	switch(flags)
	{
		case CD_SCENEHOOKDRAW_DRAW_PASS:
		{
			//Draw cluster autoweight guides
			if(mSplitDialogOpen) DrawMSplitGuides(doc,bd,bh);
			break;
		}
	}
	
	return true;
}


Bool RegisterCDMSceneHook(void)
{
	return RegisterSceneHookPlugin(ID_CDMSCENEHOOK, "Scenehook", PLUGINFLAG_HIDE|PLUGINFLAG_HIDEPLUGINMENU, CDMSceneHook::Alloc, CD_EXECUTION_RESULT_OK, 0, NULL);
}
