
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
			printf ("�� �� ��: %s\n", szHostName);
			printf ("ϵͳ�汾: %s\n", szOSVersion);
			printf ("�� �� IP: %s\n", szClientIP);
			printf ("DNS     : %u\n", uClientDNS);
			printf ("Ӳ������: %uGB\n", uDiskSize);

			printf ("CPU ��Ƶ: %uMB\n", uCPUClockSpeed);
			printf ("CPU������: %u\n", uCPUCores);
			printf ("��ǰCPUʹ���ʣ�%u%%\n", uCPUUsage);


			printf ("��ǰ�ڴ�ʹ����: %2d%%\n", uMemoryLoad);

			printf ("ϵͳ�����ڴ�����: %uKB   %uMB   %.2fGB\n", uTotalPhys, uTotalPhys/MB_DIV, uTotalPhys/(GB_DIV*1.0));
			printf ("��ǰ���������ڴ�: %dKB   %uMB   %.2fGB\n", uAvailPhys, uAvailPhys/MB_DIV, uAvailPhys/(GB_DIV*1.0));

			printf ("ϵͳ�����ڴ�����: %uKB   %uMB   %.2fGB\n", uTotalVirtual, uTotalVirtual/MB_DIV, uTotalVirtual/(GB_DIV*1.0));
			printf ("��ǰ���������ڴ�: %uKB   %uMB   %.2fGB\n", uAvailVirtual, uAvailVirtual/MB_DIV, uAvailVirtual/(GB_DIV*1.0));

			printf ("��ǰ�ϴ�����: %uKBps\n", uUpSpeed);
			printf ("��ǰ����������%uKBps%\n", uDownSpeed);

			printf ("\n");
		}

		Sleep(1000);	// �������Ϊ����ֵ
	}

	return 0;
}