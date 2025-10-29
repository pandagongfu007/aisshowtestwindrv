/**********************************************************
*  File CHR44X02_lib.h
*  DESCRIPTION CHR44X02 Windows Dynamic Link Library
*********************************************************/
#ifndef _CHR44X02_LIB_H_
#define _CHR44X02_LIB_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DLL 
#define DLL __declspec(dllimport)
#endif
#pragma comment(lib,"CHR44X02.lib")

#ifndef _PASCAL 
#define _PASCAL  __stdcall
#endif

#ifndef __CHR_DEVBUSST__
#define __CHR_DEVBUSST__
typedef struct _CHR_DEVBUSST_
{
	WORD wdDevID;			
	WORD wdVenID;
	WORD wdSubDevID;
	WORD wdSubVenID;
	WORD  wdBusNum;			//总线号	
	WORD  wdDevNum;			//设备号
	WORD  wdFunNum;			//功能号
	WORD  wdIrqNum;			//中断号
	ULONG dwMemBase[6][2];//[1]表示起始地址,[0]表示长度(长度为0表示没有)
}CHR_DEVBUSST, *pCHR_DEVBUSST;
#endif

#ifndef  __CHR_DEVPARST__
#define  __CHR_DEVPARST__
typedef struct  _CHR_DEVPARST_
{
	DWORD dwCardType;
	DWORD dwhwVersion;
	DWORD dwdvrVersion; 
	DWORD dwlibVersion;
	DWORD dwBoardID;
	DWORD dwSN;
	DWORD dwChMax;
}CHR_DEVPARST, *pCHR_DEVPARST;
#endif

#ifndef __TRIGOUT_CFG_ST__
#define __TRIGOUT_CFG_ST__
typedef struct _TRIGOUT_CFG_ST_
{
	BOOL  TrigEn;       
	BOOL  OutputEn;      //输出到外部 0:禁止，1:使能
	DWORD TrigLevel;     //触发电平 1:上升沿, 2:下降沿
	DWORD timeHigh;      //高电平宽度，分辨率为1us
	DWORD timeLow;       //低电平宽度，分辨率为1us
	DWORD idleLevel;     //空闲电平，0:低电平，1:高电平
	DWORD OutputCnt;     //输出个数，0:无限次输出，非0:有限输出个数
}TRIGOUT_CFG_ST, *pTRIGOUT_CFG_ST; 
#endif


/***********************************************************
************************ 板卡操作 **************************
************************************************************/
DLL int _PASCAL CHR44X02_OpenDev(HANDLE *phDev,BYTE devId);
DLL int _PASCAL CHR44X02_OpenDevEx(HANDLE *phDev,BYTE busNo,BYTE devNo,BYTE FuncNo);
DLL int _PASCAL CHR44X02_CloseDev(HANDLE hDev);
DLL int _PASCAL CHR44X02_ResetDev(HANDLE hDev);
DLL int _PASCAL CHR44X02_Get_DevBusInfo(HANDLE hDev,pCHR_DEVBUSST pstBusinfo);
DLL int _PASCAL CHR44X02_Get_DevParInfo(HANDLE hDev,pCHR_DEVPARST pstParInfo);
DLL int _PASCAL CHR44X02_GetDevSn(HANDLE hDev,DWORD *dwSn);

/***********************************************************
************************ 离散量输入 ************************
************************************************************/
DLL int _PASCAL CHR44X02_IO_GetInputStatus(HANDLE hDev,BYTE Channel,BYTE *pStatus); 
DLL int _PASCAL CHR44X02_IO_GetInputStatus_Multi(HANDLE hDev,DWORD pStatus[]); 

DLL int _PASCAL CHR44X02_IO_SetDIFtTime(HANDLE hDev,DWORD Time);

DLL int _PASCAL CHR44X02_IO_SetDIEdge(HANDLE hDev,BYTE Channel,BYTE DIEdge);
DLL int _PASCAL CHR44X02_IO_CreateDIEdgeEvent(HANDLE hDev, HANDLE *pEvent);
DLL DWORD _PASCAL CHR44X02_IO_WaitDIEdgeEvent(HANDLE hDev,HANDLE hEvent,DWORD dwMilliseconds);
DLL int _PASCAL CHR44X02_IO_CloseDIEdgeEvent(HANDLE hDev, HANDLE hEvent);
DLL int _PASCAL CHR44X02_IO_ReadDIEdgeStatus(HANDLE hDev, DWORD *pStatus);
DLL int _PASCAL CHR44X02_IO_ReadDIEdgeStatus_ByCh(HANDLE hDev,BYTE Channel, DWORD *pStatus);

/***********************************************************
************************ 离散量输出 ************************
************************************************************/
DLL int _PASCAL CHR44X02_IO_SetOutputStatus(HANDLE hDev,BYTE Channel,BYTE Status);         
DLL int _PASCAL CHR44X02_IO_GetOutputStatus(HANDLE hDev,BYTE Channel,BYTE *pStatus); 

DLL int _PASCAL CHR44X02_IO_SetOutputStatus_Multi(HANDLE hDev,DWORD Status[]);         
DLL int _PASCAL CHR44X02_IO_GetOutputStatus_Multi(HANDLE hDev,DWORD pStatus[]); 


//PXI触发功能
DLL int __stdcall CHR44X02_SetWorkMode(HANDLE hDev,BYTE WorkMode);
DLL int __stdcall CHR44X02_SetTrigLine(HANDLE hDev,BYTE TrigLine);
DLL int __stdcall CHR44X02_TrigOut_Reset(HANDLE hDev);
DLL int __stdcall CHR44X02_TrigOut_Config(HANDLE hDev, BYTE ClkSrc, BYTE TrigLine, pTRIGOUT_CFG_ST pstTrigOut);
DLL int __stdcall CHR44X02_TrigOut_Enable(HANDLE hDev,BOOL Enabled);
DLL int __stdcall CHR44X02_TrigOut_GetStatus(HANDLE hDev,BYTE *pStatus);
//DLL int __stdcall CHR44X02_TrigOut_Mode(HANDLE hDev,BYTE Mode);

DLL int __stdcall CHR44X02_TrigIn_Config(HANDLE hDev, BYTE TrigLine,BYTE Edge);
DLL int __stdcall CHR44X02_TrigIn_GetStatus(HANDLE hDev,BYTE *pStatus);
DLL int __stdcall CHR44X02_TrigIn_CreateEvent(HANDLE hDev, HANDLE *pEvent);
DLL DWORD __stdcall CHR44X02_TrigIn_WaitEvent(HANDLE hDev,HANDLE hEvent,DWORD dwMilliseconds);
DLL int __stdcall CHR44X02_TrigIn_CloseEvent(HANDLE hDev, HANDLE hEvent);

#ifdef __cplusplus
}
#endif

#endif