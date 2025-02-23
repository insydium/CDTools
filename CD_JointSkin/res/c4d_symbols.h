enum
{
	// string table definitions
	D_WP_RADIUS				= 1000,
	D_WP_VISIBLE_ONLY		= 1001,
	D_WP_STRENGTH			= 1002,
	D_WP_CONTRAST			= 1003,
	
	D_WP_MODE				= 1004,
		D_WP_MODE_SET		= 1005,
		D_WP_MODE_ADD		= 1006,
		
	D_WP_DYN_GROUP			= 1007,
	D_WP_DYN_TITLE			= 1008,
	D_WP_DYN_STRENGTH		= 1009,		
	
	D_WP_JLIST				= 1010,
	D_WP_JLIST_EXPAND		= 1011,
	D_WP_JCOLORS			= 1012,
	D_WP_SOFT_EDGES			= 1013,
	D_WP_SOFTNESS			= 1014,
		D_WP_MODE_BLEND		= 1015,
	D_WP_NORMALIZED			= 1016,
	
	D_JT_AUTO_SIZE			= 1017,
	D_JT_JOINT_SIZE			= 1018,
	
	D_BS_DEFAULT			= 1020,
	
	D_MIRROR_C_RENAME		= 1030,
	
	IDS_CDLINK				= 1040,
	IDS_CDUNLINK			= 1041,
	
	IDC_COLOR_TYPE			= 1042,
	IDS_COLOR_SUBCHAINS		= 1043,
	IDS_COLOR_INDIVIDUAL	= 1044,
	
	IDS_CLUSTER_RADIUS		= 1045,
	IDS_MAXWEIGHT			= 1046,
	
	IDS_CDJDISPLAY			= 1047,
	IDC_JOINT_SIZE			= 1048,
	IDC_J_SIZE_RESET		= 1049,
	IDC_SELECTED_JOINT		= 1050,
	
	IDS_POSE					= 2000,
	IDS_POSE_SET				= 2001,
	IDS_POSE_EDIT				= 2002,
	IDS_POSE_RESTORE			= 2003,
	IDS_POSE_MIRROR				= 2004,
	
	IDS_ABOUT_CDJS				= 10000,

	IDC_CDJS_SERIALNUMBER		= 10001,
	IDC_CDJS_ABOUT_IMAGE		= 10002,
	IDC_CDJS_ABOUT_PLUGININFO	= 10003,
	IDC_WEBSITE_INFO			= 10004,
	
	DLG_CDJS_REGISTER			= 10005,
	DLG_CDJS_ABOUT				= 10006,
	
	IDC_ADDBINDPOSETAG		= 10020,
	
	IDS_CDBINDPOSE			= 10029,
	IDS_CDJMIRROR			= 10030,
	D_MIRROR_AXIS			= 10032,
	IDC_MIRROR_AXIS			= 10033,
	D_MIRROR				= 10035,
	IDS_CDCLUSTERTAG		= 10036,
	
	IDS_CDJOINTOBJECT		= 10037,
	IDS_CDJOINTTOOL			= 10038,
	M_X						= 10039,
	M_Y						= 10040,
	M_Z						= 10041,
	IDS_SKINCOM				= 10042,
	IDS_SKINWTTOOL			= 10043,
	IDS_CDSKIN				= 10044,
	
	IDC_ENVELOPE			= 10045,
	IDC_INFLUENCE			= 10046,
	IDC_OPENPAINTTOOL		= 10047,
	GADGET_MIRROR			= 10048,
	D_MIRROR_RENAME			= 10049,
	IDC_OLD_NAME			= 10050,
	IDC_NEW_NAME			= 10051,
	
	IDS_CDSKINREF			= 10052,
	IDS_CDJORIENT			= 10053,
	IDS_CDJADD				= 10054,
	IDS_CDJDELETE			= 10055,
	
	D_AD_JOINT_ERR			= 10056,
	D_AD_CHILDREN			= 10057,
	D_UNBIND_FIRST			= 10058,
	
	IDS_CDMIRRORSW			= 10059,
	D_J_NOT_IN_LIST			= 10060,
	
	IDC_P_TOLERANCE			= 10061,
	D_P_TOLERANCE			= 10062,
	IDC_J_TOLERANCE			= 10063,
	D_J_TOLERANCE			= 10064,
	D_TOLERANCE				= 10065,
	NOT_SYMMETRICAL			= 10066,
	UNBIND_TO_EDIT			= 10067,

	A_SELECT_OBJECT			= 10068,
	A_NO_TAG				= 10069,
	
	IDS_CDMIRRORSCTAG		= 10070,
	
	IDC_MIRROR_DIRECTION	= 10071,
	M_NEG					= 10072,
	M_POS					= 10073,
	IDS_CDBTOJ				= 10074,
	
	IDC_JOINT_NUMBER		= 10075,
	IDC_FALLOFF				= 10076,
	IDC_SKELETON_ROOT		= 10077,

	NOT_SYMMETRICAL_J		= 10078,
	NOT_SYMMETRICAL_P		= 10079,
	
	IDS_CDCNVTOCDSKIN		= 10080,
	IDS_CDCNVFRMCDSKIN		= 10081,
	IDS_CDJTOB				= 10082,
	
	A_SEL_OBJ_SKIN			= 10083,
	A_SEL_OBJ_CB			= 10084,
	A_SEL_OBJ_BONE			= 10085,
	A_SEL_OBJ_JOINT			= 10086,
	
	IDS_CDFTRANS			= 10087,
	M_GLOBAL				= 10088,
	M_LOCAL					= 10089,
	IDC_GLOBAL_LOCAL		= 10090,
	IDC_GLOBAL				= 10091,
	IDC_LOCAL				= 10092,
	
	IDC_NORMALIZE_P_WT		= 10093,
	IDC_AUTO_SEL_CHILDREN	= 10094,
	
	IDC_JOINT_XRAY_TOGGLE	= 10095,
	IDS_CDJXRAYTOG			= 10096,
	IDS_CDCOLORJOINT		= 10097,
	IDS_ADDCLUSTER			= 10098,
	IDS_CDJPROXYTOG			= 10099,
	IDS_CDJTOP				= 10100,
	IDS_CDUNSKIN			= 10101,
	IDS_CDJMASSIGN			= 10102,
	
	IDC_INCLUDE_TIP			= 10103,
	
	CDJS_FREEZE				= 10104,
	CDJS_FRZ_CHILD			= 10105,
	CDJS_FRZ_PRIM			= 10106,
	CDJS_POS				= 10107,
	CDJS_SCA				= 10108,
	CDJS_ROT				= 10109,
	
	IDS_HLP_CNTRL_CLICK,
	IDS_HLP_CDJOINTTOOL,
	IDS_HLP_SKINWTTOOL,
	IDS_CDNORMWT,
	IDS_CDTOGLPNT,
	IDS_CDMERGE,
	IDS_CDFRZSKIN,
	IDS_CDJENVTOG,
	
	IDC_MIN_WEIGHT,
	
	IDS_WEB_SITE,
	IDS_CDREROOT,
	
	IDC_CDADDJ_AUTO_WT,
	
	IDC_RESET_ORIGINAL,
	
	C_SYMBOL,
	
	IDS_CDSKINTRANS,
	IDS_CDRGUIDE,
	IDC_CDADDJ_AUTO_BIND,
	
	IDS_CDCLTAG,
	IDS_CDRPTAG,
	
	IDS_CAWEIGHTS,
	IDS_CBWEIGHTS,
	
	A_SEL_OBJ_CA,
	A_SEL_OBJ_CDJOINT,
	
	IDS_JOINTS,
	IDS_BONES,
	
	
	MD_TOO_MANY_OBJECTS		= 20000,
	
	IDC_CDJS_UPDATE,
	IDC_CDJS_REGISTER,
	IDC_CDJS_CANCEL,
	IDC_CDJS_PURCHASE,
	
	IDC_CDJS_EULA,
	IDC_CDJS_AGREE,
	IDC_CDJS_DISAGREE,

	DLG_CDJS_OPTIONS,
	IDC_CDJS_OPTIONS_IMAGE,

	MD_PLEASE_REGISTER,
	MD_INVALID_NUMBER,
	MD_THANKYOU,
	
	MD_MODEL_TOOL_SCALE,
	MD_MIRROR_SKINNED_MESH,
	MD_MERGE_UNBIND_MESH

};
