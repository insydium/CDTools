// C4D-DialogResource

DIALOG DLG_CDTR_ABOUT
{
	NAME CDTR_TITLE;
	GROUP
	{
		COLUMNS 1;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_GROUP_IN;
		
		USERAREA IDC_CDTR_ABOUT_IMAGE { CENTER_H; SIZE -320,-60; }
		
		STATICTEXT IDC_CDTR_ABOUT_PLUGININFO { NAME CDTR_PLUGININFO; CENTER_H; }
		STATICTEXT IDC_WEBSITE_INFO { NAME WEBINFO; CENTER_H; }
		BUTTON IDC_CDTR_UPDATE { NAME CDTR_UPDATE; CENTER_H; SIZE -240,-18;  }
		
		GROUP
		{
			COLUMNS 1;
			NAME CDTRANSFER_SERIAL;
			BORDERSIZE 12,6,12,6;
			BORDERSTYLE BORDER_GROUP_IN;
			SCALE_H;

			EDITTEXT IDC_CDTR_SERIALNUMBER { SCALE_H; }
		}
	}

	DLGGROUP { OK; }
}
