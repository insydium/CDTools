CONTAINER tCDRetarget
{
	NAME tCDRetarget;
	DEFAULT 1;
	GROUP
	{
		BUTTON RTRG_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		COLUMNS 1;
		
		BOOL RTRG_SHOW_LINES { }
		COLOR RTRG_LINE_COLOR {}
			
		GROUP
		{
			COLUMNS 1;
			BOOL RTRG_ROOT_POSITION_ONLY { }
		}
		
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 1;
			LONG RTRG_RETARGET { CYCLE { RTRG_RETARGET_OFF; } }
		}
		SEPARATOR { }
		REAL RTRG_ROOT_SCALE { UNIT PERCENT; STEP 1.0; }
		VECTOR RTRG_ROOT_OFFSET	{ UNIT DEGREE; STEP 1.0; }
		GROUP
		{
			COLUMNS 1;
			BOOL RTRG_MANUAL_JOINT_ASSIGNMENT { }
		}
	}

	GROUP RTRG_ID_BIND_POSE_GROUP
	{
		GROUP
		{
			COLUMNS 4;
			
			STATICTEXT RTRG_BIND_NAME { ANIM OFF; }
			BUTTON RTRG_BIND_SET {}
			BUTTON RTRG_BIND_EDIT {}
			BUTTON RTRG_BIND_RESTORE {}
		}
	}
	
	GROUP RTRG_ID_SOURCE_TARGET_GROUP
	{
		COLUMNS 1;
		
		GROUP
		{
			COLUMNS 2;
			
			BUTTON RTRG_ADD_SRC {}
			BUTTON RTRG_SUB_SRC {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			LINK RTRG_ROOT_SOURCE_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
	GROUP RTRG_ID_JOINT_TARGET_GROUP
	{
	}
}
