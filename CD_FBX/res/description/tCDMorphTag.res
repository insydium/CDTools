CONTAINER tCDMorphTag
{
	NAME tCDMorphTag;
	DEFAULT 1;
	GROUP
	{
		BUTTON MT_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			COLUMNS 1;
			LINK  MT_DEST_LINK { ANIM ON; ACCEPT { Obase; } }
		}
		SEPARATOR {LINE;}	
		GROUP
		{
			COLUMNS 3;
			
			BUTTON MT_SET_SELECTION {}
			BUTTON MT_EDIT_SELECTION {}
			BUTTON MT_RESTORE_SELECTION {}
			BUTTON MT_SELECT_POINTS {}
			BUTTON MT_HIDE_UNSELECTED {}
			BUTTON MT_UNHIDE_ALL {}
			BUTTON MT_SET_OFFSET {}
			BUTTON MT_EDIT_OFFSET {}
			BUTTON MT_ERASE_OFFSET {}
		}
		SEPARATOR {LINE;}	
		GROUP
		{
			COLUMNS 1;
			
			BOOL MT_USE_MIDPOINT {}
		}
		GROUP
		{
			COLUMNS 3;
			
			BUTTON MT_SET_MIDPOINT {}
			BUTTON MT_EDIT_MIDPOINT {}
			BUTTON MT_ERASE_MIDPOINT {}
		}
		SEPARATOR {}	
		GROUP MT_MID_CONTROLS
		{
			COLUMNS 1;
			
			BOOL MT_USE_SLIDERS { ANIM OFF; }
			REAL MT_MID_OFFSET { UNIT PERCENT; MINSLIDER -100; MAXSLIDER 100; ANIM OFF; STEP 0.1; CUSTOMGUI REALSLIDER; ANIM OFF; }
			REAL MT_MID_WIDTH { UNIT PERCENT; MINSLIDER 0; MAXSLIDER 100; ANIM OFF; STEP 0.01; CUSTOMGUI REALSLIDER; ANIM OFF; }
		}
		SEPARATOR {LINE;}	
		GROUP
		{
			COLUMNS 2;
			
			BOOL MT_SHOW_GUIDE { }
			BOOL MT_SELECTED_ONLY { }
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR MT_GUIDE_COLOR {}
		}
	}
	GROUP MT_ID_CONTROLLER
	{
		GROUP
		{
			COLUMNS 1;
			
			REAL MT_MIX_SLIDER { UNIT PERCENT; MINSLIDER 0.0; MAXSLIDER 100.0; CUSTOMGUI REALSLIDER;}
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 1;
			BOOL MT_USE_BONE_ROTATION {}
			LONG MT_ROTATION_AXIS
			{
				CYCLE
				{
					MT_ROTATION_H;
					MT_ROTATION_P;
					MT_ROTATION_B;
				}
			}
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 4;
			
			REAL MT_MIN_VALUE { UNIT DEGREE;}
			REAL MT_MAX_VALUE { UNIT DEGREE;}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 1;
			
			BOOL MT_CLAMP_MIN {}
			BOOL MT_CLAMP_MAX {}
		}
	}
}