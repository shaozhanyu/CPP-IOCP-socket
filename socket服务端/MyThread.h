#pragma once
#include "windows.h"

class CTask;

class CBaseThreadPool;

class CMyThread
{
public:
	CMyThread(CBaseThreadPool*threadPool);
	~CMyThread(void);
public:
	bool startThread();
	bool startRECThread();
	bool CMyThread::startAPPThread();
	bool suspendThread();
	bool resumeThread();
	bool assignTask(CTask*pTask);
	bool startTask();
	static DWORD WINAPI threadProc(LPVOID pParam);//处理数据线程
	static DWORD WINAPI SaveDatathreadProc(LPVOID pParam);//存储数据线程
	static DWORD WINAPI RecvAPPthread( LPVOID pParam );
	
	DWORD   m_threadID;//保存新线程的id
	HANDLE  m_hThread;
	bool    m_bIsExit;
private:
	
	HANDLE  m_hEvent;//事件ID
	CTask   *m_pTask; //任务类指针,是另一个无关类，通过父类CTast定义的纯虚函数，可以调用子类CTestTask实现的虚函数。
	CBaseThreadPool  *m_pThreadPool;	//线程池指针，是另一个无关类，通过父类定义的纯虚函数，可以调用子类实现的虚函数。
};
