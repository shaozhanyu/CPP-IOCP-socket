#include "MyThread.h"
#include"task.h"
#include "MyThreadPool.h"
#include<cassert>
#include "mcom.h"

#define WAIT_TIME 20

CMyThread::CMyThread(CBaseThreadPool*threadPool)
{
	m_pTask=NULL;
	//m_bIsActive=false;
	m_pThreadPool=threadPool;
	m_hEvent=CreateEvent(NULL,false,false,NULL);//创建事件，返回事件ID赋值给线程类的变量
	m_bIsExit=false;
}

//bool CMyThread::m_bIsActive=false;
CMyThread::~CMyThread(void)
{
	CloseHandle(m_hEvent);
	CloseHandle(m_hThread);
}

bool CMyThread::startThread()
{
	m_hThread=CreateThread(0,0,threadProc,this,0,&m_threadID);//m_threadID保存新线程的id
	if(m_hThread==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return true;
}

bool CMyThread::startRECThread()
{
	m_hThread=CreateThread(0,0,SaveDatathreadProc,this,0,&m_threadID);//m_threadID保存新线程的id
	if(m_hThread==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return true;
}

bool CMyThread::startAPPThread()
{
	m_hThread=CreateThread(0,0,RecvAPPthread,this,0,&m_threadID);//m_threadID保存新线程的id
	if(m_hThread==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return true;
}

bool CMyThread::suspendThread()
{
	ResetEvent(m_hEvent);
	return true;
}
//有任务到来，通知线程继续执行。
bool CMyThread::resumeThread()
{
	SetEvent(m_hEvent);//触发事件唤醒线程
	return   true;
}
//////////////////////////////处理数据线程////////////////////////////////////////////
DWORD WINAPI CMyThread::threadProc( LPVOID pParam )
{
	CMyThread *pThread=(CMyThread*)pParam;
	while(!pThread->m_bIsExit)//判断线程退出标志不为1
	{
 		DWORD ret=WaitForSingleObject(pThread->m_hEvent,INFINITE);//线程一直挂起
		if(ret==WAIT_OBJECT_0)
		{
			if(pThread->m_pTask)//判断线程所属类的任务指针不空，说明有任务
			{			
				pThread->m_pTask->taskRecClientProc();//执行任务类中的具体代码。
				pThread->m_pTask=NULL;
				pThread->m_pThreadPool->SwitchActiveThread(pThread);//为当前的工作线程去任务队列查找新任务
			}
		}
	}
	return 0;
}

//////////////////////////////处理APP数据线程////////////////////////////////////////////
DWORD WINAPI CMyThread::RecvAPPthread( LPVOID pParam )
{
	CMyThread *pThread=(CMyThread*)pParam;
	while(!pThread->m_bIsExit)//判断线程退出标志不为1
	{
 		DWORD ret=WaitForSingleObject(pThread->m_hEvent,INFINITE);//线程一直挂起
		if(ret==WAIT_OBJECT_0)
		{
			if(pThread->m_pTask)//判断线程中的任务指针不空，说明有任务
			{			
				pThread->m_pTask->taskAPPRecProc();//执行任务类中的具体代码。
				pThread->m_pTask=NULL;
				pThread->m_pThreadPool->SwitchActiveThread(pThread);//为当前的工作线程去任务队列查找新任务
			}
		}
	}
	return 0;
}
/////////////////////////////存储硬件数据线程///////////////////////////////////////////
DWORD WINAPI CMyThread::SaveDatathreadProc( LPVOID pParam )
{
	CMyThread *pThread=(CMyThread*)pParam;
	while(!pThread->m_bIsExit)//判断线程退出标志不为1
	{
 		DWORD ret=WaitForSingleObject(pThread->m_hEvent,INFINITE);//线程一直挂起
		if(ret==WAIT_OBJECT_0)
		{
			if(pThread->m_pTask)//判断线程中的任务指针不空，说明有任务
			{			
				pThread->m_pTask->taskSaveSQL();//执行任务类中的具体代码。
				pThread->m_pTask=NULL;
				pThread->m_pThreadPool->SwitchActiveThread(pThread);//为当前的工作线程去任务队列查找新任务
			}
		}
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//将任务关联到线程类。
bool CMyThread::assignTask( CTask*pTask )
{
	assert(pTask);
	if(!pTask)
		return false;
	m_pTask=pTask;
	
	return true;
}
//开始执行任务。
bool CMyThread::startTask()
{
	resumeThread();
	return true;
}
