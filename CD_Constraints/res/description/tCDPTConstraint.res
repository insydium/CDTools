CONTAINER tCDPTConstraint
{
	NAME tCDPTConstraint;
	DEFAULT 1;
	GROUP
	{
		BUTTON PTC_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL PTC_SHOW_LINES {}
			COLOR PTC_LINE_COLOR {}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 1;
			
			REAL PTC_STRENGTH { UNIT PERCENT; MIN 0.0; MAX 100.0; CUSTOMGUI REALSLIDER;}
		}
	}
	GROUP PTC_ID_POINTS
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BUTTON PTC_ADD_PT {}
			BUTTON PTC_SUB_PT {}
		}
		SEPARATOR { LINE; }
	}
}