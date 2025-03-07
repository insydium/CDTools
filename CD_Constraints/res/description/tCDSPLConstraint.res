CONTAINER tCDSPLConstraint
{
	NAME tCDSPLConstraint;
	DEFAULT 1;
	GROUP
	{
		BUTTON SPLC_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			LINK  SPLC_TARGET { ANIM ON; ACCEPT { Obase; } }
			LINK  SPLC_UP_VECTOR { ANIM ON; ACCEPT { Obase; } }
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;
			
			BOOL SPLC_TANGENT { } 
			LONG SPLC_ALIGN_AXIS
			{
				CYCLE
				{
					SPLC_ALIGN_X;
					SPLC_ALIGN_NX;
					SPLC_ALIGN_Y;
					SPLC_ALIGN_NY;
					SPLC_ALIGN_Z;
					SPLC_ALIGN_NZ;
				}
			}
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;
			
			BOOL SPLC_ALIGN_CHILDREN { } 
			REAL SPLC_CHILD_OFFSET { UNIT METER; }
		}	
		SEPARATOR {}
		GROUP
		{
			COLUMNS 1;
			
			BOOL SPLC_FORCE_POS { } 
			REAL SPLC_POSITION { UNIT PERCENT; MINSLIDER 0.0; MAXSLIDER 100; STEP 0.01;  CUSTOMGUI REALSLIDER;}
			REAL SPLC_SEGMENT { UNIT REAL; MIN 0.0; }
		}	
	}

}