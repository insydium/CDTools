CONTAINER tCDMorphMix
{
	NAME tCDMorphMix;
	DEFAULT 1;
	GROUP
	{
		BUTTON MM_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		SCALE_V;
		
		GROUP MM_LIST_GROUP
		{
			COLUMNS 1;
			SCALE_V;
			
			BUTTON MM_EDIT_M_TAG {}
			IN_EXCLUDE MM_M_TAG_LIST { NUM_FLAGS 0; INIT_STATE 0; SEND_SELCHNGMSG 1; ACCEPT { 1017237; } }
		}
	}
	GROUP MM_MIXER_GROUP
	{
		COLUMNS 1;
		
	}
}