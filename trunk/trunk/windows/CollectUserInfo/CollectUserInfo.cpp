/***************************************************************

 * 项  目：收集用户信息
 * 功  能：信息：CPU（数目、主频、总使用率）物理内存（总量、当前可用）虚拟内存（总量、当前空闲）当前上传、下载流量
 * 作  者：石硕
 * 日  期：2013-04-22
 * 版  权：Copyright (c) 2012-2013
 * 兼  容：XP Win7 32位 64位
 * 版  本：0.2.2_build-130507

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

//子KEY个数代表CPU核心数
const char* pszCPUCoresPath = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
//键~MHz 中存放CPU主频
const char* pszCPUClockSpeedPath = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
//系统版本信息
const char* pszOSVersionPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";


struct _PROCESSOR_POWER_INFORMATION 
{
	ULONG Number;		//内核编号，从0开始
	ULONG MaxMhz;		//主频
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
		m_pNetFlowWatcher->SetRefreshRate(1);	// 获取流量间隔时间
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
 * 函数功能：类型转换
 * 参    数：FILETIME结构体
 * 返 回 值：Double类型时间
 **/
double CCollectUserInfo::FileTimeToDouble(FILETIME &filetime)
{
	return (double)(filetime.dwHighDateTime * 4.294967296E9) + (double)filetime.dwLowDateTime;
}

/**
 * 函数功能：获取内存使用率
 * 参    数：uMemoryLoad	内存使用率 单位：%
 *			 uTotalPhys	物理内存总量 单位：KB
 *			 uAvailPhys	当前可用物理内存 单位：KB
 *			 uTotalVirtual 虚拟内存总量	单位：KB
 *			 uAvailVirtual 当前空闲虚拟内存 单位：KB
 * 返 回 值：成功/失败
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
 * 函数功能：获取内存使用率
 * 参    数：uMemoryLoad	内存使用率 单位：%
 *			 uTotalPhys	物理内存总量 单位：KB
 *			 uAvailPhys	当前可用物理内存 单位：KB
 *			 uTotalVirtual 虚拟内存总量	单位：KB
 *			 uAvailVirtual 当前空闲虚拟内存 单位：KB
 * 返 回 值：成功/失败
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
 * 函数功能：获取CPU信息
 * 参    数：uCPUClockSpeed	CPU主频	单位：MHz
 *			 uCPUCores	CPU核心数	单位：个
 *			 uCPUUsage	CPU使用率	单位：%
 * 返 回 值：成功/失败
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
 * 函数功能：获取CPU信息
 * 参    数：uCPUUsage	CPU使用率	单位：%
 * 返 回 值：成功/失败
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
 * 函数功能：获取CPU信息
 * 参    数：uCPUClockSpeed	CPU主频	单位：MHz
 *			 uCPUCores	CPU核心数	单位：个
 * 返 回 值：成功/失败
 **/
bool CCollectUserInfo::GetCPUInfo(UINT &uCPUClockSpeed, UINT &uCPUCores)
{
	bool bSuc = false;
	uCPUClockSpeed = 0;
	uCPUCores = 0;

	//获取CPU核心数
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
		//从注册表中读取
		
		HKEY hKey;
		LONG lRet;

		//lRet = RegOpenKey(HKEY_LOCAL_MACHINE, pszCPUCoresPath, &hKey);
		//if( lRet == ERROR_SUCCESS )
		//{
		//	//获取子KEY个数，即CPU核心数
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
			//获取CPU主频
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
 * 函数功能：获取CPU使用率线程
 * 参    数：无
 * 返 回 值：无
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
 * 函数功能：通知消息，上行流量
 * 参    数：上行流量 单位 B/s
 * 返 回 值：成功/失败
 **/
bool CCollectUserInfo::OnNotifyUploadSpeed(float upspeed ) 
{
	EnterCriticalSection(&m_criLock);
	m_uUpSpeed = (UINT)(upspeed / KB_DIV);
	LeaveCriticalSection(&m_criLock);
	return true;
}

/**
 * 函数功能：通知消息，下行流量
 * 参    数：下行流量 单位 B/s
 * 返 回 值：成功/失败
 **/
bool CCollectUserInfo::OnNotifyDownloadSpeed(float downspeed )
{
	EnterCriticalSection(&m_criLock);
	m_uDownSpeed = (UINT)(downspeed / KB_DIV);	
	LeaveCriticalSection(&m_criLock);
	return true;
}

/**
 * 函数功能：获取实时上行、下行流量
 * 参    数：uUpSpeed 上行流量 单位 KB/s
 *			uDownBandwidth 下行流量 单位 KB/s
 * 返 回 值：成功/失败
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
 * 函数功能：获取主机名
 * 参    数：pszName 主机名
 * 返 回 值：成功/失败
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
 * 函数功能：获取主机IP
 * 参    数：uClientIP 主机IP
 * 返 回 值：成功/失败
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
 * 函数功能：获取主机IP
 * 参    数：pClientIP 主机IP
 * 返 回 值：成功/失败
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
 * 函数功能：整形IP转换字符串IP
 * 参    数：pszIP 字符串IP
 *			 uIP 整形IP
 * 返 回 值：成功/失败
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
 * 函数功能：获取字符串的长度
 * 参    数：pszIP 字符串
 * 返 回 值：字符串的长度
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
 * 函数功能：获取DNS的IP
 * 参    数：uClientDNS DNS的IP
 * 返 回 值：成功/失败
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
 * 函数功能：获取系统版本信息
 * 参    数：pszOSVersion 系统版本信息
 * 返 回 值：成功/失败
 **/
bool CCollectUserInfo::GetOSVersion(char* pszOSVersion)
{
	HKEY  hKey;
	LONG  ReturnValue;
	DWORD type;
	CHAR strBuf[100];
	DWORD dwSize = 100;

	//打开注册表
	ReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
								"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
								0,
								KEY_ALL_ACCESS,
								&hKey);
	if(ReturnValue != ERROR_SUCCESS)
	{
		return false;
	}

	//获取操作系统名称
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

	//获取CSDVersion
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

	//获取CurrentVersion
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
 * 函数功能：获取硬盘大小
 * 参    数：uDiskSize 硬盘大小 单位：GB
 * 返 回 值：成功/失败
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

// GetCPUEveryCoreUsage 分两步实现
/************************************
// 一、使用C#方法实现（正常运行于Windows XP及Windows 7系统）
// 这个类的GetCPUEveryCoreUseRate函数将返回一个包含各CPU各核使用率的字符串

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


// 二、使用C++调用C#方法实
// 然后使用C++调用C#代码的GetCPUEveryCoreUseRate方法获取结果


#using ".\CShapeCPUUseRateDLL.dll"	// C# LIB的位置
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
		printf("系统中CPU为%d核CPU\n",cpuUseRate->GetCPUCoreNumber());
		while (true)
		{	
			Sleep(1000);
			printf("\r当前CPU各核使用率分别为：%s     ", cpuUseRate->GetCPUEveryCoreUseRate());
		}
	}
	return 0;
}

************************************/