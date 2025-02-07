//////////////////////////////////
//     Debuging Functions       //
//////////////////////////////////

#include "c4d.h"
#include "CDDebug.h"

// print redraw calls
void PrintRedraw(LONG type, String str)
{
	switch(type)
	{
		case 0:
			GePrint("DrawViews() called from: "+str);
			break;
		case 1:
			GePrint("EventAdd(EVENT_FORCEREDRAW) called from: "+str);
			break;
	}
}

// debug console printing
String CDQuatToString(const CDQuaternion& q)
{
	return "[" + CDRealToString(q.w) + ","
	+ CDRealToString(q.v.x) + ","
	+ CDRealToString(q.v.y) + ","
	+ CDRealToString(q.v.z) + "]";
}

String VectorToString(const Vector& v)
{
	return "[" + CDRealToString(v.x) + ","
	+ CDRealToString(v.y) + ","
	+ CDRealToString(v.z) + "]";
}

String FourToString(LONG x)
{
	String res;
	CHAR s[4];
	CHAR* sx = reinterpret_cast<CHAR*>(&x);
	s[0] = sx[3];
	s[1] = sx[2];
	s[2] = sx[1];
	s[3] = sx[0];
#if API_VERSION < 12000
	res.SetCString(&s[0], 4, St7bithex);
#else
	res.SetCString(&s[0], 4, STRINGENCODING_7BITHEX);
#endif
	return res;
}

String LongFourToString(LONG x)
{
	return "{" + CDLongToString(x) + "," + FourToString(x) + "}";
}

String DescIDToString(const DescID& id)
{
	String res;
	res += "{";
	for (int i = 0; i < id.GetDepth(); ++i)
	{
		res += "id: " + LongFourToString(id[i].id) + ", dt: " + 
		CDLongToString(id[i].dtype) + ", cr: " + 
		CDLongToString(id[i].creator) + ((i != id.GetDepth() - 1) ? "  -  " : "");
	}
	res += "}";
	return res;
}

String GeDataToString(const GeData& data)
{
	switch(data.GetType())
	{
		case DA_NIL: return "DA_NIL";
		case DA_LONG: return LongFourToString(data.GetLong());
		case DA_REAL: return CDRealToString(data.GetReal());
		case DA_TIME: return "T=" + CDRealToString(data.GetTime().Get()) + "s";
		case DA_VECTOR: return VectorToString(data.GetVector());
		case DA_MATRIX: return "DA_MATRIX";
		case DA_BYTEARRAY: return "DA_BYTEARRAY";
		case DA_STRING: return "\"" + data.GetString() + "\"";
		case DA_FILENAME: return "$" + data.GetFilename().GetString() + "$";
		case DA_CONTAINER: 
			{
				BaseContainer* bc = data.GetContainer();
				if (bc == NULL) return "bc==0";
				return BCToString(*data.GetContainer());
			}
		case DA_ALIASLINK:
			{
				BaseLink* bl = data.GetBaseLink();
				if (bl == NULL || bl->GetLink(GetActiveDocument()) == 0) return "link==0";
				return bl->GetLink(GetActiveDocument())->GetName();
			}
		case DA_MARKER: return "DA_MARKER";
	}
	return "Unknown";
}

String BCToString(const BaseContainer& bc)
{
	String res;
	res += "[" + LongFourToString(bc.GetId()) + "; ";
	GeData* data = 0;
	int i = 0;
	do
	{
		data = bc.GetIndexData(i);
		if (!data) break;

		res += LongFourToString(bc.GetIndexId(i)) + "=";
		res += GeDataToString(*data);
		res += ", ";
		i += 1;
	}while (i < 50);
	res += "]";
	return res;
}

void PrintCoffeeValueType(VALUE val)
{
    LONG type = val.GetType();
    switch (type)
	{
        case CD_COFFEE_NIL:
            GePrint("DT_NIL");
			break;
        case CD_COFFEE_LONG:
            GePrint("DT_LONG");
			break;
        case CD_COFFEE_FLOAT:
            GePrint("DT_FLOAT");
			break;
        case CD_COFFEE_VECTOR:
            GePrint("DT_VECTOR");
			break;
        case CD_COFFEE_VOID:
            GePrint("DT_VOID");
			break;
        case CD_COFFEE_BYTES:
            GePrint("DT_BYTES");
			break;
        case CD_COFFEE_STRING:
            GePrint("DT_STRING");
			break;
        case CD_COFFEE_CLASS:
            GePrint("DT_CLASS");
			break;
        case CD_COFFEE_OBJECT:
            GePrint("DT_OBJECT");
			break;
        case CD_COFFEE_ARRAY:
            GePrint("DT_ARRAY");
			break;
        default:
            GePrint("Unknown Type");
			break;
    }
}


// debug check for messages
Bool ListReceivedCoreMessages(String str, LONG id, const BaseContainer &bc)
{
	switch(id)
	{
		case EVMSG_CHANGE:
			GePrint(str+"::CoreMessage = EVMSG_CHANGE");
			break;
		case EVMSG_DOCUMENTRECALCULATED:
			GePrint(str+"::CoreMessage = EVMSG_DOCUMENTRECALCULATED");
			break;
		case EVMSG_TOOLCHANGED:
			GePrint(str+"::CoreMessage = EVMSG_TOOLCHANGED");
			break;
		case EVMSG_AUTKEYMODECHANGED:
			GePrint(str+"::CoreMessage = EVMSG_AUTKEYMODECHANGED");
			break;
		case EVMSG_UPDATEHIGHLIGHT:
			GePrint(str+"::CoreMessage = EVMSG_UPDATEHIGHLIGHT");
			break;
		case EVMSG_GRAPHVIEWCHANGED:
			GePrint(str+"::CoreMessage = EVMSG_GRAPHVIEWCHANGED");
			break;
		case EVMSG_TIMELINESELECTION:
			GePrint(str+"::CoreMessage = EVMSG_TIMELINESELECTION");
			break;
		case EVMSG_BROWSERCHANGE:
			GePrint(str+"::CoreMessage = EVMSG_BROWSERCHANGE");
			break;
		case EVMSG_MATERIALSELECTION:
			GePrint(str+"::CoreMessage = EVMSG_MATERIALSELECTION");
			break;
		case EVMSG_FCURVECHANGE:
			GePrint(str+"::CoreMessage = EVMSG_FCURVECHANGE");
			break;
		case EVMSG_RAYTRACER_FINISHED:
			GePrint(str+"::CoreMessage = EVMSG_RAYTRACER_FINISHED");
			break;
		case EVMSG_MATERIALPREVIEW:
			GePrint(str+"::CoreMessage = EVMSG_MATERIALPREVIEW");
			break;
		case EVMSG_ACTIVEVIEWCHANGED:
			GePrint(str+"::CoreMessage = EVMSG_ACTIVEVIEWCHANGED");
			break;
		case EVMSG_ASYNCEDITORMOVE:
			GePrint(str+"::CoreMessage = EVMSG_ASYNCEDITORMOVE");
			break;
		case EVMSG_TIMECHANGED:
			GePrint(str+"::CoreMessage = EVMSG_TIMECHANGED");
			break;
			
	#if API_VERSION < 12000
		case EVMSG_FRAME_SCENE:
			GePrint(str+"::CoreMessage = EVMSG_FRAME_SCENE");
			break;
	#endif
			
	#if API_VERSION >= 10500
		case EVMSG_SHOWIN_SB:
			GePrint(str+"::CoreMessage = EVMSG_SHOWIN_SB");
			break;
		case EVMSG_SHOWIN_TL:
			GePrint(str+"::CoreMessage = EVMSG_SHOWIN_TL");
			break;
		case EVMSG_SHOWIN_FC:
			GePrint(str+"::CoreMessage = EVMSG_SHOWIN_FC");
			break;
		case EVMSG_SHOWIN_LM:
			GePrint(str+"::CoreMessage = EVMSG_SHOWIN_LM");
			break;
		case EVMSG_TLOM_MERGE:
			GePrint(str+"::CoreMessage = EVMSG_TLOM_MERGE");
			break;
	#endif
			
	#if API_VERSION >= 11500
		case  EVMSG_SHOWIN_MT:
			GePrint(str+"::CoreMessage = EVMSG_SHOWIN_MT");
			break;
	#endif
			
	#if API_VERSION >= 12000
		case  EVMSG_VIEWWINDOW_OUTPUT:
			GePrint(str+"::CoreMessage = EVMSG_VIEWWINDOW_OUTPUT");
			break;
		case  EVMSG_VIEWWINDOW_3DPAINTUPD:
			GePrint(str+"::CoreMessage = EVMSG_VIEWWINDOW_3DPAINTUPD");
			break;
	#endif
			
	#if API_VERSION >= 14000
		case  EVMSG_UPDATESCHEME:
			GePrint(str+"::CoreMessage = EVMSG_UPDATESCHEME");
			break;
	#endif
			
		default:
			GePrint(str+"::CoreMessage = "+CDLongToString(id));
			break;
	}
	return true;
}

Bool ListReceivedMessages(String str, LONG id, void* data)
{
	switch(id)
	{
		case MSG_POINTS_CHANGED:
			GePrint(str+"::Message = MSG_POINTS_CHANGED");
			break;
		case MSG_POLYGONS_CHANGED:
			GePrint(str+"::Message = MSG_POLYGONS_CHANGED");
			break;
		case MSG_SEGMENTS_CHANGED:
			GePrint(str+"::Message = MSG_SEGMENTS_CHANGED");
			break;
		case MSG_UPDATE:
			GePrint(str+"::Message = MSG_UPDATE");
			break;
		case MSG_SMALLUPDATE:
			GePrint(str+"::Message = MSG_SMALLUPDATE");
			break;
		case MSG_CHANGE:
			GePrint(str+"::Message = MSG_CHANGE");
			break;
		case MSG_BASECONTAINER:
			GePrint(str+"::Message = MSG_BASECONTAINER");
			break;
		case MSG_FILTER:
			GePrint(str+"::Message = MSG_FILTER");
			break;
		case MSG_TRANSFERGOALS:
			GePrint(str+"::Message = MSG_TRANSFERGOALS");
			break;
		case MSG_DESCRIPTION_INITUNDO:
			GePrint(str+"::Message = MSG_DESCRIPTION_INITUNDO");
			break;
		case MSG_DESCRIPTION_CHECKUPDATE:
			GePrint(str+"::Message = MSG_DESCRIPTION_CHECKUPDATE");
			break;
		case MSG_DESCRIPTION_COMMAND:
			GePrint(str+"::Message = MSG_DESCRIPTION_COMMAND");
			break;
		case MSG_DESCRIPTION_VALIDATE:
			GePrint(str+"::Message = MSG_DESCRIPTION_VALIDATE");
			break;
		case MSG_EDIT:
			GePrint(str+"::Message = MSG_EDIT");
			break;
		case MSG_MENUPREPARE:
			GePrint(str+"::Message = MSG_MENUPREPARE");
			break;
		case MSG_RETRIEVEPRIVATEDATA:
			GePrint(str+"::Message = MSG_RETRIEVEPRIVATEDATA");
			break;
		case MSG_DESCRIPTION_REMOVE_ENTRY:
			GePrint(str+"::Message = MSG_DESCRIPTION_REMOVE_ENTRY");
			break;
		case MSG_DESCRIPTION_EDIT_ENTRY:
			GePrint(str+"::Message = MSG_DESCRIPTION_EDIT_ENTRY");
			break;
		case MSG_DESCRIPTION_CHECKDRAGANDDROP:
			GePrint(str+"::Message = MSG_DESCRIPTION_CHECKDRAGANDDROP");
			break;
		case MSG_DESCRIPTION_GETBITMAP:
			GePrint(str+"::Message = MSG_DESCRIPTION_GETBITMAP");
			break;
		case MSG_DESCRIPTION_GETOBJECTS:
			GePrint(str+"::Message = MSG_DESCRIPTION_GETOBJECTS");
			break;
		case MSG_DESCRIPTION_USERINTERACTION_END:
			GePrint(str+"::Message = MSG_DESCRIPTION_USERINTERACTION_END");
			break;
		case MSG_MATERIALDRAGANDDROP:
			GePrint(str+"::Message = MSG_MATERIALDRAGANDDROP");
			break;
		case MSG_INITIALCHANNEL:
			GePrint(str+"::Message = MSG_INITIALCHANNEL");
			break;
		case MSG_DOCUMENTINFO:
			GePrint(str+"::Message = MSG_DOCUMENTINFO");
			break;
		case MSG_MULTI_DOCUMENTCLONED:
			GePrint(str+"::Message = MSG_MULTI_DOCUMENTCLONED");
			break;
		case MSG_MULTI_DOCUMENTIMPORTED:
			GePrint(str+"::Message = MSG_MULTI_DOCUMENTIMPORTED");
			break;
		case MSG_MULTI_MARKMATERIALS:
			GePrint(str+"::Message = MSG_MULTI_MARKMATERIALS");
			break;
		case MSG_MULTI_RENDERNOTIFICATION:
			GePrint(str+"::Message = MSG_MULTI_RENDERNOTIFICATION");
			break;
		case MSG_TRANSLATE_POINTS:
			GePrint(str+"::Message = MSG_TRANSLATE_POINTS");
			break;
		case MSG_TRANSLATE_POLYGONS:
			GePrint(str+"::Message = MSG_TRANSLATE_POLYGONS");
			break;
		case MSG_TRANSLATE_NGONS:
			GePrint(str+"::Message = MSG_TRANSLATE_NGONS");
			break;
		case MSG_TRANSLATE_SEGMENTS:
			GePrint(str+"::Message = MSG_TRANSLATE_SEGMENTS");
			break;
		case MSG_PRETRANSLATE_POINTS:
			GePrint(str+"::Message = MSG_PRETRANSLATE_POINTS");
			break;
		case MSG_PRETRANSLATE_POLYGONS:
			GePrint(str+"::Message = MSG_PRETRANSLATE_POLYGONS");
			break;
		case MSG_PRETRANSLATE_NGONS:
			GePrint(str+"::Message = MSG_PRETRANSLATE_NGONS");
			break;
		case MSG_PRETRANSLATE_SEGMENTS:
			GePrint(str+"::Message = MSG_PRETRANSLATE_SEGMENTS");
			break;
		case MSG_UPDATE_NGONS:
			GePrint(str+"::Message = MSG_UPDATE_NGONS");
			break;
		case MSG_DOCUMENT_MODE_CHANGED:
			GePrint(str+"::Message = MSG_DOCUMENT_MODE_CHANGED");
			break;
		case MSG_TOOL_RESTART:
			GePrint(str+"::Message = MSG_TOOL_RESTART");
			break;
		case MSG_MOVE_FINISHED:
			GePrint(str+"::Message = MSG_MOVE_FINISHED");
			break;
			
	#if API_VERSION < 9600
		case MSG_GETSUBPLUGINSHADER:
			GePrint(str+"::Message = MSG_GETSUBPLUGINSHADER");
			break;
		case MSG_GETALLPLUGINSHADER:
			GePrint(str+"::Message = MSG_GETALLPLUGINSHADER");
			break;
	#endif
			
	#if API_VERSION >= 9800
		case MSG_DESCRIPTION_POSTSETPARAMETER:
			GePrint(str+"::Message = MSG_DESCRIPTION_POSTSETPARAMETER");
			break;
		case MSG_GETCUSTOMICON:
			GePrint(str+"::Message = MSG_GETCUSTOMICON");
			break;
		case MSG_DRAGANDDROP:
			GePrint(str+"::Message = MSG_DRAGANDDROP");
			break;
		case MSG_DEFORMMODECHANGED:
			GePrint(str+"::Message = MSG_DEFORMMODECHANGED");
			break;
		case MSG_ANIMATE:
			GePrint(str+"::Message = MSG_ANIMATE");
			break;
	#endif
			
	#if API_VERSION >= 10500
		#if API_VERSION < 12000
			case MSG_GET_MODATASELECTION:
				GePrint(str+"::Message = MSG_GET_MODATASELECTION");
				break;
		#endif
		case MSG_MOVE_START:
			GePrint(str+"::Message = MSG_MOVE_START");
			break;
		case MSG_EDITABLE_END:
			GePrint(str+"::Message = MSG_EDITABLE_END");
			break;
		case MSG_MULTI_SETNEWMARKERS:
			GePrint(str+"::Message = MSG_MULTI_SETNEWMARKERS");
			break;
		case MSG_MULTI_CLEARSUGGESTEDFOLDER:
			GePrint(str+"::Message = MSG_MULTI_CLEARSUGGESTEDFOLDER");
			break;
		case MSG_CALCMEMUSAGE:
			GePrint(str+"::Message = MSG_CALCMEMUSAGE");
			break;
	#endif

	#if API_VERSION >= 12000
		case MSG_DESCRIPTION_POPUP:
			GePrint(str+"::Message = MSG_DESCRIPTION_POPUP");
			break;
		case MSG_DESCRIPTION_GETINLINEOBJECT:
			GePrint(str+"::Message = MSG_DESCRIPTION_GETINLINEOBJECT");
			break;
		case MSG_GETSELECTION:
			GePrint(str+"::Message = MSG_GETSELECTION");
			break;
		case MSG_SCALEDOCUMENT:
			GePrint(str+"::Message = MSG_SCALEDOCUMENT");
			break;
		case MSG_GET_INHERITANCECONTAINER:
			GePrint(str+"::Message = MSG_GET_INHERITANCECONTAINER");
			break;
		case MSG_SOFTTAG_UPDATE:
			GePrint(str+"::Message = MSG_SOFTTAG_UPDATE");
			break;
	#endif
	#if API_VERSION >= 13000
		case MSG_PYTHON_RESET:
			GePrint(str+"::Message = MSG_PYTHON_RESET");
			break;
		case MSG_REDIRECT:
			GePrint(str+"::Message = MSG_REDIRECT");
			break;
		case MSG_TOOL_TRANSFORM:
			GePrint(str+"::Message = MSG_TOOL_TRANSFORM");
			break;
		case MSG_GETACTIVEREDIRECT:
			GePrint(str+"::Message = MSG_GETACTIVEREDIRECT");
			break;
		case MSG_TRANSFORM_OBJECT:
			GePrint(str+"::Message = MSG_TRANSFORM_OBJECT");
			break;
		case MSG_STRINGUNDO:
			GePrint(str+"::Message = MSG_STRINGUNDO");
			break;
	#endif
	#if API_VERSION >= 14000
		case MSG_DESCRIPTION_IMPEXPORT_INIT:
			GePrint(str+"::Message = MSG_DESCRIPTION_IMPEXPORT_INIT");
			break;
		case MSG_TOOL_ASK:
			GePrint(str+"::Message = MSG_TOOL_ASK");
			break;
		case MSG_TAG_MODIFY:
			GePrint(str+"::Message = MSG_TAG_MODIFY");
			break;
		case MSG_TOOL_RESIZE:
			GePrint(str+"::Message = MSG_TOOL_RESIZE");
			break;
		case MSG_CURRENTSTATE_END:
			GePrint(str+"::Message = MSG_CURRENTSTATE_END");
			break;
		case MSG_GETREALCAMERADATA:
			GePrint(str+"::Message = MSG_GETREALCAMERADATA");
			break;
		case MSG_ADAPTVIEW_START:
			GePrint(str+"::Message = MSG_ADAPTVIEW_START");
			break;
		case MSG_ADAPTVIEW_END:
			GePrint(str+"::Message = MSG_ADAPTVIEW_END");
			break;
	#endif
		default:
			GePrint(str+"::Message = "+CDLongToString(id));
			break;
	}
	return true;
}

