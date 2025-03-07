CONTAINER tCDSymTag
{
	NAME tCDSymTag;
	DEFAULT 1;
	GROUP
	{
		BUTTON SYM_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BOOL SYM_SHOW_GUIDE { }
			REAL SYM_GUIDE_SIZE { UNIT REAL; MIN 0.0; STEP 1.0; }
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR SYM_GUIDE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 1;
			
			BOOL SYM_USE_SYMMETRY {}
		}
		SEPARATOR { }
		GROUP
		{
			COLUMNS 2;
			
			BUTTON SYM_SET_SYMMETRY {}
			BUTTON SYM_RELEASE_SYMMETRY {}
		}
		SEPARATOR { }
		GROUP
		{
			COLUMNS 2;
			
			REAL SYM_TOLERANCE { UNIT REAL; MIN 0.0; STEP 0.001; }
			LONG SYM_SYMMETRY_AXIS
			{
				CYCLE
				{
					SYM_MX;
					SYM_MY;
					SYM_MZ;
				}
			}
		}
		SEPARATOR { }
		GROUP
		{
			COLUMNS 1;
			
			BOOL SYM_LOCK_CENTER { }
		}
		SEPARATOR { }
		GROUP
		{
			COLUMNS 2;
			
			BOOL SYM_RESTRICT_SYM { }
			LONG SYM_RESTRICT_AXIS
			{
				CYCLE
				{
					SYM_POSITIVE;
					SYM_NEGATIVE;
				}
			}
		}
	}
}