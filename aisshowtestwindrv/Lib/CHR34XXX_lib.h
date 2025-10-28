/**********************************************************
* File CHR34XXX_lib.h
* DESCRIPTION CHR34XXX Windows Dynamic Link Library
*********************************************************/
#ifndef _CHR34XXX_LIB_H_
#define _CHR34XXX_LIB_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLL __declspec(dllexport)

#ifndef __CHR_DEVBUSST__
#define __CHR_DEVBUSST__
typedef struct	_CHR_DEVBUSST_
{
	WORD wdDevID;			
	WORD wdVenID;
	WORD wdSubDevID;
	WORD wdSubVenID;
	WORD wdBusNum;			//总线号	
	WORD wdDevNum;			//设备号
	WORD wdFunNum;			//功能号
	WORD wdIrqNum;			//中断号
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

#ifndef __CHRUART_DCB_ST__
#define __CHRUART_DCB_ST__
typedef struct _CHRUART_DCB_ST_
{
    DWORD BaudRate;  //波特率
    DWORD ByteSize;  //数据位
    DWORD StopBits;  //停止位
	DWORD Parity;    //校验位
}CHRUART_DCB_ST, *pCHRUART_DCB_ST;
#endif

#ifndef __CHRUART_ASYN_PROTOCOL_ST__
#define __CHRUART_ASYN_PROTOCOL_ST__
typedef struct _CHRUART_ASYN_PROTOCOL_ST_
{
	BYTE ProtocolNo;    
	BYTE SFH;   
	BYTE EFH;
	BYTE STOF;
	BYTE ETOF;
	BYTE PtlLen;
	BYTE CheckSum;
	BYTE CheckHead;
}CHRUART_ASYN_PROTOCOL_ST,*pCHRUART_ASYN_PROTOCOL_ST;
#endif

#ifndef __CHRUART_SYNC_ERRCHECK_ST__
#define __CHRUART_SYNC_ERRCHECK_ST__
typedef struct _CHRUART_SYNC_ERRCHECK_ST_
{
	BOOL ErrCheckEn;
	BOOL AddrCheck;
	BOOL CrcCheck;
	BOOL WordCheck;
	BOOL CodeCheck;
	BOOL LoseCheck;
}CHRUART_SYNC_ERRCHECK_ST,*pCHRUART_SYNC_ERRCHECK_ST;
#endif

#ifndef __CHRUART_SYNC_WORD_ST__
#define __CHRUART_SYNC_WORD_ST__
typedef struct _CHRUART_SYNC_WORD_ST_
{
	DWORD WordCnt;
	BYTE  FHeader[4];
	BYTE  FTail[4];
}CHRUART_SYNC_WORD_ST,*pCHRUART_SYNC_WORD_ST;
#endif

#ifndef __CHR34XXX_TRIGIN_ST__
#define __CHR34XXX_TRIGIN_ST__
typedef struct _CHR34XXX_TRIGIN_ST_
{
	BOOL  TrigEn;       //触发输入 TRUE:使能  FALSE:禁用
	DWORD TrigLine;     //触发线:0~8
	DWORD TrigLevel;    //触发电平 1:上升沿, 2:下降沿
	DWORD FilterTime;   //滤波时间 分辨率为1us
	DWORD DelayTime;    //延迟时间 分辨率为1us
}CHR34XXX_TRIGIN_ST, *pCHR34XXX_TRIGIN_ST;
#endif

#ifndef __CHR34XXX_TRIGOUT_ST__
#define __CHR34XXX_TRIGOUT_ST__
typedef struct _CHR34XXX_TRIGOUT_ST_
{
	BOOL  TrigEn;        //触发输出 TRUE:使能  FALSE:禁用
	BOOL  OutputEn;      //输出到外部 0:禁止，1:使能
	DWORD TrigLevel;     //触发电平 1:上升沿, 2:下降沿
	DWORD timeHigh;      //高电平宽度，分辨率为1us
	DWORD timeLow;       //低电平宽度，分辨率为1us
	DWORD idleLevel;     //空闲电平，0:低电平，1:高电平
	DWORD OutputCnt;     //输出个数，0:无限次输出，非0:有限输出个数
}CHR34XXX_TRIGOUT_ST, *pCHR34XXX_TRIGOUT_ST; 
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

//板卡操作API
DLL BOOL __stdcall CHR34XXX_OpenDev(IN int devId);   //打开板卡设备
DLL BOOL __stdcall CHR34XXX_OpenDevEx(OUT int *devId,IN BYTE busNo,IN BYTE DevNo,IN BYTE FuncNo);   //打开板卡设备
DLL BOOL __stdcall CHR34XXX_CloseDev(IN int devId);   //关闭板卡设备
DLL BOOL __stdcall CHR34XXX_ResetDev(IN int devId);   //复位板卡设备
DLL BOOL __stdcall CHR34XXX_GetDevParInfo(IN int devId,OUT pCHR_DEVPARST pstDevParInfo);    //获取板卡设备信息
DLL BOOL __stdcall CHR34XXX_GetDevBusInfo(IN int devId,OUT pCHR_DEVBUSST pstDevBusInfo);    //获取板卡总线信息

//串口通用操作API
DLL BOOL __stdcall CHR34XXX_Ch_SetType(IN int devId,IN BYTE Channel,IN BYTE Type);   //串口类型 0:同步串口  1:异步串口
DLL BOOL __stdcall CHR34XXX_Ch_GetType(IN int devId,IN BYTE Channel,OUT BYTE *Type);
DLL BOOL __stdcall CHR34XXX_Ch_SetMode(IN int devId,IN BYTE Channel,IN BYTE Mode);   //串口模式 0:232  1:422  2:485
DLL BOOL __stdcall CHR34XXX_Ch_GetMode(IN int devId,IN BYTE Channel,OUT BYTE *Mode);

//配置串口
DLL BOOL __stdcall CHR34XXX_Ch_SetCommState(IN int devId,IN BYTE Channel,IN pCHRUART_DCB_ST pstRsdcb); //配置串口(波特率、数据位、停止位、校验位)
DLL BOOL __stdcall CHR34XXX_Ch_SetFPDIV(IN int devId,IN BYTE Channel,IN WORD FP,IN WORD DIV);
DLL BOOL __stdcall CHR34XXX_Ch_GetFPDIV(IN int devId,IN BYTE Channel,IN WORD *FP,IN WORD *DIV);

//串口发送操作API
DLL BOOL __stdcall CHR34XXX_TxCh_SetMode(IN int devId,IN BYTE Channel,IN BYTE Mode);       //发送模式 0:单次发送  1:定时发送
DLL BOOL __stdcall CHR34XXX_TxCh_TxSetTrigCfg(IN int devId,IN BYTE Channel,IN pCHR34XXX_TRIGIN_ST pstTrigCfg,IN DWORD TotalCnt);  //触发发送配置  TotalCnt：0~0x1FFF
DLL BOOL __stdcall CHR34XXX_TxCh_SetPeriod(IN int devId,IN BYTE Channel,IN DWORD Period);  //定时发送周期
DLL BOOL __stdcall CHR34XXX_TxCh_FIFOState(IN int devId,IN BYTE Channel,OUT DWORD *State);  //发送FIFO状态
DLL BOOL __stdcall CHR34XXX_TxCh_FIFOCount(IN int devId,IN BYTE Channel,OUT DWORD *FIFOCount);   //发送FIFO数据量
DLL BOOL __stdcall CHR34XXX_TxCh_Write(IN int devId,IN BYTE Channel,IN DWORD Length,IN BYTE *pBuffer,OUT DWORD *pWritten);   //写发送数据
DLL BOOL __stdcall CHR34XXX_TxCh_Start(IN int devId,IN BYTE Channel);   //开始定时发送
DLL BOOL __stdcall CHR34XXX_TxCh_Stop(IN int devId,IN BYTE Channel);    //停止定时发送
DLL BOOL __stdcall CHR34XXX_Asyn_TxCh_SetWordGap(IN int devId,IN BYTE Channel,IN DWORD Gap);  //发送字间隔（仅适用于异步串口）

//串口接收公共操作API
DLL BOOL __stdcall CHR34XXX_RxCh_ClearOFFlag(IN int devId,IN BYTE Channel);  //清空缓冲区溢出标记
DLL BOOL __stdcall CHR34XXX_RxCh_ClearFIFO(IN int devId,IN BYTE Channel);    //清空接收缓冲区
DLL BOOL __stdcall CHR34XXX_RxCh_Start(IN int devId,IN BYTE Channel);        //开始数据接收
DLL BOOL __stdcall CHR34XXX_RxCh_Stop(IN int devId,IN BYTE Channel);         //停止数据接收

//接收中断操作API
DLL BOOL __stdcall CHR34XXX_RxInt_Enable(IN int devId,IN BYTE Channel,IN BOOL Enabled);   //接收中断使能
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_IntDepth(IN int devId,IN BYTE Channel,IN DWORD Depth);  //接收中断触发深度（仅适用于异步串口透明接收）
DLL BOOL __stdcall CHR34XXX_RxInt_CreateEvent(IN int devId,OUT HANDLE *phEvt);
DLL DWORD __stdcall CHR34XXX_RxInt_WaitEvent(IN int devId,IN HANDLE hEvt,IN DWORD dwMilliseconds);
DLL BOOL __stdcall CHR34XXX_RxInt_CloseEvent(IN int devId,IN HANDLE hEvt);

//异步串口操作API
DLL BOOL __stdcall CHR34XXX_Asyn_Ch_Reset(IN int devId,IN BYTE Channel);   //通道复位（仅适用于异步串口）
DLL BOOL __stdcall CHR34XXX_Asyn_Ch_RS485LoopBack(IN int devId,IN BYTE Channel,IN BOOL Enabled);  //485自环模式（仅适用于异步串口）
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_SetMode(IN int devId,IN BYTE Channel,IN BYTE Mode);  //接收模式 0:透明接收  1:协议接收（仅适用于异步串口）
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_SetProtocol(IN int devId,IN BYTE Channel,IN pCHRUART_ASYN_PROTOCOL_ST pstFrame);  //设置协议帧（仅适用于异步串口协议接收）
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_SetFrameGap(IN int devId,IN BYTE Channel,IN DWORD dwFGap);  //设置帧间隔

//异步串口透明接收操作API
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_FIFOState(IN int devId,IN BYTE Channel,OUT DWORD *pState);    //接收FIFO状态（仅适用于异步串口透明接收）
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_FIFOCount(IN int devId,IN BYTE Channel,OUT DWORD *pCount);    //接收FIFO数据量（仅适用于异步串口透明接收）
DLL BOOL __stdcall CHR34XXX_Asyn_RxCh_Read(IN int devId,IN BYTE Channel,IN DWORD Length,OUT BYTE *pBuffer,OUT DWORD *pReturned);  //读取FIFO数据（仅适用于异步串口透明接收）

//异步协议&同步串口操作API
DLL BOOL __stdcall CHR34XXX_FM_Ch_TimeOutEn(IN int devId,IN BYTE Channel,IN BYTE Enabled);     //超时丢帧使能
DLL BOOL __stdcall CHR34XXX_FM_Ch_SetTimeOut(IN int devId,IN BYTE Channel,IN DWORD TimeOut);   //超时丢帧时间
DLL BOOL __stdcall CHR34XXX_FM_Ch_GetTimeOutCnt(IN int devId,IN BYTE Channel,OUT DWORD *Cnt);  //超时丢帧数量
DLL BOOL __stdcall CHR34XXX_FM_Ch_ClearTimeOut(IN int devId,IN BYTE Channel);                  //超时丢帧清空

DLL BOOL __stdcall CHR34XXX_FM_Ch_RxMode(IN int devId,IN BYTE Channel,IN BYTE RxMode);         //接收模式 0:FIFO 1:刷新
DLL BOOL __stdcall CHR34XXX_FM_Ch_RxFrmCount(IN int devId,IN BYTE Channel,OUT DWORD *FrmCnt);  //缓冲区帧数量
DLL BOOL __stdcall CHR34XXX_FM_Ch_ReadFrm(IN int devId,IN BYTE Channel,IN DWORD BufSize,OUT BYTE *pBuffer,OUT DWORD *pReturned);        //读取一帧数据（FIFO模式）
DLL BYTE __stdcall CHR34XXX_FM_Ch_ReadRefreshFrm(IN int devId,IN BYTE Channel,IN DWORD BufSize,OUT BYTE *pBuffer,OUT DWORD *pReturned); //读取最新一帧数据（刷新模式）
DLL BOOL __stdcall CHR34XXX_FM_Ch_ReadMultiFrm(IN int devId,IN BYTE Channel,IN DWORD FrameCnt, IN DWORD BufSize,
	                                           OUT BYTE *pBuffer, OUT DWORD *FrmReturned, OUT DWORD *BytesReturned);  //读取多帧数据（FIFO模式）

//同步串口配置操作API
DLL BOOL __stdcall CHR34XXX_Sync_Ch_LocalAddr(IN int devId,IN BYTE Channel,IN BYTE LocalAddr);   //同步串口本地地址
DLL BOOL __stdcall CHR34XXX_Sync_Ch_SetCrc(IN int devId,IN BYTE Channel,IN BYTE CrcVal,IN BYTE CrcOrder);   //同步串口CRC设置  
DLL BOOL __stdcall CHR34XXX_Sync_Ch_FrmId(IN int devId,IN BYTE Channel,IN WORD IdCnt,IN WORD IdPos,IN WORD IdOrder);  //设置同步帧ID
DLL BOOL __stdcall CHR34XXX_Sync_Ch_MonitorEn(IN int devId,IN BYTE Channel,IN BOOL Enabled);        //同步串口监听模式使能
DLL BOOL __stdcall CHR34XXX_Sync_Ch_TREdge(IN int devId,IN BYTE Channel,IN BYTE TxEdge,IN BYTE RxEdge);   //设置同步串口时钟边沿(接收时钟边沿和发送时钟边沿必须不同)
DLL BOOL __stdcall CHR34XXX_Sync_Ch_SetSyncWord(IN int devId,IN BYTE Channel,IN pCHRUART_SYNC_WORD_ST pstSyncWord);   //设置同步字
DLL BOOL __stdcall CHR34XXX_Sync_Ch_SetErrCheck(IN int devId,IN BYTE Channel,IN pCHRUART_SYNC_ERRCHECK_ST pstErrCheck);  //同步串口接收错误检测
DLL BOOL __stdcall CHR34XXX_Sync_Ch_SetCrcType(IN int devId,IN BYTE Channel,IN BYTE CrcType,IN BOOL CrcXorEn);//设置CRC类型（新卡)

//触发输出操作API
DLL BOOL __stdcall CHR34XXX_TrigOut_Reset(IN int devId);																	 //触发输出复位
DLL BOOL __stdcall CHR34XXX_TrigOut_Config(IN int devId,IN BYTE ClkSrc,IN BYTE TrigLine,IN pCHR34XXX_TRIGOUT_ST pstTrigOut); //触发输出配置
DLL BOOL __stdcall CHR34XXX_TrigOut_Enable(IN int devId,IN BYTE TrigLine,IN BOOL Enabled);									 //输出使能
DLL BOOL __stdcall CHR34XXX_TrigOut_Count(IN int devIdd,IN BYTE TrigLine,OUT DWORD *pCnt);									 //获取输出个数
DLL BOOL __stdcall CHR34XXX_TrigOut_State(IN int devId,IN BYTE TrigLine,OUT DWORD *pState);									 //获取输出状态







/************************************快速使用************************************/
#ifndef __CHR34XXX_CH_STATUS_ST__
#define __CHR34XXX_CH_STATUS_ST__
typedef struct _CHR34XXX_CH_STATUS_ST_
{
	DWORD TxFIFOCnt;		//发送缓存当前已存储的字节数
	DWORD RxFIFOCnt;		//接收缓存当前已存储的字节数
	DWORD TxFIFOStatus;		//发送FIFO当前状态（0：空，1：不空，2：满，3：溢出）
	DWORD RxFIFOStatus;		//接收FIFO当前状态（0：空，1：不空，2：满，3：溢出）
}CHR34XXX_CH_STATUS_ST, *pCHR34XXX_CH_STATUS_ST;
#endif
/***********板卡使用、停止使用、复位***********/
DLL BOOL __stdcall CHR34XXX_StartDevice(IN int devId);   //通过自定义板卡id打开板卡设备（方式1）
DLL BOOL __stdcall CHR34XXX_StartDeviceWithDevInfo(OUT int *devId,IN BYTE busNo,IN BYTE DevNo,IN BYTE FuncNo);   //通过设备信息打开板卡设备（方式2）
DLL BOOL __stdcall CHR34XXX_StopDevice(IN int devId);   //关闭板卡设备
//获取板卡硬件设备厂商信息
DLL BOOL __stdcall CHR34XXX_GetDevHardInfo(IN int devId,OUT pCHR_DEVPARST pstDevParInfo,OUT pCHR_DEVBUSST pstDevBusInfo);    //获取板卡硬件信息（设备信息和总线信息）
/***********串口通道操作***********/
//复位串口通道
DLL BOOL __stdcall CHR34XXX_Ch_Reset(IN int devId,IN BYTE Channel);//复位串口通道（清除发送缓存，接收缓存）配置恢复到默认（115200，422，非自环，8，1，无校验）
//设置串口通道模式
DLL BOOL __stdcall CHR34XXX_Ch_SetRsMode(IN int devId,IN BYTE Channel,IN BYTE Mode,IN BOOL SelfLoop485En);//获取串口通道的缓存状态
//获取板卡串口通道当前状态（包含发送缓存占用字节数和状态，接收缓存占用字节数和状态）
DLL BOOL __stdcall CHR34XXX_Ch_GetFIFOStatus(IN int devId,IN BYTE Channel,IN pCHR34XXX_CH_STATUS_ST pstRsChCachStu);//获取串口通道的缓存状态
//设备串口通道发送数据
DLL BOOL __stdcall CHR34XXX_Ch_WriteFile(IN int devId,IN BYTE Channel,IN DWORD Length,IN BYTE *pBuffer,OUT DWORD *pWritten);
//设备串口通道读取数据
DLL BOOL __stdcall CHR34XXX_Ch_ReadFile(IN int devId,IN BYTE Channel,IN DWORD Length,OUT BYTE *pBuffer,OUT DWORD *pReturned);


#ifdef __cplusplus
}
#endif

#endif

