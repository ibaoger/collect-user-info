
#include <stdio.h>
#include "CollectUserInfo.h"

const int MB_DIV = 1024;
const int GB_DIV = 1024 * 1024;
const int TB_DIV = 1024 * 1024 * 1024;

int main()
{
	CCollectUserInfo userInf;

	UINT uMemoryLoad, uTotalPhys, uAvailPhys, uTotalVirtual, uAvailVirtual, uCPUClockSpeed, uCPUCores, uCPUUsage, uUpSpeed, uDownSpeed;
	ULONG uClientIP, uClientDNS, uDiskSize;
	char szHostName[255] = {0};
	char szOSVersion[255] = {0};
	char szClientIP[255] = {0};
	bool bSuc = true;


	while (1)
	{
		bSuc = bSuc & userInf.GetMemoryStatus(uMemoryLoad, uTotalPhys, uAvailPhys, uTotalVirtual, uAvailVirtual);
		bSuc = bSuc & userInf.GetCPUStatus(uCPUClockSpeed, uCPUCores, uCPUUsage);
		bSuc = bSuc & userInf.GetNetFlowStatus(uUpSpeed, uDownSpeed);
		bSuc = bSuc & userInf.GetHostName(szHostName);
		bSuc = bSuc & userInf.GetClientIP(szClientIP);
		bSuc = bSuc & userInf.GetClientDNS(uClientDNS);
		bSuc = bSuc & userInf.GetOSVersion(szOSVersion);
		bSuc = bSuc & userInf.GetDiskSize(uDiskSize);

		if (bSuc)
		{
			printf ("主 机 名: %s\n", szHostName);
			printf ("系统版本: %s\n", szOSVersion);
			printf ("本 地 IP: %s\n", szClientIP);
			printf ("DNS     : %u\n", uClientDNS);
			printf ("硬盘总量: %uGB\n", uDiskSize);

			printf ("CPU 主频: %uMB\n", uCPUClockSpeed);
			printf ("CPU核心数: %u\n", uCPUCores);
			printf ("当前CPU使用率：%u%%\n", uCPUUsage);


			printf ("当前内存使用率: %2d%%\n", uMemoryLoad);

			printf ("系统物理内存总量: %uKB   %uMB   %.2fGB\n", uTotalPhys, uTotalPhys/MB_DIV, uTotalPhys/(GB_DIV*1.0));
			printf ("当前可用物理内存: %dKB   %uMB   %.2fGB\n", uAvailPhys, uAvailPhys/MB_DIV, uAvailPhys/(GB_DIV*1.0));

			printf ("系统虚拟内存总量: %uKB   %uMB   %.2fGB\n", uTotalVirtual, uTotalVirtual/MB_DIV, uTotalVirtual/(GB_DIV*1.0));
			printf ("当前空闲虚拟内存: %uKB   %uMB   %.2fGB\n", uAvailVirtual, uAvailVirtual/MB_DIV, uAvailVirtual/(GB_DIV*1.0));

			printf ("当前上传流量: %uKBps\n", uUpSpeed);
			printf ("当前下载流量：%uKBps%\n", uDownSpeed);

			printf ("\n");
		}

		Sleep(1000);	// 这里可以为任意值
	}

	return 0;
}