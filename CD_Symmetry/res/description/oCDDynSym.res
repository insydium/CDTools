CONTAINER oCDDynSym
{
	NAME oCDDynSym;
	DEFAULT 1;
	GROUP
	{
		BUTTON DS_PURCHASE {}
	}
	INCLUDE Obase;

	GROUP ID_OBJECTPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BOOL DS_SHOW_GUIDE { }
			REAL DS_GUIDE_SIZE { UNIT REAL; MIN 0.0; STEP 1.0; }
		}
		GROUP
		{
			COLUMNS 1;
			
			COLOR DS_GUIDE_COLOR {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 1;
			
			LONG DS_SYMMETRY_AXIS
			{
				CYCLE
				{
					DS_MX;
					DS_MY;
					DS_MZ;
				}
			}
			LONG DS_ACTIVE_SYMMETRY
			{
				CYCLE
				{
					DS_POSITIVE;
					DS_NEGATIVE;
				}
			}
			REAL DS_TOLERANCE { UNIT REAL; MIN 0.0; STEP 0.001; }
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 1;
			
			BOOL DS_AUTO_UPDATE { }
			BOOL DS_LOCK_CENTER { }
			BOOL DS_KEEP_SEL { }
			BOOL DS_KEEP_UVW { }
		}
	}
}