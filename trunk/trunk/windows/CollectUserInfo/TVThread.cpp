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
	//����߳��Ѿ�����������Ҫ�ٴ���
	if (!m_hThread){ 
		//�����������̺߳���
		m_hThread = CreateThread(
					NULL,			//the handle cannot be inherited. 
                    0,				//the default Thread Stack Size is set
                    InitThreadProc,	//�̺߳���
                    this,			//�̺߳����Ĳ���
                    0,				//ʹ�̺߳������������������
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

	// �����߳̾��
	HANDLE hThread = (HANDLE)InterlockedExchange((LONG *)&m_hThread, 0);
	if (hThread) {
		// �ȴ��߳���ֹ
		WaitForSingleObject(hThread, INFINITE);
		// �ر��߳̾��
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
	if (m_bThreadControlStart)			//�Ѿ�ִ�������߳�
	{
		m_bThreadControlStart = false;
		WaitForStop();

		m_bThreadControlStart = true;
		StartThread();
	} 
	else								//��ûִ����������
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
