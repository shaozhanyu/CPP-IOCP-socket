#include "Task.h"
#include "windows.h"
//#include "SaveQueue.h"

CTask::CTask(int id)
{
	m_ID=id;

}
CTask::~CTask(void)
{
}

int CTask::getID()
{
	return m_ID;
}
