CONTAINER tCDLinkageIK
{
	NAME tCDLinkageIK;
	DEFAULT 1;
	GROUP
	{
		BUTTON LNKIK_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BOOL LNKIK_SHOW_LINES { }
			
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR LNKIK_LINE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			LONG LNKIK_POLE_AXIS
			{
				CYCLE
				{
					LNKIK_POLE_X;
					LNKIK_POLE_Y;
					LNKIK_POLE_NX;
					LNKIK_POLE_NY;
				}
			
			}
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;

			BOOL LNKIK_CONNECT_BONES { }
			BOOL LNKIK_CONNECT_NEXT { }
		}
	}
	GROUP LNKIK_POLE_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK LNKIK_POLE_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
	GROUP LNKIK_GOAL_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK LNKIK_GOAL_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
}