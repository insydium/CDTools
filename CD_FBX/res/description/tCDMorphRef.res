CONTAINER tCDMorphRef
{
	NAME tCDMorphRef;
	DEFAULT 1;
	GROUP
	{
		BUTTON MR_PURCHASE {}
	}
	INCLUDE Texpression;
	
	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BUTTON MR_EDIT_MESH {}
			BUTTON MR_RESET_REF {}
		}
	}
}