CONTAINER tCDNDisplay
{
	NAME tCDNDisplay;
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT 1;
		COLUMNS 2;
		
		BOOL ND_NORMAL_SHOW {}
		REAL ND_NORMAL_SIZE { UNIT METER; MIN 1.0; STEP 1.0; }
	}
}
