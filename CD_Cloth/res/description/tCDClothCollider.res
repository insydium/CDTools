CONTAINER tCDClothCollider
{
	NAME tCDClothCollider;
	DEFAULT 1;
	GROUP
	{
		BUTTON CLD_PURCHASE {}
	}
	INCLUDE Texpression;

	GROUP ID_TAGPROPERTIES
	{
		DEFAULT	1;
		
		GROUP
		{
			COLUMNS 1;
			
			LONG CLD_COLLIDER_TYPE
			{
				CYCLE
				{
					CLD_PLANE;
					CLD_SPHERE;
					CLD_JOINT;
					CLD_POLYGON;
				}
			}
		}
		SEPARATOR {}
		GROUP
		{
			COLUMNS 2;
			
			REAL CLD_RADIUS { UNIT METER; MIN 0.0; }
			REAL CLD_OFFSET { UNIT METER; MIN 0.0; }
			LONG CLD_PLANE_NORM
			{
				CYCLE
				{
					CLD_NORM_XP;
					CLD_NORM_XN;
					CLD_NORM_YP;
					CLD_NORM_YN;
					CLD_NORM_ZP;
					CLD_NORM_ZN;
				}
			}
		}
		GROUP CLD_SELECTION_GROUP
		{
			DEFAULT	1;
			COLUMNS 3;
			
			BUTTON CLD_SET_SELECTION {}
			BUTTON CLD_EDIT_SELECTION {}
			BUTTON CLD_RESTORE_SELECTION {}
			
		}
	}

}