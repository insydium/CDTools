// C4D-DialogResource

DIALOG DLG_CDSK_ABOUT
{
	NAME CDSK_TITLE;
	GROUP
	{
		COLUMNS 1;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_GROUP_IN;
		
		USERAREA IDC_CDSK_ABOUT_IMAGE { CENTER_H; SIZE -320,-60; }
		
		STATICTEXT IDC_CDSK_ABOUT_PLUGININFO { NAME CDSK_PLUGININFO; CENTER_H; }
		STATICTEXT IDC_WEBSITE_INFO { NAME WEBINFO; CENTER_H; }
		BUTTON IDC_CDSK_UPDATE { NAME CDSK_UPDATE; CENTER_H; SIZE -240,-18;  }
		GROUP
		{
			COLUMNS 1;
			NAME CDSK_SERIAL;
			BORDERSIZE 12,6,12,6;
			BORDERSTYLE BORDER_GROUP_IN;
			SCALE_H;

			EDITTEXT IDC_CDSK_SERIALNUMBER { SCALE_H; }
		}
	}

	DLGGROUP { OK; }
}
