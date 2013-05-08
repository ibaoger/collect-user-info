#pragma once

#define _WIN32_WINNT 0x0501	//xp sp1
#include <WinSock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include "TVThread.h"
#include "INetFlowWatcher.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Powrprof.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "IPHLPAPI.lib")


class CCollectUserInfo
	: public CSafeThread
	, public INetFlowWatcherNotify
{
public:
	CCollectUserInfo(void);
	~CCollectUserInfo(void);

	bool GetMemoryStatus(UINT &uMemoryLoad,
						UINT &uTotalPhys,
						UINT &uAvailPhys,
						UINT &uTotalVirtual,
						UINT &uAvailVirtual);
	bool GetCPUStatus(UINT &uCPUClockSpeed, UINT &uCPUCores, UINT &uCPUUsage);
	bool GetNetFlowStatus(UINT &uUpSpeed, UINT &uDownSpeed);
	void Release();

	bool GetHostName(char* pszName);
	bool GetClientIP(ULONG &uClientIP);
	bool GetClientIP(char* pClientIP);
	bool GetClientDNS(ULONG &uClientDNS);
	bool GetOSVersion(char* pszOSVersion);
	bool GetDiskSize(ULONG &uDiskSize);

	/**
	 * CSafeThread
	 **/
	virtual void SafeThreadProcMain();

	/**
	 * INetFlowWatcher
	 **/
	virtual bool OnNotifyDownloadSpeed(float downspeed );
	virtual bool OnNotifyUploadSpeed( float upspeed ) ;	

private:
	double	FileTimeToDouble(FILETIME &filetime);
	bool	GetCPUInfo(UINT &uCPUClockSpeed, UINT &uCPUCores);
	bool	ComputerCPUUsage(UINT &uCPUUsage);
	bool	ComputerMemoryUsage(UINT &uMemoryLoad,
								UINT &uTotalPhys,
								UINT &uAvailPhys,
								UINT &uTotalVirtual,
								UINT &uAvailVirtual);
	bool	LongIPToStringIP(char* pszIP, ULONG uIP);
	int		StringLength(char* pszIP);

private:
	INetFlowWatcher*	m_pNetFlowWatcher;				// 流量统计

	UINT				m_uCPUUsage;					// 单位 %
	UINT				m_uCPUClockSpeed;				// 单位 MHz
	UINT				m_uCPUCores;					// 单位

	UINT				m_uMemoryLoad;					// 单位 %
	UINT				m_uTotalPhys;					// 单位 MB
	UINT				m_uAvailPhys;					// 单位 MB
	UINT				m_uTotalVirtual;				// 单位 MB
	UINT				m_uAvailVirtual;				// 单位 MB

	bool				m_bThreadStarted;
	CRITICAL_SECTION	m_criLock;						// 临界区
	UINT				m_uUpSpeed;						// 上传流量  单位 KB/s
	UINT				m_uDownSpeed;					// 下载流量  单位 KB/s
};
