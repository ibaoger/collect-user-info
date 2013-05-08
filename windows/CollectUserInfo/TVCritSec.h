//TVCritSec.h
//
#ifndef _TV_CRITSEC_H
#define _TV_CRITSEC_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
/************************************************************************
e.g.

CTVCritSec m_lockData;			//定义变量

CTVAutoLock l(&m_lockData);		//加锁
************************************************************************/

class CTVCritSec
{
public:
    CTVCritSec() {
        InitializeCriticalSection(&m_CritSec);
    };

    ~CTVCritSec() {
        DeleteCriticalSection(&m_CritSec);
    };

    void Lock() {
        EnterCriticalSection(&m_CritSec);
    };

    void Unlock() {
        LeaveCriticalSection(&m_CritSec);
    };
protected:
    CRITICAL_SECTION m_CritSec;
};



class CTVAutoLock
{
public:
    CTVAutoLock(CTVCritSec * plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ~CTVAutoLock() {
        m_pLock->Unlock();
    };
protected:
    CTVCritSec * m_pLock;
};


class CTVEvent
{
public:
    CTVEvent(bool bManualReset = false)
	{
	    m_hEvent = CreateEvent(NULL, bManualReset, FALSE, NULL);
	};

    virtual ~CTVEvent()
	{
		if(m_hEvent) 
		{
			CloseHandle(m_hEvent);
			m_hEvent=NULL;
		}		
	};
public:
    void Set(void) 
	{
		SetEvent(m_hEvent);
	};

    bool Wait(unsigned long ulTimeout)
	{
		if (ulTimeout==0)
			ulTimeout=-1;

		return (WaitForSingleObject(m_hEvent, ulTimeout) == WAIT_OBJECT_0);
    };

    void Reset(void) 
	{ 
		ResetEvent(m_hEvent); 
	};
protected:
	HANDLE m_hEvent;

};

#endif	// _TV_CRITSEC_H