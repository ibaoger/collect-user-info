#pragma once
/******************************************************************************************************
项目名称：流量监测
文件名称：INetFlowWatcher.h
功    能：
作    者：
日	  期：2011-10-12
******************************************************************************************************/


/************************************************************************
lib依赖:

************************************************************************/

#include <string>
using namespace std;

class INetFlowWatcherNotify
{
public:
	virtual bool OnNotifyDownloadSpeed(float downspeed ) = 0;		// 单位：B/s
	virtual bool OnNotifyUploadSpeed(float upspeed) = 0;			// 单位：B/s
};

class INetFlowWatcher
{ 
public:
	/*
	函 数 名：	Start
	接口功能：	开始
	参    数：	无
	返 回 值：	成功、失败
	*/
	virtual bool Start() = 0;

	/*
	函 数 名：	Stop
	接口功能：	停止
	参    数：	无
	返 回 值：	成功、失败
	*/
	virtual bool Stop() = 0;


	/*
	函 数 名：	Release
	接口功能：	释放
	参    数：	无
	返 回 值：	无
	*/
	virtual void Release() = 0;
	/*
	函 数 名：	SetRefreshRate
	接口功能：	设置刷新频率
	参    数：	iRefreshFrequece(secs/time)
	返 回 值：	无
	*/
	virtual void SetRefreshRate(int iRefreshFrequece) = 0;
};


/*__declspec(dllexport)*/ INetFlowWatcher* CreateNetFlowWatcher(INetFlowWatcherNotify *pNotify);		//strError如果错误，说明错误信息

