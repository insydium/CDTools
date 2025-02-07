CONTAINER tCDBindPose
{
	NAME tCDBindPose;
	DEFAULT 1;
	GROUP
	{
		BUTTON BND_PURCHASE {}
	}
	INCLUDE Tbase;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 3;
			
			BOOL BND_RIG_MIRROR {}
			LONG BND_MIRROR_AXIS
			{
				CYCLE
				{
					BND_X_AXIS;
					BND_Y_AXIS;
					BND_Z_AXIS;
				}
				ANIM OFF; 
			}
			BOOL BND_LOCAL_CENTER {}
		}
		SEPARATOR { LINE; }
		GROUP
		{
			COLUMNS 4;
			
			STATICTEXT BND_POSE_NAME { ANIM OFF; }
			BUTTON BND_POSE_SET {}
			BUTTON BND_POSE_EDIT {}
			BUTTON BND_POSE_RESTORE {}
		}
	}
	GROUP BND_OBJECT_GROUP
	{
		SCALE_V;
		
		GROUP
		{
			COLUMNS 1;
			SCALE_V;
			
			BUTTON BND_ASSIGN_MIRROR {}
			IN_EXCLUDE BND_OBJECT_LIST { NUM_FLAGS 0; SEND_SELCHNGMSG 1; SCALE_V; ACCEPT { Obase; } }
		}
	}
	GROUP BND_POSE_GROUP
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 2;
			
			BUTTON BND_POSE_ADD {}
			BUTTON BND_POSE_SUB {}
		}
		SEPARATOR { LINE; }
		GROUP BND_POSE_SUBGROUP
		{
			
		}
	}
}