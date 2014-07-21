// Tcp.cpp : Defines the class behaviors for the TCP Module
//

#include "stdafx.h"
#include "TcpStack.h"

# include "Tcp.h"
# include "Ftp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////
///////////////TCB��ʵ��///////////////////
///////////////////////////////////////////


TCB::TCB():link(NULL)
{
	//���ģ��ʼ��
	InputStru.InputType = 0;
	InputStru.LastInputType = 0;
	memset(InputStru.ParaInfo, 0, sizeof(InputStru.ParaInfo));
	InputStru.ParaLen = 0;
	memset(InputStru.SelfDirect, 0, sizeof(InputStru.SelfDirect));
	memset(InputStru.RemoteDirect, 0, sizeof(InputStru.RemoteDirect));
	InputStru.m_MaxSendSize = 0;
	memset(InputStru.InputBuffer, 0, PerPacketSize);
	InputStru.BufferSize = 0;
	InitializeCriticalSection(&InputStru.Cs_IBuffer); 
	InputStru.InputEvent = CreateEvent(NULL,TRUE,TRUE,NULL); //manual,set it

	//OutputStru.BufferSize = 0;
	//InitializeCriticalSection(&OutputStru.Cs_OBuffer);

    RecCtrler.LastAckNum = 0;
	RecCtrler.ISN = 0;
	RecCtrler.WindowSize = 0;
	RecCtrler.TempWinSize = 0;
	RecCtrler.DelayTime = TIME_RCV_DELAY;

    SendCtrler.ISN = 0;
	SendCtrler.CurAckNum = 0;
	SendCtrler.WindowSize = INIT_WIN_SIZE;
	SendCtrler.ReTranTime = INIT_RETRAN_TIME;

	m_Conn = NULL;

	m_bBlockAck = FALSE;
	m_bBlockData = FALSE;

	m_bQuit = FALSE;
	m_bConnected = FALSE;
	m_bContinueSend = FALSE;

	m_pMyStimer = new MyTimers(1, this, TRUE);
	m_pMyRtimer = new MyTimers(1, this, FALSE);

	m_CommEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it 
	m_Comm_Data_Event = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it

    memset(m_IpAddr, 0, sizeof(m_IpAddr));
	memset(m_SubIpAddr, 0, sizeof(m_SubIpAddr));
	m_SrcPort = 1050;
	m_DstPort = 80;

	m_ReTranCount = RETRAN_COUNT;             
	dwStart = 0;                   
	
    CurStatus = STATUS_CLOSED;

	m_bCallTime = TRUE;                 
	m_bRcvDelay = TRUE;                 

	m_CurrISN = 0;

	m_TcbNum = -1;
	m_RemoteTcbNum = -1;
}


TCB::TCB(UINT TcbNum):link(NULL)
{
	//���ģ��ʼ��
	InputStru.InputType = 0;
	InputStru.LastInputType = 0;
	memset(InputStru.ParaInfo, 0, sizeof(InputStru.ParaInfo));
	InputStru.ParaLen = 0;
	memset(InputStru.SelfDirect, 0, sizeof(InputStru.SelfDirect));
	memset(InputStru.RemoteDirect, 0, sizeof(InputStru.RemoteDirect));
	InputStru.m_MaxSendSize = 0;
	memset(InputStru.InputBuffer, 0, PerPacketSize);
	InputStru.BufferSize = 0;
	InitializeCriticalSection(&InputStru.Cs_IBuffer); 
	InputStru.InputEvent = CreateEvent(NULL,TRUE,TRUE,NULL); //manual,set it

	//OutputStru.BufferSize = 0;
	//InitializeCriticalSection(&OutputStru.Cs_OBuffer);

    RecCtrler.LastAckNum = 0;
	RecCtrler.ISN = 0;
	RecCtrler.WindowSize = 0;
	RecCtrler.TempWinSize = 0;
	RecCtrler.DelayTime = TIME_RCV_DELAY;

    SendCtrler.ISN = 0;
	SendCtrler.CurAckNum = 0;
	SendCtrler.WindowSize = INIT_WIN_SIZE;
	SendCtrler.ReTranTime = INIT_RETRAN_TIME;      
	
	m_Conn = NULL;		
	
	m_bBlockAck = FALSE;
	m_bBlockData = FALSE;

	m_bQuit = FALSE;
	m_bConnected = FALSE;
	m_bContinueSend = FALSE;

	m_pMyStimer = new MyTimers(1, this, TRUE);
	m_pMyRtimer = new MyTimers(1, this, FALSE);

	m_CommEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it 
	m_Comm_Data_Event = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it
  
	memset(m_IpAddr, 0, sizeof(m_IpAddr));
	memset(m_SubIpAddr, 0, sizeof(m_SubIpAddr));
	m_SrcPort = 1050;
	m_DstPort = 80;

	m_ReTranCount = RETRAN_COUNT;             
	dwStart = 0;                   
	
    CurStatus = STATUS_CLOSED;

	m_bCallTime = TRUE;                 
	m_bRcvDelay = TRUE;                 

	m_CurrISN = 0;

	m_TcbNum = TcbNum;
	m_RemoteTcbNum = -1;
}


TCB::~TCB()
{
	m_pMyRtimer->stopTimer();
	m_pMyStimer->stopTimer();
}
void TCB::InsertAfterSelf(TCB* tcb)
{
	tcb->link = link;
	link = tcb;
}

TCB* TCB::RemoveAfterSelf()
{
	TCB*temp = link;
	if(link == NULL) return NULL;
	link = temp->link;
	return temp;
}

BOOL TCB::DealWithInput(int InputType, PUCHAR packet)
{      
	if(m_bQuit)
		return FALSE;

	InputStru.InputType = InputType;

	switch(InputType)
	{
	    //������յ����������Ϣ��ֱ�ӵ�������������
		case CONNECT_SENDING:
			{
				//ͬ���������ͺ�ȴ���Ӧ��Ϣ
				if(m_bQuit)
					break;
                
				BOOL FirstTime = TRUE;

				for(;;)
				{
					ResetEvent(m_CommEvent);
				    //������Ϣ��֪ͨ�Է��Լ�Ҫ�����ļ�
				    Dispatch(CONNECT_SENDING, packet);
                    WaitForSingleObject(m_CommEvent, INFINITE);
				
				    m_Conn->m_Active = m_bConnected;

					if(m_Conn->m_Active)
					{
						if(FirstTime)
						{
							ErrMsg(0, L"���ӳɹ���");
							FirstTime = FALSE;
						}
					}
					else
					{
						ErrMsg(0, L"����ʧ�ܣ�");
					}

					Sleep(INIT_RETRAN_TIME * 2);
				}

				break;
			}

	    case CONNECT_F_COMMING:
			{
				Dispatch(InputType, packet);
				break;
			}

	    case CONNECT_T_COMMING:
			{
				Dispatch(InputType, packet);
				SetEvent(m_CommEvent);
				break;
			}
			

	    //������յ����������Ϣ��ֱ�ӵ�������������
		case DATA_FILE_SENDING:
		case DATA_FILE_COMMING:
		case DATA_ACK_SENDING:
		case DATA_ACK_COMMING:
			{
				Dispatch(InputType, packet);
				break;
			}


		//������յ��Է�������,Ҫ�����List
		case COMM_LIST_GET_COMMING:
			{
				break;
			}

        //������յ��Է�������,Ҫ����List
		case COMM_LIST_POST_COMMING:
			{
				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����List
		case COMM_LIST_GET_SENDING:
			{
				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����List
		case COMM_LIST_POST_SENDING:
			{
				break;
			}

		//������յ��Է�������,Ҫ�����Screen
		case COMM_SCREEN_GET_COMMING:
			{
				break;
			}

        //������յ��Է�������,Ҫ����Screen
		case COMM_SCREEN_POST_COMMING:
			{
				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����Screen
		case COMM_SCREEN_GET_SENDING:
			{
				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����Screen
		case COMM_SCREEN_POST_SENDING:
			{
				break;
			}

		//������յ��Է�������,Ҫ�󱾷�����File
		case COMM_FILE_GET_COMMING:
			{
				//�õ���ǰ��·��
				//��������£�m_Conn->m_SelfCurDirectΪ���ص�ǰ·����InputStru.ParaInfoΪҪ���յ��ļ���
				//�õ��ļ�ȫ����������
				strcat(strcat(InputStru.SelfDirect, "\\"), InputStru.ParaInfo);
				OutputStru.RcvFile = fopen(InputStru.SelfDirect,"wb");
				if(OutputStru.RcvFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE RCV FILE");
					break;
				}
				
				//�ж�Ҫ���յ��ļ������ļ������Ⱥ��ļ���С����Ϣ�Ƿ��ѳɹ���FTPģ���и�ֵ��TCB����
                if(InputStru.ParaLen == 0 && InputStru.m_MaxSendSize == 0)
					break;
       
				//�������������̣����ͻ�Ӧ����Ϊ�˱��շ�����
				Dispatch(InputType, packet);
				
				//�ȴ�һ��ʱ��
				Sleep(SendCtrler.ReTranTime * 2);

				//�����ڵ������̵�״̬��ΪDATA_ESTABLISHED
							
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				CurStatus = STATUS_DATA_ESTABLISHED;
				//�ȴ����չ������
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(OutputStru.RcvFile);
				break;
			}

        //������յ��Է�������,Ҫ�󱾷�����File
		case COMM_FILE_POST_COMMING:
			{
				//�ж��ļ������ļ������ȡ��ļ���С�Ƿ��ѳɹ���ֵ��TCB������
				if(InputStru.ParaLen == 0)
					break;

				//�õ���ǰ·�����Ӷ��õ��ļ�ȫ·����Ȼ����ļ�
				//��������£��ļ�ȫ���ڶԷ��ṩ�İ��е�ParaInfo��
				strcpy(InputStru.SelfDirect, InputStru.ParaInfo);
				InputStru.TransFile = fopen(InputStru.SelfDirect, "rb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//�õ��ļ���С������ֵ��m_MaxSendSize
				struct stat st;
                if(stat(InputStru.SelfDirect, &st) == 0)
					InputStru.m_MaxSendSize = st.st_size;

				//����Ӧ��Ϣ
				Dispatch(COMM_FILE_POST_COMMING, packet);
			
				//�ȴ�һ��ʱ��
				Sleep(SendCtrler.ReTranTime * 2);

				//���Է��������ˣ���״̬ת��ΪDATA_ESTABLISHED
				CurStatus = STATUS_DATA_ESTABLISHED;
				
				//��ʽ����
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				Dispatch(DATA_FILE_SENDING, packet);
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);
				
				fclose(InputStru.TransFile);
				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����File
		case COMM_FILE_GET_SENDING:
			{
				//�ж��ļ������ļ������ȡ��ļ���С�Ƿ��ѳɹ���ֵ��TCB������
				if(InputStru.ParaLen == 0 || InputStru.m_MaxSendSize == 0)
					break;

				//�õ���ǰ·�����Ӷ��õ��ļ�ȫ·����Ȼ����ļ�
				//��������£�SelfDirect�д�ű����ļ�ȫ·����ParaInfo��ΪҪ�Է����յ��ļ���

				InputStru.TransFile = fopen(InputStru.SelfDirect, "rb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//ͬ���������ͺ�ȴ���Ӧ��Ϣ
				if(m_bQuit)
					break;

				ResetEvent(m_CommEvent);
				//������Ϣ��֪ͨ�Է��Լ�Ҫ�����ļ�
				Dispatch(COMM_FILE_GET_SENDING, packet);
                WaitForSingleObject(m_CommEvent, INFINITE);

				//ACK���ˣ���ʱ���Է��������ˣ���״̬ת��ΪDATA_ESTABLISHED������ֹACK����
				m_bBlockAck = TRUE;
				CurStatus = STATUS_DATA_ESTABLISHED;

				//�ȴ�һ��ʱ�䣬�����շ���Ϊ���Խ�������ʱ�ٷ�����
				Sleep(SendCtrler.ReTranTime * 2);
				
				//��ʽ����
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				Dispatch(DATA_FILE_SENDING, packet);
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(InputStru.TransFile);
				
            	ErrMsg(0, L"�����ļ�����ɣ�");

				break;
			}

		//����Ƿ���������Է���Ҫ��Է�����File
		case COMM_FILE_POST_SENDING:
			{
				//�ж��ļ������ļ��������Ƿ��ѳɹ���ֵ��TCB������
				if(InputStru.ParaLen == 0)
					break;

				//�õ���ǰ·�����Ӷ��õ��ļ�ȫ·����Ȼ����ļ�
				//��������£�RemoteDirectΪҪ��Է����͵�ȫ·����InputStru.ParaInfoҲҪ���Ҫ�Է����͵��ļ���ȫ·��
                //SelfDirect���Ǳ��صĵ�ǰ·��
				strcat(InputStru.SelfDirect, InputStru.ParaInfo);
				strcpy(InputStru.ParaInfo, InputStru.RemoteDirect);
				InputStru.TransFile = fopen(InputStru.SelfDirect, "wb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//ͬ���������ͺ�ȴ���Ӧ��Ϣ
				if(m_bQuit)
					break;

				ResetEvent(m_CommEvent);
				//������Ϣ��֪ͨ�Է��Լ�Ҫ�����ļ�
				Dispatch(COMM_FILE_POST_SENDING, packet);
                WaitForSingleObject(m_CommEvent, INFINITE);

				//ACK���ˣ���ʱ���Խ��������ˣ���״̬ת��ΪDATA_ESTABLISHED������ֹACK����
				m_bBlockAck = TRUE;

				//�ȴ�һ��ʱ�䣬�����ͷ���Ϊ���Է�������ʱ�ٽ�������
				//Sleep(SendCtrler.ReTranTime * 2);
				
				//��ʽ����
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				CurStatus = STATUS_DATA_ESTABLISHED;
				//�ȴ����չ������
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(OutputStru.RcvFile);
				break;
			}
        
	    //����յ���Ӧ��Ϣ
		case COMM_ACK_COMMING:
			{
				Dispatch(COMM_ACK_COMMING, packet);
				break;
			}
			
		//����յ���ʱ��Ϣ
		case TIME_OVER:
			{
				Dispatch(TIME_OVER, packet);
				break;
			}

		//����յ������ر���Ϣ
		case COMM_CLOSE_COMMING:
			{
				Dispatch(COMM_CLOSE_COMMING, packet);
				break;
			}

		//����յ������ر���Ϣ
		case COMM_CLOSE_SENDING:
			{
				Dispatch(COMM_CLOSE_SENDING, packet);
				break;
			}

		default:
			break;
		}
		
	return TRUE;
}


BOOL TCB::DealWithOutput(int OutputType)
{
	//���巢�Ͱ�ʱ��Ҫ�����ı���
	PUCHAR Data = NULL;
	USHORT DataLen = 0;
	UINT   FileSize = 0;
	UINT   Isn = 0x3450;
	UINT   Ack = 0x1300;
	UCHAR  Mark = 0;            //ACK 0x10 , SYN 0x02 , RST 0x04
	USHORT Window = 0;
    UCHAR  Comm[2] = {0,0};
	UCHAR  CurTcbStatus[2] = {0,0};
	UINT   TcbNum = 0;
	UINT   RemoteTcbNum = 0;

	PUCHAR  DstIp = m_IpAddr;
	USHORT  DstPort = m_DstPort;
	USHORT  SrcPort = m_SrcPort;
				
	TcbNum = m_TcbNum;
	RemoteTcbNum = m_RemoteTcbNum;
    FileSize = InputStru.m_MaxSendSize;
	CurTcbStatus[0] = HIBYTE(CurStatus);
	CurTcbStatus[1] = LOBYTE(CurStatus);

	switch(OutputType)
	{
	case CONNECT_SENDING:
		{
			//��������ֵ
			Mark = 0x02;
			Comm[0] = HIBYTE(CONNECT_SENDING);
			Comm[1] = LOBYTE(CONNECT_SENDING);

			RemoteTcbNum = -1;

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND CONNECT_SENDING");

			break;
		}

	case CONNECT_F_COMMING:
		{
			//��������ֵ
			Mark = 0x02;
			Comm[0] = HIBYTE(CONNECT_F_COMMING);
			Comm[1] = LOBYTE(CONNECT_F_COMMING);
			
			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND CONNECT_F_COMMING");

			break;
		}

	case COMM_CLOSE_SENDING:
		{
			//��������ֵ
			Mark = 0x04;
			Comm[0] = HIBYTE(COMM_CLOSE_SENDING);
			Comm[1] = LOBYTE(COMM_CLOSE_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_CLOSE_SENDING");

			break;
		}

	case COMM_LIST_GET_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_LIST_GET_SENDING);
			Comm[1] = LOBYTE(COMM_LIST_GET_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_LIST_GET_SENDING");

			break;
		}

	case COMM_LIST_POST_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_LIST_POST_SENDING);
			Comm[1] = LOBYTE(COMM_LIST_POST_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_LIST_POST_SENDING");

			break;
		}

	case COMM_SCREEN_GET_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_SCREEN_GET_SENDING);
			Comm[1] = LOBYTE(COMM_SCREEN_GET_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_SCREEN_GET_SENDING");

			break;
		}

	case COMM_SCREEN_POST_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_SCREEN_POST_SENDING);
			Comm[1] = LOBYTE(COMM_SCREEN_POST_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_SCREEN_POST_SENDING");

			break;
		}

	case COMM_FILE_GET_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_FILE_GET_SENDING);
			Comm[1] = LOBYTE(COMM_FILE_GET_SENDING);
 
			DataLen = InputStru.ParaLen;
			Data = PchTOUch(InputStru.ParaInfo, DataLen);
		
			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_FILE_GET_SENDING");

			break;
		}

	case COMM_FILE_POST_SENDING:
		{
			//��������ֵ
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_FILE_POST_SENDING);
			Comm[1] = LOBYTE(COMM_FILE_POST_SENDING);

			DataLen = InputStru.ParaLen;
			Data = PchTOUch(InputStru.ParaInfo, DataLen);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_FILE_POST_SENDING");

			break;
		}

	case COMM_ACK_SENDING:
		{
			//��������ֵ
			Mark = 0x10;
			Comm[0] = HIBYTE(COMM_ACK_SENDING);
			Comm[1] = LOBYTE(COMM_ACK_SENDING);

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_ACK_SENDING");

			break;
		}
		
	case DATA_FILE_SENDING:
		{
            //��������ֵ
			Data = InputStru.InputBuffer;
			DataLen = InputStru.BufferSize;
			Isn = SendCtrler.ISN;
			Ack = SendCtrler.ISN + 1;
			Mark = 0x00;
			Comm[0] = HIBYTE(DATA_FILE_SENDING);
			Comm[1] = LOBYTE(DATA_FILE_SENDING);
			Window = SendCtrler.WindowSize;

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND DATA_FILE_SENDING");

			break;
		}

	case DATA_ACK_SENDING:
		{
			//��������ֵ
			Mark = 0x10;
			Comm[0] = HIBYTE(DATA_ACK_SENDING);
			Comm[1] = LOBYTE(DATA_ACK_SENDING);
			Window = RecCtrler.WindowSize;
			Ack = RecCtrler.LastAckNum;

			//���Ͱ�
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND DATA_ACK_SENDING");

			break;
		}

	case APP_ERR_SENDING:
		{
			//��Ӧ�ó�����Ϣ���������
			ErrMsg(0, L"THE OPERATION HAS CANCELED");
			break;
		}

	default:break;
	}
	return TRUE;
}

void TCB::Quit()
{
	//��ֹ��һ�����գ���ֹ��һ����������
	m_bQuit = TRUE;

	//���Ͷ���RST�Ͷ�Ӧ�ó���Ĵ��󱨸�
	for(int i=0; i<3; i++)
		DealWithOutput(COMM_CLOSE_SENDING);
    DealWithOutput(APP_ERR_SENDING);

	//Ϊ�����˳�����ϰ�
	SetEvent(m_CommEvent);
	SetEvent(m_Comm_Data_Event);
	SetEvent(InputStru.InputEvent);

	Sleep(SendCtrler.ReTranTime * 2);

	SetEvent(m_CommEvent);
	SetEvent(m_Comm_Data_Event);
	SetEvent(InputStru.InputEvent);

}

void TCB::Dispatch(int InputType, PUCHAR packet)
{
	if(m_bQuit)
		return;

	switch(CurStatus)
	{
	//
	//�����ǰ״̬Ϊ�ر�
	//
	case STATUS_CLOSED:
		{
			switch(InputType)
			{
			case CONNECT_SENDING:
				{
					if(!DealWithOutput(CONNECT_SENDING))
						break;
										
					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
				    //���´���һ���µĶ�ʱ��
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);
				
					dwStart = GetTickCount();

				    break;
				}

			case CONNECT_F_COMMING:          //SEND REQUEST
				{
					DealWithOutput(CONNECT_F_COMMING);
				    break;
				}

		    case CONNECT_T_COMMING:          //ACK
				{
					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
						
					m_ReTranCount = RETRAN_COUNT;
					
					if(m_bConnected == FALSE)
					{
						SendCtrler.ReTranTime = GetTickCount() - dwStart;
						if(SendCtrler.ReTranTime < TIME_RCV_DELAY)
							SendCtrler.ReTranTime += TIME_RCV_DELAY;
				
						m_Conn->m_ConnRetranTime = SendCtrler.ReTranTime;
					}

					m_bConnected = TRUE;

				    break;
				}

		    case TIME_OVER:
				{				
					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					if(m_ReTranCount-- <= 0)
					{
						m_bConnected = FALSE;
						SetEvent(m_CommEvent);
						break;
					}

					dwStart = GetTickCount();

					if(!DealWithOutput(CONNECT_SENDING))
						break;

				    //���´���һ���µĶ�ʱ��
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);
					
				    break;
				}

			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

			default:break;	
			}

			break;
		}

	//
	//�����ǰ״̬Ϊ�ѽ�����������
	//
	case STATUS_COMMAND_ESTABLISHED:
		{
			switch(InputType)
			{
			//������յ��Է�������,Ҫ�����List
			case COMM_LIST_GET_COMMING:
				{
					break;
				}

            //������յ��Է�������,Ҫ����List
			case COMM_LIST_POST_COMMING:
				{
					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����List
			case COMM_LIST_GET_SENDING:
				{
					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����List
			case COMM_LIST_POST_SENDING:
				{
					break;
				}
			//������յ��Է�������,Ҫ�����Screen
			case COMM_SCREEN_GET_COMMING:
				{
					break;
				}

            //������յ��Է�������,Ҫ����Screen
			case COMM_SCREEN_POST_COMMING:
				{
					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����Screen
			case COMM_SCREEN_GET_SENDING:
				{
					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����Screen
			case COMM_SCREEN_POST_SENDING:
				{
					break;
				}

			//������յ��Է�������,Ҫ�󱾷�����File
			case COMM_FILE_GET_COMMING:
				{
					//����������ACK��Ӧ
					DealWithOutput(COMM_ACK_SENDING);
					DealWithOutput(COMM_ACK_SENDING);
					
					break;
				}

            //������յ��Է�������,Ҫ�󱾷�����File
			case COMM_FILE_POST_COMMING:
				{
					//����������ACK��Ӧ
					DealWithOutput(COMM_ACK_SENDING);
					DealWithOutput(COMM_ACK_SENDING);

					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����File
			case COMM_FILE_GET_SENDING:
				{
					//������Ϣ��֪ͨ�Է�Ҫ������ļ�
					if(!DealWithOutput(COMM_FILE_GET_SENDING))
					{
						ErrMsg(GetLastError(), L"CAN NOT SEND COMM_FILE_GET_SENDING");
						break;
					}

					InputStru.LastInputType = COMM_FILE_GET_SENDING;
					
					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
				    //���´���һ���µĶ�ʱ��
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);

					break;
				}

			//����Ƿ���������Է���Ҫ��Է�����File
			case COMM_FILE_POST_SENDING:
				{
					break;
				}
        
		    //����յ���Ӧ��Ϣ
			case COMM_ACK_COMMING:
				{
				    //�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					//����ʱ��ȫ�ּ�������
					m_ReTranCount = RETRAN_COUNT;

					//���ȴ��¼�����
					SetEvent(m_CommEvent);
					
					break;
				}
			
			//����յ���ʱ��Ϣ
			case TIME_OVER:
				{
					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					//�ж��Ƿ񳬹�������ط�����
					if( m_ReTranCount -- <= 0)
					{
					    Quit();
						break;
					}

					//������Ϣ��֪ͨ�Է�Ҫ������ļ�
					if(!DealWithOutput(InputStru.LastInputType))
					{
						ErrMsg(GetLastError(), L"CAN NOT SEND COMM_FILE_GET_SENDING");
						break;
					}

				    //���´���һ���µĶ�ʱ��
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime * (RETRAN_COUNT - m_ReTranCount + 1), TRUE);
					
					break;
				}

			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

			//��������յ�������Ϣ����RSTȡ��Զ��TCB
			case DATA_FILE_COMMING:
				{
					for(int i=0; i<3; i++)
						DealWithOutput(COMM_CLOSE_SENDING);
					break;
				}

			default:
				break;
			}
			break;
		}
	//
	//�����ǰ״̬Ϊ�ѽ�����������
	//
	case STATUS_DATA_ESTABLISHED:
		{
			switch(InputType)
			{
			//��������
			case DATA_FILE_SENDING:
				{				
	                while((SendCtrler.ISN < InputStru.m_MaxSendSize || !m_bContinueSend) && !m_bQuit )
					{
 						ResetEvent(InputStru.InputEvent);
						
					    EnterCriticalSection(&InputStru.Cs_IBuffer);

						m_bCallTime = TRUE;
						
				        while(SendCtrler.ISN < InputStru.m_MaxSendSize && SendCtrler.ISN / PerPacketSize < SendCtrler.CurAckNum / PerPacketSize + SendCtrler.WindowSize)
						{
							if(m_bQuit)
								break;
							if(m_bCallTime)
							{				            
					            //�����˶���ʱ��
				                if(m_pMyStimer->getTimerId() != 0)
									m_pMyStimer->stopTimer();
				                //���´���һ���µĶ�ʱ��
 				                m_pMyStimer->startTimer(SendCtrler.ReTranTime + TIME_RCV_DELAY, TRUE);

								if(m_CurrISN <= SendCtrler.ISN)
								{
									dwStart = GetTickCount();
								    m_CurrISN = SendCtrler.ISN;
								}
							
							    m_bCallTime = FALSE;
							}
					        //���������

							ErrPtf("��������", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

						    fseek(InputStru.TransFile, SendCtrler.ISN, 0);
						    fread(InputStru.InputBuffer,
								sizeof(UCHAR),
								InputStru.BufferSize = (PerPacketSize) > (InputStru.m_MaxSendSize - SendCtrler.ISN) ? (InputStru.m_MaxSendSize - SendCtrler.ISN) : (PerPacketSize),
							    InputStru.TransFile
								);
//							DWORD ddwStart = GetTickCount();
					        if(!DealWithOutput(DATA_FILE_SENDING))
								break;
//							ddwStart = GetTickCount() - ddwStart;
//						    ErrPtf("�߼�����", ddwStart, 0, 0, 0, 0);
					        SendCtrler.ISN = ftell(InputStru.TransFile);
						}
				    
					    LeaveCriticalSection(&InputStru.Cs_IBuffer);
										
 					    WaitForSingleObject(InputStru.InputEvent, INFINITE);
					}

					break;
				}
				
			//�յ�ȷ�ϣ�����Է��͵�
		    case DATA_ACK_COMMING:
				{
					UINT AckNum = GetAckNumFromPacket(packet);
										   
					EnterCriticalSection(&InputStru.Cs_IBuffer);

		            if(AckNum > SendCtrler.CurAckNum)
					{
						if(AckNum > m_CurrISN)
						{
							//���ټ���ͬһ���ε�ʱ��
							m_CurrISN = SendCtrler.ISN;
 						    m_bCallTime = TRUE;
							
						    //����RRT
						    DWORD TempRrt = GetTickCount() - dwStart;
				            SendCtrler.ReTranTime = (DWORD)(ALPHA * SendCtrler.ReTranTime) + (DWORD)((1 - ALPHA) * TempRrt);
						}
				         
						//�����˶���ʱ��
				        if(m_pMyStimer->getTimerId() != 0)
							m_pMyStimer->stopTimer();
						    
						//�����ش�����
					    m_ReTranCount = RETRAN_COUNT;	

						//�õ����ڴ�С,Ӧ�ò�С��1������15000����ÿ��10M
						
						ErrPtf("��Ӧ����", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

						SendCtrler.WindowSize = (SendCtrler.WindowSize + GetWindowSizeFromPacket(packet)) / 2;
				        if(SendCtrler.WindowSize == 0)
							SendCtrler.WindowSize = 1;
						if(SendCtrler.WindowSize >= 15000)
							SendCtrler.WindowSize = 15000;

						//����ISN
				        SendCtrler.CurAckNum = AckNum;
				        //��������
					    SetEvent(InputStru.InputEvent);	

						if(SendCtrler.CurAckNum >= InputStru.m_MaxSendSize)
						{
							m_bContinueSend = TRUE;
							SetEvent(m_Comm_Data_Event);
						}
					}
										  
					LeaveCriticalSection(&InputStru.Cs_IBuffer);

			        break;
				}

			//��������
		    case DATA_FILE_COMMING:
				{
					//�õ����Ͷ˴��ڴ�С
				    RecCtrler.WindowSize = GetWindowSizeFromPacket(packet);
				    if(m_bRcvDelay)
					{
						m_bRcvDelay = FALSE;

						RecCtrler.TempWinSize = 0;
					
					    //�����˶���ʱ��
				        if(m_pMyRtimer->getTimerId() != 0)
							m_pMyRtimer->stopTimer();
				        //���´���һ���µĶ�ʱ��
				        m_pMyRtimer->startTimer(TIME_RCV_DELAY, TRUE);

					}
				    //�����ӳٷ���ȷ�ϵ�ʱ��
				    //RecCtrler.DelayTime = GetWindowSizeFromPacket(packet);
                
				    RecCtrler.ISN = GetIsnFromPacket(packet);
			     	UINT DataLen = GetDataLenFromPacket(packet);

					if(RecCtrler.ISN > InputStru.m_MaxSendSize || RecCtrler.ISN < RecCtrler.LastAckNum)
						break;
		
				    //д���ļ�
				    fseek(OutputStru.RcvFile, RecCtrler.ISN, 0);
				    fwrite(GetDataFromPacket(packet), 
						sizeof(UCHAR),
					    DataLen,
					    OutputStru.RcvFile
						);
				   
					if(RecCtrler.LastAckNum == RecCtrler.ISN)
					{
						RecCtrler.TempWinSize ++;
					    RecCtrler.LastAckNum += DataLen;
					}
		
					if(RecCtrler.LastAckNum >= InputStru.m_MaxSendSize)
					{
						//˯һ��ʱ�䣬�ý���������ʱ�䷢��Ӧ��Ϣ
						m_bBlockData = TRUE;
						Sleep(RecCtrler.DelayTime *2);
						//������ȴ��¼���λ
						SetEvent(m_Comm_Data_Event);
					}
				     break;
				}
				
			//��ʱ��������Է��͵�
		    case TIME_OVER:
				{
					
					ErrPtf("��ʱ����", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

					//�����˶���ʱ��
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
					
					UINT CurAck = SendCtrler.CurAckNum;
					
					if(m_ReTranCount -- > 0)
					{
						//�ش�,��������뷢�����ƣ�ֻ��ֻ�ش�һ����
					    EnterCriticalSection(&InputStru.Cs_IBuffer);
					   	
						//�����Ӧ��Ȼû���Ļ�
// 						if(CurAck == SendCtrler.CurAckNum)
						{
							//���������
					        SendCtrler.ISN = SendCtrler.CurAckNum;
			
				            fseek(InputStru.TransFile, SendCtrler.ISN, 0);
				            fread(InputStru.InputBuffer,
								sizeof(UCHAR),
							    InputStru.BufferSize = (PerPacketSize) > (InputStru.m_MaxSendSize-SendCtrler.ISN) ? (InputStru.m_MaxSendSize-SendCtrler.ISN) : (PerPacketSize),
						        InputStru.TransFile
							    );
					        if(!DealWithOutput(DATA_FILE_SENDING))
								break;
					        SendCtrler.ISN = ftell(InputStru.TransFile);
				        
						    //���´���һ���µĶ�ʱ��
						    //�������ⳬʱ����
				            m_pMyStimer->startTimer(SendCtrler.ReTranTime * (RETRAN_COUNT - m_ReTranCount + 1) + TIME_RCV_DELAY, TRUE);
						}
						
					    LeaveCriticalSection(&InputStru.Cs_IBuffer);
					}
				    //����泬ʱ�ˣ�֤��������
				    else
					{
						//��Ӧ�ò㷢������Ϣ���ԶԷ���RST
					    DealWithOutput(COMM_CLOSE_SENDING);
					    DealWithOutput(APP_ERR_SENDING);
					}
				    break;
				}

						
			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

		   default:break;

		}


		break;
		}
								

	//
	//����
	//
	default:break;
	}


	return ;
}
///////////////////////////////////////////
//////////////TCB��ʵ�ֽ���////////////////
///////////////////////////////////////////





///////////////////////////////////////////
///////////////TCBLIST��ʵ��///////////////
///////////////////////////////////////////

TCBLIST::~TCBLIST(){}
	
void TCBLIST::MakeEmpty()
{
	TCB *temp;
	while(First->link != NULL)
	{
		temp = First->link;
		First->link = temp->link;
		delete temp;
	}	
	Last = First;
}
	
int TCBLIST::Length() const
{
	TCB *temp = First->link;
	int count = 0;
	while(temp != NULL)
	{
		temp = temp->link;
		count++;
	}
	return count;
}
	
	
TCB* TCBLIST::Find(UINT TcbNum)
{
	TCB *temp = First->link;
	while(temp != NULL && temp->m_TcbNum != TcbNum)
		temp = temp->link;
	return temp;
}
	
void TCBLIST::AppendList(TCB *add)
{
	Last->link = add;
	add->link = NULL;
	Last = add;
}

void TCBLIST::RemoveNode(TCB *tcb)
{
	TCB *temp = First->link;
	TCB *pretemp = First;
	while(temp != NULL && temp != tcb)
	{
		pretemp = temp;
		temp = temp->link;
	}
	if(temp != NULL)
	{
		if(temp == Last)
			Last = pretemp;
		pretemp->RemoveAfterSelf(); 
		delete temp;
	}
}
///////////////////////////////////////////
///////////////TCBLIST��ʵ�ֽ���///////////
///////////////////////////////////////////




//��CMMTimers��ļ̳�
MyTimers::MyTimers(UINT x, TCB* tcb, BOOL IsSend) : CMMTimers(x)
{
	m_tcb = tcb;
	m_IsSend = IsSend;
}

void MyTimers::timerProc()
{
//	AfxMessageBox("TIME OVER");
	//���ͳ�ʱ��Ϣ
	if(m_IsSend)
		m_tcb->DealWithInput(TIME_OVER, NULL);
	else
	{
		if(m_tcb->RecCtrler.WindowSize <= 5000)
			m_tcb->RecCtrler.WindowSize = (m_tcb->RecCtrler.TempWinSize >= m_tcb->RecCtrler.WindowSize ? m_tcb->RecCtrler.WindowSize * 3 : m_tcb->RecCtrler.TempWinSize);
		else
            m_tcb->RecCtrler.WindowSize = m_tcb->RecCtrler.TempWinSize;
		
		m_tcb->DealWithOutput(DATA_ACK_SENDING);
	    m_tcb->m_bRcvDelay = TRUE;
	}
		
}




///////////////////////////////////////////
//////////////CONNECTION��ʵ��/////////////
///////////////////////////////////////////

CONNECTION::CONNECTION():link(NULL)
{
	memset(m_IP, 0, 4);
	memset(m_SubIP, 0, 4);
	m_SrcPort = 1080;
	m_DstPort = 80;
	m_Active = FALSE;
	
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);

	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

CONNECTION::CONNECTION(UINT nIP, UINT nSubIP, USHORT SrcPort, USHORT DstPort):link(NULL)
{
	Lhtonl(nIP, m_IP);
	Lhtonl(nSubIP, m_SubIP);
	m_SrcPort = SrcPort;
	m_DstPort = DstPort;
	m_Active = FALSE;
	
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);

	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

CONNECTION::CONNECTION(PUCHAR puIP, PUCHAR puSubIP, USHORT SrcPort, USHORT DstPort)
{
    memcpy(m_IP, puIP, 4);
	memcpy(m_SubIP, puSubIP, 4);
	m_SrcPort = SrcPort;
	m_DstPort = DstPort;
	m_Active = FALSE;
		
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);
	
	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

void CONNECTION::InsertAfterSelf(CONNECTION* conn)
{
	conn->link = link;
	link = conn;
}

CONNECTION* CONNECTION::RemoveAfterSelf()
{
	CONNECTION* temp = link;
	if(link == NULL) return NULL;
	link = temp->link;
	return temp;
}





///////////////////////////////////////////
//////////////CONNECTION��ʵ�ֽ���/////////
///////////////////////////////////////////



///////////////////////////////////////////
//////////////CONNECTIONLIST��ʵ��/////////
///////////////////////////////////////////

CONNECTIONLIST::~CONNECTIONLIST(){}
	
void CONNECTIONLIST::MakeEmpty()
{
	CONNECTION *temp;
	while(First->link != NULL)
	{
		temp = First->link;
		First->link = temp->link;
		delete temp;
	}	
	Last = First;
}
	
int CONNECTIONLIST::Length() const
{
	CONNECTION *temp = First->link;
	int count = 0;
	while(temp != NULL)
	{
		temp = temp->link;
		count++;
	}
	return count;
}
	
void CONNECTIONLIST::AppendList(CONNECTION *add)
{
	Last->link = add;
	add->link = NULL;
	Last = add;
}

CONNECTION* CONNECTIONLIST::FindByIPAndSubIP(PUCHAR puIP, PUCHAR puSubIP)
{
	CONNECTION *temp = First->link;
	while(temp != NULL && !(IsUcSame(temp->m_IP, puIP, 4) && IsUcSame(temp->m_SubIP, puSubIP, 4)))
		temp = temp->link;
	return temp;
}

void CONNECTIONLIST::RemoveNode(CONNECTION *del)
{
	CONNECTION *temp = First->link;
	CONNECTION *pretemp = First;
	while(temp != NULL && temp != del)
	{
		pretemp = temp;
		temp = temp->link;
	}
	if(temp != NULL)
	{
		if(temp == Last)
			Last = pretemp;
		pretemp->RemoveAfterSelf(); 
	}
}
///////////////////////////////////////////
//////////////CONNECTIONLIST��ʵ�ֽ���/////
///////////////////////////////////////////



