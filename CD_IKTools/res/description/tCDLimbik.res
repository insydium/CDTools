CONTAINER tCDLimbIK
{
	NAME tCDLimbIK;
	DEFAULT 1;
	GROUP
	{
		BUTTON LMBIK_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL LMBIK_USE { }
			REAL LMBIK_BLEND { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}	
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			BOOL LMBIK_SHOW_LINES { }
			
			LONG LMBIK_LINE_TARGET
			{
				CYCLE
				{
					LMBIK_LINE_ROOT;
					LMBIK_LINE_TIP;
				}
			}
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR LMBIK_LINE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;
			
			LONG LMBIK_POLE_AXIS
			{
				CYCLE
				{
					LMBIK_POLE_X;
					LMBIK_POLE_Y;
					LMBIK_POLE_NX;
					LMBIK_POLE_NY;
				}
			
			}
			REAL LMBIK_TWIST { UNIT DEGREE;}
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;

			BOOL LMBIK_DAMPING_ON { }
			REAL LMBIK_DAMPING_VALUE { UNIT PERCENT; MIN 0.0; MAX 100.0;}
		}
		GROUP
		{
			COLUMNS 2;

			BOOL LMBIK_CONNECT_BONES { }
			BOOL LMBIK_CONNECT_NEXT { }
		}
	}
	GROUP LMBIK_STRETCH_GROUP
	{
		GROUP
		{
			COLUMNS 3;
			
			BUTTON LMBIK_SET_LENGTH {}
			BUTTON LMBIK_CLEAR_LENGTH {}
			BUTTON LMBIK_RESET_LENGTH {}
		}
		SEPARATOR { LINE; } 
		GROUP
		{
			COLUMNS 1;
			
			BOOL LMBIK_SQUASH_N_STRETCH { }
			BOOL LMBIK_CHANGE_VOLUME { }
		}
		GROUP
		{
			COLUMNS 1;
			
			REAL LMBIK_VOLUME_STRENGTH { UNIT PERCENT; MIN 0.0; MAXSLIDER 100.0; CUSTOMGUI REALSLIDER;}
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 1;
			
			BOOL LMBIK_CLAMP_SQUASH { }
		}
		GROUP
		{
			COLUMNS 3;
			
			REAL LMBIK_SQUASH_DIST { UNIT REAL; MIN 0.0; }
			BUTTON LMBIK_SET_SQUASH_DIST {}
			REAL LMBIK_MIX_SQ_CLAMP { UNIT PERCENT; MIN 0.0; MAX 100.0; }
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 1;
			
			BOOL LMBIK_CLAMP_STRETCH { }
		}
		GROUP
		{
			COLUMNS 3;
			
			REAL LMBIK_STRETCH_DIST { UNIT REAL; MIN 0.0; }
			BUTTON LMBIK_SET_STRETCH_DIST {}
			REAL LMBIK_MIX_ST_CLAMP { UNIT PERCENT; MIN 0.0; MAX 100.0; }
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 1;
			
			BOOL LMBIK_USE_BIAS_CURVE { }
			SPLINE LMBIK_BIAS_CURVE
			{ 
				SHOWGRID_H; 
				SHOWGRID_V; 

				GRIDSIZE_H 8; 
				GRIDSIZE_V 8; 

				HAS_PRESET_BTN; 

				MINSIZE_H 120;
				MINSIZE_V 80; 

				EDIT_H; 
				EDIT_V; 

				LABELS_H; 
				LABELS_V; 

				HAS_ROUND_SLIDER;

				X_MIN 0; 
				X_MAX 1.00; 

				Y_MIN 0; 
				Y_MAX 1.00; 

				X_STEPS 0.1; 
				Y_STEPS 0.1;
			}	
		}
	}
	GROUP LMBIK_SOLVER_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK LMBIK_SOLVER_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}
	GROUP LMBIK_GOAL_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			BOOL LMBIK_GOAL_TO_BONE { }
			LINK LMBIK_GOAL_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}
	}

}
