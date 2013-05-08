// TVThread.cpp: implementation of the CTVThread class.
//
//////////////////////////////////////////////////////////////////////


#include <windows.h>
#include "TVThread.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTVThread::CTVThread()
{
	m_dwThreadID=0;
	m_hThread=NULL;
	m_evStop = CreateEvent(NULL,TRUE,TRUE,NULL);
	SetEvent(m_evStop);
}

CTVThread::~CTVThread()
{
	CloseHandle(m_evStop);
	m_evStop = NULL;
}

bool CTVThread::StartThread()
{
	//如果线程已经创建，则不需要再创建
	if (!m_hThread){ 
		//创建并启动线程函数
		m_hThread = CreateThread(
					NULL,			//the handle cannot be inherited. 
                    0,				//the default Thread Stack Size is set
                    InitThreadProc,	//线程函数
                    this,			//线程函数的参数
                    0,				//使线程函数创建完后立即启动
                    &m_dwThreadID	//receives the thread identifier
					);
                
        }//end if (!m_hThread...

	if (m_hThread) {
		ResetEvent(m_evStop);
	}

	return	m_hThread != NULL;
}

void CTVThread::WaitForStop()
{
	WaitForSingleObject(m_evStop,INFINITE);

	// 返回线程句柄
	HANDLE hThread = (HANDLE)InterlockedExchange((LONG *)&m_hThread, 0);
	if (hThread) {
		// 等待线程终止
		WaitForSingleObject(hThread, INFINITE);
		// 关闭线程句柄
		CloseHandle(hThread);
		m_hThread = NULL;
	}// end if (hThread...
}

unsigned long  CTVThread::ThreadProc()
{
	ThreadProcMain();
	SetEvent(m_evStop);

	return 0;
}




CSafeThread::CSafeThread()
: m_bThreadControlStart(false)
{
}

CSafeThread::~CSafeThread()
{
}


void CSafeThread::SafeStartThread()
{
	if (m_bThreadControlStart)			//已经执行启动线程
	{
		m_bThreadControlStart = false;
		WaitForStop();

		m_bThreadControlStart = true;
		StartThread();
	} 
	else								//还没执行启动命令
	{
		m_bThreadControlStart = true;
		StartThread();
	}
}

void CSafeThread::SafeStopThread()
{
	if (!m_bThreadControlStart)
	{
		return;
	}
	m_bThreadControlStart = false;
	WaitForStop();
}

void CSafeThread::ThreadProcMain()
{
	this->SafeThreadProcMain();
}
