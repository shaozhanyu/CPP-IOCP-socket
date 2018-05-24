#pragma once
#include "stdafx.h"
#include "SocketTCP.h"
#include "mcom.h"

//////////////////////////////////////////////////////
#define  FLAG_THRED_EXIT  0xFFFFFFFF

CTestTask *p=new CTestTask(1);//创建任务
///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
unsigned __stdcall WorkerThread( void* pArguments )
{
	CSocketTCP* pParent = reinterpret_cast<CSocketTCP*>(pArguments);
	if (NULL == pParent)
	{
		//log
		return -1;
	}
	return pParent->IOCP_Run();

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

CSocketTCP::CSocketTCP()
{
	static windowsSocketInit loadDLLOnce;
	m_sockfd = -1;
	m_newClinetSockfd = -1;
	m_CompletionPort = INVALID_HANDLE_VALUE;
	memset(&m_overlap,0,sizeof(WSAOVERLAPPED));
}
CSocketTCP::~CSocketTCP()
{
	//等待子线程关闭，否则会崩溃
	Close();
}


int  CSocketTCP::Create(char* cIP,int iPort,bool bRebind)
{
	if (NULL == cIP || 0 > iPort)
	{
		return -1;
	}
	char opt=1;
	if((m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		cout << "硬件 socket 初始化 failed!" << endl;
		perror("socket");
#ifdef  _PrintError_
		perror("socket");
#endif
		return -2;
	}
	else
	{

		printf("创建硬件socket套接字成功！\r\n");
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
	if (SOCKET_ERROR == ret)
	{
		cout << "bind failed!" << endl;
#ifdef _PrintError
		printf("bind failed %s \n", strerror(errno));
#endif
		return -3;
	}
	else
	{
		printf("硬件服务端绑定套接字成功！\r\n");
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
	//setsockopt(m_sockfd, SOL_SOCKET, SO_LINGER, ( const char* )&m_sLinger, sizeof( linger ) );

	#define   SIO_RCVALL                         _WSAIOW(IOC_VENDOR,1)  
	#define   SIO_RCVALL_MCAST             _WSAIOW(IOC_VENDOR,2)  
	#define   SIO_RCVALL_IGMPMCAST     _WSAIOW(IOC_VENDOR,3)  
	#define   SIO_KEEPALIVE_VALS         _WSAIOW(IOC_VENDOR,4)  
	#define   SIO_ABSORB_RTRALERT       _WSAIOW(IOC_VENDOR,5)  
	#define   SIO_UCAST_IF                     _WSAIOW(IOC_VENDOR,6)  
	#define   SIO_LIMIT_BROADCASTS     _WSAIOW(IOC_VENDOR,7)  
	#define   SIO_INDEX_BIND                 _WSAIOW(IOC_VENDOR,8)  
	#define   SIO_INDEX_MCASTIF           _WSAIOW(IOC_VENDOR,9)    
	#define   SIO_INDEX_ADD_MCAST       _WSAIOW(IOC_VENDOR,10)  
	#define   SIO_INDEX_DEL_MCAST       _WSAIOW(IOC_VENDOR,11)  
	struct tcp_keepalive {
		u_long     onoff;
		u_long     keepalivetime; //第一次开始发送的时间（单位毫秒）
		u_long     keepaliveinterval;//每次检测的间隔 （单位毫秒）
	};

	// 开启KeepAlive
	BOOL bKeepAlive = TRUE;
	int nRet = setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
	if (nRet == SOCKET_ERROR)
	{
		printf("open-keepalive-error\r\n");
		return -11;
	}

	// 设置KeepAlive参数
	tcp_keepalive alive_in = { 0 };
	tcp_keepalive alive_out = { 0 };
	alive_in.keepalivetime = 20000;       // 开始首次KeepAlive探测前的TCP空闭时间
	alive_in.keepaliveinterval = 1000;  // 两次KeepAlive探测间的时间间隔
	alive_in.onoff = TRUE;
	unsigned long ulBytesReturn = 0;
	nRet = WSAIoctl(m_sockfd, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	if (nRet == SOCKET_ERROR)
	{
		printf("set-keepalive-error\r\n");
		return -12;
	}
	//在send(),recv()过程中有时由于网络状况等原因，发收不能预期进行,而设置收发时限：
	//int nNetTimeout=1000;//1秒
	//发送时限
	//setsockopt(m_sockfd , SOL_SOCKET , SO_SNDTIMEO ,(char *)&nNetTimeout,sizeof(int));
	//接收时限
	//setsockopt(m_sockfd , SOL_SOCKET , SO_RCVTIMEO ,(char *)&nNetTimeout,sizeof(int));

	return 0;
}


int  CSocketTCP::Listen(int lNum )
{
	if(0 > lNum)
	{
		return  -1;
	}
	if(SOCKET_ERROR == m_sockfd)
	{
		return -2;
	}
	if(listen(m_sockfd,lNum)== SOCKET_ERROR)
	{
#ifdef _PrintError_
		perror("listen");
#endif
		return -3;
	}
	else
	{
		printf("启动硬件客户端监听成功!\r\n");
	}

	// 创建IOCP线程

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
		HANDLE hWorkThread = (HANDLE)_beginthreadex( NULL, 0, &WorkerThread, this, 0, &threadID );//创建线程,this指针
		m_IOCPThread.insert(hWorkThread);//把新建的线程句柄加入句柄集合中

	}
	// 开始处理IO数据
	printf( "硬件socket服务器已就绪，正在等待客户端接入....\r\n");
	
	return 0;

}

void CSocketTCP::closeAllClient()
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

	for (set<IOCPContextKey*>::iterator itor = m_setIOCPKEY.begin();itor != m_setIOCPKEY.end();++itor)
	{
		IOCPContextKey* pIOCPKey = (*itor);
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
int CSocketTCP::IOCP_Run()
{
	DWORD dwBytesTransfered = 0;
	void* pVoidContextKey = NULL;
	//OVERLAPPED* pOverlapped = NULL;
	int* pnThreadNo = NULL;
	IOCPContextKey* pIOCPContext = NULL; //类指针必须new，因为没有实例化，而对象是直接实例化到内存的
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

		pIOCPContext = (IOCPContextKey *)pVoidContextKey;//获取当前连接,类指针指向一个实例指针

		if (TRUE == nRetCode && FLAG_THRED_EXIT == dwBytesTransfered && 0 == pVoidContextKey)
		{
			//service exit thread
			printf("client-close-keepalive-noreponse!\r\n");
			break;
		}
		if (FALSE == nRetCode && 0 == dwBytesTransfered  && NULL == pVoidContextKey)
		{
			//CloseHandle(m_CompletionPort);
			//完成端口关闭
			//break;
			printf("client-close-keepalive-noreponse111!\r\n");
			UnbindSocketCard(pIOCPContext->clientSocket);//刪除socket和卡號關聯表紀録
			if (0 == m_setIOCPKEY.size())
			{
				break;
			}
			{
				CMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
				m_setIOCPKEY.erase(pIOCPContext);//删除当前连接
			}
			shutdown(pIOCPContext->clientSocket, SD_BOTH);
			closesocket(pIOCPContext->clientSocket);
			delete pIOCPContext;//释放类指针
			pIOCPContext = NULL;

			continue;
		}
		if (TRUE == nRetCode && 0 == dwBytesTransfered  && NULL != pVoidContextKey)//客户端主动断开连接
		{
			printf("client-closed333!\r\n");
			UnbindSocketCard(pIOCPContext->clientSocket);//刪除socket和卡號關聯表紀録
			//client close tcp
			if (0 == m_setIOCPKEY.size())
			{
				break;
			}
			{
				CMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
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
			printf("client-close-keepalive-noreponse444!\r\n");
			continue;
		}

		nSocket = pIOCPContext->clientSocket; //获取当前的socket连接

		//业务逻辑处理
		switch(pIOCPContext->opType)
		{
			case  RECV_POSTED: //1) 收到客户端数据流								
			{
					//totalcnt+=dwBytesTransfered;
					//printf("接收数据累计：%d\n" , totalcnt);

					if(pIOCPContext->Buffer.len>0 && pIOCPContext->Buffer.len < REC_SIZE)
					{
						EnterCriticalSection(&m_RECdataLock);//硬件数据接收线程加锁

						memcpy( m_SocReUnit.RecData , pIOCPContext->Buffer.buf ,pIOCPContext->Buffer.len);//全局变量拷贝数据			
						m_SocReUnit.DataLen = dwBytesTransfered;
						m_SocReUnit.SocketNum =nSocket;
						//硬件交互数据线程池，解包硬件数据.添加进任务队列，排队等待线程来处理
						//这里的任务队列其实就是存了需要执行的任务次数，触发一次，就入队一次。实际数据没有存储，是全局变量
						WorkDatathreadpool.addTask(p,NORMAL);//查询空闲的线程并唤醒，从而执行任务类的具体代码
						/*******
						EnterCriticalSection(&card_list_Lock);//待存储数据加锁
							char  temp[20];
							for(int i=0;i<4;i++)
								sprintf(&temp[2*i], "%02x", (pIOCPContext->Buffer.buf[25+i])); //小写16 进制，宽度占2个位置，右对齐，不足补0	
							temp[8]='\0';	
							string str_card = temp;
							card_list  +=str_card;
							card_list += "\r\n";
							LeaveCriticalSection(&card_list_Lock);//解锁
							*******/
						//if(m_SocReUnit.DataLen > FULL_SIZE)
						//{
							//m_SocReUnit.DataLen = 0;
							//printf("123\n");
							//SetEvent(m_hEvent[0]);   
						//}
						LeaveCriticalSection(&m_RECdataLock);//解锁
					}
										
					//投递下个RECV命令
					memset(pIOCPContext->Buffer.buf,0,pIOCPContext->Buffer.len);//接收缓存清0
					pIOCPContext-> Buffer.len = DATA_LEN;//设置缓存大小
					pIOCPContext->NumberOfBytesRecv = 0;
					pIOCPContext->NumberOfBytesSend = 0;
					pIOCPContext->opType = RECV_POSTED;
					int iRecv = WSARecv(nSocket,&pIOCPContext->Buffer,1,&recvBytes,&Flags,&m_overlap,NULL);//接收socket数据
					if (SOCKET_ERROR == iRecv && WSA_IO_PENDING != WSAGetLastError())
					{
						printf("client-close-keepalive-noreponse222!\r\n");
						UnbindSocketCard(pIOCPContext->clientSocket);//刪除socket和卡號關聯表紀録
						//接收错误，清除此socket
						if (0 == m_setIOCPKEY.size())
						{
							break;
						}
						{
							CMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
							m_setIOCPKEY.erase(pIOCPContext);
						}
						shutdown(pIOCPContext->clientSocket,SD_BOTH);
						closesocket(pIOCPContext->clientSocket);
						//如果设置了调用closesocket()时还有数据未发送完，允许等待。那就不会关闭成功。下次send不会返回-1
						delete pIOCPContext;
						pIOCPContext = NULL;
						continue;
					}
					else
					{
						//ok
					}
				

			}break;
#if   1
			case  SEND_POSTED :
			//业务逻辑处理
			//2> 处理发送请求处理（必须处理的是继续投递信息）
			{

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
							CMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
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
				{
					//投递下个RECV命令
					memset(pIOCPContext->Buffer.buf,0,pIOCPContext->Buffer.len);//接收缓存清0
					pIOCPContext-> Buffer.len = DATA_LEN;//设置缓存大小
					pIOCPContext->NumberOfBytesRecv = 0;
					pIOCPContext->NumberOfBytesSend = 0;
					pIOCPContext->opType = RECV_POSTED;
					int iRecv = WSARecv(nSocket,&pIOCPContext->Buffer,1,&recvBytes,&Flags,&m_overlap,NULL);//接收socket数据
					if (SOCKET_ERROR == iRecv && WSA_IO_PENDING != WSAGetLastError())
					{
						//接收错误，清除此socket
						if (0 == m_setIOCPKEY.size())
						{
							break;
						}
						{
							CMutex::Lock lock(m_mutex);//创建结构体类lock，初始化锁变量给构造函数，出函数，自动解析释放解锁
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
			}break;
#endif
			default:
					break;
		}//end switch

	}//end while
	return 0;
}

void CSocketTCP::Close()
{
	//关闭客户端
	closeAllClient();

	//关闭服务端socket
	if (-1 != m_sockfd)
	{
		//closesocket(m_sockfd);
		m_sockfd = -1;//特别注意关闭后，把socket对象置为INVALID_SOCKET
	}
}

int CSocketTCP::GetLastError()
{
	return WSAGetLastError();
}

char* CSocketTCP::GetSocketIP()
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

int CSocketTCP::GetSocketPort()
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