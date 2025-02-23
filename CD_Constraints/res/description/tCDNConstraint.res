CONTAINER tCDNConstraint
{
	NAME tCDNConstraint;
	DEFAULT 1;
	GROUP
	{
		BUTTON NC_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL NC_SHOW_LINES {}
			COLOR NC_LINE_COLOR {}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 1;
			
			REAL NC_STRENGTH { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}
	}
	
	GROUP NC_ID_TARGET
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			LINK  NC_TARGET { ANIM ON; ACCEPT { Obase; } }
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 3;
			
			BOOL NC_POS_ONLY {}
			BUTTON NC_N_SET {}
			BUTTON NC_N_RELEASE {}
		}
	}
}