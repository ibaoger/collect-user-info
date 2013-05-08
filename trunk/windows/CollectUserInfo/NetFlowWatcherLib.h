#pragma once

#include "TVThread.h"
#include "INetFlowWatcher.h"
#include "NetFlowFromSystemPerformanceCounter.h"

class NetFlowWatcher
	: public INetFlowWatcher
	, public CTVThread
{
public:
	NetFlowWatcher(INetFlowWatcherNotify *pNotify);
	~NetFlowWatcher();

	//INetFlowWatcher
	virtual bool Start() ;
	virtual bool Stop() ;
	virtual void Release() ;
	virtual void SetRefreshRate(int iRefreshFrequece) ;

	//TVThread
	virtual void ThreadProcMain(void);

	//int GetNetStreamSpeedForV5_2();
	int GetNetStreamSpeedForV5();		//@@ fit for win 2000 and xp
	//int GetNetStreamSpeedForV6();		//@@ fit for win vista and win7
	int GetNetFlowByOsVersion();
	int GetLoaclNetCard();
private:
	int m_iRefreshRate;
	bool m_bThreadControlStart;
	DWORD m_dwPreInFlow;
	DWORD m_dwPreOutFlow;
	INetFlowWatcherNotify *m_pNotify;
	bool m_FirstCount;								//@@ data first got is incorrect.

	NetFlowFromSystemPerformanceCounter m_NetFlowFromPerformanceCounter;

	struct NetCardInfo
	{
		/*NetCardInfo():pNetCardId(NULL), pNetCardDescription(NULL)
		{}
		~NetCardInfo()
		{
			if (pNetCardId)
			{
				delete [] pNetCardId;
				pNetCardId = NULL;
			}
			if (pNetCardId)
			{
				delete [] pNetCardDescription;
				pNetCardDescription = NULL;
			}
		}*/
		char  NetCardId[255];
		char  NetCardDescription[255];
	};
	vector<NetCardInfo> m_vecNetCardnIfo;
};