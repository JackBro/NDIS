// Mac.h: interface for the Mac class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_)
#define AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000





#include <winioctl.h>
#include "winsvc.h"

//�����뷢����ص���
#define IO_SEND_PACKET_MAC			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IO_GET_SRCMAC_MAC           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x927, METHOD_BUFFERED, FILE_ANY_ACCESS)

# define MAC_ADDR_LEN               6            //��ѯ��MAC��ַ�ĳ��� 
# define OID_802_3_CURRENT_ADDRESS  128          //�����ѯ ��̫��/802_3 ��ԴMAC��ַ


# define MAX_SEND_ARPPKTLEN         60           //����ARP���ʱ����󳤶ȣ�������ͷ����
# define  _IP_MAC_LEN               0xFF         //IP��ַ��MAC��ַ�Ķ�Ӧ��ĳ���

class Mac  
{
public:
	Mac();
	virtual ~Mac();

    /////////////////////////////////////////
    //�Զ������
    //////////////////////////////////////////
private:
    //��̫����װ��ʽ
    typedef struct _ETHERNET_HEADER
	{
		unsigned char DestMac[6];
	    unsigned char SrcMac[6];
	    unsigned char EthType[2];
	}ETH_HEADER, * P_ETH_HEADER;

    //ARP��ͷ��ʽ
    typedef struct _ARP_HEADER
	{
	    unsigned char HardWareType[2];
	    unsigned char ProtocolType[2];
	    unsigned char HardWareAddrLen;
	    unsigned char ProtocolAddrLen;
	    unsigned char Operation[2];
	    unsigned char SrcMacAddr[6];
	    unsigned char SrcIpAddr[4];
	    unsigned char DestMacAddr[6];
	    unsigned char DestIpAddr[4];
	}ARP_HEADER,* P_ARP_HEADER;
    
    typedef struct _IP_MAC
	{
	    unsigned char IP[4];
	    unsigned char MAC[6];
	}IP_MAC;
    IP_MAC IP_MAC_STABLE[_IP_MAC_LEN];
    int CUR_IP_MAC_LEN;

    //��ǰĿ��IP��ַ������������ARP��
    unsigned char DestIpAddr[4];

    //ԴIP��ַ�������������ж��Ƿ�Ϊͬһ����
    unsigned char SrcIpAddr[4];

    //ԴMAC��ַ
    unsigned char SrcMacAddr[6];

    //��ǰĿ��MAC��ַ
    unsigned char DestMacAddr[6];

    //����MAC��ַ
    unsigned char GateWayMacAddr[6];

    //��������
    unsigned char SubNetMask[4];

    //����ָʾ�Ƿ��ѳɹ�����ARP����õ�Ŀ��MAC
    HANDLE   ArpMacEvent;   

    //��ѯ��Ϣ�ṹ
    typedef struct _SECLAB_QUERY_OID
	{
    //NDIS_OID        Oid;
	ULONG             Oid;
    UCHAR             Data[sizeof(ULONG)];
	} SECLAB_QUERY_OID, *PSECLAB_QUERY_OID;

public:

    //������̫����ͷ������ӿ�1
    BOOL  DecodeMac(unsigned char* &packet);

    //Ϊ��������̫����ͷ������ӿ�2
    BOOL  SendWithMac(unsigned char* &packet, int PacketLen, HANDLE DriverHandle);

    //��ʼ��MACģ��
    void  InitMac();

    //��ArpӦ������ҳ�Ŀ���ַ
    BOOL  GetMacFromArpPacket(unsigned char *packet);

    //�õ���������
    int   GetSubNetMask();

    //�õ�����MAC��ַ
    int   GetSrcMac(HANDLE DriverHandle);

    //�����ݰ��в��ұ��غ�Զ��IP��ַ
    void  GetSrcAndDestIpAddr(unsigned char *packet);

    //�жϵ�ǰͨ��˫���Ƿ���ͬһ������
    BOOL  BothInLocalNetwork();

    //�жϱ����Ƿ��Ѿ�����ʼ��
    BOOL IsEmpty(unsigned char *s, int len);

    //�ж�������ַ��Ϣ�Ƿ���ͬ
    BOOL IsSame(unsigned char *p, unsigned char *q, int len);

    //����Ӧ�����Ƿ��е�ǰͨ��˫���Ķ�Ӧ��Ϣ
    BOOL CheckRecord();

    //����Ӧ������뵱ǰͨ��˫����Ϣ
    void AddToRecord();

    //��ARP���Եõ�MAC��ַ
    BOOL SendArpToGetMac(HANDLE DriverHandle);

};




#endif // !defined(AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_)
