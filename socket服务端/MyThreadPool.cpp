#include "MyThreadPool.h"
#include "MyThread.h"
#include "Task.h"
#include<cassert>
#include<iostream>
#include "MyQueue.h"
CMyThreadPool::CMyThreadPool()
{
	
}
//////////////////////创建num个硬件解包线程//////////////////////////////
void  CMyThreadPool::CreatWorkDataThread(int num)
{
	m_bIsExit=false;
	for(int i=0;i<num;i++)
	{
		CMyThread *p =new CMyThread(this);
		m_IdleThreadStack.push(p);//向线程池类的空闲线程栈对象压人创建的线程类的指针
		p->startThread();//创建线程
	}
}
//////////////////////创建num个存储线程//////////////////////////////
void  CMyThreadPool::CreatSaveDataThread(int num)
{
	m_bIsExit=false;
	for(int i=0;i<num;i++)
	{
		CMyThread *p =new CMyThread(this);
		m_IdleThreadStack.push(p);//向线程池类的空闲线程栈对象压人创建的线程类的指针
		p->startRECThread();//创建线程		
	}
}

/////////////////////创建num个APP解包线程///////////////////////////////
void  CMyThreadPool::CreatAPPDataThread(int num)
{
	m_bIsExit=false;
	for(int i=0;i<num;i++)
	{
		CMyThread *p =new CMyThread(this);
		m_IdleThreadStack.push(p);//向线程池类的空闲线程栈对象压人创建的线程类的指针
		p->startAPPThread();//创建线程
		
	}
}

CMyThreadPool::~CMyThreadPool(void)
{
}
CMyThread* CMyThreadPool::PopIdleThread()
{
	CMyThread *pThread=m_IdleThreadStack.pop();//从空闲线程栈中取出空闲的线程
	//pThread->m_bIsActive=true;
	return pThread;
}
//将线程从活动队列取出，放入空闲线程栈中。在取之前判断此时任务队列是否有任务。如任务队列为空时才挂起。否则从任务队列取任务继续执行。
bool CMyThreadPool::SwitchActiveThread( CMyThread  *t)
{
	if(!m_TaskQueue.isEmpty())//任务队列不为空，继续取任务执行。
	{
		CTask *pTask=NULL;
		pTask=m_TaskQueue.pop();//栈中取出任务
		//std::cout<<"线程ID："<<t->m_threadID<<"   执行任务编号: "<<pTask->getID()<<std::endl;
	
		t->assignTask(pTask); //为线程类的任务指针分配当前取出的任务指针
		t->startTask();	// 调用线程的启动任务方法，开始执行任务
	}
 	else//任务队列为空，该线程挂起。
	{
		m_ActiveThreadList.removeThread(t);//从活动线程列表中删除当前线程，因为无事可做，他该空闲了
		m_IdleThreadStack.push(t); //空闲线程栈中压入该线程
	}
	return true;
}
////////////////////////////////////////////////////////////////
bool CMyThreadPool::addTask( CTask  *t,  PRIORITY priority )
{
	assert(t);
	if(!t||m_bIsExit)
		return false;	
	CTask *task=NULL;
	//std::cout<<"[任务ID:"<<t->getID()<<"]添加成功！"<<std::endl;
	if(priority== NORMAL)
	{
		m_TaskQueue.push(t);//任务加入任务队列。
	}
	else if(HIGH)
	{
		m_TaskQueue.pushFront(t);//高优先级任务。
	}
	
	if(!m_IdleThreadStack.isEmpty())//存在空闲线程。调用空闲线程处理任务。
	{
			task=m_TaskQueue.pop();//取出列头任务。
			if(task==NULL)
			{
				//static unsigned long  cnt=0;
				std::cout<<"任务取出出错!!!"<<std::endl;
				return 0;
			}
			CMyThread*pThread = PopIdleThread();  //从空闲线程栈中取出空闲的线程
			//std::cout<<"【线程ID:"<<pThread->m_threadID<<"】 执行任务ID:【"<<task->getID()<<"】"<<std::endl;
			m_ActiveThreadList.addThread(pThread);
			pThread->assignTask(task); //
			pThread->startTask(); // 开始执行任务SetEvent事件，激活挂起的线程	
	}
	return 0;
	
}
bool CMyThreadPool::start()
{
	return 0;
}
CTask* CMyThreadPool::GetNewTask()
{
	if(m_TaskQueue.isEmpty())
	{
		return NULL;
	}
	CTask *task=m_TaskQueue.pop();//取出列头任务。
	if(task==NULL)
	{
		std::cout<<"任务取出出错。"<<std::endl;
		return 0;
	}
	return task;
}
bool CMyThreadPool::destroyThreadPool()
{
	
	m_bIsExit=true;
	m_TaskQueue.clear();
	m_IdleThreadStack.clear();
	m_ActiveThreadList.clear();
	return true;
}
