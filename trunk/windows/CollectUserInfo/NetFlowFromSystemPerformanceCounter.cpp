 
#include "NetFlowFromSystemPerformanceCounter.h"   
#include "float.h"   
   
#ifdef _DEBUG   
#undef THIS_FILE   
static char THIS_FILE[]=__FILE__;   
//#define new DEBUG_NEW   
#endif   

//#define  KB_DIV_BIT 
#include "winperf.h"   
   
//////////////////////////////////////////////////////////////////////   
// Construction/Destruction   
//////////////////////////////////////////////////////////////////////   
   
NetFlowFromSystemPerformanceCounter::NetFlowFromSystemPerformanceCounter()   
{   
    lasttrafficAll = 0.0;   
    lasttrafficIn = 0.0;   
    lasttrafficOut = 0.0;   
    CurrentInterface = -1;   
    CurrentTrafficType = AllTraffic;   
    GetInterfaces();   
}   
   
NetFlowFromSystemPerformanceCounter::~NetFlowFromSystemPerformanceCounter()   
{   
       
}   
   
// Little helper functions   
// Found them on CodeGuru, but do not know who has written them originally   
   
PERF_OBJECT_TYPE *FirstObject(PERF_DATA_BLOCK *dataBlock)   
{   
  return (PERF_OBJECT_TYPE *) ((BYTE *)dataBlock + dataBlock->HeaderLength);   
}   
   
PERF_OBJECT_TYPE *NextObject(PERF_OBJECT_TYPE *act)   
{   
  return (PERF_OBJECT_TYPE *) ((BYTE *)act + act->TotalByteLength);   
}   
   
PERF_COUNTER_DEFINITION *FirstCounter(PERF_OBJECT_TYPE *perfObject)   
{   
  return (PERF_COUNTER_DEFINITION *) ((BYTE *) perfObject + perfObject->HeaderLength);   
}   
   
PERF_COUNTER_DEFINITION *NextCounter(PERF_COUNTER_DEFINITION *perfCounter)   
{   
  return (PERF_COUNTER_DEFINITION *) ((BYTE *) perfCounter + perfCounter->ByteLength);   
}   
   
PERF_COUNTER_BLOCK *GetCounterBlock(PERF_INSTANCE_DEFINITION *pInstance)   
{   
  return (PERF_COUNTER_BLOCK *) ((BYTE *)pInstance + pInstance->ByteLength);   
}   
   
PERF_INSTANCE_DEFINITION *FirstInstance (PERF_OBJECT_TYPE *pObject)   
{   
  return (PERF_INSTANCE_DEFINITION *)  ((BYTE *) pObject + pObject->DefinitionLength);   
}   
   
PERF_INSTANCE_DEFINITION *NextInstance (PERF_INSTANCE_DEFINITION *pInstance)   
{   
  // next instance is after   
  //    this instance + this instances counter data   
   
  PERF_COUNTER_BLOCK  *pCtrBlk = GetCounterBlock(pInstance);   
   
  return (PERF_INSTANCE_DEFINITION *) ((BYTE *)pInstance + pInstance->ByteLength + pCtrBlk->ByteLength);   
}   
   
char *WideToMulti(wchar_t *source, char *dest, int size)   
{   
  WideCharToMultiByte(CP_ACP, 0, source, -1, dest, size, 0, 0);   
   
  return dest;   
}   
   
/*  
    returns the traffic of given interface  
*/   
double NetFlowFromSystemPerformanceCounter::GetTraffic(int interfaceNumber)   
{   
    try   
    {   
#define DEFAULT_BUFFER_SIZE 40960L   
           
        // add by lxy for debug   
        if (NetFlowFromSystemPerformanceCounter::CurrentTrafficType == 506)   
        {   
            int a = 1;   
        }   
                           
        //POSITION pos;   
        string InterfaceName;   
        /*pos = Interfaces.FindIndex(interfaceNumber);   
        if(pos==NULL)   
            return 0.0;   
        InterfaceName = Interfaces.GetAt(pos);  */ 
         InterfaceName = Interfaces[interfaceNumber];
           
        // buffer for performance data   
        unsigned char *data = new unsigned char [DEFAULT_BUFFER_SIZE];   
        // return value from RegQueryValueEx: ignored for this application   
        DWORD type;   
        // Buffer size   
        DWORD size = DEFAULT_BUFFER_SIZE;   
        // return value of RegQueryValueEx   
        DWORD ret;   
           
        // request performance data from network object (index 510)    
        while((ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "510", 0, &type, (LPBYTE)data, &size)) != ERROR_SUCCESS) {   
            if(ret == ERROR_MORE_DATA)    
            {   
                // buffer size was too small, increase allocation size   
                size += DEFAULT_BUFFER_SIZE;   
                   
                delete [] data;   
                data = new unsigned char [size];   
            }    
            else    
            {   
                // some unspecified error has occured   
                return 1;   
            }   
        }   
           
        PERF_DATA_BLOCK *dataBlockPtr = (PERF_DATA_BLOCK *)data;   
           
        // enumerate first object of list   
        PERF_OBJECT_TYPE *objectPtr = FirstObject(dataBlockPtr);   
           
        // trespassing the list    
        for(int a=0 ; a<(int)dataBlockPtr->NumObjectTypes ; a++)    
        {   
            char nameBuffer[255];   
               
            // did we receive a network object?   
            if(objectPtr->ObjectNameTitleIndex == 510) //zz   
            {   
                // Calculate the offset   
                DWORD processIdOffset = ULONG_MAX;   
                   
                // find first counter    
                PERF_COUNTER_DEFINITION *counterPtr = FirstCounter(objectPtr);   
                   
                // walking trough the list of objects   
                for(int b=0 ; b<(int)objectPtr->NumCounters ; b++)    
                {   
                    // Check if we received datatype wished   
                    if((int)counterPtr->CounterNameTitleIndex == CurrentTrafficType)   
                        processIdOffset = counterPtr->CounterOffset;   
                       
                    // watch next counter   
                    counterPtr = NextCounter(counterPtr);   
                }   
                   
                if(processIdOffset == ULONG_MAX) {   
                    delete [] data;   
                    return 1;   
                }   
                   
                   
                // Find first instance   
                PERF_INSTANCE_DEFINITION *instancePtr = FirstInstance(objectPtr);   
                   
                DWORD fullTraffic;   
                DWORD traffic;   
                for(int b=0 ; b<objectPtr->NumInstances ; b++)    
                {   
                    // evaluate pointer to name   
                    wchar_t *namePtr = (wchar_t *) ((BYTE *)instancePtr + instancePtr->NameOffset);   
                       
                    // get PERF_COUNTER_BLOCK of this instance   
                    PERF_COUNTER_BLOCK *counterBlockPtr = GetCounterBlock(instancePtr);   
                       
                    // now we have the interface name   
                       
                    char *pName = WideToMulti(namePtr, nameBuffer, sizeof(nameBuffer));   
                    /*CString iName;   
                    iName.Format("%s",pName);  */ 
					string iName = pName;
                   
                  /*  POSITION pos = TotalTraffics.FindIndex(b);   
                    if(pos!=NULL)   
                    {   
                        fullTraffic = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));   
                        TotalTraffics.SetAt(pos,fullTraffic);   
                    }  */ 
   
					if (b < TotalTraffics.size())
					{
						fullTraffic = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));   
						TotalTraffics[b] = fullTraffic;   
					}
                    // If the interface the currently selected interface?   
                    if(InterfaceName == iName)   
                    {   
                        traffic = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));   
                        double acttraffic = (double)traffic;   
                        double trafficdelta;   
                        // Do we handle a new interface (e.g. due a change of the interface number   
                        if(CurrentInterface != interfaceNumber)   
                        {   
                            switch(CurrentTrafficType) {   
                            case IncomingTraffic:   
                                lasttrafficIn = acttraffic;   
                                trafficdelta = 0.0;   
                                break;   
                            case OutGoingTraffic:   
                                lasttrafficOut = acttraffic;   
                                trafficdelta = 0.0;   
                                break;   
                            default:   
                                lasttrafficAll = acttraffic;   
                                trafficdelta = 0.0;   
                            }   
   
                            CurrentInterface = interfaceNumber;   
                        }   
                        else   
                        {   
                            switch(CurrentTrafficType) {   
                            case IncomingTraffic:   
                               /* trafficdelta = acttraffic - lasttrafficIn;   
                                lasttrafficIn = acttraffic;  */ 
								trafficdelta = acttraffic;
                                break;   
                            case OutGoingTraffic:   
                                /*trafficdelta = acttraffic - lasttrafficOut;   
                                lasttrafficOut = acttraffic;  */ 
								trafficdelta = acttraffic;
                                break;   
                            default:   
                                trafficdelta = acttraffic - lasttrafficAll;   
                                lasttrafficAll = acttraffic;   
                            }   
   
   
                        }   
   
                        delete [] data;   
                        return(trafficdelta);   
                    }   
                       
                    // next instance   
                    instancePtr = NextInstance(instancePtr);   
                }   
            }   
               
            // next object in list   
            objectPtr = NextObject(objectPtr);   
        }   
           
        delete [] data;   
        return 0;   
    }   
   
    catch(...)   
    {   
        return 0;   
    }   
}   
   
/*  
    Enumerate installed interfaces.   
    See comments above  
*/   
BOOL NetFlowFromSystemPerformanceCounter::GetInterfaces()   
{   
    try   
    {   
#define DEFAULT_BUFFER_SIZE 40960L   
           
        //Interfaces.RemoveAll();   
		Interfaces.clear();
        unsigned char *data = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);   
        DWORD type;   
        DWORD size = DEFAULT_BUFFER_SIZE;   
        DWORD ret;   
           
        char s_key[4096];   
        sprintf( s_key , "%d" , 510 );   
           
        while((ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, s_key, 0, &type, data, &size)) != ERROR_SUCCESS) {   
            while(ret == ERROR_MORE_DATA)    
            {   
                size += DEFAULT_BUFFER_SIZE;   
                data = (unsigned char*) realloc(data, size);   
            }    
            if(ret != ERROR_SUCCESS)   
            {   
                return FALSE;   
            }   
        }   
           
        PERF_DATA_BLOCK  *dataBlockPtr = (PERF_DATA_BLOCK *)data;   
        PERF_OBJECT_TYPE *objectPtr = FirstObject(dataBlockPtr);   
           
        for(int a=0 ; a<(int)dataBlockPtr->NumObjectTypes ; a++)    
        {   
            char nameBuffer[255];   
            if(objectPtr->ObjectNameTitleIndex == 510)    
            {   
                DWORD processIdOffset = ULONG_MAX;   
                PERF_COUNTER_DEFINITION *counterPtr = FirstCounter(objectPtr);   
                   
                for(int b=0 ; b<(int)objectPtr->NumCounters ; b++)    
                {   
                    if(counterPtr->CounterNameTitleIndex == 520)			//@@ Network object counter index:520
                        processIdOffset = counterPtr->CounterOffset;   
                       
                    counterPtr = NextCounter(counterPtr);   
                }   
                   
                if(processIdOffset == ULONG_MAX) {   
                    free(data);   
                    return 1;   
                }   
                   
                PERF_INSTANCE_DEFINITION *instancePtr = FirstInstance(objectPtr);   
                   
                for(int b=0 ; b<objectPtr->NumInstances ; b++)    
                {   
                    wchar_t *namePtr = (wchar_t *) ((BYTE *)instancePtr + instancePtr->NameOffset);   
                    PERF_COUNTER_BLOCK *counterBlockPtr = GetCounterBlock(instancePtr);   
                    char *pName = WideToMulti(namePtr, nameBuffer, sizeof(nameBuffer));   
                    DWORD bandwith = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));                 
                    DWORD tottraff = 0;   
   
                    //Interfaces.AddTail(CString(pName));   
                    //Bandwidths.AddTail(bandwith);   
                    //TotalTraffics.AddTail(tottraff);  // initial 0, just for creating the list   
					Interfaces.push_back(pName);
                     //Interfaces.push_back(CString(pName));
					 Bandwidths.push_back(bandwith);
					 TotalTraffics.push_back(tottraff);

                    instancePtr = NextInstance(instancePtr);   
                }   
            }   
            objectPtr = NextObject(objectPtr);   
        }   
        free(data);   
           
        return TRUE;   
    }   
    catch(...)   
    {   
        return FALSE;   
    }   
}   
   
/*  
    Returns the count of installed interfaces  
*/   
int NetFlowFromSystemPerformanceCounter::GetNetworkInterfacesCount()   
{   
    //return Interfaces.GetCount()-1;   
	return Interfaces.size();
}   
   
/*  
    Returns the name of the given interface (-number)  
*/   
BOOL NetFlowFromSystemPerformanceCounter::GetNetworkInterfaceName(string& InterfaceName, int index)   
{   
    /*POSITION pos = Interfaces.FindIndex(index);   
    if(pos==NULL)   
        return FALSE;  */ 
   
    //InterfaceName->Format("%s",Interfaces.GetAt(pos));  
	InterfaceName =  Interfaces[index];
	//InterfaceName->Format("%s",Interfaces[index]); 
    return TRUE;   
}   
   
/*  
    Returns bandwith of interface e.g. 100000 for 100MBit  
*/   
DWORD NetFlowFromSystemPerformanceCounter::GetInterfaceBandwidth(int index)   
{   
   /* POSITION pos = Bandwidths.FindIndex(index);   
    if(pos==NULL)   
        return 0;   
   
    else    
        return Bandwidths.GetAt(pos) / 8;   */
	if (index >= Bandwidths.size())
	{
		return 0;  
	}
	return Bandwidths[index];
   
}   
   
/*  
    Sometime it is nice to know, how much traffic has a specific interface sent and received  
*/   
   
DWORD NetFlowFromSystemPerformanceCounter::GetInterfaceTotalTraffic(int index)   
{   
    DWORD       totaltraffic = 0;   
   /* POSITION    pos;   
    pos= TotalTraffics.FindIndex(index);   
    if(pos!=NULL)   
    {   
        totaltraffic = TotalTraffics.GetAt(pos);    
        if(totaltraffic == 0.0)   
        {   
            GetTraffic(index);   
            pos= TotalTraffics.FindIndex(index);   
            if(pos!=NULL)   
            {   
                totaltraffic = TotalTraffics.GetAt(pos);    
            }   
        }   
    }   */
	if (index < TotalTraffics.size())
	{
		totaltraffic = TotalTraffics[index];
		if(totaltraffic == 0.0)   
		{   
			GetTraffic(index);   
			totaltraffic = TotalTraffics[index];    
		}
	}
   
    return(totaltraffic);   
}   

DWORD NetFlowFromSystemPerformanceCounter::GetInterfaceUpTraffic(int interfaceNumber)
{
	SetTrafficType(OutGoingTraffic);
	return (DWORD)(GetTraffic(interfaceNumber)/1024);
}
DWORD NetFlowFromSystemPerformanceCounter::GetInterfaceDownTraffic(int interfaceNumber)
{
	SetTrafficType(IncomingTraffic);
	return (DWORD)(GetTraffic(interfaceNumber)/1024);
}
/*  
    To prevent direct manipulation of member variables....  
*/   
void NetFlowFromSystemPerformanceCounter::SetTrafficType(int trafficType)   
{   
    CurrentTrafficType = trafficType;   
}   
   
/*  
    add by lxy  
    返回指定接口的流量整体描述  
*/   
int NetFlowFromSystemPerformanceCounter::GetFullDesribe(const int intfIndex,char *data)   
{      
    int conInters;   
   
    double intfBandw ;   
    double preInTraf,preOutTraf,preTotalTraf;   
    double newInTraf,newOutTraf,newTotalTraf;   
    string sInterName,sInterNameForm,sInTraf,sOutTraf,sTotalTraf;    
    string sFullDesribe;   
   
   
    conInters = GetNetworkInterfacesCount();       
    if (intfIndex >= conInters)   
    {   
        data = "";   
        return 0;   
    }   
       
    intfBandw= GetInterfaceBandwidth(intfIndex) / MB_DIV_BIT;   
           
    GetNetworkInterfaceName(sInterName, intfIndex);   
    sInterNameForm = sInterName;   
           
           
    // 输出   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::OutGoingTraffic);   
    //GetTraffic(intfIndex);   
    //GetTraffic(intfIndex);   
    preOutTraf = GetTraffic(intfIndex) / KB_DIV_BIT;   
   
    // 输入   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::IncomingTraffic);   
    GetTraffic(intfIndex);   
    //GetTraffic(intfIndex);   
    preInTraf = GetTraffic(intfIndex) / KB_DIV_BIT;   
   
    // 总   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::AllTraffic);   
    //GetTraffic(intfIndex);   
    //GetTraffic(intfIndex);   
    preTotalTraf = GetInterfaceTotalTraffic(intfIndex) / KB_DIV_BIT;   
           
    Sleep(1000);   
           
    // 输出   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::OutGoingTraffic);         
    newOutTraf = GetTraffic(intfIndex) / KB_DIV_BIT;   
    newOutTraf = newOutTraf - preOutTraf;   
           
    // 输入   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::IncomingTraffic);   
    newInTraf = GetTraffic(intfIndex) / KB_DIV_BIT ;   
    newInTraf = newInTraf - preInTraf;   
   
    // 总   
    SetTrafficType(NetFlowFromSystemPerformanceCounter::AllTraffic);   
    newTotalTraf = GetInterfaceTotalTraffic(intfIndex) / KB_DIV_BIT;       
    newTotalTraf = newTotalTraf - preTotalTraf;   
           
    /*sFullDesribe.Format("Interface Name:%s, BandWidth:%.1fMb/s, incoming Traffic:%.1fkb/s, Out Traffic:%.1fkb/s"   
                            ,sInterNameForm.GetBuffer(sizeof(sInterNameForm)),intfBandw,newInTraf,newOutTraf);   */
	//sFullDesribe = "Interface Name:" + sInterNameForm + ", BandWidth:" + 
 //      
 //   if ( strlen(data) < strlen(sFullDesribe.GetBuffer(sizeof(sFullDesribe)))+1 )   
 //       return -1;   
   
    //strcpy(data,sFullDesribe.GetBuffer(sizeof(sFullDesribe)) );   
   
    return 0;   
}  