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
	DWORD	m_dwThreadID;		// �̱߳�ʶ
	HANDLE	m_hThread;			// �߳̾��
	HANDLE	m_evStop;			// �˳��¼�
};





class CSafeThread
	: public CTVThread
{
public:
	CSafeThread();
	virtual ~CSafeThread();

	void SafeStartThread();						//�����̣߳�����Ѿ�������������
	void SafeStopThread();

	virtual void SafeThreadProcMain()=0;

protected:
	virtual void	ThreadProcMain();
	bool			m_bThreadControlStart;		//�߳̿��ƣ��������̳��಻�ܹ�����
	CTVCritSec		m_lockData;
};

#endif
