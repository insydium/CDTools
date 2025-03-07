// C4D-DialogResource

DIALOG DLG_CDSY_REGISTER
{
	NAME CDSY_TITLE;
	GROUP
	{
		COLUMNS 1;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_GROUP_IN;
		
		USERAREA IDC_CDSY_ABOUT_IMAGE { CENTER_H; SIZE -320,-60; }
		
		STATICTEXT IDC_CDSY_ABOUT_PLUGININFO { NAME CDSY_PLUGININFO; CENTER_H; }
		STATICTEXT IDC_WEBSITE_INFO { NAME WEBINFO; CENTER_H; }
		GROUP
		{
			COLUMNS 1;
			NAME CDSYANSFER_SERIAL;
			BORDERSIZE 12,6,12,6;
			BORDERSTYLE BORDER_GROUP_IN;
			SCALE_H;

			EDITTEXT IDC_CDSY_SERIALNUMBER { SCALE_H; }
		}
	}

	GROUP
	{
		COLUMNS 3;
		CENTER_H;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_NONE;
		
		BUTTON IDC_CDSY_CANCEL { NAME CDSY_CANCEL; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDSY_REGISTER { NAME CDSY_REGISTER; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDSY_PURCHASE { NAME CDSY_PURCHASE; CENTER_H; SIZE -40,-18; }
	}
}
