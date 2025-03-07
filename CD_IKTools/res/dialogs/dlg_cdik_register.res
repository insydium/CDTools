// C4D-DialogResource

DIALOG DLG_CDIK_REGISTER
{
	NAME TITLE;
	GROUP
	{
		COLUMNS 1;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_GROUP_IN;
		
		USERAREA IDC_CDIK_ABOUT_IMAGE { CENTER_H; SIZE -320,-60; }
		
		STATICTEXT IDC_CDIK_ABOUT_PLUGININFO { NAME CDIK_PLUGININFO; CENTER_H; }
		STATICTEXT IDC_WEBSITE_INFO { NAME WEBINFO; CENTER_H; }
		GROUP
		{
			COLUMNS 1;
			NAME CDIKTOOLS_SERIAL;
			BORDERSIZE 12,6,12,6;
			BORDERSTYLE BORDER_GROUP_IN;
			SCALE_H;

			EDITTEXT IDC_CDIK_SERIALNUMBER { SCALE_H; }
		}
	}

	GROUP
	{
		COLUMNS 3;
		CENTER_H;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_NONE;
		
		BUTTON IDC_CDIK_CANCEL { NAME CDIK_CANCEL; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDIK_REGISTER { NAME CDIK_REGISTER; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDIK_PURCHASE { NAME CDIK_PURCHASE; CENTER_H; SIZE -40,-18; }
	}
}
