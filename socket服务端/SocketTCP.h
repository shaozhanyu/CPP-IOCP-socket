//此为完成端口服务端,业务逻辑请在多线程函数中(IOCP_Run)中填写
//CSocketTCP tcpServer;
//tcpServer.Create("",9999,true);
//tcpServer.Listen();
//while(true)
//{
//	if (0 != tcpServer.Accept())
//	{
//		//error
//		break;
//	}
//
//}

#pragma once
#include "stdafx.h"
#include <process.h>
#include <winsock2.h>
#include <set>
#include <Windows.h>//<Windows.h>这个头文件不能在<winsock2.h>前面
#pragma comment(lib,"ws2_32.lib")
////////////////////////////////////
#define   DATA_LEN   4096*10

///////////////////////////////////
using namespace std;
////////////////////////////////////
enum  OPERATION_TYPE
{
	RECV_POSTED = 0,
	SEND_POSTED
};
struct IOCPContextKey //全局结构体
{
	IOCPContextKey()
	{
		NumberOfBytesRecv = 0;
		NumberOfBytesSend = 0;
		Buffer.len = DATA_LEN;
		Buffer.buf = szMessage;
	}
	OPERATION_TYPE opType;
	SOCKET   clientSocket;
	WSABUF  Buffer;
	char  szMessage[DATA_LEN];
	DWORD          NumberOfBytesRecv;
	DWORD          NumberOfBytesSend;

};


class CMutex
{
public:
	CMutex(void)
	{
		InitializeCriticalSection(&section_);
	}
	~CMutex(void)
	{
		DeleteCriticalSection(&section_);
	}
	void enter()
	{
		EnterCriticalSection(&section_); //互斥量进入临界区
	}
	void leave()
	{
		LeaveCriticalSection(&section_); //互斥量退出临界区
	}
	struct Lock
	{
		Lock(CMutex& s) : s(s)
		{
			s.enter(); 
		}  
		~Lock()
		{
			s.leave();
		}  
	private:
		CMutex& s;  
	};
private:
	CMutex& operator=(const CMutex& );
	CRITICAL_SECTION section_;
};

class CSocketTCP
{
public:
	CSocketTCP(void);
	~CSocketTCP(void);
	/* brif 创建tcp
	* param[1]tcp的ip地址; param[2]tcp的端口;param[3]是否允许端口重用
	* return 0 success;-1参数校验失败;-2创建socket失败;-3绑定端口失败
	*/
	int Create(char* cIP ="",int iPort = 0,bool bRebind = false);

	/* brif tcp服务监听端口;
	* param[1]允许未完成连接的tcp内核长度;
	* return >0 发送的数据长度;-1参数检查失败
	*/
	int Listen(int lNum = 5);

	/* brif tcp接受新的客户端,此函数阻塞;
	* return 0 成功新的客户端连接，非0失败
	*/
	int Accept();

	//关闭tcp
	void Close();

	// 获取最后错误
	int GetLastError();

	// 获取tcp的ip地址
	char* GetSocketIP();

	// 获取tcp的端口
	int GetSocketPort();

	//完成端口的多线程执行函数
	int IOCP_Run();

	//关闭所有的客户端
	void closeAllClient();
public:

	//做为客户端的本socket;
	SOCKET m_sockfd;

	//作为服务的时候，过来连接的tcp描述符
	SOCKET m_newClinetSockfd;
public:
	//完成端口
	HANDLE  m_CompletionPort;

	//完成端口应用层与内核衔接值,必须初始化，才能使用
	//这个结构可以作为参数传递进行扩展
	WSAOVERLAPPED  m_overlap;

	//客户端连接上来的key列表
	set<IOCPContextKey*> m_setIOCPKEY;

	//工作线程的句柄
	set<HANDLE> m_IOCPThread;

	//锁
	CMutex m_mutex;

};