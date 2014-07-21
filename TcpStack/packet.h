// packet.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A58__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A58__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



/////////////////////////////////////////
//�Զ������
//////////////////////////////////////////



///
//���հ�ģ������صı���
///

# include <winioctl.h>
# include "winsvc.h"
# include "commen.h"
# include <process.h>
//
//���հ���صı���
//
     

//����������Ϣ���������ݲ���
typedef struct _PACKET          
{
	unsigned char* pPacketContent;
	UINT   PacketLenth;
} PACKET, *P_PACKET;


//��C++����˼�빹��һ�����У�ʢ���������İ�
class PACKETQUEUE
{
public:
	PACKETQUEUE(long=1024);
	~PACKETQUEUE(){delete []Packets;}
	BOOL EnQueue(const PACKET & Packet);
	P_PACKET DeQueue();
	void MakeEmpty(){front=rear=0;}
	int IsEmpty() const {return front==rear;}
	int IsFull() const {return (rear+1)%maxSize == front;}
	int Length() const {return (rear-front+maxSize)%maxSize;}
private:
	int rear,front;
    P_PACKET Packets;
	int maxSize;
};	

#define MY_MTU                   (1024)      //������󳤶�
#define MAXQUEUELEN              (1024*8)    //��Ű��Ķ��е���󳤶�


//IP��ͷ��װ��ʽ
typedef struct _IP_HEADER
{
	unsigned char Ver_IHL;				//��ʱ��һ���ֽڴ洢�����4�汾�ź�4ͷ����
	//unsigned char Version;			//4�汾��
	//unsigned char IpHeaderLen;		//4ͷ����
	unsigned char TOS;					//8��������
	unsigned char IpLen[2];				//16�ܳ���
	unsigned char Id[2];				//16�����
	unsigned char Flag_Frag[2];			//��ʱ��2���ֽڴ洢�����3��Ƭ��־��13��Ƭƫ��
	//unsigned char IpFlag;				//3��Ƭ��־
	//unsigned char IpFragment[2];		//13��Ƭƫ��
	unsigned char TTL;					//8����ʱ��
	unsigned char IpType;				//8Э������
	unsigned char CheckSum[2];			//16У���
	unsigned char SrcIpAddr[4];			//32ԴIP��ַ
	unsigned char DestIpAddr[4];		//32Ŀ��IP��ַ
}IP_HEADER, * P_IP_HEADER;

//TCP��ͷ��װ��ʽ
typedef struct _TCP_HEADER
{
	unsigned char SrcPort[2];			//16λԶ�˿ں�
	unsigned char DestPort[2];			//16λĿ�Ķ˿ں�
	unsigned char ISN[4];		        //32λ���
	unsigned char ACK[4];			    //32λȷ�����
    unsigned char TcpLen;               //4λ�ײ�����+4λ����
	unsigned char MARK;                 //2λ����+6λ��־
	unsigned char Window[2];            //16λ���ڴ�С
	unsigned char CheckSum[2];          //16λУ���
	unsigned char UrgenPointer[2];      //16λ����ָ��
}TCP_HEADER, * P_TCP_HEADER;


# define  MYSELF_SIG1      128
# define  MYSELF_SIG2      151
# define  MYSELF_SIG3      23
# define  MYSELF_SIG4      79
	

//�Զ���TCP���ݲ��֣����ڱ�ʶ
typedef struct _TCP_DATA_IDENTIFIER
{
	unsigned char Myself[4];            //32λ���ڱ�ʶ�Լ���
	unsigned char TcbNum[4];            //32λ���ڱ�ʶTCB��
	unsigned char RemoteTcbNum[4];      //32λ���ڱ�ʶ�Է�TCB��
	unsigned char Comm[2];              //16λ���ڱ�ʶ����
	unsigned char CurStatus[2];         //16λ���ڱ�ʶ��ǰTCB״̬
}TCP_DATA_IDENTIFIER, *P_TCP_DATA_IDENTIFIER;

//�Զ���TCP���ݲ��ֵĸ����ֶΣ����ڴ�����Ϣ
typedef struct _TCP_DATA_APPENDIX
{
	unsigned char SubIpAddress[4];      //32λ���ڱ�ʶ�����ڲ�IP��ַ
	unsigned char DataLen[2];           //16λ����ָʾ���ݳ���
	unsigned char FileSize[4];          //32λ����ָʾ���ݵ��ļ��Ĵ�С
}TCP_DATA_APPENDIX, *P_TCP_DATA_APPENDIX;

//�Զ���TCP���ݲ��֣�����β������Լ���
typedef struct _TCP_DATA_TAIL_IDENTIFIER
{
	unsigned char Myself[3];            //24λ���ڱ���Լ���
}TCP_DATA_TAIL_IDENTIFIER, *P_TCP_DATA_TAIL_IDENTIFIER;

//TCPαͷ���ķ�װ��ʽ
typedef struct _TEMP_TCP_HEADER
{
	unsigned char SrcIpAddr[4];
	unsigned char DestIpAddr[4];
	unsigned char Reserved;
	unsigned char nProtocolType;
	unsigned char TcpLen[2];
}TEMP_TCP_HEADER, * P_TEMP_TCP_HEADER;

//����IO����
#define IO_RECEIVE_PACKET		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x925, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_SEND_PACKET			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS) 

///
//���Ͱ�ģ������صı���
///

# define MAX_SEND_IPPKTLEN          1024         //����IP���ʱ�İ�����󳤶ȣ�������������·����
# define PACKETNUM                  2048         //���ͷ������         


//
//����ģ�麯��
//

//�����߳�,�����
/*UINT*/VOID
ReceiveThreadFunction(void*);
//�������̣߳�������
/*UINT*/VOID
DecodeThreadFunction(void*);

//�����������
void DecodePacket(UCHAR* pPacketContent,UINT len);


//
//����ģ�麯��
//

//�õ�����IP��ַ,�����䱣�浽ȫ�ֱ�����
int GetSrcIpAddr();

//ת���������ֽ�˳��
USHORT Lhtons(USHORT hostshort);

//����IPУ���
USHORT CheckSumFunc(USHORT *pAddr,int len);

//����TCPУ���
USHORT UdpOrTcpCheckSumFunc(PUCHAR pIPBuf, int size);

//����TCP��
BOOL SendTcpPacket(USHORT SrcPort, USHORT DstPort, PUCHAR DstIpAddr, PUCHAR Data, USHORT DataLen, UINT FileSize, UINT Isn, UINT Ack, UCHAR Mark, USHORT Window, PUCHAR Comm, PUCHAR CurStatus, UINT TcbNum, UINT RemoteTcbNum);


//�Ӱ��еõ�SRC IP��Ϣ
PUCHAR GetSrcIpFromPacket(PUCHAR packet);

//�Ӱ��еõ�SubIP��Ϣ
PUCHAR GetSubIPFromPacket(PUCHAR packet);

//�Ӱ��еõ�Զ�̶˿���Ϣ
USHORT GetDstPortFromPacket(PUCHAR packet);

//�Ӱ��еõ����ض˿���Ϣ
USHORT GetSrcPortFromPacket(PUCHAR packet);

//�Ӱ��еõ�ACK NUM��Ϣ
UINT GetAckNumFromPacket(PUCHAR packet);

//�Ӱ��еõ����ڴ�С��Ϣ
USHORT GetWindowSizeFromPacket(PUCHAR packet);

//�Ӱ��еõ�ISN��Ϣ
UINT GetIsnFromPacket(PUCHAR packet);

//�Ӱ��еõ����ݳ�����Ϣ
USHORT GetDataLenFromPacket(PUCHAR packet);

//�Ӱ��еõ�������Ϣ
PUCHAR GetDataFromPacket(PUCHAR packet);

//�Ӱ��еõ��ļ���С��Ϣ
UINT GetFileSizeFromPacket(PUCHAR packet);

//�Ӱ��еõ����ݱ�ʶ��Ϣ
P_TCP_DATA_IDENTIFIER GetIdentifierFromPacket(PUCHAR packet);

//�Ӱ��еõ����ݱ�ʶ��Ϣ
P_TCP_DATA_TAIL_IDENTIFIER GetTailIdentifierFromPacket(PUCHAR packet);

//�Ӱ��еõ����ݸ�����Ϣ
P_TCP_DATA_APPENDIX GetAppendixFromPacket(PUCHAR packet);

#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

