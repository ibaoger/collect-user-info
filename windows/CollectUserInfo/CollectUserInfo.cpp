/***************************************************************

 * ��  Ŀ���ռ��û���Ϣ
 * ��  �ܣ���Ϣ��CPU����Ŀ����Ƶ����ʹ���ʣ������ڴ棨��������ǰ���ã������ڴ棨��������ǰ���У���ǰ�ϴ�����������
 * ��  �ߣ�ʯ˶
 * ��  �ڣ�2013-04-22
 * ��  Ȩ��Copyright (c) 2012-2013
 * ��  �ݣ�XP Win7 32λ 64λ
 * ��  ����0.2.2_build-130507

***************************************************************/

#include ".\collectuserinfo.h"
#include <vector>
using std::vector;


#ifdef  __cplusplus  
extern  "C"  {  
#endif  
#include <Powrprof.h>
#ifdef  __cplusplus  
}  
#endif  


const int KB_DIV = 1024;
#define STATUS_SUCCESS ((LONG)0x0L) 
#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES			3
#define MALLOC(x)			HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)				HeapFree(GetProcessHeap(), 0, (x))

//��KEY��������CPU������
const char* pszCPUCoresPath = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
//��~MHz �д��CPU��Ƶ
const char* pszCPUClockSpeedPath = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
//ϵͳ�汾��Ϣ
const char* pszOSVersionPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";


struct _PROCESSOR_POWER_INFORMATION 
{
	ULONG Number;		//�ں˱�ţ���0��ʼ
	ULONG MaxMhz;		//��Ƶ
	ULONG CurrentMhz;  
	ULONG MhzLimit;  
	ULONG MaxIdleState;  
	ULONG CurrentIdleState; 

	_PROCESSOR_POWER_INFORMATION()
	{
		ZeroMemory( this, sizeof(_PROCESSOR_POWER_INFORMATION) );
	}
};


CCollectUserInfo::CCollectUserInfo(void)
: m_uCPUUsage(0)
, m_uCPUClockSpeed(0)
, m_uCPUCores(0)
, m_uMemoryLoad(0)
, m_uTotalPhys(0)
, m_uAvailPhys(0)
, m_uAvailVirtual(0)
, m_uTotalVirtual(0)
, m_bThreadStarted(false)
, m_uUpSpeed(0)
, m_uDownSpeed(0)
, m_pNetFlowWatcher(NULL)
{
	m_pNetFlowWatcher = CreateNetFlowWatcher(this) ;
	if (m_pNetFlowWatcher)
	{
		m_pNetFlowWatcher->Start();
		m_pNetFlowWatcher->SetRefreshRate(1);	// ��ȡ�������ʱ��
	}
	InitializeCriticalSection(&m_criLock);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);

	m_bThreadStarted = true;
	SafeStartThread();
}

CCollectUserInfo::~CCollectUserInfo(void)
{
	if (m_pNetFlowWatcher)
	{
		m_pNetFlowWatcher->Stop();
		m_pNetFlowWatcher->Release();
	}
	m_bThreadStarted = false;
	SafeStopThread();

	DeleteCriticalSection(&m_criLock);

	WSACleanup();
}

void CCollectUserInfo::Release()
{
	delete this;
}

/**
 * �������ܣ�����ת��
 * ��    ����FILETIME�ṹ��
 * �� �� ֵ��Double����ʱ��
 **/
double CCollectUserInfo::FileTimeToDouble(FILETIME &filetime)
{
	return (double)(filetime.dwHighDateTime * 4.294967296E9) + (double)filetime.dwLowDateTime;
}

/**
 * �������ܣ���ȡ�ڴ�ʹ����
 * ��    ����uMemoryLoad	�ڴ�ʹ���� ��λ��%
 *			 uTotalPhys	�����ڴ����� ��λ��KB
 *			 uAvailPhys	��ǰ���������ڴ� ��λ��KB
 *			 uTotalVirtual �����ڴ�����	��λ��KB
 *			 uAvailVirtual ��ǰ���������ڴ� ��λ��KB
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::ComputerMemoryUsage( UINT &uMemoryLoad,
											UINT &uTotalPhys,
											UINT &uAvailPhys,
											UINT &uTotalVirtual,
											UINT &uAvailVirtual)
{
	MEMORYSTATUSEX memStatusEx = {0};
	memStatusEx.dwLength = sizeof(memStatusEx);
	if ( GlobalMemoryStatusEx(&memStatusEx) )
	{
		uMemoryLoad = (UINT)(memStatusEx.dwMemoryLoad);
		uTotalPhys = (UINT)(memStatusEx.ullTotalPhys / KB_DIV);
		uAvailPhys = (UINT)(memStatusEx.ullAvailPhys / KB_DIV);
		uTotalVirtual = (UINT)(memStatusEx.ullTotalVirtual / KB_DIV);
		uAvailVirtual = (UINT)(memStatusEx.ullAvailVirtual / KB_DIV);

		return true;
	}
	else
	{
		return false;
	}
}

/**
 * �������ܣ���ȡ�ڴ�ʹ����
 * ��    ����uMemoryLoad	�ڴ�ʹ���� ��λ��%
 *			 uTotalPhys	�����ڴ����� ��λ��KB
 *			 uAvailPhys	��ǰ���������ڴ� ��λ��KB
 *			 uTotalVirtual �����ڴ�����	��λ��KB
 *			 uAvailVirtual ��ǰ���������ڴ� ��λ��KB
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetMemoryStatus( UINT &uMemoryLoad,
										UINT &uTotalPhys,
										UINT &uAvailPhys,
										UINT &uTotalVirtual,
										UINT &uAvailVirtual)
{
	EnterCriticalSection(&m_criLock);
	uMemoryLoad = m_uMemoryLoad;
	uTotalPhys = m_uTotalPhys;
	uAvailPhys = m_uAvailPhys;
	uTotalVirtual = m_uTotalVirtual;
	uAvailVirtual = m_uAvailVirtual;
	LeaveCriticalSection(&m_criLock);

	return true;
}

/**
 * �������ܣ���ȡCPU��Ϣ
 * ��    ����uCPUClockSpeed	CPU��Ƶ	��λ��MHz
 *			 uCPUCores	CPU������	��λ����
 *			 uCPUUsage	CPUʹ����	��λ��%
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetCPUStatus(UINT &uCPUClockSpeed, UINT &uCPUCores, UINT &uCPUUsage)
{
	EnterCriticalSection(&m_criLock);
	uCPUClockSpeed = m_uCPUClockSpeed;
	uCPUCores = m_uCPUCores;
	uCPUUsage = m_uCPUUsage;
	LeaveCriticalSection(&m_criLock);

	return true;
}

/**
 * �������ܣ���ȡCPU��Ϣ
 * ��    ����uCPUUsage	CPUʹ����	��λ��%
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::ComputerCPUUsage(UINT &uCPUUsage)
{
	bool bSuc = true;

	double fOldCPUIdleTime = 0.0;
	double fOldCPUKernelTime = 0.0;
	double fOldCPUUserTime = 0.0;

	FILETIME ftIdle = {0}, ftKernel = {0}, ftUser = {0};
	if ( GetSystemTimes(&ftIdle, &ftKernel, &ftUser) )
	{
		fOldCPUIdleTime = FileTimeToDouble(ftIdle);
		fOldCPUKernelTime = FileTimeToDouble(ftKernel);
		fOldCPUUserTime = FileTimeToDouble(ftUser);
	}
	else
	{
		bSuc = bSuc && false;
	}

	Sleep(1000);

	ZeroMemory(&ftIdle, sizeof(ftIdle));
	ZeroMemory(&ftKernel, sizeof(ftKernel));
	ZeroMemory(&ftUser, sizeof(ftUser));
	if ( GetSystemTimes(&ftIdle, &ftKernel, &ftUser) )
	{
		double fCPUIdleTime = FileTimeToDouble(ftIdle);
		double fCPUKernelTime = FileTimeToDouble(ftKernel);
		double fCPUUserTime = FileTimeToDouble(ftUser);

		uCPUUsage = (UINT)(100.0 - (fCPUIdleTime - fOldCPUIdleTime) / (fCPUKernelTime - fOldCPUKernelTime + fCPUUserTime - fOldCPUUserTime) * 100.0);
	}
	else
	{
		bSuc = bSuc && false;
		uCPUUsage = 0;
	}

	return bSuc;
}

/**
 * �������ܣ���ȡCPU��Ϣ
 * ��    ����uCPUClockSpeed	CPU��Ƶ	��λ��MHz
 *			 uCPUCores	CPU������	��λ����
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetCPUInfo(UINT &uCPUClockSpeed, UINT &uCPUCores)
{
	bool bSuc = false;
	uCPUClockSpeed = 0;
	uCPUCores = 0;

	//��ȡCPU������
	SYSTEM_INFO sysInfo;
	::GetSystemInfo(&sysInfo);
	uCPUCores = (UINT)sysInfo.dwNumberOfProcessors;

	vector<_PROCESSOR_POWER_INFORMATION> vecBuf(uCPUCores);

	LONG ntStatus = CallNtPowerInformation( ProcessorInformation, NULL, 0, (PVOID)&vecBuf[0], sizeof(_PROCESSOR_POWER_INFORMATION)*vecBuf.size() );

    if ( ntStatus == STATUS_SUCCESS )
	{
		uCPUClockSpeed = vecBuf[0].MaxMhz;
		bSuc = true;
	}
	else
	{
		uCPUClockSpeed = 0;
		//��ע����ж�ȡ
		
		HKEY hKey;
		LONG lRet;

		//lRet = RegOpenKey(HKEY_LOCAL_MACHINE, pszCPUCoresPath, &hKey);
		//if( lRet == ERROR_SUCCESS )
		//{
		//	//��ȡ��KEY��������CPU������
		//	DWORD dwSubKeys;
		//	lRet = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		//	if( lRet == ERROR_SUCCESS )
		//	{
		//		uCPUCores = dwSubKeys;
		//		bSuc = true;
		//	}
		//}
		//RegCloseKey(hKey);

		lRet = RegOpenKey(HKEY_LOCAL_MACHINE, pszCPUClockSpeedPath, &hKey);
		if( lRet == ERROR_SUCCESS )
		{
			//��ȡCPU��Ƶ
			DWORD dwSize = 0;
			unsigned char* pMHz = new unsigned char[4];
			lRet = RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE)pMHz, &dwSize);
			if( lRet == ERROR_SUCCESS )
			{
				uCPUClockSpeed = pMHz[0] + pMHz[1]*256 + pMHz[2]*256*256 + pMHz[3]*256*256*256;
				bSuc = true;
			}
		}
		RegCloseKey(hKey);
	}

	return bSuc;
}

/**
 * �������ܣ���ȡCPUʹ�����߳�
 * ��    ������
 * �� �� ֵ����
 **/
void CCollectUserInfo::SafeThreadProcMain()
{
	UINT uCPUUsage = 0;
	UINT uMemoryLoad = 0; 
	UINT uTotalPhys = 0;
	UINT uAvailPhys = 0; 
	UINT uTotalVirtual = 0; 
	UINT uAvailVirtual = 0;
	UINT uCPUClockSpeed = 0;
	UINT uCPUCores = 0;

	while( m_bThreadControlStart && m_bThreadStarted )
	{
		uCPUUsage = uMemoryLoad = uTotalPhys = uAvailPhys = uTotalVirtual = uAvailVirtual = uCPUClockSpeed = uCPUCores = 0;
		ComputerCPUUsage(uCPUUsage);
		ComputerMemoryUsage(uMemoryLoad, uTotalPhys, uAvailPhys, uTotalVirtual, uAvailVirtual);
		GetCPUInfo(uCPUClockSpeed, uCPUCores);

		EnterCriticalSection(&m_criLock);
		m_uCPUUsage = uCPUUsage;
		m_uMemoryLoad = uMemoryLoad;
		m_uTotalPhys = uTotalPhys;
		m_uAvailPhys = uAvailPhys;
		m_uTotalVirtual = uTotalVirtual;
		m_uAvailVirtual = uAvailVirtual;
		m_uCPUClockSpeed = uCPUClockSpeed;
		m_uCPUCores = uCPUCores;
		LeaveCriticalSection(&m_criLock);

		Sleep(1000);
	}
}

/**
 * �������ܣ�֪ͨ��Ϣ����������
 * ��    ������������ ��λ B/s
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::OnNotifyUploadSpeed(float upspeed ) 
{
	EnterCriticalSection(&m_criLock);
	m_uUpSpeed = (UINT)(upspeed / KB_DIV);
	LeaveCriticalSection(&m_criLock);
	return true;
}

/**
 * �������ܣ�֪ͨ��Ϣ����������
 * ��    ������������ ��λ B/s
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::OnNotifyDownloadSpeed(float downspeed )
{
	EnterCriticalSection(&m_criLock);
	m_uDownSpeed = (UINT)(downspeed / KB_DIV);	
	LeaveCriticalSection(&m_criLock);
	return true;
}

/**
 * �������ܣ���ȡʵʱ���С���������
 * ��    ����uUpSpeed �������� ��λ KB/s
 *			uDownBandwidth �������� ��λ KB/s
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetNetFlowStatus(UINT &uUpSpeed, UINT &uDownBandwidth)
{
	EnterCriticalSection(&m_criLock);
	uUpSpeed = m_uUpSpeed;
	uDownBandwidth = m_uDownSpeed;
	LeaveCriticalSection(&m_criLock);
	return true;
}

/**
 * �������ܣ���ȡ������
 * ��    ����pszName ������
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetHostName(char* pszName)
{
	char hostName[255];
	if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) 
	{
		return false;
	}
	strcpy(pszName, hostName);

	return true;
}

/**
 * �������ܣ���ȡ����IP
 * ��    ����uClientIP ����IP
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetClientIP(ULONG &uClientIP)
{
	char hostName[255];
	if (!GetHostName(hostName)) 
	{
		return false;
	}

	struct hostent *pHostent = gethostbyname(hostName);
	if (NULL == pHostent) 
	{
		return false;
	}
	struct in_addr addr;
	memcpy(&addr, pHostent->h_addr_list[0], sizeof(struct in_addr));
	uClientIP = addr.S_un.S_addr;

	return true;
}

/**
 * �������ܣ���ȡ����IP
 * ��    ����pClientIP ����IP
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetClientIP(char* pClientIP)
{
	ULONG uClientIP;
	if (!GetClientIP(uClientIP)) 
	{
		return false;
	}

	if (!LongIPToStringIP(pClientIP, uClientIP))
	{
		return false;
	}

	return true;
}

/**
 * �������ܣ�����IPת���ַ���IP
 * ��    ����pszIP �ַ���IP
 *			 uIP ����IP
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::LongIPToStringIP(char* pszIP, ULONG uIP)
{
	if (NULL == pszIP)
	{
		return false;
	}

	ULONG uIPTmp = uIP;
	ULONG uTmp = 0, uLen = 0;
	char pBuf[4] = {0};
	memset(pszIP, 0, _msize(pszIP));

	uTmp = (uIPTmp >> 24) & 0xFF;
	_itoa(uTmp, pBuf, 10);
	uLen = StringLength(pBuf);
	memcpy(pszIP, pBuf, uLen);

	uTmp = (uIPTmp >> 16) & 0xFF;
	_itoa(uTmp, pBuf, 10);
	uLen = StringLength(pBuf);
	memcpy(pszIP, pBuf, uLen);

	uTmp = (uIPTmp >> 8) & 0xFF;
	_itoa(uTmp, pBuf, 10);
	uLen = StringLength(pBuf);
	memcpy(pszIP, pBuf, uLen);

	uTmp = uIPTmp & 0xFF;
	_itoa(uTmp, pBuf, 10);
	uLen = StringLength(pBuf);
	memcpy(pszIP, pBuf, uLen);

	return true;
}

/**
 * �������ܣ���ȡ�ַ����ĳ���
 * ��    ����pszIP �ַ���
 * �� �� ֵ���ַ����ĳ���
 **/
int CCollectUserInfo::StringLength(char* pszIP)
{
	if (NULL == pszIP)
	{
		return false;
	}

	int uLen = _msize(pszIP);
	for (int i=0; i<uLen; i++)
	{
		if ('0' == pszIP[i])
		{
			return i;
		}
	}
	return 0;
}

/**
 * �������ܣ���ȡDNS��IP
 * ��    ����uClientDNS DNS��IP
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetClientDNS(ULONG &uClientDNS)
{
	uClientDNS = 0;
	DWORD dwRetVal = 0;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	ULONG family = AF_INET;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = WORKING_BUFFER_SIZE;
	ULONG Iterations = 0;
	do 
	{
		pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
		if (pAddresses == NULL) 
		{
			return false;
		}
		dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
		if (dwRetVal == ERROR_BUFFER_OVERFLOW) 
		{
			FREE(pAddresses);
			pAddresses = NULL;
		} 
		else 
		{
			break;
		}
		Iterations++;
	} 
	while ((dwRetVal==ERROR_BUFFER_OVERFLOW) && (Iterations<MAX_TRIES));

	if (dwRetVal == NO_ERROR) 
	{
		sockaddr_in DNSAddr;
		memcpy(&DNSAddr, pAddresses->FirstDnsServerAddress->Address.lpSockaddr, sizeof(SOCKADDR));
		uClientDNS = DNSAddr.sin_addr.s_addr;
	}
	if (pAddresses)
	{
		FREE(pAddresses);
		pAddresses = NULL;
	}

	return true;
}

/**
 * �������ܣ���ȡϵͳ�汾��Ϣ
 * ��    ����pszOSVersion ϵͳ�汾��Ϣ
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetOSVersion(char* pszOSVersion)
{
	HKEY  hKey;
	LONG  ReturnValue;
	DWORD type;
	CHAR strBuf[100];
	DWORD dwSize = 100;

	//��ע���
	ReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
								"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
								0,
								KEY_ALL_ACCESS,
								&hKey);
	if(ReturnValue != ERROR_SUCCESS)
	{
		return false;
	}

	//��ȡ����ϵͳ����
	ReturnValue = RegQueryValueEx(hKey, 
								"ProductName", 
								NULL, 
								&type, 
								(BYTE *)strBuf, 
								&dwSize);
	if(ReturnValue == ERROR_SUCCESS)
	{
		strcpy(pszOSVersion, strBuf);
	}

	//��ȡCSDVersion
	ReturnValue = RegQueryValueEx(hKey,
								"CSDVersion",
								NULL,
								&type,
								(BYTE *)strBuf,
								&dwSize);
	if(ReturnValue == ERROR_SUCCESS)
	{
		strcat(pszOSVersion, "_");
		strcat(pszOSVersion, strBuf);
	}

	//��ȡCurrentVersion
	ReturnValue = RegQueryValueEx(hKey,
								"CurrentVersion",
								NULL,
								&type,
								(BYTE *)strBuf,
								&dwSize);
	if(ReturnValue == ERROR_SUCCESS)
	{
		strcat(pszOSVersion, "_");
		strcat(pszOSVersion, strBuf);
	}

	return true;
}

/**
 * �������ܣ���ȡӲ�̴�С
 * ��    ����uDiskSize Ӳ�̴�С ��λ��GB
 * �� �� ֵ���ɹ�/ʧ��
 **/
bool CCollectUserInfo::GetDiskSize(ULONG &uDiskSize)
{
	HANDLE hDevice;
	hDevice = CreateFile("\\\\.\\PhysicalDrive0", 
						GENERIC_READ | GENERIC_WRITE, 
						FILE_SHARE_READ | FILE_SHARE_WRITE, 
						NULL,
						OPEN_EXISTING,
						0,
						NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	GET_LENGTH_INFORMATION nDiskSizeByBytes;
	DWORD dwReturnSize;
	DeviceIoControl(hDevice,
					IOCTL_DISK_GET_LENGTH_INFO,
					NULL,
					0,
					(LPVOID)&nDiskSizeByBytes,
					sizeof(GET_LENGTH_INFORMATION),
					&dwReturnSize,
					NULL);
	uDiskSize = (ULONG)(nDiskSizeByBytes.Length.QuadPart/(1000000000));		//GB

	return true;
}

// GetCPUEveryCoreUsage ������ʵ��
/************************************
// һ��ʹ��C#����ʵ�֣�����������Windows XP��Windows 7ϵͳ��
// ������GetCPUEveryCoreUseRate����������һ��������CPU����ʹ���ʵ��ַ���

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;

namespace CShapeCPUUseRateDLL
{
	public class CShapeCPUUseRate
	{
		public int Initialize()
		{
			try
			{
				m_nCPUCoreNumber = System.Environment.ProcessorCount;
				m_pfCounters = new PerformanceCounter[m_nCPUCoreNumber]; 
				for(int i = 0; i < m_nCPUCoreNumber; i++)        
				{            
					m_pfCounters[i] = new PerformanceCounter("Processor", "% Processor Time", i.ToString()); 
				}  
			}
			catch (System.Exception e)
			{
				return 0;
			}
			return 1;
		}
		public int GetCPUCoreNumber()
		{
			return m_nCPUCoreNumber;
		}
		public string GetCPUEveryCoreUseRate()
		{
			StringBuilder strBuild = new StringBuilder();
			float fRate = m_pfCounters[0].NextValue();
			int nRate = Convert.ToInt32(fRate);
			strBuild.Append(nRate.ToString());
			for(int i = 1; i < m_nCPUCoreNumber; i++)       
			{
				fRate = m_pfCounters[i].NextValue();
				nRate = Convert.ToInt32(fRate);
				strBuild.Append("," + nRate.ToString());
			}
			return strBuild.ToString();
		}
		private PerformanceCounter[]   m_pfCounters;
		private int                    m_nCPUCoreNumber;
	}
}


// ����ʹ��C++����C#����ʵ
// Ȼ��ʹ��C++����C#�����GetCPUEveryCoreUseRate������ȡ���


#using ".\CShapeCPUUseRateDLL.dll"	// C# LIB��λ��
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
using namespace CShapeCPUUseRateDLL;

int main()  
{
	CShapeCPUUseRate ^ cpuUseRate = gcnew CShapeCPUUseRate;
	if (!cpuUseRate->Initialize())
	{
		printf("Error!\n");
		getch();
		return -1;
	}
	else
	{
		printf("ϵͳ��CPUΪ%d��CPU\n",cpuUseRate->GetCPUCoreNumber());
		while (true)
		{	
			Sleep(1000);
			printf("\r��ǰCPU����ʹ���ʷֱ�Ϊ��%s     ", cpuUseRate->GetCPUEveryCoreUseRate());
		}
	}
	return 0;
}

************************************/