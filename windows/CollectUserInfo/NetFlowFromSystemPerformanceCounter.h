#pragma once
#include <atlstr.h>
#include <atlsimpstr.h>
#include <vector>
#include <string>
using std::vector;
using std::string;

#define KB_DIV_BIT 1024.0 
#define MB_DIV_BIT (KB_DIV_BIT * KB_DIV_BIT) 

#define SAMPLING_INTERVAL_SECOND 1  
#define SLEEP_INTERVAL (SAMPLING_INTERVAL_SECOND*1000)

class NetFlowFromSystemPerformanceCounter   
{ 
public: 
	enum TrafficType  
	{ 
		AllTraffic		= 388, 
		IncomingTraffic	= 264, 
		OutGoingTraffic	= 506 
	}; 

	void SetTrafficType(int trafficType); 
	DWORD	GetInterfaceTotalTraffic(int index); 
	BOOL	GetNetworkInterfaceName(string& InterfaceName, int index); 
	int		GetNetworkInterfacesCount(); 
	double	GetTraffic(int interfaceNumber); 

	DWORD	GetInterfaceBandwidth(int index); 

	DWORD GetInterfaceUpTraffic(int index);
	DWORD GetInterfaceDownTraffic(int index);
	// add by lxy 
	int NetFlowFromSystemPerformanceCounter::GetFullDesribe(const int intfIndex,char *data); 

	NetFlowFromSystemPerformanceCounter(); 
	virtual ~NetFlowFromSystemPerformanceCounter(); 
private: 
	BOOL		GetInterfaces();

	//CStringList Interfaces; 
	//CList < DWORD, DWORD &>		Bandwidths; 
	//CList < DWORD, DWORD &>		TotalTraffics; 
	//vector<CString>   Interfaces;
	vector<string>   Interfaces;
	vector<DWORD> Bandwidths;
	vector<DWORD> TotalTraffics;

	int CurrentInterface; 
	int CurrentTrafficType; 

	// add by lxy 
	double		lasttrafficAll; 
	double		lasttrafficIn; 
	double		lasttrafficOut; 


}; 
