CONTAINER tCDSpinal
{
	NAME tCDSpinal;
	DEFAULT 1;
	GROUP
	{
		BUTTON SPN_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL SPN_USE { }
			REAL SPN_BLEND { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}	
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 1;
			
			BOOL SPN_SHOW_LINES { }
			COLOR SPN_LINE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 2;

			LONG SPN_BONES_IN_GROUP	{ MIN 2; }
			LONG SPN_POLE_AXIS
			{
				CYCLE
				{
					SPN_POLE_X;
					SPN_POLE_Y;
					SPN_POLE_Z;
					SPN_POLE_NX;
					SPN_POLE_NY;
					SPN_POLE_NZ;
				}
			
			}
		}
		GROUP
		{
			COLUMNS 2;

			BOOL SPN_CONNECT_BONES { }
			BOOL SPN_CONNECT_NEXT { }
		}
	}
	
	GROUP SPN_CONTROL_GROUP
	{
		GROUP
		{
			COLUMNS 2;
			BOOL SPN_USE_TARGET_TWIST { }
			LONG SPN_INTERPOLATION
			{
				CYCLE
				{
					SPN_SHORTEST;
					SPN_AVERAGE;
					SPN_LONGEST;
				}
			}
			BOOL SPN_INCLUDE_TIP_TWIST { }
		} 
		SEPARATOR { LINE; } 
		GROUP
		{
			COLUMNS 1;
			LINK  SPN_TIP_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			}
		} 
		GROUP
		{
			COLUMNS 3;
			REAL SPN_TIP_DEPTH { UNIT METER; MIN 0.0; }
			REAL SPN_TIP_TWIST { UNIT DEGREE; }
		}
		SEPARATOR { LINE; } 
		GROUP
		{
			COLUMNS 1;
			LINK  SPN_BASE_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		} 
		GROUP
		{
			COLUMNS 2;
			REAL SPN_BASE_DEPTH { UNIT METER; MIN 0.0; }
			REAL SPN_BASE_TWIST { UNIT DEGREE; }
		}
	}
	GROUP SPN_STRETCH_GROUP
	{
		GROUP
		{
			COLUMNS 3;
			
			BUTTON SPN_SET_LENGTH {}
			BUTTON SPN_CLEAR_LENGTH {}
			BUTTON SPN_RESET_LENGTH {}
		}
		SEPARATOR { LINE; } 
		GROUP
		{
			COLUMNS 1;
			
			BOOL SPN_SQUASH_N_STRETCH { }
			BOOL SPN_CHANGE_VOLUME { }
		}
		GROUP
		{
			COLUMNS 1;
			
			REAL SPN_VOLUME_STRENGTH { UNIT PERCENT; MIN 0.0; MAXSLIDER 100.0; CUSTOMGUI REALSLIDER;}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 3;
			
			BOOL SPN_DEPTH_S_N_S { }
			BOOL SPN_SQUASH_DEPTH { }
			BOOL SPN_STRETCH_DEPTH { }
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 3;
			
			BOOL SPN_CLAMP_SQUASH { }
			REAL SPN_SQUASH_DIST { UNIT REAL; MIN 0.0; }
			BUTTON SPN_SET_SQUASH_DIST {}
			BOOL SPN_CLAMP_STRETCH { }
			REAL SPN_STRETCH_DIST { UNIT REAL; MIN 0.0; }
			BUTTON SPN_SET_STRETCH_DIST {}
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 2;
			
			BOOL SPN_USE_BIAS_CURVE { }
			BOOL SPN_MIX_VOLUME { }
		}
		SEPARATOR {} 
		GROUP
		{
			COLUMNS 1;
			
			SPLINE SPN_BIAS_CURVE
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

}
