
#ifndef  M_COMM_H_
#define  M_COM_H_

#pragma once
#include "stdafx.h"
#include <iostream>
#include <string>// 注意是C++标准库是<string>，不是<string.h>，带.h的是C语言中的头文件
//#include <Windows.h>
#include "MyThreadPool.h"//注意，这头文件不能放到前面
#include "MyThread.h"
#include "TestTask.h"
#include "json.h"

////////////////////////////////////////////////////////////////////////////

using namespace std;

#define  WIN_64_DEF       1

#define MYPORT    9888
#define APP_PORT  8999
#define  MAX_APP_TCP_NUM            10000
#define  MAX_HARD_TCP_NUM           10000
////////////////////////////////////////////////////////////////////////////
#define  REC_SIZE         1024*80*1
#define  FULL_SIZE        1024*90*1
#define  HARD_DATA_SIZE   300  //待存储的解析后的单包硬件数据
#define GUID_LEN 64
////////////////////////////////////////////////////////////////////////////
typedef   struct   RecUnit{   //自定义

	unsigned  char  RecData[REC_SIZE];
	unsigned  long  DataLen;
	SOCKET  SocketNum;
}SocketRecUnit;
/////////////////////////////////////////////////////////////////////////
typedef   struct   HardUnit{   //自定义用来插入队列中数据结构

	unsigned  char  RecData[HARD_DATA_SIZE];
	unsigned  int  DataLen;
	int  cmd;
	SOCKET  SocketNum;
	
}HardData;


//////////////////////////////////////////////////////////////////////////// 
#define     RF_CARD_NUM              1*10000  //可存储的卡号数量
#define     RF_CARD_LEN              4
////////////////////////////////////////////////////////////////////////////
#define    DATA_HEAD_LEN                  4+19 //固定协议头长度
#define    DATA_CLIENT_DATA_NUM_LEN       2  //数据区独立数据个数和每个独立数据的长度
#define    DATA_CLIENT_DATA_LEN           6  //每个独立数据段数据 卡号+状态+锁定
#define    DATA_CARD_LEN                  (DATA_CLIENT_DATA_NUM_LEN+DATA_CLIENT_DATA_LEN)
#define    DATA_END_LEN                   1  //校验
#define    APP_MIN_DATA_LEN               (DATA_HEAD_LEN+DATA_END_LEN) //APP最小数据包
#define    DATA_WHOLE_PACKET_LEN          (DATA_HEAD_LEN+DATA_CLIENT_DATA_NUM_LEN+DATA_CLIENT_DATA_LEN+DATA_END_LEN)
////////////////////////////////////////////////////////////////////////////
#define     SEVER_TO_CLIENT       0x5A
#define     CLIENT_TO_SEVER       0x5B
#define     CLIENT_GPS_INFO       0x5C    //硬件上传GPS定位的信息
#define     CLIENT_BASE_STATION_INFO       0x5D    //硬件上传无GPS定位的信息，上传基站信息
////////////////////////////////////////////////////
#define     APP_SET_LOCK          0x2A    //APP设置锁车请求
#define     APP_SET_UNLOCK        0x2B    //APP设置解锁请求
#define     APP_CONFIG_WEILAN     0x2C    //APP配置启停围栏
#define     APP_REGESTER_USER     0x1E    //APP注册用户
#define     APP_SET_DENG_LU       0x1F    //APP登录请求
#define     APP_SET_ZHUXIAO       0x20    //APP注销请求
#define     APP_SET_BANGDING      0x2D    //APP绑定设备请求
#define     APP_SET_SHANCHU       0x2E    //APP删除设备请求
#define     APP_SET_GET_STATE     0x2F    //APP查询设备状态请求
#define     APP_GET_ALL_DEVICE_STATE     0x30    //APP查询当前用户名下所有设备状态请求
#define     APP_REQUEST_WEILAN_ALARM     0x31  //查询围栏报警请求
#define     APP_REQUEST_WEILAN_DATA      0x32  //查询围栏设置
////////////////////////////////////////////////////////////////////////////
typedef  struct RFIDcard {
	unsigned  char  Data[RF_CARD_LEN];//EPC卡号	
	unsigned  char  State;	//卡状态
	unsigned  char  LockFlag; //锁定标志
	unsigned  char  DataLen; //卡号长度
	unsigned  char  UpdataFlag; //更新上传标志
	unsigned  char  ReadCardDevice; //读卡设备号
	unsigned  char  ValidDataFlag;	//有效数据标志
	unsigned  long  socket; 

}RFcard; //该结构体占24字节，其中两个字节浪费,4B对齐导致

//定义hash表和基本数据节点
typedef struct _NODE 
{ 
  int data; 
  struct _NODE* next; 
}NODE; 
  
typedef struct _HASH_TABLE 
{ 
  NODE* value[10]; 
}HASH_TABLE; 

///////////////////////////////////////////////////////////////////////////
extern  HANDLE  RecEvent[2];  //两事件
extern  char m_uuid[GUID_LEN];
extern  SocketRecUnit   m_SocReUnit; //自定义数据缓存结构体，包括socket连接
extern  SocketRecUnit   mAPP_RevUnit;
extern  string   card_list;//card列表
extern  unsigned  long  totalcnt;
extern  RFcard  RfCardData[RF_CARD_NUM];
extern  CMyThreadPool  WorkDatathreadpool; //硬件交互数据解包线程池
extern  CMyThreadPool  SaveDatathreadpool;//硬件上传数据存储线程池
extern  CMyThreadPool  APPthreadpool; //APP交互数据线程池
extern  CRITICAL_SECTION  m_RECdataLock;
extern  CRITICAL_SECTION  mAPP_RECLock;
extern  CRITICAL_SECTION  mSaveDataLock;//硬件上传的数据存储线程互斥锁
extern  CRITICAL_SECTION  card_list_Lock;
///////////////////////////////////////////////////////////////////////////
char*  MakeUUID();
int  ParseJsonFromString(char *mstr , Json::Value  *mJson); 
unsigned  char    DataS_Compare(unsigned  char * p1  ,unsigned  char  *p2, unsigned  short  len);
unsigned  char  * FindByteBuff(unsigned  char  *src , unsigned  long srclen ,  unsigned char *dec , unsigned  short datalen );
char   SaveClientData(SOCKET ClientS , int cmd , unsigned  char * src, unsigned  int  len);
char   RecClientData(SOCKET ClientS ,  unsigned char * src, unsigned  int  len );
char  SaveDataSQL(SOCKET ClientS ,int Command,  char * src, unsigned  int  len);
int   SaveHardStateInfo(SOCKET ClientS , Json::Value  mJsonValue);
int   SaveGPSData(SOCKET ClientS ,  unsigned  char * src ,unsigned  int  len);
int   SaveBaseStationData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len);
char  RecAPPData(SOCKET ClientS, unsigned char * src, unsigned  int  len );
char  SaveAPPData(SOCKET ClientS ,int Command, char * src, unsigned  int  len);
int  CheckUserToken(SOCKET ClientS , string tok);
int  InitInsertBikeDatabaseUser(void);
int  InitInsertBikeDatabaseCardinfo(void);
int  UpdateCardState(void);
int  APPUpdateUserState(SOCKET ClientS ,Json::Value  mJsonValue);
int  FindToken(string  token);
int  getSQLstr(string  m_findstr, Json::Value  mJsonValue );
int  responseCheckTokenErr(SOCKET ClientS);
///////
int  responseAPPregester(SOCKET ClientS ,int Command, char * src, unsigned  int  len);
int  responseAPPdenglu(SOCKET ClientS ,int Command, char * src, unsigned  int  len);
int  responseAPPbangding(SOCKET ClientS ,Json::Value  mJsonValue);
int  responseAPPbind (SOCKET ClientS ,int Command, char * src, unsigned  int  len);
int  responseAPPgetstate(SOCKET ClientS ,Json::Value  mJsonValue);
int  responseAPPjiebang(SOCKET ClientS ,Json::Value  mJsonValue);
string  getNullStr(const char* str ); 
int  GetVaule_CheckStr(string *str , Json::Value  mJsonValue , string  str_value);
int  responseAPPgetalldevicestate(SOCKET ClientS ,Json::Value  mJsonValue);
/////
int  responseAPPrequest_weilan_alarm(SOCKET   ClientS ,Json::Value  mJsonValue);
///////////////////
int  APPCheckAlarmTable(SOCKET ClientS , string gps , string  card);
////////////////////////////////
static double GPSdufen_to_du(double dufen);
int  PanDuanWeiLan(string  weilan_radius , string  weilan_gps  , string  gps);
int  APPSetWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue);
int  APPUnSetWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue);
int  APPConfigWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue);
int  responseAPPrequest_weilan_data(SOCKET  ClientS , Json::Value  m_json);

#endif


