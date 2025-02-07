CONTAINER tCDRotator
{
	NAME tCDRotator;
	DEFAULT 1;
	GROUP
	{
		BUTTON RTR_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BOOL RTR_SHOW_LINES { }
			
			LONG RTR_LINE_TARGET
			{
				CYCLE
				{
					RTR_LINE_ROOT;
					RTR_LINE_TIP;
				}
			}
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR RTR_LINE_COLOR {}
		}
		GROUP
		{
			COLUMNS 1;

			LONG RTR_BONES_IN_GROUP	{ MIN 1; }
		}
		GROUP
		{
			COLUMNS 2;
			
			BOOL RTR_SNAP_ON { }
			
			LONG RTR_SNAP_TO
			{
				CYCLE
				{
					RTR_LOCK_CONTROLLER;
					RTR_LOCK_BONE;
				}
			}

		}
		GROUP
		{
			COLUMNS 2;

			BOOL RTR_CONNECT_BONES { }
			BOOL RTR_CONNECT_NEXT { }
		}
		SEPARATOR { }
		GROUP
		{
			COLUMNS 2;

			BOOL RTR_USE_BIAS_CURVE { }
			REAL RTR_CURVE_SCALE { UNIT REAL; MIN 0.0; STEP 0.01; }
		}
		GROUP
		{
			COLUMNS 1;
			
			SPLINE RTR_BIAS_CURVE
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

	GROUP RTR_CONTROLLER_GROUP
	{
		DEFAULT	1;
		
		GROUP 
		{ 
			LINK  RTR_CONTROLLER_LINK
			{ 
				ANIM MIX; ACCEPT { Obase; } 
				REFUSE { Osky; Oforeground; } 
			} 
		}

	}

}
