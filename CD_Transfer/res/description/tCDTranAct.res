CONTAINER tCDTranAct
{
	NAME tCDTranAct;
	DEFAULT 1;
	GROUP
	{
		BUTTON TRNSA_PURCHASE {}
	}
	INCLUDE Tbase;

	GROUP ID_TAGPROPERTIES
	{
		
		GROUP
		{
			COLUMNS 1;
			
			BOOL TRNSA_TRANSFER_ON { }
			LINK  TRNSA_TARGET
			{ 
				ANIM MIX; ACCEPT { } 
				REFUSE { Osky; Oforeground; } 
			}
		}	
	}
}
