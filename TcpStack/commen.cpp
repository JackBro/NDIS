// Commen.cpp: implementation of the CCommen class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TcpStack.h"
#include "Commen.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////




/*ϵͳ���д˺���������֮*/
//ת���������ֽ�˳��
USHORT Lhtons(USHORT hostshort)
{
	return ((hostshort<<8) | (hostshort>>8));
}
//���������ֽ�ת����UCHAR����
void Lhtonl(UINT hostlong, PUCHAR buddle)
{
	buddle[0] = (UCHAR)(hostlong >> 24);
	buddle[1] = (UCHAR)((hostlong >> 16) & 0x000000FF);
	buddle[2] = (UCHAR)((hostlong >> 8) & 0x000000FF);
	buddle[3] = (UCHAR)(hostlong & 0x000000FF);
}
//��UCHAR����ת����UINT��
UINT Lnltoh(PUCHAR buddle)
{
	UINT temp1 = buddle[0], temp2 = buddle[1], temp3 = buddle[2], temp4 = buddle[3];
	return (((temp1 << 24) & 0xFF000000) | ((temp2 << 16) & 0x00FF0000) | ((temp3 << 8) & 0x0000FF00) | (temp4 & 0x000000FF));
}
//��UCHAR����ת����USHORT��
USHORT Lnstoh(PUCHAR buddle)
{
	USHORT temp1 = buddle[0], temp2 = buddle[1];
	return (((temp1 << 8) & 0xFF00) | (temp2 & 0x00FF));
}
//�ж�����UCHAR�����Ƿ���ͬ
BOOL IsUcSame(PUCHAR Uc1, PUCHAR Uc2, int len)
{
    for(int i=0; i<len; i++)  
		if(Uc1[i] != Uc2[i])
			return FALSE;
	return TRUE;
}
//���޷����ַ���ת��Ϊ�з����ַ���
PCHAR PUchTOch(PUCHAR puch, USHORT len)
{
	PCHAR pch = new CHAR[len];
	for(USHORT i=0; i<len; i++)
		pch[i] = (char)(puch[i]);
	return pch;
}
//���з����ַ���ת��Ϊ�޷����ַ���
PUCHAR PchTOUch(PCHAR pch, USHORT len)
{
	PUCHAR puch = new UCHAR[len];
	for(USHORT i=0; i<len; i++)
		puch[i] = (char)(pch[i]);
	return puch;
}