CONTAINER tCDIKHandle
{
	NAME tCDIKHandle;
	DEFAULT 1;
	GROUP
	{
		BUTTON IKH_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL IKH_USE { }
			REAL IKH_BLEND { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			BOOL IKH_SHOW_LINES { }
			
			LONG IKH_LINE_TARGET
			{
				CYCLE
				{
					IKH_LINE_ROOT;
					IKH_LINE_TIP;
				}
			}
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR IKH_LINE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;

			LONG IKH_SOLVER
			{
				CYCLE
				{
					IKH_IKRP;
					IKH_IKSC;
					IKH_IKHD;
				}
			}
			LONG IKH_BONES_IN_GROUP	{ MIN 1; MAX 100;}
		}
		GROUP
		{
			COLUMNS 2;
			
			BOOL IKH_UNLOCK_ROOT {}
			BOOL IKH_RESTORE_REST {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			LONG IKH_POLE_AXIS
			{
				CYCLE
				{
					IKH_POLE_X;
					IKH_POLE_Y;
					IKH_POLE_NX;
					IKH_POLE_NY;
				}
			
			}
			REAL IKH_TWIST { UNIT DEGREE;}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;

			BOOL IKH_CONNECT_BONES { }
			BOOL IKH_CONNECT_NEXT { }
		}
	}

	GROUP IKH_POLE_GROUP
	{
		DEFAULT	1;
		
		GROUP
		{
			LINK IKH_POLE_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			}
			SEPARATOR {}
			VECTOR IKH_IKSC_POLE_VECTOR { UNIT METER; STEP 0.1; }
		}
	}
	GROUP IKH_HANDLE_GROUP
	{
		DEFAULT	1;
		
		GROUP
		{
			LINK IKH_GOAL_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
}
