#pragma once
#include "stdafx.h"
#include "SocketAPP.h"
#include "mcom.h"

//////////////////////////////////////////////////////
#define  FLAG_THRED_EXIT  0xFFFFFFFF

CTestTask *pTaskAPP=new CTestTask(1);//创建任务
///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
unsigned __stdcall RecvThread( void* pArguments )
{
	CSocketAPP* pParent = reinterpret_cast<CSocketAPP*>(pArguments);
	if (NULL == pParent)
	{
		//log
		return -1;
	}
	return pParent->IOCP_Recv();

} 
/////////////////Winsocket初始化/////////////////////////////////
class windowsSocketInit
{
public:
	windowsSocketInit():
	  m_init(false)
	  {
		  WORD wVersionRequested;
		  WSADATA wsaData;
		  int err;

		  wVersionRequested = MAKEWORD(2, 2);
		  //加载dll
		  err = WSAStartup(wVersionRequested, &wsaData);
		  if (err != 0) 
		  {
			  return;
		  }
		  else
		  {
			  printf("加载WSAdll成功!\r\n");
		  }
		  if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
		  {
			  //printf("Could not find a usable version of Winsock.dll\n");
			  WSACleanup();
			  return ;
		  }
		  else
		  {
			  m_init = true;
			  //printf("The Winsock 2.2 dll was found okay\n");
		  }
	  }
	  ~windowsSocketInit()
	  {
		  //卸载dll
		  if (m_init)
		  {
			  WSACleanup();
		  }

	  }
private: 
	bool m_init;
};

CSocketAPP::CSocketAPP()
{
	//static windowsSocketInit loadDLLOnce;
	m_sockfd = -1;
	m_newClinetSockfd = -1;
	m_CompletionPort = INVALID_HANDLE_VALUE;
	memset(&m_overlap,0,sizeof(WSAOVERLAPPED));
}
CSocketAPP::~CSocketAPP()
{
	//等待子线程关闭，否则会崩溃
	Close();
}


int  CSocketAPP::Create(char* cIP,int iPort,bool bRebind)
{
	if (NULL == cIP || 0 > iPort)
	{
		return -1;
	}
	char opt=1;
	if((m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
	{
#ifdef  _PrintError_
		perror("socket");
#endif
		return -2;
	}
	else
	{
		printf("创建socket套接字\r\n");
	}
	// 创建IOCP的内核对象
	/****
	 * 需要用到的函数的原型：
	 * HANDLE WINAPI CreateIoCompletionPort(
     *    __in   HANDLE FileHandle,		// 已经打开的文件句柄或者空句柄，一般是客户端的句柄
     *    __in   HANDLE ExistingCompletionPort,	// 已经存在的IOCP句柄
     *    __in   ULONG_PTR CompletionKey,	// 完成键，包含了指定I/O完成包的指定文件
     *    __in   DWORD NumberOfConcurrentThreads // 真正并发同时执行最大线程数，一般推介是CPU核心数*2
     * );
	 ****/
	// Create completion port，创建内核队列IOCP
	m_CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);////创建完成端口句柄
	if (NULL == m_CompletionPort){	// 创建IO内核对象失败
		printf("CreateIoCompletionPort failed. Error: %d" , GetLastError());
		system("pause");
		return -1;
	}
	// 绑定SOCKET到本机
	struct sockaddr_in clientAddr;
	clientAddr.sin_family=AF_INET;
	clientAddr.sin_port=htons(iPort);
	if(bRebind)
	{
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//SO_REUSEADDR是让端口释放后立即就可以被再次使用
	}
	if(0 == strlen(cIP))
	{
		clientAddr.sin_addr.s_addr=htonl(INADDR_ANY); //监听所有IP地址段
	}
	else
	{
		clientAddr.sin_addr.s_addr=inet_addr(cIP); //监听指定的IP
	}

	int ret = bind(m_sockfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));//绑定套接字
	if ( -1 == ret)
	{
#ifdef _PrintError
		printf("bind failed %s \n", strerror(errno));
#endif
		return -3;
	}
	else
	{
		printf("绑定套接字成功！\r\n");
	}

	//某些具体程序要求待未发送完的数据发送出去后再关闭socket，可通过设置让程序满足要求：
	struct linger {
		u_short l_onoff;
		u_short l_linger;
	};
	linger m_sLinger;
	m_sLinger.l_onoff = 1; 
	//在调用closesocket()时还有数据未发送完，允许等待
	// 若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
	m_sLinger.l_linger = 2; //设置等待时间为2秒
	setsockopt(m_sockfd, SOL_SOCKET, SO_LINGER, ( const char* )&m_sLinger, sizeof( linger ) );

	//在send(),recv()过程中有时由于网络状况等原因，发收不能预期进行,而设置收发时限：
	int nNetTimeout=1000;//1秒
	//发送时限
	setsockopt(m_sockfd , SOL_SOCKET , SO_SNDTIMEO ,(char *)&nNetTimeout,sizeof(int));
	//接收时限
	//setsockopt(m_sockfd , SOL_SOCKET , SO_RCVTIMEO ,(char *)&nNetTimeout,sizeof(int));

	return 0;
}


int  CSocketAPP::Listen(int lNum )
{
	if(0 > lNum)
	{
		return  -1;
	}
	if(-1 == m_sockfd)
	{
		return -2;
	}
	if(listen(m_sockfd,lNum)<0)
	{
#ifdef _PrintError_
		perror("listen");
#endif
		return -3;
	}
	else
	{
		printf("启动监听成功!\r\n");
	}

// 创建IOCP线程--线程里面创建线程池

	// 确定处理器的核心数量比如4核心8核心
	SYSTEM_INFO   systeminfo;
	GetSystemInfo(&systeminfo);
	int workThredNum = systeminfo.dwNumberOfProcessors*2;
	if (0 > workThredNum)
	{
		return -4;
	}
	printf("CPU核心数：%d\r\n" , workThredNum/2);
	// 基于处理器的核心数量创建线程
	for (int i = 0; i < workThredNum; i++)
	{
		unsigned threadID;
		/*********_beginthreadex()函数在创建新线程时会分配并初始化一个_tiddata块。
		这个_tiddata块自然是用来存放一些需要线程独享的数据。
		事实上新线程运行时会首先将_tiddata块与自己进一步关联起来。
		然后新线程调用标准C运行库函数如strtok()时就会先取得_tiddata块的地址再将需要保护的数据存入_tiddata块中。
		这样每个线程就只会访问和修改自己的数据而不会去篡改其它线程的数据了。
		因此，如果在代码中有使用标准C运行库中的函数时，尽量使用_beginthreadex()来代替CreateThread()。
		*********/
		HANDLE hWorkThread = (HANDLE)_beginthreadex( NULL, 0, &RecvThread, this, 0, &threadID );//创建线程,this指针
		m_IOCPThread.insert(hWorkThread);//把新建的线程句柄加入句柄集合中

	}
	// 开始处理IO数据
	printf( "socket服务器已就绪，正在等待客户端接入....\r\n");
	
	return 0;

}
void CSocketAPP::closeAllClient()
{
	//会唤醒所有线程,并通知退出线程;
	//close all thread
	int iThreadNum = m_IOCPThread.size();
	if (0 < iThreadNum)
	{
		//notice all thread exit
		for (int i = 0; i < iThreadNum;++i)
		{
			DWORD  dwNumberOfBytesTrlansferred = FLAG_THRED_EXIT;
			DWORD  dwCompletlonKey = 0;
			PostQueuedCompletionStatus(m_CompletionPort,dwNumberOfBytesTrlansferred,DWORD(dwCompletlonKey), &m_overlap);
		}

		//wait for thread close;
		for (set<HANDLE>::iterator itor = m_IOCPThread.begin();itor != m_IOCPThread.end();++itor)
		{
			HANDLE threadHandle = (*itor);
			WaitForSingleObject(threadHandle,INFINITE);
			CloseHandle(threadHandle);
		}
		m_IOCPThread.clear();

	}

	//clear resource
	if (INVALID_HANDLE_VALUE != m_CompletionPort )
	{
		CloseHandle(m_CompletionPort);
		m_CompletionPort = INVALID_HANDLE_VALUE;
	}

	for (set<IOCPContextKeyAPP*>::iterator itor = m_setIOCPKEY.begin();itor != m_setIOCPKEY.end();++itor)
	{
		IOCPContextKeyAPP* pIOCPKey = (*itor);
		if (NULL == pIOCPKey)
		{
			continue;
		}
		//close all client
		shutdown(pIOCPKey->clientSocket,SD_BOTH);
		closesocket(pIOCPKey->clientSocket);
		delete pIOCPKey;
		pIOCPKey = NULL;
	}
	m_setIOCPKEY.clear();
}
/*****
IOCP端口完成运行主线程
********/
int CSocketAPP::IOCP_Recv()
{
	DWORD dwBytesTransfered = 0;
	void* pVoidContextKey = NULL;
	//OVERLAPPED* pOverlapped = NULL;
	int* pnThreadNo = NULL;
	IOCPContextKeyAPP *pIOCPContext = NULL ; //类指针必须new，因为没有实例化，而对象是直接实例化到内存的
	SOCKET nSocket = -1;
	DWORD sendBytes = 0;
	DWORD recvBytes = 0;
	DWORD Flags = 0;
	BOOL  nRetCode = false;
	while (true)
	{
		/***GetQueuedCompletionStatus功能：获取队列完成状态。
		返回值：调用成功，则返回非零数值，相关数据存于lpNumberOfBytes、lpCompletionKey、lpoverlapped变量中。
		失败则返回零值。****/
		nRetCode  = GetQueuedCompletionStatus(m_CompletionPort,&dwBytesTransfered,(PULONG_PTR)&pVoidContextKey,
			(LPOVERLAPPED *)&m_overlap,INFINITE);

		pIOCPContext = (IOCPContextKeyAPP *)pVoidContextKey;//获取当前连接,类指针指向一个实例指针

		if (TRUE == nRetCode && FLAG_THRED_EXIT == dwBytesTransfered && 0 == pVoidContextKey)
		{
			//service exit thread
			break;
		}
		if (FALSE == nRetCode && 0 == dwBytesTransfered  && NULL == pVoidContextKey)
		{
			//CloseHandle(m_CompletionPort);
			//完成端口关闭
			break;
		}
		if (TRUE == nRetCode && 0 == dwBytesTransfered  && NULL != pVoidContextKey)
		{
			//client close tcp
			if (0 == m_setIOCPKEY.size())
			{
				break;
			}
			{
				CLockMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
				m_setIOCPKEY.erase(pIOCPContext);//删除当前连接
			}
			shutdown(pIOCPContext->clientSocket,SD_BOTH);
			closesocket(pIOCPContext->clientSocket);
			delete pIOCPContext;//释放类指针
			pIOCPContext = NULL;
			continue;
		}
		if (NULL == pVoidContextKey)
		{
			//不许key为NULL
			continue;
		}

		nSocket = pIOCPContext->clientSocket; //获取当前的socket连接

		//业务逻辑处理
		switch(pIOCPContext->opType)
		{
			case  APP_RECV_POSTED: //1) 收到客户端数据流								
			{		
					

					if( (dwBytesTransfered > 0) && dwBytesTransfered < REC_SIZE)
					{

						EnterCriticalSection(&mAPP_RECLock);//加锁

						memcpy( mAPP_RevUnit.RecData , pIOCPContext->Buffer.buf ,dwBytesTransfered);
						mAPP_RevUnit.DataLen = dwBytesTransfered;
						mAPP_RevUnit.SocketNum =nSocket;
						//totalcnt++;
						//cout<<"收到数据"<<totalcnt<<endl;
						//这里的任务队列其实就是存了需要执行的任务次数，触发一次，就入队一次。实际数据没有存储，是全局变量						
						APPthreadpool.addTask(pTaskAPP,NORMAL);//任务放入线程池中并执行任务，会唤醒挂起的线程，从而执行任务类的具体代码

						LeaveCriticalSection(&mAPP_RECLock);//解锁
					}					
										
			/*****/
					//投递下个RECV命令
					memset(pIOCPContext->szMessage,0,pIOCPContext->Buffer.len);//当前缓存清0				
					pIOCPContext-> Buffer.len = DATA_LEN;//设置缓存大小
					ZeroMemory( &m_overlap,sizeof(OVERLAPPED) );
					pIOCPContext->NumberOfBytesRecv = 0;
					pIOCPContext->NumberOfBytesSend = 0;
					pIOCPContext->opType = APP_RECV_POSTED;
					int iRecv = WSARecv(nSocket,&pIOCPContext->Buffer,1,&recvBytes,&Flags,&m_overlap,NULL);//接收socket数据
					if (SOCKET_ERROR == iRecv && WSA_IO_PENDING != WSAGetLastError())
					{
						//接收错误，清除此socket
						if (0 == m_setIOCPKEY.size())
						{
							break;
						}
						{
							CLockMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
							m_setIOCPKEY.erase(pIOCPContext);
						}
						shutdown(pIOCPContext->clientSocket,SD_BOTH);
						closesocket(pIOCPContext->clientSocket);
						delete pIOCPContext;
						pIOCPContext = NULL;
						continue;
					}
					else
					{
						//ok
					}
					/**********/
#if    0
				//pIOCPContext->NumberOfBytesRecv = dwBytesTransfered;
				pIOCPContext->opType = APP_SEND_POSTED;
				memcpy(pIOCPContext->Buffer.buf,"RECOK!",6);
				//设置发送数据长度(这个长度不可能超出收到缓存的长度)
				pIOCPContext->Buffer.len = 6;
				//do your logic by default send origin data
				//发送数据 WSASend函数，可以支持一次发送多个BUFFER的请求,减少了send的调用次数，实际检验证明，使用WSASend可以提高50%的性能甚至更多
				int iRetSend = WSASend(nSocket,&pIOCPContext->Buffer,1,&sendBytes,0,&m_overlap,NULL);            
				if (SOCKET_ERROR == iRetSend && WSA_IO_PENDING != WSAGetLastError())
				{
					//发送错误，清除此socket
					if (0 == m_setIOCPKEY.size())
					{
						break;
					}
					{
						CLockMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
						m_setIOCPKEY.erase(pIOCPContext);
					}
					shutdown(pIOCPContext->clientSocket,SD_BOTH);
					closesocket(pIOCPContext->clientSocket);
					delete pIOCPContext;
					pIOCPContext = NULL;
					continue;
				}
				else
				{
					//ok
				}
#endif


			}break;
#if   0
			case  APP_SEND_POSTED :
			//业务逻辑处理
			//2> 处理发送请求处理（必须处理的是继续投递信息）
			{
				/**************
				pIOCPContext->NumberOfBytesSend += dwBytesTransfered;
				//查看数据有没有发送完成(以目前接到到数据全部返回的默认例子,应该发送数据长度为原始接收到的数据长度)
				if ( pIOCPContext->NumberOfBytesSend  <  pIOCPContext->NumberOfBytesRecv )
				{
					//没有发送完成，继续发送,否则就继续接收数据
					pIOCPContext->Buffer.buf = pIOCPContext->Buffer.buf + pIOCPContext->NumberOfBytesSend;
					pIOCPContext->Buffer.len = (pIOCPContext->NumberOfBytesRecv - pIOCPContext->NumberOfBytesSend);

					int iRetSend = WSASend(nSocket,&pIOCPContext->Buffer,1,&sendBytes,0, &m_overlap,NULL); //继续发送剩余的   

					if (SOCKET_ERROR == iRetSend && WSA_IO_PENDING != WSAGetLastError())
					{
						//发送错误，清除此socket
						if (0 == m_setIOCPKEY.size())
						{
							break;
						}
						{
							CLockMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
							m_setIOCPKEY.erase(pIOCPContext);
						}
						shutdown(pIOCPContext->clientSocket,SD_BOTH);
						closesocket(pIOCPContext->clientSocket);
						delete pIOCPContext;
						pIOCPContext = NULL;
						continue;
					}
					else
					{
						//ok
					}

				} 
				else //发送完毕
				*************/
				/*******/
				{
					//投递下个RECV命令
					memset(pIOCPContext->Buffer.buf,0,pIOCPContext->Buffer.len);//接收缓存清0
					pIOCPContext-> Buffer.len = DATA_LEN;//设置缓存大小
					pIOCPContext->NumberOfBytesRecv = 0;
					pIOCPContext->NumberOfBytesSend = 0;
					pIOCPContext->opType = APP_RECV_POSTED;
					int iRecv = WSARecv(nSocket,&pIOCPContext->Buffer,1,&recvBytes,&Flags,&m_overlap,NULL);//接收socket数据
					if (SOCKET_ERROR == iRecv && WSA_IO_PENDING != WSAGetLastError())
					{
						//接收错误，清除此socket
						if (0 == m_setIOCPKEY.size())
						{
							break;
						}
						{
							CLockMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
							m_setIOCPKEY.erase(pIOCPContext);
						}
						shutdown(pIOCPContext->clientSocket,SD_BOTH);
						closesocket(pIOCPContext->clientSocket);
						delete pIOCPContext;
						pIOCPContext = NULL;
						continue;
					}
					else
					{
						//ok
					}
					
				}
				/*********/
			}break;
#endif
			default:
					break;
		}//end switch

	}//end while
	return 0;
}

void CSocketAPP::Close()
{
	//关闭客户端
	closeAllClient();

	//关闭服务端socket
	if (-1 != m_sockfd)
	{
		//closesocket(m_sockfd);
		m_sockfd = -1;
	}
}

int CSocketAPP::GetLastError()
{
	return WSAGetLastError();
}

char* CSocketAPP::GetSocketIP()
{
	sockaddr_in sin;
	int len = sizeof(sin);
	if(getsockname(m_sockfd, (struct sockaddr *)&sin, &len) != 0)
	{
		//printf("getsockname() error:%s\n", strerror(errno));
		return NULL;
	}
	return inet_ntoa(sin.sin_addr);
}
int CSocketAPP::GetSocketPort()
{
	sockaddr_in sin;
	int len = sizeof(sin);
	if(getsockname(m_sockfd, (struct sockaddr *)&sin, &len) != 0)

	{
		//printf("getsockname() error:%s\n", strerror(errno));
		return 0;
	}
	return ntohs(sin.sin_port);
}