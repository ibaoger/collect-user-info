// TVThread.h: interface for the CTVThreadclass.
//
//////////////////////////////////////////////////////////////////////

#ifndef __TV_THREAD_H__
#define __TV_THREAD_H__

#include "TVCritSec.h"

class CTVThread
{
public:
	CTVThread();
	virtual ~CTVThread();
public:
	bool StartThread(void);
	void WaitForStop(void);

	static DWORD WINAPI InitThreadProc(PVOID pObj){
		return	((CTVThread*)pObj)->ThreadProc();
	}
	
	unsigned long ThreadProc(void);

protected:
	virtual void ThreadProcMain(void)=0;
protected:
	DWORD	m_dwThreadID;		// 线程标识
	HANDLE	m_hThread;			// 线程句柄
	HANDLE	m_evStop;			// 退出事件
};





class CSafeThread
	: public CTVThread
{
public:
	CSafeThread();
	virtual ~CSafeThread();

	void SafeStartThread();						//启动线程，如果已经启动，则重启
	void SafeStopThread();

	virtual void SafeThreadProcMain()=0;

protected:
	virtual void	ThreadProcMain();
	bool			m_bThreadControlStart;		//线程控制，启动，继承类不能够更改
	CTVCritSec		m_lockData;
};

#endif
