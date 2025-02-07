// C4D-DialogResource

DIALOG DLG_CDTR_REGISTER
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

	GROUP
	{
		COLUMNS 3;
		CENTER_H;
   		BORDERSIZE 8,8,8,8;
		BORDERSTYLE BORDER_NONE;
		
		BUTTON IDC_CDTR_CANCEL { NAME CDTR_CANCEL; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDTR_REGISTER { NAME CDTR_REGISTER; CENTER_H; SIZE -40,-18; }
		BUTTON IDC_CDTR_PURCHASE { NAME CDTR_PURCHASE; CENTER_H; SIZE -40,-18; }
	}
}
