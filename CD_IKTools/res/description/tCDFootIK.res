CONTAINER tCDFootIK
{
	NAME tCDFootIK;
	DEFAULT 1;
	GROUP
	{
		BUTTON FT_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		
		GROUP
		{
			COLUMNS 1;
						
			BOOL FT_IK_USE { }
			REAL FT_IK_BLEND { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;			
			
			BOOL FT_IK_SHOW_LINES { }
			LONG FT_IK_POLE_AXIS
			{
				CYCLE
				{
					FT_IK_POLE_X;
					FT_IK_POLE_Y;
					FT_IK_POLE_NX;
					FT_IK_POLE_NY;
				}
			}
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR FT_LINE_COLOR {}
			BOOL FT_CONNECT_BONES { }
		}
	}

	GROUP FT_IK_SOLVER_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 			
			LINK FT_IK_SOLVER_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}

	GROUP FT_IK_GOAL_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK FT_IK_GOAL_A_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
			LINK FT_IK_GOAL_B_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}

}
