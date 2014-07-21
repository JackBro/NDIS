// Mac.cpp: implementation of the Mac class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TcpStack.h"
#include "Mac.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mac::Mac()
{
	//��ʼ��
    CUR_IP_MAC_LEN = 0;
	for(int i=0;i<4;i++)
	{
		DestIpAddr[i] = 0;
		SrcIpAddr[i] = 0;
		SubNetMask[i] = 0;
	}
	for(int j=0;j<6;j++)
	{
	    SrcMacAddr[j] = 0;	
		DestMacAddr[j] = 0;
        GateWayMacAddr[j] = 0;
	}
	ArpMacEvent = NULL;
}

Mac::~Mac()
{

}

BOOL Mac::DecodeMac(unsigned char* &packet)
{
	//�����ArpӦ��������ܴ��еõ�Mac��ַ�򷵻�Ϊ��
	if(GetMacFromArpPacket(packet))
		return FALSE;
	//�����ȥ��̫��ͷ�������棬֤������һ��IP��
	else
		packet += sizeof(ETH_HEADER);
	return TRUE;
}

BOOL Mac::GetMacFromArpPacket(unsigned char *packet)
{
	if(packet[12]==0x08 
		&& packet[13]==0x06
		&& packet[20]==0x00
	    && packet[21]==0x02
		&& packet[28]==DestIpAddr[0]
		&& packet[29]==DestIpAddr[1]
		&& packet[30]==DestIpAddr[2]
		&& packet[31]==DestIpAddr[3]
	  )
	{
		//�õ�MAC��ַ
		for(int i=0; i<6; i++)
			DestMacAddr[i]=packet[i+6];

	    SetEvent(ArpMacEvent);

		return TRUE;		
	}
	return FALSE;
}

BOOL Mac::SendWithMac(unsigned char* &packet ,int PacketLen, HANDLE DriverHandle)
{
	//���仺����
	UINT dwStart = GetTickCount();

	unsigned char *MacPacket=(unsigned char*)malloc(PacketLen + sizeof(ETH_HEADER));
	memset(MacPacket,0,PacketLen + sizeof(ETH_HEADER));
	MoveMemory(MacPacket + sizeof(ETH_HEADER), packet, PacketLen);

	//�����������Ϊ�գ������ȵõ���������
	if(IsEmpty(SubNetMask,4))
		GetSubNetMask();

	if(IsEmpty(SrcMacAddr,4))
		GetSrcMac(DriverHandle);

	GetSrcAndDestIpAddr(packet);
	
	//���ͨ��˫����ͬһ����
	if(BothInLocalNetwork())
	{
		//�����Ӧ����û��˫����Ϣ����ARP��ѯ�� ����в�ѯ��ͬʱ�ͽ�
		//DestMacAddr��ֵ
		if(!CheckRecord())
		{
			//��ARP��ѯMAC�����ڱ������˫����Ϣ
			if(SendArpToGetMac(DriverHandle))
				AddToRecord();
		}
	}
	else
	{
		if(IsEmpty(GateWayMacAddr, 6))
		{
			if(SendArpToGetMac(DriverHandle))
				MoveMemory(GateWayMacAddr,DestMacAddr,6);
		}
		else
			MoveMemory(DestMacAddr, GateWayMacAddr, 6);
	}

	//���
	MoveMemory(MacPacket, DestMacAddr, 6);
	MoveMemory(MacPacket+6, SrcMacAddr, 6);
	MacPacket[12] = 0x08;
	MacPacket[13] = 0x00;

	//�滻ԭ�а�
	unsigned char *temp = packet;
	packet = MacPacket;
	temp = NULL;

	dwStart = GetTickCount() - dwStart;

	return TRUE;
}

int Mac::GetSrcMac(HANDLE DriverHandle)
{
    DWORD       BytesReturned;
    UCHAR       QueryBuffer[sizeof(SECLAB_QUERY_OID) + MAC_ADDR_LEN];
    PSECLAB_QUERY_OID  pQueryOid;

    pQueryOid = (PSECLAB_QUERY_OID)&QueryBuffer[0];
    pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;

	HRESULT hr;

    if(!(hr=DeviceIoControl(
               DriverHandle,
               IO_GET_SRCMAC_MAC,
               (LPVOID)&QueryBuffer[0],
               sizeof(QueryBuffer),
               (LPVOID)&QueryBuffer[0],
               sizeof(QueryBuffer),
               &BytesReturned,
               NULL)))
	{
		AfxMessageBox("CAN NOT GET SRC MAC");
		return 0;
	}

	memcpy(SrcMacAddr, pQueryOid->Data, MAC_ADDR_LEN);
    return 1;
}

int Mac::GetSubNetMask()
{
	HKEY hKey;
	//DWORD disposition;
 
	LPBYTE owner_Get=new BYTE[256]; 
    DWORD Type=REG_SZ; 
	DWORD DataSize=256;

	//�����洢�м���
	TCHAR StrMid[256];
    memset(StrMid,0,256);

	//��ʱ�ַ���������������̽��ֵ
	TCHAR CommonSignal[5];
	memset(CommonSignal,0,5);
	int   num=0;

	//�ַ���������������Ӽ�ֵ
	char SubKey[256];

	for(num=0;num<1000;num++)
	{
		sprintf(CommonSignal,"%d",num);
		if((RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    strcat(strcpy(SubKey,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\"),CommonSignal),
			0,
		    KEY_READ,//����Ϊ0,���������ǲ�ѯ�����޸�/ɾ������ΪKEY_READ��KEY_WRITE
		    &hKey))!=ERROR_SUCCESS)
			continue;
		else
			break;
		return 0;
	}

    if( RegQueryValueEx(
		 hKey,
		 "ServiceName",
		 NULL,
		 &Type,
		 owner_Get,
		 &DataSize)== ERROR_SUCCESS)
	{
		 strcpy(StrMid,(char*)owner_Get);
	}
	else 
		return 0;

	//
	//�����Ǹ���StrMid��ֵ���õ�SUBNETMARK
	//
    if((RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		strcat(strcat(strcpy(SubKey,"SYSTEM\\CurrentControlSet\\Services\\"),StrMid),"\\Parameters\\Tcpip"),
		0,
		KEY_READ,
		&hKey))==ERROR_SUCCESS)
	{
		 if( RegQueryValueEx(
		     hKey,
		     "SubnetMask",
		     NULL,
		     &Type,
		     owner_Get,
		     &DataSize)== ERROR_SUCCESS)
		 {
			 //�õ�IP��ַ�����䱣�浽ȫ�ֱ���SrcIpAddr�У���������
		     strcpy(StrMid,(char*)owner_Get);
			 char *token= strtok( StrMid, "." );
			 int i=0;
             while( token != NULL)
			 {
                 SubNetMask[i]=(unsigned char)(atoi(token));
				 token = strtok( NULL, "." );
				 i++;
			 }
			 return 1;
		 }
	}

	return 0;
}

void Mac::GetSrcAndDestIpAddr(unsigned char *packet)
{
	MoveMemory(SrcIpAddr, packet+12, 4);
    MoveMemory(DestIpAddr, packet+16, 4);
}

BOOL Mac::IsEmpty(unsigned char *s, int len) 
{
	for(int i=0;i<len;i++)
		if(s[i] != 0)
			return FALSE;
	return TRUE;
}
BOOL Mac::BothInLocalNetwork()
{
	if( IsEmpty(SubNetMask,4) || IsEmpty(DestIpAddr,4) || IsEmpty(SrcIpAddr,4) )
	{
		AfxMessageBox("ERROR IN IP!");
		return FALSE;
	}

	//��ʱ����
	unsigned char tempsrcip[4];
	unsigned char tempdestip[4];

	for(int i=0; i<4; i++)
	{
		tempsrcip[i] = (unsigned char) (SubNetMask[i] & SrcIpAddr[i]);
		tempdestip[i] = (unsigned char) (SubNetMask[i] & DestIpAddr[i]);
		if(tempsrcip[i] != tempdestip[i])
			return FALSE;
	}

	return TRUE;
}

BOOL Mac::IsSame(unsigned char *p, unsigned char *q, int len)
{
    for(int i=0; i<len; i++)  
		if(p[i] != q[i])
			return FALSE;
	return TRUE;
}

BOOL Mac::CheckRecord()
{
	for(int i=0; i<CUR_IP_MAC_LEN; i++)
	{
		if(IsSame(IP_MAC_STABLE[i].IP, DestIpAddr, 4))
		{
			MoveMemory(DestMacAddr, IP_MAC_STABLE[i].MAC, 6);
			return TRUE;
		}
	}
	return FALSE;
}

void Mac::AddToRecord()
{
	MoveMemory(IP_MAC_STABLE[CUR_IP_MAC_LEN].IP, DestIpAddr, 4);
	MoveMemory(IP_MAC_STABLE[CUR_IP_MAC_LEN].MAC, DestMacAddr, 6);
	if(CUR_IP_MAC_LEN < _IP_MAC_LEN-1)
		CUR_IP_MAC_LEN ++;
}

BOOL Mac::SendArpToGetMac(HANDLE DriverHandle)
{
	//����arp���ģ�Ȼ���ڽ���ģ���з���Ҫ�õ���MAC��ַ
    unsigned char pPacketContent[MAX_SEND_ARPPKTLEN];
	
	P_ETH_HEADER  pEthHeader;                  //��̫��ͷ
	P_ARP_HEADER  pArpHeader;                  //ARPͷ
	PUCHAR  pData;                             //���ݲ���

	DWORD dwReturnBytes;
    HRESULT hr;
	
	if(ArpMacEvent == NULL)
		ArpMacEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	do
	{
		 ResetEvent(ArpMacEvent);

		 memset(pPacketContent,0,MAX_SEND_ARPPKTLEN);
  
		 pEthHeader = (P_ETH_HEADER)pPacketContent;
         pEthHeader->EthType[0] = 0x08;
         pEthHeader->EthType[1] = 0x06;//ARP������Ϊ0x0806
		 
         memcpy(pEthHeader->SrcMac, SrcMacAddr, 6);
		 pEthHeader->DestMac[0]=0xFF;
		 pEthHeader->DestMac[1]=0xFF;
		 pEthHeader->DestMac[2]=0xFF;
		 pEthHeader->DestMac[3]=0xFF;
		 pEthHeader->DestMac[4]=0xFF;
		 pEthHeader->DestMac[5]=0xFF;

         pArpHeader = (P_ARP_HEADER)(pPacketContent+sizeof(ETH_HEADER));
		 pArpHeader->HardWareType[0]=0x00;
		 pArpHeader->HardWareType[1]=0x01;//0x00 01��ʾ��̫������
         pArpHeader->ProtocolType[0]=0x08;
         pArpHeader->ProtocolType[1]=0x00;//0x08 00��ʾIPЭ������
         pArpHeader->HardWareAddrLen=6;//��̫������������Ӳ����ַ����Ϊ6
         pArpHeader->ProtocolAddrLen=4;//����IP��ַ����Ϊ4
         pArpHeader->Operation[0]=0x00;
         pArpHeader->Operation[1]=0x01;//��ʾARP�������
		 memcpy(pArpHeader->SrcMacAddr, SrcMacAddr, 6);//���Ͷ˼�Դ�˵�MAC��ַ
		 memcpy(pArpHeader->SrcIpAddr, SrcIpAddr,4);//Դ�˵�IP��ַ
		 memcpy(pArpHeader->DestIpAddr, DestIpAddr,4);//Ŀ��IP��ַ��ע�����ﲻ�����Ŀ��MAC��ַ

         pData=pPacketContent+sizeof(ETH_HEADER)+sizeof(ARP_HEADER);

	     for (UINT i = 0; i < MAX_SEND_ARPPKTLEN - sizeof(ETH_HEADER)-sizeof(ARP_HEADER); i++)
		 {
              *pData++ = (UCHAR)i;
		 }

		 //for (int SendCount = 1; SendCount<PACKETNUM; SendCount++)
         {			
            if(!(hr=DeviceIoControl(DriverHandle, 
						IO_SEND_PACKET_MAC, 
						pPacketContent, 
						sizeof(pPacketContent),
						NULL, 
						0, 
						&dwReturnBytes, 
						NULL 
						)))
			{
		         AfxMessageBox("CAN NOT SEND ARP PACKET");
		         return 0;
			}

		 }

	}
	while(FALSE);

	WaitForSingleObject(ArpMacEvent , INFINITE);
	
	return 1;
}