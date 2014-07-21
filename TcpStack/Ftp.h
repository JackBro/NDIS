// Ftp.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A56__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A56__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



/////////////////////////////////////////
//�Զ������
//////////////////////////////////////////

#include "packet.h"
#include "Mac.h"
#include "Tcp.h"

//�豸����Լ��Ƿ�������б�־
typedef struct _LISTENER_INFO
{
	HANDLE DebugPrintDriver;
	HANDLE ReceiveEvent;
	HANDLE DecodeEvent;
	HANDLE DecodeWaiter;
	bool   ReceiveKeepGoing;
	bool   DecodeKeepGoing;
} LISTENER_INFO, *PLISTENER_INFO;


//
//����ȫ�ֱ���
//
typedef struct _GLOBAL
{
	LISTENER_INFO*   ListenerInfo;         //ȫ�ֱ���
    CRITICAL_SECTION cs_pq;                //���ڱ����ѻ��������ٽ�������
    CRITICAL_SECTION cs_ConnList;          //���ڱ������ӵ��ٽ�������
    CRITICAL_SECTION cs_TcbList;           //���ڱ���Tcb���ٽ�������

    PACKETQUEUE*     PacketQueue;          //ȫ�ֱ���,���հ�����

	unsigned char SrcIpAddr[4];            //ԴIP��ַ

	BOOL     HasStarted;                   //����ָʾ�豸�Ƿ��������ɹ�
	BOOL     HasReceived;                  //����ָʾ�Ƿ��Ѿ���ȡ����

	Mac*     mac;                          //MACģ����ָ��

	CONNECTIONLIST * ConnList;             //���Ӹ����б������Ӧ�ó�����DLL�����

	UINT GlobalTcbNum;                     //ȫ��TCB��
    
} GLOBAL, *PGLOBAL;  

//ȫ�ֱ���
extern GLOBAL      Global;


//
//�Զ��庯��
//

BOOL InstallTheDriver();                                   //��װ����
BOOL CreateWorkDevice();                                   //�����豸���      
VOID StartReceive();                                       //��ʼ�������� 


//FTP��������
void FtpControlCenter(int Type, char* pszAppendix, USHORT SzLen, char* SelfPath, char* RemotePath, UINT FileSize, PUCHAR IP, PUCHAR SubIP, USHORT SrcPort, USHORT DstPort, UINT RemoteTcbNum, int CurStatus);                   
                   

//������ֲ������߳�
/*UINT*/VOID
ControlThreadFunction(void*);

//
//���ñ���������
//

# define     InstallExeFile      "IGN SIGN.exe"
# define     DRIVERNAME          "\\\\.\\Passthru"

//��������ʾ
VOID ErrMsg (HRESULT hr,
             LPCWSTR  lpFmt,
             ...);


VOID ErrPtf (char* str, UINT ISN, UINT ACKNUM, UINT CURISN, USHORT WINDOW, UINT TIME);


//��װ

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>
#include "netcfgx.h"
#include "netcfgn.h"
#include <setupapi.h>
#include <devguid.h>
#include <objbase.h>

#include <windowsx.h>
#include <commctrl.h>        // For common controls, e.g. Tree


#define LOCK_TIME_OUT     5000
#define APP_NAME          L"BindView"

VOID ReleaseRef (IN IUnknown* punk);

HRESULT GetKeyValue (HINF hInf,
                     LPCWSTR lpszSection,
                     LPCWSTR lpszKey,
                     DWORD  dwIndex,
                     LPWSTR *lppszValue);

HRESULT GetPnpID (LPWSTR lpszInfFile,
                  LPWSTR *lppszPnpID);

HRESULT HrGetINetCfg (IN BOOL fGetWriteLock,
                      IN LPCWSTR lpszAppName,
                      OUT INetCfg** ppnc,
                      OUT LPWSTR *lpszLockedBy);

HRESULT HrReleaseINetCfg (IN INetCfg* pnc,
                          IN BOOL fHasWriteLock);

HRESULT HrInstallComponent(IN INetCfg* pnc,
                           IN LPCWSTR szComponentId,
                           IN const GUID* pguidClass);

HRESULT HrInstallNetComponent (IN INetCfg *pnc,
                               IN LPCWSTR lpszComponentId,
                               IN const GUID    *pguidClass,
                               IN LPCWSTR lpszInfFullPath);

HRESULT InstallSpecifiedComponent (LPWSTR lpszInfFile,
                                   LPWSTR lpszPnpID,
                                   const GUID *pguidClass);




#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

