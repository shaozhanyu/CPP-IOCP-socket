
#pragma once

class CTask
{
	public:
		CTask(int id);
		~CTask(void);
	public:
		virtual  void taskRecClientProc()=0;		
		virtual  void taskSaveSQL()=0;
		virtual  void taskAPPRecProc()=0;
		int getID();
	private:
		int m_ID;
};


