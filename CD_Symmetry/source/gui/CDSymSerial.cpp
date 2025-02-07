//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDSymmetry.h"

Bool RegisterCDSY(void)
{
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) return true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) return true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) return false;
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) return true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) return false;
#endif
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) return false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) return false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdsnr,pK,2);
	b = GetKeyByte(seed,aK,bK,cK);
	if(kb != LongToHexString(b,2)) return false;
	
	return true;
}

LONG GetSeed(String sn)
{
	CHAR c4dnr[C4D_SERIAL_SIZE];
	INT	snInt[C4D_SERIAL_SIZE];
	LONG i, seed = 0, res = 0;

	sn.GetCString(c4dnr,C4D_SERIAL_SIZE);
	for(i=4; i<11; i++)
	{
		snInt[i] = c4dnr[i] - '0';
		res = snInt[i];
		seed = seed * 10 + res;
	}
	
	return seed;
}

String GetKeyByte(const String key, LONG start, LONG cnt)
{
	String 	s = key;
  
	LONG pos;
	Bool h = s.FindFirst("-",&pos);
	while(h)
	{
		s.Delete(pos,1);
		h = s.FindFirst("-",&pos);
	}
	s.ToUpper();
	
	return s.SubStr(start,cnt);
}

CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c)
{
	CHAR ch;
	
	a = Mod(a,25);
	b = Mod(b,3);
	if(Mod(a,2) == 0) ch = ((seed >> a) & 0x000000FF) ^ ((seed >> b) | c);
	else ch = ((seed >> a) & 0x000000FF) ^ ((seed >> b) & c);

	return ch;
}

String GetChecksum(const String s)
{
	CHAR ch[19];
	LONG i, sum = 0;
	
	s.GetCString(ch,19);
	if(s.GetLength() > 0)
	{
		for(i=1; i<=s.GetLength(); i++)
		{
			sum += (LONG)ch[i];
		}
	}
	return LongToHexString(sum, 4);
}

Bool CheckKeyChecksum(const String key)
{
	String 	s = key, c;
  
	LONG pos;
	Bool h = s.FindFirst("-",&pos);
	while(h)
	{
		s.Delete(pos,1);
		h = s.FindFirst("-",&pos);
	}
	s.ToUpper();
	if(s.GetLength() != 22) return false;
	
	c = s.SubStr(18,4);
	s.Delete(18,4);
	
	return (c == GetChecksum(s));
} 

String LongToHexString(LONG value, const LONG size)
{
	String hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(value);
	
	ch[1] = 0;
	for(i=0; i<size; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}

	return hStr;
}

CHAR GetHexCharacter(LONG n)
{
	CHAR ch = 0;
	
	switch(n)
	{
		case 0:
			ch = 48;
			break;
		case 1:
			ch = 49;
			break;
		case 2:
			ch = 50;
			break;
		case 3:
			ch = 51;
			break;
		case 4:
			ch = 52;
			break;
		case 5:
			ch = 53;
			break;
		case 6:
			ch = 54;
			break;
		case 7:
			ch = 55;
			break;
		case 8:
			ch = 56;
			break;
		case 9:
			ch = 57;
			break;
		case 10:
			ch = 65;
			break;
		case 11:
			ch = 66;
			break;
		case 12:
			ch = 67;
			break;
		case 13:
			ch = 68;
			break;
		case 14:
			ch = 69;
			break;
		case 15:
			ch = 70;
			break;
	}
	
	return ch;
}

