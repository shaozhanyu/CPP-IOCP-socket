#pragma once
#include<list>
#include "MyMutex.h"
#include "MyStack.h"
#include "MyList.h"
#include"MyQueue.h"

class CMyThread;
class CTask;
enum PRIORITY
{
	NORMAL,
	HIGH
};
class CBaseThreadPool
{
public:
	virtual bool SwitchActiveThread(CMyThread*)=0;
};

class CMyThreadPool:public CBaseThreadPool
{

public:
	CMyThreadPool(void);
	~CMyThreadPool(void);
	void  CreatWorkDataThread(int num);//创建硬件数据处理类对象，类调用了创建线程方法
	void  CreatSaveDataThread(int num);//创建保存硬件数据类对象
	void  CreatAPPDataThread(int num);//创建APP数据处理类对象
	
public:
	virtual CMyThread * PopIdleThread(); //取出空闲的线程所属类的对象
	virtual bool SwitchActiveThread(CMyThread*);//
	virtual CTask  *GetNewTask();

public:
	//priority为优先级。高优先级的任务将被插入到队首。
	bool addTask(CTask*t,PRIORITY priority);//添加任务进队列
	bool start();//开始调度。
	bool destroyThreadPool();
private:
	int m_nThreadNum;
	bool m_bIsExit;
	
	CMyStack m_IdleThreadStack;//定义空闲线程栈
	CMyList m_ActiveThreadList;//定义活动线程列表,正在工作的线程
	CMyQueue m_TaskQueue;//等待处理的任务队列
};

