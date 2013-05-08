#pragma once
/******************************************************************************************************
��Ŀ���ƣ��������
�ļ����ƣ�INetFlowWatcher.h
��    �ܣ�
��    �ߣ�
��	  �ڣ�2011-10-12
******************************************************************************************************/


/************************************************************************
lib����:

************************************************************************/

#include <string>
using namespace std;

class INetFlowWatcherNotify
{
public:
	virtual bool OnNotifyDownloadSpeed(float downspeed ) = 0;		// ��λ��B/s
	virtual bool OnNotifyUploadSpeed(float upspeed) = 0;			// ��λ��B/s
};

class INetFlowWatcher
{ 
public:
	/*
	�� �� ����	Start
	�ӿڹ��ܣ�	��ʼ
	��    ����	��
	�� �� ֵ��	�ɹ���ʧ��
	*/
	virtual bool Start() = 0;

	/*
	�� �� ����	Stop
	�ӿڹ��ܣ�	ֹͣ
	��    ����	��
	�� �� ֵ��	�ɹ���ʧ��
	*/
	virtual bool Stop() = 0;


	/*
	�� �� ����	Release
	�ӿڹ��ܣ�	�ͷ�
	��    ����	��
	�� �� ֵ��	��
	*/
	virtual void Release() = 0;
	/*
	�� �� ����	SetRefreshRate
	�ӿڹ��ܣ�	����ˢ��Ƶ��
	��    ����	iRefreshFrequece(secs/time)
	�� �� ֵ��	��
	*/
	virtual void SetRefreshRate(int iRefreshFrequece) = 0;
};


/*__declspec(dllexport)*/ INetFlowWatcher* CreateNetFlowWatcher(INetFlowWatcherNotify *pNotify);		//strError�������˵��������Ϣ

