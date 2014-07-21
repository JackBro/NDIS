// Tcp.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A59__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "mmTimers.h"
# include <sys/stat.h>
/////////////////////////////////////////
//�Զ������
//////////////////////////////////////////


# define    TIME_RCV_DELAY       1000       //���ڶ�ʱ����������������ӳٷ���ȷ��
# define    RETRAN_COUNT         3          //�ش�����
# define    INIT_WIN_SIZE        1          //��ʼ����ʱ���ڴ�С
# define    INIT_RETRAN_TIME     9000       //��ʼ����ʱ�ش�ʱ��
# define    ALPHA                0.8        //���ڼ����ش�ʱ��Ĳ���
# define    PerPacketSize        800        //ÿ�����Ĵ�С

 
class MyTimers;                    //����MyTimers��

class CONNECTION;                  //����CONNECTION��

class TCBLIST;                     //����TCB ����ÿ��TCB��ʾһ������

//TCB,TCP���ư�,�����װ����
class TCB
{
	friend class TCBLIST;          //������Ԫ��
public:

	//�������Ա����
	TCB();
	TCB(UINT TcbNum);
	
	~TCB();
	TCB* NextTCB() {return link;}     //��ǰ�ڵ����һ���ڵ�
	void InsertAfterSelf(TCB*);       //�ڵ�ǰ�ڵ�����һ���½ڵ�
	TCB* RemoveAfterSelf();           //ɾ����ǰ�ڵ����һ���ڵ�
	
	//��״̬ͼת���йصĺ���
    void Dispatch(int InputType, PUCHAR packet);           //״̬ת�������غ���
	
	BOOL DealWithInput(int InputType, PUCHAR packet);      //���봦����
	BOOL DealWithOutput(int OutputType);                   //���������

	void Quit();                                           //�˳�
	
//private:
public:
	
	//������ƽṹ
	typedef struct _INPUT_STRUCT
	{
        int   InputType;                     //��������
		int   LastInputType;                 //�ϴε��������ͣ���������ͳ�ʱʱ���ط�
		FILE  *TransFile;                    //Ҫ���͵��ļ�ָ��
		char  SelfDirect[MAX_PATH];          //������ǰ·��
		char  RemoteDirect[MAX_PATH];        //�Է���ǰ·��
		char  ParaInfo[MAX_PATH];            //������Ϣ�����ļ���
		USHORT ParaLen;                      //������Ϣ����
		UINT  m_MaxSendSize;                 //Ҫ���͵��ļ�����
		UCHAR InputBuffer[PerPacketSize];    //���뻺����
		USHORT BufferSize;                   //���뻺������С
		CRITICAL_SECTION Cs_IBuffer;         //���ڱ������뻺�������ٽ�������
		HANDLE InputEvent;                   //�ȴ��¼������ڷ�������ʱ�ȴ�ȷ��
	} INPUT_STRUCT, *P_INPUT_STRUCT;

	//������ƽṹ
	typedef struct _OUTPUT_STRUCT
	{
        int    OutputType;             //�������
		FILE   *RcvFile;               //�����ļ�
		//PUCHAR OutputBuffer;         //���������
		//ULONG  BufferSize;           //�����������С
		//CRITICAL_SECTION Cs_OBuffer; //���ڱ���������������ٽ�������
	} OUTPUT_STRUCT, *P_OUTPUT_STRUCT;
	
	//������ؿ��Ʊ���
	typedef struct _RECEIVE_CONTROLER
	{
		UINT LastAckNum;              //���յ�ǰȷ�Ϻ�,��Щ�����ļ�ƫ����
		UINT ISN;                     //�������к� 
		UINT WindowSize;              //���մ��ڴ�С
		UINT TempWinSize;             //���ڼ����ѽ��յ���˳�����İ��ĸ���
		UINT DelayTime;               //�ӳٷ���ȷ��ʱ��
	}RECEIVE_CONTROLER, *P_RECEIVE_CONTROLER;

	//������ؿ��Ʊ���
    typedef struct _SEND_CONTROLER
	{
		UINT ISN;                     //���͵�ǰ���к�,��Щ�����ļ�ƫ����
		UINT CurAckNum;               //���͵�ǰȷ�Ϻ�
		UINT WindowSize;              //���ʹ��ڴ�С
		UINT ReTranTime;              //�����ش�ʱ��
	}SEND_CONTROLER, *P_SEND_CONTROLER;

    INPUT_STRUCT InputStru;           //�������
    OUTPUT_STRUCT OutputStru;         //�������
    RECEIVE_CONTROLER RecCtrler;      //���տ���
    SEND_CONTROLER SendCtrler;        //���Ϳ���

	CONNECTION*  m_Conn;              //��ָָ�룬ָ�򱾲������ڵ�����

	BOOL    m_bBlockAck;              //��ʶ�Ƿ���ֹACK����ĵ���
	BOOL    m_bBlockData;             //��ʶ�Ƿ���ֹDATA���ݵĵ���

	BOOL    m_bQuit;                  //�˳���־
	BOOL    m_bConnected;             //�����Ƿ�ɹ���ʶ
	BOOL    m_bContinueSend;          //�Ƿ�������ͱ�ʶ

	MyTimers *m_pMyStimer;            //���ڷ��ͳ�ʱ�Ķ�ʱ��
	MyTimers *m_pMyRtimer;            //���ڽ����ӳٵĶ�ʱ��

    HANDLE  m_CommEvent;              //�ȴ��¼������ں�FTP�������������ͨ��
	HANDLE  m_Comm_Data_Event;        //�ȴ��¼����������������ݲ�����ͬ��

	UCHAR  m_IpAddr[4];               //IP��ַ
	UCHAR  m_SubIpAddr[4];            //�ڲ�IP��ַ
	USHORT m_SrcPort;                 //���ض˿ں�
	USHORT m_DstPort;                 //Զ�̶˿ں�

	int   m_ReTranCount;              //�ش�����
	DWORD dwStart;                    //��ʱ��ʼ
	
    int  CurStatus;                    //��ǰ״̬

	BOOL m_bCallTime;                 //��ʱ��־
	BOOL m_bRcvDelay;                 //�����ӳٱ�־

	UINT m_CurrISN;                   //��ǰ���͵İ���ISN

	UINT m_TcbNum;                    //TCB��
	UINT m_RemoteTcbNum;              //�Է���TCB��
	TCB* link;                        //��һ���ڵ�ָ��
};

//TCB������
class TCBLIST
{
public:

	//�������Ա����
	TCBLIST(){Last = First = new TCB();}
	~TCBLIST();
	void MakeEmpty();                        //��������Ϊ�ձ�
	int Length() const;                      //��������ĳ���
	TCB* Find(UINT TcbNum);                  //����TCB��Ѱ��ָ����TCB�ڵ�
	void AppendList(TCB *add);               //���½ڵ�ӵ�������
	void RemoveNode(TCB *tcb);               //��ָ���ڵ�ɾ��
	
private:
	TCB *First, *Last;
};


//��CMMTimers��ļ̳�
class MyTimers : public CMMTimers
{
public:
	MyTimers(UINT x, TCB* tcb, BOOL IsSend);
	void timerProc();
protected:
	TCB *m_tcb;
	BOOL m_IsSend;
};


class CONNECTIONLIST;  //����CONNECTION����ÿ��CONNECTION��ʾһ���������������ӣ�ÿ�����Ӷ�Ӧһ��IP

class CONNECTION
{
	friend class CONNECTIONLIST;                //������Ԫ��
	
public:

	//�������Ա����
	CONNECTION();
	CONNECTION(UINT nIP, UINT nSubIP, USHORT SrcPort, USHORT DstPort);
	CONNECTION(PUCHAR puIP, PUCHAR puSubIP, USHORT SrcPort, USHORT DstPort);
	CONNECTION* NextConn() {return link;}      //��ǰ�ڵ����һ���ڵ�
	void InsertAfterSelf(CONNECTION*);         //�ڵ�ǰ�ڵ�����һ���½ڵ�
	CONNECTION* RemoveAfterSelf();             //ɾ����ǰ�ڵ����һ���ڵ�

//private:
public:
	
	CONNECTION *link;

	BOOL   m_Active;                            //��ʶ�������Ƿ��ڻ״̬

	//�������ԣ�����������ʶһ������

	UCHAR  m_IP[4];                             //�������IP,�Է�IP                     
    UCHAR  m_SubIP[4];                          //�����������IP,�Է�������IP
	USHORT m_SrcPort;                           //Դ�˿ں�
	USHORT m_DstPort;                           //Ŀ�Ķ˿ں�

	UINT   m_ConnRetranTime;                    //�����ӵ�����ʱ���ʼֵ
	    
	TCBLIST* TcbList;                           //TCB�б�

	//�������������������ʾ���������״̬�Լ�������Ϣ

	char   m_SelfCurDirect[MAX_PATH];           //������ǰ·��
	char   m_RemoteCurDirect[MAX_PATH];         //�Է���ǰ·��
	int  m_CurType;                             //������ĵ�ǰ״̬
	char m_pszAppendix[MAX_PATH];               //����������Ϣ
	USHORT m_SzLen;                             //������Ϣ����
	UINT m_FileSize;                            //���β����ļ���С
	UINT m_TcbNum;                              //����TCB��
	UINT m_RemoteTcbNum;                        //Զ��TCB��
	int  m_CurStatus;                           //��ǰTCB״̬  
	
	CRITICAL_SECTION Cs_ConPara1;               //���ڱ�������������ٽ�������
	CRITICAL_SECTION Cs_ConPara2;               //���ڱ�������������ٽ�������
};

//CONNECTION������
class CONNECTIONLIST
{
public:

	//�������Ա����
	CONNECTIONLIST(){Last = First = new CONNECTION();}
	~CONNECTIONLIST();
	void MakeEmpty();                                                 //��������Ϊ�ձ�
	int Length() const;                                               //��������ĳ���
	CONNECTION* FindByIPAndSubIP(PUCHAR puIP, PUCHAR puSubIP);        //����IP��SubIPѰ��ָ���ڵ�
	void AppendList(CONNECTION *add);                                 //���½ڵ�ӵ�������
	void RemoveNode(CONNECTION *del);                                 //��ָ���ڵ�ɾ��
 
private:
	CONNECTION *First, *Last;
};


#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

