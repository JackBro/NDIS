// Commen.h: interface for the CCommen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_)
#define AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//��������
typedef enum
{
	//���ڳ�ʼ������
	CONNECT_SENDING,
	CONNECT_F_COMMING,
	CONNECT_T_COMMING,

	//���ڶ�ʱ
	TIME_OVER,
	
	//��������ɶԳ��֣���һ�������ϲ㷢��TCB������
	//�ڶ�������Զ�̷���TCB������
	COMM_CLOSE_SENDING,
	COMM_CLOSE_COMMING,
	
	COMM_LIST_GET_SENDING,
	COMM_LIST_GET_COMMING,

	COMM_LIST_POST_SENDING,
	COMM_LIST_POST_COMMING,

	COMM_SCREEN_GET_SENDING,
	COMM_SCREEN_GET_COMMING,

	COMM_SCREEN_POST_SENDING,
	COMM_SCREEN_POST_COMMING,

	COMM_FILE_GET_SENDING,
	COMM_FILE_GET_COMMING,

	COMM_FILE_POST_SENDING,
	COMM_FILE_POST_COMMING,

	COMM_ACK_SENDING,
	COMM_ACK_COMMING,

	//�������ݣ��ɶԳ��֣���һ�������ϲ㷢��TCB������
	//�ڶ�������Զ�̷���TCB������
//	DATA_LIST_SENDING,
//	DATA_LIST_COMMING,

//	DATA_SCREEN_SENDING,
//	DATA_SCREEN_COMMING,

	DATA_FILE_SENDING,
	DATA_FILE_COMMING,

	DATA_ACK_SENDING,
	DATA_ACK_COMMING,

    //���ڸ��ϲ�ͨ��
	APP_ERR_SENDING

} OPERATION_TYPE;










//״̬���
typedef enum 
{
	STATUS_CLOSED,                //�ر�״̬
    STATUS_COMMAND_ESTABLISHED,   //���������ѽ���
    STATUS_DATA_ESTABLISHED,      //���������ѽ���
}STATUS_SIGNAL;










/*ϵͳ���д˺���������֮*/
//ת���������ֽ�˳��
USHORT Lhtons(USHORT hostshort);


//���������ֽ�ת����UCHAR����
void Lhtonl(UINT hostlong, PUCHAR buddle);


//��UCHAR����ת����UINT��
UINT Lnltoh(PUCHAR buddle);


//��UCHAR����ת����USHORT��
USHORT Lnstoh(PUCHAR buddle);


//�ж�����UCHAR�����Ƿ���ͬ
BOOL IsUcSame(PUCHAR Uc1, PUCHAR Uc2, int len);

//���޷����ַ���ת��Ϊ�з����ַ���
PCHAR PUchTOch(PUCHAR puch, USHORT len);

//���з����ַ���ת��Ϊ�޷����ַ���
PUCHAR PchTOUch(PCHAR pch, USHORT len);


#endif // !defined(AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_)
