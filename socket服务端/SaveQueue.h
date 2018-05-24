#pragma once
#include <deque>
#include "MyMutex.h"
#include "mcom.h"

///////////////硬件待存储数据类，包含了数据队列/////////////////////////////
class CMySaveQueue
{

public:
	CMySaveQueue(void);
	~CMySaveQueue(void);

public:
	HardData* pop();//出队
	bool push(HardData  *t);//插入队尾
	bool pushFront(HardData  *t);//对象插到队首。
	bool isEmpty();//判断队列空
	bool clear();//清空队列

private:
	deque<HardData*>m_DataQueue;//数据存储结构体队列

	CMyMutex   m_data_mutex;
};
