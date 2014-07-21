// packet.cpp : Defines the class behaviors for the TCP Module
//

#include "stdafx.h"
#include "TcpStack.h"

#include "packet.h"
#include "Ftp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////
//////�������
//////


//�����߳�
/*UINT*/VOID ReceiveThreadFunction(void*)
{
	DWORD dwReturnBytes;
	unsigned char EventRead[MY_MTU];   
	PACKET Packet;

	HANDLE FileIOWaiter = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(FileIOWaiter == NULL)
		return;
	
	OVERLAPPED ol;
	ol.Offset = 0;
	ol.OffsetHigh = 0;
	ol.hEvent = FileIOWaiter;

    for(;;)
 	{
		//�ʵ���Ϣһ��
		/*���ܼ򵥵���Ϣһ��ʱ�䣬�����п��ƣ�����ᶪ��*/
        DWORD TxdBytes;
		memset(EventRead, 0, MY_MTU);
		ResetEvent(FileIOWaiter);
		
		if(!DeviceIoControl(Global.ListenerInfo->DebugPrintDriver, 
						IO_RECEIVE_PACKET,  
						NULL, 
						0, 
						EventRead, 
						MY_MTU,
						&dwReturnBytes, 
						&ol/*NULL*/
						))	
		{
		      if( GetLastError()!=ERROR_IO_PENDING)
			  {
				   ErrMsg(0, L"CAN NOT READ INFO");
				   break;
			  }	
			  while(WaitForSingleObject(FileIOWaiter, 100) == WAIT_TIMEOUT)
			  {
				  if(!Global.ListenerInfo->ReceiveKeepGoing)
				  {
					  // Cancel the pending read
			          CancelIo(Global.ListenerInfo->DebugPrintDriver);
			          goto Exit;
				  } 
 			  }
			  if(!GetOverlappedResult(Global.ListenerInfo->DebugPrintDriver,
				  &ol,
				  &TxdBytes,
				  FALSE))
			  {
				  continue;
			  }
		}
	    else
		{
			  if( dwReturnBytes < 14)
			  {
			       continue;
			  }
		      //MessageBox(NULL,"OK","SUCESS",MB_OK);

		      Packet.pPacketContent=(UCHAR*)malloc(dwReturnBytes);
		      Packet.PacketLenth=dwReturnBytes;

              memcpy(Packet.pPacketContent,EventRead,dwReturnBytes);

			  EnterCriticalSection(&Global.cs_pq);
		      Global.PacketQueue->EnQueue(Packet);
			  LeaveCriticalSection(&Global.cs_pq);

//  		  SetEvent(Global.ListenerInfo->DecodeWaiter);
		      //AfxMessageBox(EventRead);
		}
	}
  
Exit:
	CloseHandle(FileIOWaiter);
	SetEvent(Global.ListenerInfo->ReceiveEvent);
	//_endthread();
	return /*1*/;
}

//�ж�����UCHAR�����Ƿ���ͬ
BOOL IsSame(unsigned char *p, unsigned char *q, int len)
{
    for(int i=0; i<len; i++)  
		if(p[i] != q[i])
			return FALSE;
	return TRUE;
}

//���Ľ�����ĿǰֻĿǰֻ֧��EtherNet��·���ʽ��IP�߲�Э��
void DecodePacket(UCHAR* pPacketContent, UINT len)
{
	//�ж��Ƿ�ΪARP���ذ���������򲻴���
	if(!Global.mac->DecodeMac(pPacketContent))
		return;
    
	//��ʱ�İ��Ѿ�ȥ����MACͷ���ж��Ƿ�Ϊ�Լ��İ�����������򲻴���
	P_TCP_DATA_IDENTIFIER pTcpDataIdentifier = GetIdentifierFromPacket(pPacketContent);
	P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier = GetTailIdentifierFromPacket(pPacketContent);
	if(
	   pTcpDataIdentifier == NULL                          ||
	   pTcpDataIdentifier->Myself[0] != MYSELF_SIG1        || 
	   pTcpDataIdentifier->Myself[1] != MYSELF_SIG2        ||
	   pTcpDataIdentifier->Myself[2] != MYSELF_SIG3        ||
	   pTcpDataIdentifier->Myself[3] != MYSELF_SIG4        ||
	   pTcpDataTailIdentifier == NULL                      ||
	   pTcpDataTailIdentifier->Myself[0] != MYSELF_SIG1    ||
	   pTcpDataTailIdentifier->Myself[1] != MYSELF_SIG2    ||
	   pTcpDataTailIdentifier->Myself[2] != MYSELF_SIG3
	  )
	{
		return;
	}

	//�õ�TcbNum����������Ѱ�Ҵ�������ӵ�TCB��
	UINT ComingRemoteTcbNum = Lnltoh(pTcpDataIdentifier->RemoteTcbNum);

	//�õ�������Ϣ
	UINT ComingTcbNum = Lnltoh(pTcpDataIdentifier->TcbNum);
	int type = Lnstoh(pTcpDataIdentifier->Comm) + 1;
	int curstatus = Lnstoh(pTcpDataIdentifier->CurStatus);
	USHORT datalen = GetDataLenFromPacket(pPacketContent);
    PUCHAR pdata = GetDataFromPacket(pPacketContent);
	PUCHAR srcip = GetSrcIpFromPacket(pPacketContent);
	PUCHAR subip = GetSubIPFromPacket(pPacketContent);
	USHORT srcport = GetSrcPortFromPacket(pPacketContent);
	USHORT dstport = GetDstPortFromPacket(pPacketContent);
	UINT   filesize = GetFileSizeFromPacket(pPacketContent);
	
	//����Srcip��Subip���ж��Ƿ���ڻ���ᣬ�����һ���ж��ĸ�TCB����˲�����
	//�����˳�
	CONNECTION* conn = Global.ConnList->FindByIPAndSubIP(srcip, subip);

/*  //�������˾�Ϊ������ַ���û�ʹ�� 	
    if(conn == NULL || conn->m_Active == FALSE && type != CONNECT_F_COMMING && type != CONNECT_T_COMMING)
	{
		return;
	}
*/
    //����������һ��Ϊ�ⲿIP��ַ���û�ʹ��  
	if(conn == NULL && type != CONNECT_F_COMMING)
	{
		return;
	}
	if(conn != NULL)
	{
		if(conn->m_Active == FALSE && type != CONNECT_F_COMMING && type != CONNECT_T_COMMING)
		{
			return;
		}
	}

/*  //�������˾�Ϊ������ַ���û�ʹ�� 	

	//���û�д����TCB��˵����һ���µ���������������������������ж�
// 	EnterCriticalSection(&Global.cs_TcbList);
	TCB * tcb = conn->TcbList->Find(ComingRemoteTcbNum);
*/ 
    //����������һ��Ϊ�ⲿIP��ַ���û�ʹ��
	TCB* tcb = NULL;
	if(conn)
	{
		tcb = conn->TcbList->Find(ComingRemoteTcbNum);
	}
	else
		tcb = NULL;
	
	if(tcb == NULL)
	{
// 		LeaveCriticalSection(&Global.cs_TcbList);
		//�����һ���µ����������ֱ�ӽ�����FTPģ�飬��FTPģ�����½�TCB��������
		//������������TCB��Ա����DealWithInput�д���
		if(ComingRemoteTcbNum == -1)
		{
            FtpControlCenter(type, PUchTOch(pdata, datalen), datalen, NULL, NULL, filesize, srcip, subip, srcport, dstport, ComingTcbNum, curstatus);
		}
		else
			return;
	}  
	
	//������ҵ������TCB��������������ACK��������Զ��TCB�Ÿ���
	//�����ж����ڵ�Զ��TCB�ź͸������Ƿ���ͬ����ͬ�����˰�
	else
	{
		if((type == COMM_ACK_COMMING || type == CONNECT_T_COMMING) && tcb->m_bBlockAck == FALSE)
			tcb->m_RemoteTcbNum = ComingTcbNum;
	    else if(tcb->m_RemoteTcbNum != ComingTcbNum)
			return;
	
	    //����TCB��DealWithInput
		if(tcb->m_bBlockData == FALSE)
			tcb->DealWithInput(type, pPacketContent);

// 		LeaveCriticalSection(&Global.cs_TcbList);
	}
	
}

//�Ӷ�����ȡ�����ݰ�����������߳�
/*UINT*/VOID
DecodeThreadFunction(void*)
{
	for(;;)
	{
		//�ʵ���Ϣһ��
		/*���ܼ򵥵���Ϣһ��ʱ�䣬�����п��ƣ�����ᶪ��*/
		
		unsigned char EventWrite[MY_MTU];

		EnterCriticalSection(&Global.cs_pq);
	    P_PACKET pPacket=Global.PacketQueue->DeQueue();
		LeaveCriticalSection(&Global.cs_pq);

	    if(pPacket==NULL)
		{
		    //WaitForSingleObject(Global.ListenerInfo->DecodeWaiter, 10);
			Sleep(1);
			continue;
		}

		else
		{
			memcpy(EventWrite, pPacket->pPacketContent, pPacket->PacketLenth);
		    free(pPacket->pPacketContent);

		    //���а��Ľ�����Ŀǰֻ֧��EtherNet��·���ʽ��IP�߲�Э��
            DecodePacket(EventWrite, pPacket->PacketLenth);
		}
	}

	CloseHandle(Global.ListenerInfo->DecodeWaiter);
	SetEvent(Global.ListenerInfo->DecodeEvent);
	//_endthread();
	return /*1*/;
}




/////
/////�������
/////


//
//�õ�������IP��ַ
//
int GetSrcIpAddr()
{
	//����ע���,�˳������Ӧ�ò�������㶼ֻ֧�ֵ�������û�п��Ƕ����������⡣
	//�����Ӧ�ó�����ֻ��ҪѰ�ҵ�������IP��������������ϵ�IP�����ܱ�֤�ó���
	//��ȷ
	
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
	//�����Ǹ���StrMid��ֵ���õ�IP
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
		     "IPAddress",
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
                 Global.SrcIpAddr[i]=(unsigned char)(atoi(token));
				 token = strtok( NULL, "." );
				 i++;
			 }
			 return 1;
		 }
	}

	return 0;
}


//����IPУ���
USHORT CheckSumFunc(USHORT *pAddr,int len)
{
	int sum=0;
	while(len>1)
	{
		sum+=Lhtons(*pAddr++);
		len-=2;
	}
	if(len>0)
		sum+=*(UCHAR*)pAddr;
	sum=(sum>>16)+(sum & 0xffff);
	sum+=(sum>>16);
	return (~sum) & 0xffff;
}

//����TCPУ���
USHORT UdpOrTcpCheckSumFunc(PUCHAR pIPBuf, int size) 
{ 
	UCHAR *pCheckIP = new UCHAR[size];
	memset(pCheckIP,0,size);

    P_TEMP_TCP_HEADER pCheckHeader;//ָ��αͷ 

    unsigned char *p;//ָ��TCP or UDP����ͷ�� 
    unsigned char *p1;//ָ��Ŀ�Ķ˿ں�  

    P_IP_HEADER  pTempIP; 
    P_TCP_HEADER pTCPHeader; 

    USHORT IPHeaderLen ; 
    USHORT TCPHeaderLen; 
    USHORT nCheckSum = 0; 

    pTempIP=(IP_HEADER*)pIPBuf; 

    p=(UCHAR*)pTempIP+(UCHAR)((pTempIP->Ver_IHL)&0x0f)*4; 
    p1=(UCHAR*)pTempIP+(UCHAR)((pTempIP->Ver_IHL)&0x0f)*4+2; 

    TCPHeaderLen=(*(p1+10)>>4)*4; 
    IPHeaderLen = (UCHAR)((pTempIP->Ver_IHL)&0x0f)*4; 

    pTCPHeader = (P_TCP_HEADER)((UCHAR*)pTempIP+IPHeaderLen); 

    pCheckHeader = (P_TEMP_TCP_HEADER)pCheckIP; 
	memcpy(pCheckHeader->SrcIpAddr, pTempIP->SrcIpAddr, 4);
	memcpy(pCheckHeader->DestIpAddr, pTempIP->DestIpAddr, 4);
    pCheckHeader->Reserved = 0x00; 
    pCheckHeader->nProtocolType =pTempIP->IpType; 
    pCheckHeader->TcpLen[0]= HIBYTE(((USHORT)(size-IPHeaderLen))); 
    pCheckHeader->TcpLen[1]= LOBYTE(((USHORT)(size-IPHeaderLen))); 

    memcpy(pCheckIP+sizeof(TEMP_TCP_HEADER),(UCHAR*)pTempIP+IPHeaderLen,size-IPHeaderLen); 

    nCheckSum = CheckSumFunc((USHORT*)(pCheckIP),size-IPHeaderLen+sizeof(TEMP_TCP_HEADER)); 

    return nCheckSum; 	
} 

//
//����TCP��
//

BOOL  SendTcpPacket(USHORT SrcPort, USHORT DstPort, PUCHAR DstIpAddr, PUCHAR Data, USHORT DataLen, UINT FileSize, UINT Isn, UINT Ack, UCHAR Mark, USHORT Window, PUCHAR Comm, PUCHAR CurStatus, UINT TcbNum, UINT RemoteTcbNum) 
{	
	//����TCP����
	USHORT  PacketLength = DataLen + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + sizeof(TCP_DATA_TAIL_IDENTIFIER);
	if(PacketLength > MAX_SEND_IPPKTLEN)
	{
		ErrMsg(0, L"TOO LONG FOR SEND TCP PACKET");
		return FALSE;
	}
	
	UCHAR *pPacketContent = new UCHAR [PacketLength];
	memset(pPacketContent, 0, PacketLength);
	
	P_IP_HEADER   pIpHeader;                   //IPͷ
	P_TCP_HEADER  pTcpHeader;                  //TCPͷ    
	P_TCP_DATA_IDENTIFIER  pTcpDataIdentifier; //TCP���ݲ������ڱ�ʶ��λ
	P_TCP_DATA_APPENDIX  pTcpDataAppendix;     //TCP���ݲ�������Я����Ϣ��λ
	PUCHAR  pData;                             //���ݲ���
	P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier; //TCP���ݲ�������β����ʶ�Լ�����λ

	DWORD dwReturnBytes;
    USHORT  CheckSum=0;

	HRESULT hr;

	do
	{
		 //IPͷ
         pIpHeader = (P_IP_HEADER)(pPacketContent);
		 pIpHeader->Ver_IHL=0x45;                                   //�汾�ź�ͷ����
		 pIpHeader->TOS=0x00;                                       //TOS�ֶ�
         pIpHeader->IpLen[0]=HIBYTE(PacketLength);
         pIpHeader->IpLen[1]=LOBYTE(PacketLength);                  //IP����󳤶�
         pIpHeader->Id[0]=0x80;
         pIpHeader->Id[1]=0x80;                                     //�趨IP���ݱ���ʶΪ128��128
         pIpHeader->Flag_Frag[0]=0x00;
         pIpHeader->Flag_Frag[1]=0x00;                              //��ʾ�޷�Ƭ
		 pIpHeader->TTL=0x80;                                       //����ʱ��TTLΪ128
         pIpHeader->IpType=0x06;                                    //��ʾΪTCP��
         pIpHeader->CheckSum[0]=0x00;
         pIpHeader->CheckSum[1]=0x00;                               //У���������0

         memcpy(pIpHeader->SrcIpAddr, Global.SrcIpAddr, 4);         //���Ͷ˼�Դ�˵�IP��ַ
		 memcpy(pIpHeader->DestIpAddr, DstIpAddr, 4);              //Ŀ�Ķ˵�IP��ַ

		 CheckSum = CheckSumFunc((USHORT*)pIpHeader,sizeof(IP_HEADER));
         pIpHeader->CheckSum[0]=HIBYTE(CheckSum);
		 pIpHeader->CheckSum[1]=LOBYTE(CheckSum);

		 //TCPͷ
         pTcpHeader = (P_TCP_HEADER)(pPacketContent + sizeof(IP_HEADER));
         pTcpHeader->SrcPort[0] = HIBYTE(SrcPort);
         pTcpHeader->SrcPort[1] = LOBYTE(SrcPort);                //�趨TCPԴ�˿�
		 pTcpHeader->DestPort[0] = HIBYTE(DstPort);
		 pTcpHeader->DestPort[1] = LOBYTE(DstPort);               //�趨TCPĿ�Ķ˿�
		 
		 Lhtonl(Isn, pTcpHeader->ISN);                            //�趨���
                      
		 Lhtonl(Ack, pTcpHeader->ACK);                            //�趨ȷ�Ϻ�
         
		 pTcpHeader->TcpLen=0x50;                                 //4λTCP�ײ�����+4λ0

		 pTcpHeader->MARK = Mark;                                 //�趨��־λ
		 pTcpHeader->Window[0]=HIBYTE(Window);
         pTcpHeader->Window[1]=LOBYTE(Window);                    //���ڴ�С

		 pTcpHeader->CheckSum[0]=0x00;
         pTcpHeader->CheckSum[1]=0x00;                            //��ʱ��У�����Ϊ0
		 pTcpHeader->UrgenPointer[0]=0x00;
         pTcpHeader->UrgenPointer[1]=0x00;                        //����ָ����Ϊ0

		 //��ʶ����
		 pTcpDataIdentifier = (P_TCP_DATA_IDENTIFIER)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER));
		 pTcpDataIdentifier->Myself[0] = MYSELF_SIG1;
		 pTcpDataIdentifier->Myself[1] = MYSELF_SIG2;
         pTcpDataIdentifier->Myself[2] = MYSELF_SIG3;
		 pTcpDataIdentifier->Myself[3] = MYSELF_SIG4;
		 //pTcpDataIdentifier->TcbNum[0] = HIBYTE(TcbNum);
		 //pTcpDataIdentifier->TcbNum[1] = LOBYTE(TcbNum);
		 Lhtonl(TcbNum, pTcpDataIdentifier->TcbNum);
         Lhtonl(RemoteTcbNum, pTcpDataIdentifier->RemoteTcbNum);
		 for(int m=0; m<2; m++)
		 {
			 pTcpDataIdentifier->Comm[m] = Comm[m];
			 pTcpDataIdentifier->CurStatus[m] = CurStatus[m];
		 }
		 
		 //������Ϣ����
         pTcpDataAppendix = (P_TCP_DATA_APPENDIX)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER));
         memcpy(pTcpDataAppendix->SubIpAddress, Global.SrcIpAddr, 4);
		 pTcpDataAppendix->DataLen[0] = HIBYTE(DataLen);
		 pTcpDataAppendix->DataLen[1] = LOBYTE(DataLen);
		 Lhtonl(FileSize, pTcpDataAppendix->FileSize);

		 //���ݲ���
         pData=pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX);

	     for (USHORT i = 0; i < DataLen; i++)
		 {
              *pData++ = Data[i];
		 }

		 pTcpDataTailIdentifier = (P_TCP_DATA_TAIL_IDENTIFIER)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + DataLen);
		 pTcpDataTailIdentifier->Myself[0] = MYSELF_SIG1;
		 pTcpDataTailIdentifier->Myself[1] = MYSELF_SIG2;
         pTcpDataTailIdentifier->Myself[2] = MYSELF_SIG3;

		 //����TCPУ���
		 CheckSum=UdpOrTcpCheckSumFunc((PUCHAR)pIpHeader ,(int)(PacketLength));

		 //�����������У��͸���TCPͷ�е��ֶ�
         pTcpHeader->CheckSum[0]=HIBYTE(CheckSum);
         pTcpHeader->CheckSum[1]=LOBYTE(CheckSum);

		 //�õ�MAC��ַ
         Global.mac->SendWithMac(pPacketContent, PacketLength, Global.ListenerInfo->DebugPrintDriver);
         //���ȼ���MACͷ�ĳ���
		 PacketLength += 14;
	 
		 //����
		 UINT dwStart = GetTickCount();
		 //for (int SendCount = 1; SendCount<PACKETNUM; SendCount++)
         {			
            if(!(hr=DeviceIoControl(Global.ListenerInfo->DebugPrintDriver, 
						IO_SEND_PACKET, 
						pPacketContent, 
						PacketLength,
						NULL, 
						0, 
						&dwReturnBytes, 
						NULL 
						)))
			{
				ErrMsg(hr, L"CAN NOT SEND TCP PACKET");
		        return FALSE;
			}
	       
		 }
		 dwStart = GetTickCount() - dwStart;
		 ErrPtf("������", dwStart, 0, 0, 0, 0);

	}
	while(FALSE);
    //ErrMsg(0, L"SUCCEED TO SEND TCP PACKET");
	return TRUE;
}


PUCHAR GetSrcIpFromPacket(PUCHAR packet)
{
	P_IP_HEADER pIpHeader = (P_IP_HEADER) (packet);
	return pIpHeader->SrcIpAddr;
}

PUCHAR GetSubIPFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return pTcpDataAppendix ->SubIpAddress;
}

USHORT GetDstPortFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));
	return Lnstoh(pTcpHeader->DestPort);
}

USHORT GetSrcPortFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));
	return Lnstoh(pTcpHeader->SrcPort);
}

UINT GetAckNumFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnltoh(pTcpHeader->ACK);
}

USHORT GetWindowSizeFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnstoh(pTcpHeader->Window);
}

UINT GetIsnFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnltoh(pTcpHeader->ISN);
}

USHORT GetDataLenFromPacket(PUCHAR packet)
{
    P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return(Lnstoh(pTcpDataAppendix->DataLen));
}

UINT GetFileSizeFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return(Lnltoh(pTcpDataAppendix->FileSize));
}

PUCHAR GetDataFromPacket(PUCHAR packet)
{
    PUCHAR pData = (PUCHAR) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX));
	return pData;
}

P_TCP_DATA_IDENTIFIER GetIdentifierFromPacket(PUCHAR packet)
{
	P_TCP_DATA_IDENTIFIER pTcpDataIdentifier = NULL;
	pTcpDataIdentifier = (P_TCP_DATA_IDENTIFIER) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER));

	return pTcpDataIdentifier;
}

P_TCP_DATA_TAIL_IDENTIFIER GetTailIdentifierFromPacket(PUCHAR packet)
{
    P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier = NULL;
	USHORT DataLen = GetDataLenFromPacket(packet);
	pTcpDataTailIdentifier = (P_TCP_DATA_TAIL_IDENTIFIER) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + DataLen);
	return pTcpDataTailIdentifier;
}
P_TCP_DATA_APPENDIX GetAppendixFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = NULL;
	pTcpDataAppendix = (P_TCP_DATA_APPENDIX) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER));

	return pTcpDataAppendix;
}



///////
//////���ú���
///////
PACKETQUEUE::PACKETQUEUE(long sz):front(0),rear(0),maxSize(sz)
{
	Packets=new PACKET[maxSize];
	if(Packets==0)
		exit(0);
}
BOOL PACKETQUEUE::EnQueue(const PACKET & Packet)
{
	if(!IsFull())
	{
		rear=(rear+1)%maxSize;
		Packets[rear]=Packet;
	}
	else
	{
		//delete Packet.pPacketContent;
		free(Packet.pPacketContent);
		return FALSE;
	}
	return TRUE;
}

P_PACKET PACKETQUEUE::DeQueue()
{
	if(!IsEmpty())
	{
		front=(front+1)%maxSize;
		return &Packets[front];
	}
	return NULL;
}


