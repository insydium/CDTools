CONTAINER tCDKeepPlaner
{
	NAME tCDKeepPlaner;
	DEFAULT 1;
	GROUP
	{
		BUTTON KP_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		COLUMNS 1;
		
		BOOL KP_SHOW_LINES { }
		COLOR KP_LINE_COLOR {}
			
		SEPARATOR { LINE; }
		REAL KP_LENGTH { UNIT METER; STEP 1.0; }
		
		SEPARATOR {}
		LINK KP_TARGET_A
		{ 
			ANIM MIX; ACCEPT { Obase; } 
			REFUSE { Osky; Oforeground; } 
		} 
		LINK KP_TARGET_B
		{ 
			ANIM MIX; ACCEPT { Obase; } 
			REFUSE { Osky; Oforeground; } 
		} 
		LINK KP_TARGET_C
		{ 
			ANIM MIX; ACCEPT { Obase; } 
			REFUSE { Osky; Oforeground; } 
		} 
	}
}
