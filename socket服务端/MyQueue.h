#pragma once
#include<deque>
#include"MyMutex.h"
class CTask;
class CMyQueue
{
public:
	CMyQueue(void);
	~CMyQueue(void);
public:
	CTask*pop();
	bool push(CTask*t);
	bool pushFront(CTask*t);//插到队首。
	bool isEmpty();
	bool clear();
private:
	std::deque<CTask*>m_TaskQueue; //任务队列
	CMyMutex m_mutex;
};

