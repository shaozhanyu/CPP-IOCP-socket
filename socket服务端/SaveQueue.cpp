
#include "SaveQueue.h"

CMySaveQueue::CMySaveQueue(void)
{
}


CMySaveQueue::~CMySaveQueue(void)
{
}

HardData* CMySaveQueue::pop()
{
	HardData *p = NULL;
	m_data_mutex.Lock();
	if(!m_DataQueue.empty())
	{
		p=m_DataQueue.front();
		m_DataQueue.pop_front();
	}
	m_data_mutex.Unlock();
	return p;
}

bool CMySaveQueue::push(HardData *t)
{
	if(!t)
	{
		return false;
	}
	m_data_mutex.Lock();
	m_DataQueue.push_back(t);
	m_data_mutex.Unlock();
	return true;
}

bool CMySaveQueue::isEmpty()
{
	bool ret=false;
	m_data_mutex.Lock();
	ret=m_DataQueue.empty();
	m_data_mutex.Unlock();
	return ret;
}

bool CMySaveQueue::pushFront( HardData  *t )
{
	if(!t)
	{
		return false;
	}
	m_data_mutex.Lock();
	m_DataQueue.push_front(t);
	m_data_mutex.Unlock();
	return true;
}

bool CMySaveQueue::clear()
{
	m_data_mutex.Lock();
	m_DataQueue.clear();
	m_data_mutex.Unlock();
	return true;
}

