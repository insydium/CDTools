CONTAINER tCDPistonIK
{
	NAME tCDPistonIK;
	DEFAULT 1;
	GROUP
	{
		BUTTON PSTIK_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		GROUP
		{
			COLUMNS 1;
			
			BOOL PSTIK_SHOW_LINES { }
			COLOR PSTIK_LINE_COLOR { }
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			LONG PSTIK_BONES_IN_GROUP	{ MIN 1; }
			LONG PSTIK_POLE_AXIS
			{
				CYCLE
				{
					PSTIK_POLE_X;
					PSTIK_POLE_Y;
					PSTIK_POLE_NX;
					PSTIK_POLE_NY;
				}
			}
		}
		GROUP PSTIK_MIX_GROUP
		{
			DEFAULT	1;
			COLUMNS 1;
		}
	}
	
	GROUP PSTIK_POLE_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK PSTIK_POLE_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
	GROUP PSTIK_GOAL_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK PSTIK_GOAL_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
}
