
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "NetFlowWatcherLib.h"

/*__declspec(dllexport)*/ INetFlowWatcher* CreateNetFlowWatcher(INetFlowWatcherNotify *pNotify)
{
	return new NetFlowWatcher(pNotify);
}

//@@ get local netcard list
int NetFlowWatcher::GetLoaclNetCard()
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	ULONG ulOutBufLen;
	pAdapterInfo=(PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// 第一次调用GetAdapterInfo获取ulOutBufLen大小
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	}

	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) 
	{
		pAdapter = pAdapterInfo;
		while (pAdapter) 
		{
			NetCardInfo netcard;
			try
			{
				/*netcard.pNetCardId = new char [strlen(pAdapter->AdapterName)];
				netcard.pNetCardDescription = new char [strlen(pAdapter->Description)];*/
				memcpy(netcard.NetCardId, pAdapter->AdapterName, strlen(pAdapter->AdapterName));
				memcpy(netcard.NetCardDescription, pAdapter->Description, strlen(pAdapter->Description));
				m_vecNetCardnIfo.push_back(netcard);
			}
			catch (...)
			{
				return -1;
			}
			
			pAdapter = pAdapter->Next;
		}
	}
	return 0;
}
int NetFlowWatcher::GetNetFlowByOsVersion()
{
	OSVERSIONINFOEX os;
	os.dwOSVersionInfoSize=sizeof(os);
	if(!GetVersionEx((OSVERSIONINFO *)&os))
	{
		return -1;
	}
	if(os.dwMajorVersion == 5){ //5.0 :windows 2000 ,5.1:win xp
		//GetNetStreamSpeedForV6();
		GetNetStreamSpeedForV5();
	}
	else  //6.0 :windows vista ,6.1:win 7
	{
		//GetNetStreamSpeedForV6();
		GetNetStreamSpeedForV5();
		//GetNetStreamSpeedForV5_2();
	}
	return 0;
}


//for getiftable
int NetFlowWatcher::GetNetStreamSpeedForV5()
{
	/* variables used for GetIfTable and GetIfEntry */
	MIB_IFTABLE *pIfTable = NULL;
	MIB_IFROW *pIfRow = NULL;
	DWORD dwSize;
	DWORD dwRetVal;
	DWORD inFlow = 0;
	DWORD outFlow = 0;
	// Allocate memory for our pointers.
	pIfTable = (MIB_IFTABLE *) malloc(sizeof (MIB_IFTABLE));
	if (pIfTable == NULL) {
		//printf("Error allocating memory needed to call GetIfTable\n");
		exit (1);
	}
	// Before calling GetIfEntry, we call GetIfTable to make
	// sure there are entries to get and retrieve the interface index.

	// Make an initial call to GetIfTable to get the
	// necessary size into dwSize
	dwSize = sizeof (MIB_IFTABLE);
	if (GetIfTable(pIfTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
		free(pIfTable);
		pIfTable = (MIB_IFTABLE *) malloc(dwSize);
		if (pIfTable == NULL) {
			//printf("Error allocating memory\n");
			exit (1);
		}
	}
	// Make a second call to GetIfTable to get the actual
	// data we want.
	if ((dwRetVal = GetIfTable(pIfTable, &dwSize, 0)) == NO_ERROR) 
	{
		if (pIfTable->dwNumEntries > 0) {
			pIfRow = (MIB_IFROW *) malloc(sizeof (MIB_IFROW));
			if (pIfRow == NULL) {
				//printf("Error allocating memory\n");
				if (pIfTable != NULL) {
					free(pIfTable);
					pIfTable = NULL;
				}
				exit (1);
			}

			//printf("\tNum Entries: %ld\n\n", pIfTable->dwNumEntries);
			for (unsigned int i = 0; i < pIfTable->dwNumEntries; i++) {
				pIfRow->dwIndex = pIfTable->table[i].dwIndex;
				if ((dwRetVal = GetIfEntry(pIfRow)) == NO_ERROR) {
					if (pIfRow->dwType != MIB_IF_TYPE_LOOPBACK)
					{
						for (unsigned int ix = 0; ix < m_vecNetCardnIfo.size(); ix++)
						{
							if (memcmp(pIfRow->bDescr, m_vecNetCardnIfo[ix].NetCardDescription, pIfRow->dwDescrLen-1) == 0)
							{
								inFlow += pIfRow->dwInOctets;  
								outFlow += pIfRow->dwOutOctets;
							}
						}
						
					}
					
				}

				else {
					/*printf("GetIfEntry failed for index %d with error: %ld\n",
						i, dwRetVal);*/
					// Here you can use FormatMessage to find out why 
					// it failed.

				}
			}
		} else {
			//printf("\tGetIfTable failed with error: %ld\n", dwRetVal);
		}

	}

	//
	if (m_FirstCount == true)
	{
		m_pNotify->OnNotifyDownloadSpeed((float)(inFlow-m_dwPreInFlow)/m_iRefreshRate);
		m_pNotify->OnNotifyUploadSpeed((float)(outFlow-m_dwPreOutFlow)/m_iRefreshRate);
	}

	m_FirstCount = true;
	m_dwPreInFlow = inFlow;
	m_dwPreOutFlow = outFlow;
	free(pIfTable);  
	free(pIfRow); 

	return 0;
}
void NetFlowWatcher::ThreadProcMain(void)
{
	while( m_bThreadControlStart )
	{
		//GetNetStreamSpeed();
		GetNetFlowByOsVersion();
		Sleep(1000*m_iRefreshRate);
		//Sleep(3000);
	}
	
}
bool NetFlowWatcher::Start() 
{
	//GetNetStreamSpeed();
	m_bThreadControlStart = true;
	StartThread();
	return true;
}
bool NetFlowWatcher::Stop() 
{
	m_bThreadControlStart = false;
	WaitForStop();
	 return true;
}
void NetFlowWatcher::Release() 
{
	delete this;
}
//@@ How many seconds to refresh
void NetFlowWatcher::SetRefreshRate(int iRefreshFrequece)
{
	if (iRefreshFrequece != 0)
	{
		m_iRefreshRate = iRefreshFrequece;
	}

}
NetFlowWatcher::~NetFlowWatcher()
{
	m_bThreadControlStart = false;
	WaitForStop();
}
NetFlowWatcher::NetFlowWatcher(INetFlowWatcherNotify *pNotify)
: m_iRefreshRate(1)
, m_bThreadControlStart(false)
, m_dwPreInFlow(0)
, m_dwPreOutFlow(0)
, m_pNotify(pNotify)
, m_FirstCount(false)
{
	GetLoaclNetCard();
}