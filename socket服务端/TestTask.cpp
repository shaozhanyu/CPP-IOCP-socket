
#include <winsock2.h>
#include "TestTask.h"
#include "mcom.h"
#include "stdio.h"
#include "SaveQueue.h"
#include<ctime> 

#ifdef  WIN_64_DEF
#include <mysql.h>
#endif

#ifdef  WIN_64_DEF
#pragma comment(lib,"libmysql.lib") 
#endif
//////////////////////////////////////////////////////////////////////
CMySaveQueue   m_SaveQueueSQL;//创建对象，实例化一个结构体队列

CTestTask *pTask =new CTestTask(10);//创建任务
int  saveCNT=0;
//////////////////////////////////////////////////////////////////////
CTestTask::CTestTask(int id)
	:CTask(id)
{

}
CTestTask::~CTestTask(void)
{
}
////////////////////////定时查询报警表数据线程////////////////////////////////////
DWORD  WINAPI  CheckHardConfigThread(LPVOID lpParameter)
{
	int ret1, ret2, ret3;
	int CheckCnt = 3;
	int CheckRet1 = 10, CheckRet2 = 10, CheckRet3 = 10;
	int nIndex = 0;
	while (true)
	{
		//每次等5000毫秒   
		nIndex = WaitForMultipleObjects(1, CheckHardConfigEvent, FALSE, 5000);

		if (nIndex == WAIT_OBJECT_0) //第一个事件发生    
		{
			//printf("线程启动，查询下发指令表！\r\n");
			ret1 = WX_SendBufang_ToHard();
			ret2 = WX_Send_MotorLock();
			ret3 = WX_Send_DeviceOpenToHard();			
		}
		else if (nIndex == WAIT_TIMEOUT) //超时    
		{   //超时可作定时用   
			ret1 = WX_SendBufang_ToHard();
			ret2 = WX_Send_MotorLock();
			ret3 = WX_Send_DeviceOpenToHard();
			/**
			SYSTEMTIME sys;
			GetLocalTime( &sys ); 
			printf( "线程扫描控制操作处理完毕:%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",
				sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		**/
			//Sleep(50);
			//SetEvent(CheckHardConfigEvent[0]);//触发事件唤醒线程
		}
	}
	printf("线程结束\n");

	return 0;

}
////////////////////硬件接收数据解包///////////////////////////////
void CTestTask::taskRecClientProc()
{
	RecClientData(m_SocReUnit.SocketNum ,  m_SocReUnit.RecData ,m_SocReUnit.DataLen);
}

////////////////////接收APP数据解包///////////////////////////////
void CTestTask::taskAPPRecProc()
{
	RecAPPData(mAPP_RevUnit.SocketNum ,  mAPP_RevUnit.RecData ,mAPP_RevUnit.DataLen);
}
///////////////////硬件解析后数据的存储///////////////////////////////
void CTestTask::taskSaveSQL()
{
	//HardData  mdata;
	HardData  *pmdata;

	EnterCriticalSection(&mSaveDataLock);//待存储数据加锁

	if(!m_SaveQueueSQL.isEmpty())//判断队列非空
	{
		HardData  *pd = m_SaveQueueSQL.pop();//解析后的队列数据出队一组
		if(pd==NULL)
		{
			LeaveCriticalSection(&mSaveDataLock);//解锁
			std::cout<<"硬件数据队列出队失败!!!"<<std::endl;
			
		}else
		{
			//memcpy(mdata.RecData , pd->RecData ,  pd->DataLen);
			//mdata.DataLen =  pd->DataLen;	
			//mdata.cmd =  pd->cmd;
			//mdata.SocketNum =  pd->SocketNum;
			//pmdata = &mdata;
			pmdata = pd;

			LeaveCriticalSection(&mSaveDataLock);//解锁
			//启动数据保存函数
			SaveDataSQL( pmdata->SocketNum , pmdata->cmd,  (char*)pmdata->RecData , pmdata->DataLen);//采集数据包中的实际用户数据信息
		
		}
	}
	else
	{
		LeaveCriticalSection(&mSaveDataLock);//解锁
	}

}

/************************数据比较*****************************************/
unsigned  char    DataS_Compare(unsigned  char * p1  ,unsigned  char  *p2, unsigned  short  len)
{
 	unsigned 	short i=0;
	for(i=0;i<len;i++)
	{
		if(*(p1++) == *(p2++) )
		{

		}
		else
		return 0 ; //比较不同为0
	 	
	}

	return  1 ;
}

/**************************查找制定连续个字节*************************************/
unsigned  char  * FindByteBuff(unsigned  char  *src , unsigned  long srclen ,  unsigned char *dec , unsigned  short datalen )
{
	unsigned  char *pt = dec;
	unsigned	char *ps = src;
	unsigned	char *pd = dec;
	int c = 0;

	while( (unsigned  long )(ps-src) < srclen )
	{
			if(*ps == *pd) //查找到首个相同
			{
					
					while(  (datalen >c) && (srclen-(ps-src)) >0 )	//接着查找剩余的个数
					{
							if(*(ps++) == *(pd++) )
							{
								c++;
							}
							else //不相同，半途而废
							{
								c=0;
								break;
							}
					}
			}

			else	//不相同继续下移查找
			{
					ps++;
			}

			if( c == datalen)	 //如果查找到结尾，达到了所有相同的个数
			{						
				return (unsigned  char *)(ps - c);  //返回首字符位置
			}
			
			if( (ps -src)== srclen )
			return 0;  //返回失败
			
			c = 0;	 //
			pd = pt; //
			
	}

	return 0;
	
}

/////////////////////////////解析出来的硬件数据压入队列，并添加存储数据任务队列////////////////////////////////////////////////
char  SaveClientData(SOCKET   ClientS ,int cmd , unsigned  char * src, unsigned  int  len)
{
	HardData  mdata , *pmdata ;

	EnterCriticalSection(&mSaveDataLock);//待存储数据队列操作，公共对象先加锁
	
	memcpy(mdata.RecData , src , len);
	mdata.DataLen = len;	
	mdata.cmd = cmd;
	mdata.SocketNum = ClientS;
	pmdata= &mdata;
	
	//具体数据压入待存储队列
	m_SaveQueueSQL.push(pmdata);//待存储的数据入队，全局共用变量要加锁操作，因为该函数在多线程中调用了
	//这里的任务队列其实就是存了需要执行的任务次数，触发一次，就入队一次。实际数据没有存储，是全局变量						
	SaveDatathreadpool.addTask(pTask,NORMAL);//任务放入线程池中并执行任务，会唤醒挂起的线程，从而执行任务类的具体代码
	//saveCNT++;
	//printf("数据入队成功 %d\n",saveCNT);//打印卡号
	LeaveCriticalSection(&mSaveDataLock);//解锁

	return 1;
	
}

/***********************************************************************
char   RecClientData
接收解析硬件上传数据
************************************************************************/
char   RecClientData(SOCKET ClientS, unsigned char * src, unsigned  int  len )
{
	unsigned  char decdata[6]={ 0x96,0x69,0x96,0x69};
	char  loop=1;
	unsigned  char *pstr;
	unsigned  char *srcdat=src;
	unsigned  char *pdata = src;
	long  DataLen=0;
	long  length=0;

	SocketRecUnit   mRECdataS; //数据缓存结构体，包括socket连接

	EnterCriticalSection(&m_RECdataLock);//硬件解包线程加锁，保护全局变量
	memcpy( mRECdataS.RecData , src  ,len );
	mRECdataS.DataLen = len;
	
	srcdat=mRECdataS.RecData;
	pdata = srcdat;
	DataLen = mRECdataS.DataLen;
	LeaveCriticalSection(&m_RECdataLock);//解锁

	if(DataLen >0 && DataLen < REC_SIZE)
	{	
		while(loop)
		{
			if( (DataLen-(pdata-srcdat)) >= APP_MIN_DATA_LEN )
			{
				pstr = FindByteBuff(pdata , (DataLen-(pdata-srcdat)) , decdata , 4);//查找命令头
				
				if(pstr!=0)//查找到包头
				{		
					pdata=pstr;
					length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
					if( length >= 0 )//判断剩余数据长度是否够一个整包
					{
						switch (*(pdata+4))//判断命令头
						{								
							case  CLIENT_TO_SEVER :													
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;					
							break;
							case   CLIENT_GPS_INFO:  //GPS上传
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//判断剩余数据长度
								else
								loop=0;	
							break;
							case   CLIENT_BASE_STATION_INFO:  //基站信息上传
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//判断剩余数据长度
								else
								loop=0;	
							break;
							case  CLIENT_ALARM_INFO:
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//判断剩余数据长度
								else
								loop=0;
								break;
							default: 
							loop =0;	
							break;
						}
					}
					else
					{
						loop=0;
					}
				}
				else
				{
						loop=0;
				}
			}
			else
			{
				loop=0;
			}
		}


			return  1;
	}
	else
	{
		
			return 0;
	}
	
}
//////////////////////////////////具体的硬件数据存储数据库函数////////////////////////////////////////////
char  SaveDataSQL(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	
	unsigned  int  i=0;
	string str_card ;
	int  ret=0;

	Json::Value  m_json;
	Json::Reader *pJsonParser;
	
	switch(Command)
	{
		case   CLIENT_GPS_INFO: //硬件上传GPS
			#ifdef  WIN_64_DEF
			try {
				SaveGPSData(ClientS, (unsigned  char *)src, len);
			}
			catch (exception E) {
				cout << "保存硬件数据出现异常！可能是数据库插入导致的！" << endl;
				return -1;
			}
			#endif
			return 0;
		break;

		case   CLIENT_TO_SEVER :	//
		/***************由于Jsoncpp解析非法json时，会自动容错成字符类型。对字符类型取下标时，会触发assert终止进程。
		解决方法：启用严格模式，让非法的json解析时直接返回false，不自动容错。这样，在调用parse的时候就会返回false。
		*************************/
			#ifdef  WIN_64_DEF
			pJsonParser = new Json::Reader(Json::Features::strictMode());//实例化

			if(!pJsonParser->parse(src, m_json)) //char* 转json并做合法性检查
			{
				cout << "parse error" << endl;
				delete  pJsonParser;
				return -5;
			}
			delete  pJsonParser;
			//ParseJsonFromString(src, &m_json);//char* 转json
			//cout<<m_json<<endl;
			//Json::FastWriter  fast_writer;//查看json内容对象
			//string str = fast_writer.write( m_json); //json转string
			//cout<<str<<endl;//这里是带引号的""，当做string内容
			//ret = FindToken(m_json["token"].asString());//比对token
			//static  int  reccnt=0;//只会初始化一次
			//reccnt++;
			//cout<<"验证结束啦啦:  "<<totalcnt<<endl;
			
			SaveHardStateInfo(ClientS , m_json );
			#endif
			return 0;
		break;
		case  CLIENT_BASE_STATION_INFO: //硬件上传无GPS的基站信息
			SaveBaseStationData(ClientS , (unsigned  char *)src , len);
			break;
		case  CLIENT_ALARM_INFO:  //保存报警信息
			SaveAlarmData(ClientS , (unsigned  char *)src , len);
			break;
		default: 
			cout<<"队列数据命令字错误！"<<endl;				
			return -4;
		break;
	
	}

	return 1;
	//printf("数据存储\n");
	//for(int i=0;i<4;i++)
	//printf("%02x", *(src+i));

	//printf("\n");

}
//////////////////////////////////////////////////////////////////////////////
int  SaveHardStateInfo(SOCKET   ClientS , Json::Value  mJsonValue)
{
	//time_t now_time; 
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
	
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	my_ulonglong  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_gps,str_time; 

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	
	str_soc = to_string((long)ClientS);//socket连接号,整型转string
	
	if( GetVaule_CheckStr(&str_state , mJsonValue , "card_state") == 0 )
	{
		cout<< "json数据类型错误card_state应该为string"<<endl;
		return  -5;
	}

	if(mJsonValue["card_lock"].isInt())
		str_lock = mJsonValue["card_lock"].asInt() + '0';
	else
	{		
		cout<< "json数据value类型错误card_lock应该为int"<<endl;
		return  -5;

	}

	if( GetVaule_CheckStr(&str_gps , mJsonValue , "gps") == 0 )
	{
		cout<< "json数据类型错误gps应该为string"<<endl;
		return  -5;
	}

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";
	
	//now_time = time(NULL); 
	//cout<<now_time<<endl; 

	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		////cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		return -11;
	}
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
	
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		cout << "select username error!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW(3) ) ";

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	if(!res  )
	{	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
			
		send(ClientS , (char *)str.data(), (int)str.length() , 0);  // 发送信息 
		//_sleep(500);
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		//cout << "///////sucess!\n" << endl;
		return 0;  		
	}
	else
	{
	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("update_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(),(int)str.length(), 0);  // 发送信息 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return -1;

}


////////////////////////////保存GPS数据////////////////////////////////////////
int   SaveGPSData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{ 
	int ret1=1 ,ret2=1,ret3 =1 ;
	int i=0;
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
//	return 0;
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	int  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_gps,str_time; 

	char  temp[200];
	for(i=0;i<4;i++)
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //小写16 进制，宽度占2个位置，右对齐，不足补0
	
	temp[8]='\0';
	str_card = temp;
	//if( *(src+2+3)< 50 )
	//printf("%s\n",str_card);//打印卡号

	//EnterCriticalSection(&card_list_Lock);//待存储数据加锁

	//card_list  +=str_card;
	//card_list += "\r\n";
	//LeaveCriticalSection(&card_list_Lock);//解锁
	//send(ClientS , (char *)(src+2), 8, 0);  // 发送信息 
	//return 0;
	
	str_soc = to_string((long)ClientS);//socket连接号,整型转string
	if ('N' == *(src + 0))//判断第一路输出开关状态
		str_state = "ACLOSE,";
	else 
		str_state = "AOPEN,";
	if ('N' == *(src + 1))//判断第二路输出开关状态
		str_state += "BCLOSE,";
	else 
		str_state += "BOPEN,";
	if ('O' == *(src + 6))
		str_state += "Out";
	else
		str_state += "In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	if(len-8 > 200)
		len=200;
	memcpy(temp,src+8,len-8);
	temp[len-8]='\0';
	str_gps = getNullStr(temp);

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'  ";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		Sleep(300);
		return -1;
	}
  
	if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		Sleep(300);
		return -11;
	}
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		Sleep(300);
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		cout << "GPS---查询card失败!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW(3) ) ";

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	if(!res  )
	{		

		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 	
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 		
		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("insert_gps_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "Insert GPS Data error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return -1;

}

////////////////////////////保存基站数据////////////////////////////////////////
int   SaveBaseStationData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int ret1, ret2, ret3;
	int i=0;
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	int  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_base_station,str_time; 

	char  temp[200];
	for(i=0;i<4;i++)
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //小写16 进制，宽度占2个位置，右对齐，不足补0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	
	str_soc = to_string((long)ClientS);//socket连接号,整型转string
	if ('N' == *(src + 0))//判断第一路输出开关状态
		str_state = "ACLOSE,";
	else
		str_state = "AOPEN,";
	if ('N' == *(src + 1))//判断第二路输出开关状态
		str_state += "BCLOSE,";
	else
		str_state += "BOPEN,";
	if ('O' == *(src + 6))
		str_state += "Out";
	else
		str_state += "In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	if(len-8 > 200)
		len=200;
	memcpy(temp,src+8,len-8);
	temp[len-8]='\0';
	str_base_station = getNullStr(temp);

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		////cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		Sleep(300);
		return -1;
	}
    if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		Sleep(300);
		return -11;
	}
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		Sleep(300);
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		cout << "基站定位---查询card失败!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_base_station_data(  card , card_socket, card_state , card_lock , base_station ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_base_station+ "', NOW(3) ) ";

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	if(!res  )
	{		
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		//cout << "SaveBaseStation-----sucess!\n" << endl;
		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("update_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "Insert 基站数据 SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return -1;

}
////////////////////////////保存报警列表////////////////////////////////////////
int   SaveAlarmData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int ret1, ret2, ret3;
	int i=0;
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	int  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_base_station,str_time; 

	char  temp[200];
	for(i=0;i<4;i++)
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //小写16 进制，宽度占2个位置，右对齐，不足补0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	
	str_soc = to_string((long)ClientS);//socket连接号,整型转string
	
	if('1'==*(src+6))
	str_state="Out";
	else
	str_state="In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	string str_send ="1";
	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		return -11;
	}
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		//num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    //MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		cout << "报警信息保存---查询card失败!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_alarm(  card , card_state , card_lock ,send,time )   VALUES( '" +
		str_card + "', '"+ str_state + "',"+ str_lock + ","+ str_send+ ", NOW(3) ) ";

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	if(!res  )
	{		
		cout << "SaveCardAlarm-----sucess!\n" << endl;
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("save_alarm_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "Insert Alarm SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return -1;

}
//////////////////////查找等待设定的设备表，并发送给硬件//////////////////////////////////////////
int   WX_SendBufang_ToHard( )
{
	SOCKET ClientS;
	SOCKET gps_client, station_client;
	string card_lock;
	string  gps_time ,gps_lock, station_time,station_lock , setalarm_time;
	bool  gps_data_success =false , station_data_success = false;
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";  
	unsigned int port = 3306;   
    char table[] = "bike";    
	//return -1;
	struct tm ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string   str_lock;
	string   allow_alarm;
	string  m_strToken ;
		            
    MYSQL myCont;
    MYSQL_RES *setalarm_result;//查询报警表
	MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
	my_ulonglong  setalarm_num_row, setalarm_num_col;
	MYSQL_ROW  setalarm_data_row;
	int setalarm_f1, setalarm_f2;
	LONG64  total_cnt = 0;
    int res;

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL)) 
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else 
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}

	if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		return -11;
	}

	//return -1;
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {  

    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		return -2;
    }
	
	int  find_loop = 3;
	//while (find_loop--)
	while(1)
	{
		m_strToken = "SELECT  *  FROM  set_card_alarm   ORDER BY time ASC  LIMIT  " + to_string(total_cnt) + " , 10 ";

		getsucess = false;
		res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
		if (!res)
		{
			//保存查询到的数据到result
			setalarm_result = mysql_store_result(&myCont);
			setalarm_num_row = mysql_num_rows(setalarm_result); //读取行数
			if (setalarm_num_row < 1)//没有数据
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长			
				mysql_close(&myCont);
				mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
				return -1;
			}
			total_cnt += setalarm_num_row;//行数自增
			setalarm_num_col = mysql_num_fields(setalarm_result); //读取列数
			//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
			MYSQL_FIELD* fields = mysql_fetch_fields(setalarm_result); //返回列名称的数组
			if (setalarm_num_row > 0)
			{
				for (setalarm_f1 = 0; setalarm_f1 < setalarm_num_row; setalarm_f1++) //循环行只取了最新的一行
				{
					setalarm_data_row = mysql_fetch_row(setalarm_result); //获取每行的数据内容

					for (setalarm_f2 = 0; setalarm_f2 < setalarm_num_col; setalarm_f2++) //循环列
					{
						//printf("%s\n", setalarm_data_row[setalarm_f2]);

						if (fields[setalarm_f2].name != NULL)
						{
							if (!strcmp(fields[setalarm_f2].name, "allow_alarm")) //判断当前列的字段名称
							{
								allow_alarm = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "card")) //判断当前列的字段名称
							{
								str_card = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "time")) //判断当前列的字段名称
							{
								setalarm_time = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}


						}
						//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
					}//取出一条待发送布防操作指令
					if (getsucess == true)
					{

					}
					else
					{
						mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
						mysql_close(&myCont);
						mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存										//cout << "no alarm_set list !\n" << endl;
						return -1;//布防指令表没有待发送的指令，返回-1
					}
					mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
					
					gps_data_success = false;
					m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回列名称的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											gps_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											gps_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
										else if (!strcmp(fields[f2].name, "card_lock")) //判断当前列的字段名称
										{
											gps_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}
							

						}
						else
						{
							gps_time = "2011-11-11 00:00:00";
						}
						
						mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

					}

					m_strToken = "SELECT  *  FROM  card_base_station_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											station_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											station_data_success = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											station_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
										}
										else if (!strcmp(fields[f2].name, "card_lock")) //判断当前列的字段名称
										{
											station_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}
							
						}
						else
						{
							station_time = "2011-11-11 00:00:00";
						}
		
						mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
					}
					//GPS表和基站表都没有数据，直接放弃发送
					if ((gps_data_success == false) && (station_data_success == false))
					{
						continue;
					}
					else
					{
						//有数据，比对最新数据
						time_t tm_setalarm_time, tm_gps_time, tm_station_time;
						tm_gps_time = convert_string_to_time_t(gps_time);//string转时间类
						tm_station_time = convert_string_to_time_t(station_time);//string转时间类
						tm_setalarm_time = convert_string_to_time_t(setalarm_time);
						double dec_value;//时间差值 
						//time(&tm_now_time);//获取当前时间  
						dec_value = difftime(tm_gps_time, tm_station_time);//计算时间差值，秒级  
						//printf("gps和基站最新数据时间差:  %f秒\n", dec_value);//
						if (dec_value > 0)
						{
							if (difftime(tm_gps_time, tm_setalarm_time) > 0)//数据比布防指令创建的晚
							{
								ClientS = gps_client;
								card_lock = gps_lock;
								//printf("采用gps数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else
							{
								continue;
							}
						}
						else
						{
							if (difftime(tm_station_time, tm_setalarm_time) > 0)
							{
								ClientS = station_client;
								card_lock = station_lock;
								//printf("采用基站数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else //数据比布防指令创建的早，不一定是真实的连接。暂放弃发送
							{
								continue;
							}
						}
					}
					if (0 == card_lock.compare(allow_alarm))//要布防的操作和硬件实际状态一致，就删除布防命令
					{
						m_strToken = "DELETE    FROM  set_card_alarm   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
						res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
						if (!res)
						{
							if(total_cnt>0)
								total_cnt--;//当前起始行数减去1
							continue;
						}
						else
							continue;
					}
					else
					{
						if (0 == allow_alarm.compare("1"))
						{
							Json::Value root;             // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["lock"] = Json::Value("setlock");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送设防指令！" << endl;
						}
						else
						{
							Json::Value root;             // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["lock"] = Json::Value("setunlock");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送撤防指令！" << endl;
						}
					}
				}
			}
			else //如果没有数据
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
			}
		}//查询布防命令列表
		else
		{
			break;//退出程序
		}
	}
	mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return  0;

}

//////////////////////查找等待设定的设备表，并发送给硬件//////////////////////////////////////////
int   WX_Send_MotorLock( )
{
	SOCKET ClientS;
	bool getsucess = false;
	const char user[] = "root";
	const char pswd[] = "123456";
	const char host[] = "localhost";
	unsigned int port = 3306;
	char table[] = "bike";
	SOCKET gps_client, station_client;
	string card_lock;
	string  gps_time, gps_lock, station_time, station_lock, setalarm_time;
	bool  gps_data_success = false, station_data_success = false;
	struct tm;
	string   tos, str_token, str_card, str_gps, str_username;
	string   str_lock;
	string   allow_alarm;
	string  m_strToken;
	
	MYSQL myCont;

	MYSQL_RES *setalarm_result;//查询报警表
	MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1, f2, num_row, num_col;
	my_ulonglong  setalarm_num_row, setalarm_num_col;
	MYSQL_ROW  setalarm_data_row;
	int setalarm_f1, setalarm_f2;
	LONG64  total_cnt = 0;
	int res;

	SYSTEMTIME sys;
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
	if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		return -11;
	}

	if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
	{
	}
	else
	{
		//cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		return -2;
	}
	int  find_loop = 3;
	//while (find_loop--)
	while(1)
	{
		m_strToken = "SELECT  *  FROM  set_motor_lock   ORDER BY time ASC  LIMIT  " + to_string(total_cnt) + " , 10 ";

		getsucess = false;
		res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
		if (!res)
		{
			//保存查询到的数据到result
			setalarm_result = mysql_store_result(&myCont);
			setalarm_num_row = mysql_num_rows(setalarm_result); //读取行数
			if (setalarm_num_row < 1)//没有数据
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长			
				mysql_close(&myCont);
				mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
				return -1;
			}
			total_cnt += setalarm_num_row;//行数自增
			setalarm_num_col = mysql_num_fields(setalarm_result); //读取列数
																  //printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
			MYSQL_FIELD* fields = mysql_fetch_fields(setalarm_result); //返回列名称的数组
			if (setalarm_num_row > 0)
			{
				for (setalarm_f1 = 0; setalarm_f1 < setalarm_num_row; setalarm_f1++) //循环行只取了最新的一行
				{
					setalarm_data_row = mysql_fetch_row(setalarm_result); //获取每行的数据内容

					for (setalarm_f2 = 0; setalarm_f2 < setalarm_num_col; setalarm_f2++) //循环列
					{
						//printf("%s\n", setalarm_data_row[setalarm_f2]);

						if (fields[setalarm_f2].name != NULL)
						{
							if (!strcmp(fields[setalarm_f2].name, "motor_lock")) //判断当前列的字段名称
							{
								allow_alarm = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "card")) //判断当前列的字段名称
							{
								str_card = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "time")) //判断当前列的字段名称
							{
								setalarm_time = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
						}
						//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
					}//取出一条待发送布防操作指令
					if (getsucess == true)
					{

					}
					else
					{
						mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
						mysql_close(&myCont);
						mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存										//cout << "no alarm_set list !\n" << endl;
						return -1;//布防指令表没有待发送的指令，返回-1
					}
	
					mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
			
					gps_data_success = false;
					m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回列名称的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											gps_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											gps_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
										else if (!strcmp(fields[f2].name, "card_state")) //判断当前列的字段名称
										{
											gps_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}							

						}
						else
						{
							gps_time = "2011-11-11 00:00:00";
						}
						mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

					}

					m_strToken = "SELECT  *  FROM  card_base_station_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											station_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											station_data_success = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											station_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
										}
										else if (!strcmp(fields[f2].name, "card_state")) //判断当前列的字段名称
										{
											station_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}
							
						}
						else
						{
							station_time = "2011-11-11 00:00:00";
						}
						mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

					}
					//GPS表和基站表都没有数据，直接放弃发送
					if ((gps_data_success == false) && (station_data_success == false))
					{
						continue;
					}
					else
					{
						//有数据，比对最新数据
						time_t tm_setalarm_time, tm_gps_time, tm_station_time;
						tm_gps_time = convert_string_to_time_t(gps_time);//string转时间类
						tm_station_time = convert_string_to_time_t(station_time);//string转时间类
						tm_setalarm_time = convert_string_to_time_t(setalarm_time);
						double dec_value;//时间差值 
						//time(&tm_now_time);//获取当前时间  
						dec_value = difftime(tm_gps_time, tm_station_time);//计算时间差值，秒级  
						//printf("gps和基站最新数据时间差:  %f秒\n", dec_value);//
						if (dec_value > 0)
						{
							if (difftime(tm_gps_time, tm_setalarm_time) > 0)//数据比布防指令创建的晚
							{
								ClientS = gps_client;
								card_lock = gps_lock;
								//printf("采用gps数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else
							{
								continue;
							}
						}
						else
						{
							if (difftime(tm_station_time, tm_setalarm_time) > 0)
							{
								ClientS = station_client;
								card_lock = station_lock;
								printf("采用基站数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else //数据比布防指令创建的早，不一定是真实的连接。暂放弃发送
							{
								continue;
							}
						}
					}
					if ((card_lock.find("ACLOSE") != string::npos && (0 == allow_alarm.compare("0"))) ||
						(card_lock.find("AOPEN") != NULL && (0 == allow_alarm.compare("1"))))
					{
						m_strToken = "DELETE    FROM  set_motor_lock   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
						res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
						if (!res)
						{
							if(total_cnt>0)
								total_cnt--;//当前起始行数减去1
							continue;
						}
						else
							continue;
					}
					else
					{
						if (0 == allow_alarm.compare("1"))
						{
							Json::Value root;   // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["motorlock"] = Json::Value("setmotorlock");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送锁电机指令！" << endl;
						}
						else
						{
							Json::Value root;  // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["motorlock"] = Json::Value("setmotorunlock");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送解锁电机指令！" << endl;
						}
					}
				}
			}
			else
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
			}
			
		}//查询锁电机命令列表
		else
		{
			break;
		}
	}
	////////////////////////////////////////////////
	mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return   0;

}

//////////////////////查找等待设定的设备表，并发送给硬件//////////////////////////////////////////
int   WX_Send_DeviceOpenToHard()
{
	SOCKET ClientS;
	bool getsucess = false;
	const char user[] = "root";
	const char pswd[] = "123456";
	const char host[] = "localhost";
	unsigned int port = 3306;
	char table[] = "bike";
	SOCKET gps_client, station_client;
	string card_lock;
	string  gps_time, gps_lock, station_time, station_lock, setalarm_time;
	bool  gps_data_success = false, station_data_success = false;
	struct tm;
	string   tos, str_token, str_card, str_gps, str_username;
	string   str_lock;
	string   allow_alarm;
	string  m_strToken;

	MYSQL myCont;

	MYSQL_RES *setalarm_result;//查询报警表
	MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1, f2, num_row, num_col;
	my_ulonglong  setalarm_num_row, setalarm_num_col;
	MYSQL_ROW  setalarm_data_row;
	int setalarm_f1, setalarm_f2;
	LONG64  total_cnt = 0;
	int res;

	SYSTEMTIME sys;
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//初始化数据库  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
	if (mysql_init(&myCont) == NULL)//初始化mysql
	{
		printf("inital mysql handle error");
		return -11;
	}

	if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
	{
	}
	else
	{
		//cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
		return -2;
	}
	int  find_loop = 3;
	//while (find_loop--)
	while(1)
	{
		m_strToken = "SELECT  *  FROM  set_device_open   ORDER BY time ASC  LIMIT  " + to_string(total_cnt) + " , 10 ";

		getsucess = false;
		res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
		if (!res)
		{
			//保存查询到的数据到result
			setalarm_result = mysql_store_result(&myCont);
			setalarm_num_row = mysql_num_rows(setalarm_result); //读取行数
			if (setalarm_num_row < 1)//没有数据
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长			
				mysql_close(&myCont);
				mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
				return -1;
			}
			total_cnt += setalarm_num_row;//行数自增
			setalarm_num_col = mysql_num_fields(setalarm_result); //读取列数
			//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
			MYSQL_FIELD* fields = mysql_fetch_fields(setalarm_result); //返回列名称的数组
			if (setalarm_num_row > 0)
			{
				for (setalarm_f1 = 0; setalarm_f1 < setalarm_num_row; setalarm_f1++) //循环行只取了最新的一行
				{
					setalarm_data_row = mysql_fetch_row(setalarm_result); //获取每行的数据内容

					for (setalarm_f2 = 0; setalarm_f2 < setalarm_num_col; setalarm_f2++) //循环列
					{
						//printf("%s\n", setalarm_data_row[setalarm_f2]);

						if (fields[setalarm_f2].name != NULL)
						{
							if (!strcmp(fields[setalarm_f2].name, "device_open")) //判断当前列的字段名称
							{
								allow_alarm = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "card")) //判断当前列的字段名称
							{
								str_card = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
							else if (!strcmp(fields[setalarm_f2].name, "time")) //判断当前列的字段名称
							{
								setalarm_time = getNullStr(setalarm_data_row[setalarm_f2]); //获取字段内容，里面判断非NULL			
								getsucess = true;
								//cout<<allow_alarm<<endl;
							}
						}
						//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
					}//取出一条待发送布防操作指令
					if (getsucess == true)
					{

					}
					else
					{
						mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
						mysql_close(&myCont);
						mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存										//cout << "no alarm_set list !\n" << endl;
						return -1;//布防指令表没有待发送的指令，返回-1
					}
			
					mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
			
					gps_data_success = false;
					m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回列名称的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											gps_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											gps_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
										else if (!strcmp(fields[f2].name, "card_state")) //判断当前列的字段名称
										{
											gps_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}
							mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

						}
						else
							gps_time = "2015-11-11 00:00:00";

					}

					m_strToken = "SELECT  *  FROM  card_base_station_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
					getsucess = false;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if (!res)
					{
						//保存查询到的数据到result
						result = mysql_store_result(&myCont);
						num_row = mysql_num_rows(result); //读取行数
						num_col = mysql_num_fields(result); //读取列数
						MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
						if (num_row > 0)
						{
							for (f1 = 0; f1 < 1; f1++) //循环行只取了最新的一行
							{
								mysql_row = mysql_fetch_row(result); //获取每行的内容

								for (f2 = 0; f2 < num_col; f2++) //循环列
								{
									if (fields[f2].name != NULL)
									{
										if (!strcmp(fields[f2].name, "card_socket")) //判断当前列的字段名称
										{
											station_client = atoi(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											station_data_success = true;
											//cout <<"motor_lock: "<< allow_alarm << endl;
										}
										else if (!strcmp(fields[f2].name, "time")) //判断当前列的字段名称
										{
											station_time = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
										}
										else if (!strcmp(fields[f2].name, "card_state")) //判断当前列的字段名称
										{
											station_lock = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL			
											getsucess = true;
											gps_data_success = true;
											//cout<<allow_alarm<<endl;
										}
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据			
								}
							}
							mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
						}
						else
							station_time = "2011-11-11 00:00:00";

					}
					//GPS表和基站表都没有数据，直接放弃发送
					if ((gps_data_success == false) && (station_data_success == false))
					{
						continue;
					}
					else
					{
						//有数据，比对最新数据
						time_t tm_setalarm_time, tm_gps_time, tm_station_time;
						tm_gps_time = convert_string_to_time_t(gps_time);//string转时间类
						tm_station_time = convert_string_to_time_t(station_time);//string转时间类
						tm_setalarm_time = convert_string_to_time_t(setalarm_time);
						double dec_value;//时间差值 
						 //time(&tm_now_time);//获取当前时间  
						dec_value = difftime(tm_gps_time, tm_station_time);//计算时间差值，秒级  
						//printf("gps和基站最新数据时间差:  %f秒\n", dec_value);//
						if (dec_value > 0)
						{
							if (difftime(tm_gps_time, tm_setalarm_time) > 0)//数据比布防指令创建的晚
							{
								ClientS = gps_client;
								card_lock = gps_lock;
								//printf("采用gps数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else
							{
								continue;
							}
						}
						else
						{
							if (difftime(tm_station_time, tm_setalarm_time) > 0)
							{
								ClientS = station_client;
								card_lock = station_lock;
								//printf("采用基站数据记录的端口号作为硬件新socket端口号%d\r\n", ClientS);
							}
							else //数据比布防指令创建的早，不一定是真实的连接。暂放弃发送
							{
								continue;
							}
						}
					}
					//要布防的操作和硬件实际状态一致，就删除布防命令
					if ( (card_lock.find("BCLOSE") != string::npos && (0 == allow_alarm.compare("0"))) ||
						(card_lock.find("BOPEN") != string::npos && (0 == allow_alarm.compare("1"))) )
					{
						m_strToken = "DELETE    FROM  set_device_open   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
						res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
						if (!res)
						{
							total_cnt--;//当前起始行数减去1
							continue;
						}
						else
							continue;
					}
					else
					{
						if (0 == allow_alarm.compare("1"))
						{
							Json::Value root;             // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["deviceopen"] = Json::Value("setdeviceopen");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送开启电源指令！" << endl;
						}
						else
						{
							Json::Value root;             // 表示整个 json 对象
							root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
							root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
							root["deviceopen"] = Json::Value("setdeviceclose");
							root["card"] = Json::Value(str_card);
							Json::FastWriter  fast_writer;//查看json内容对象
							string str = fast_writer.write(root); //json转string	
							send(ClientS, str.c_str(), str.length(), 0);  // 发送信息
							cout << "发送关闭电源指令！" << endl;
						}
					}
				}
			}
			else
			{
				mysql_free_result(setalarm_result); //释放缓存，特别注意，不释放会导致内存增长
			}
		}//查询开电门命令列表
		else
		{
			break;
		}
	}
	 ////////////////////////////////////////////////
	mysql_close(&myCont);
	mysql_library_end();//，记得在 mysql_close 之后调用 mysql_library_end() 来释放未被释放的内存
	return   0;

}

///////////////////////时间转换string转time_t////////////////////////////////////////////////
time_t convert_string_to_time_t(const std::string & time_string)
{
	struct tm tm1;
	time_t time1;
	int i = sscanf(time_string.c_str(), "%d-%d-%d %d:%d:%d",
		&(tm1.tm_year),
		&(tm1.tm_mon),
		&(tm1.tm_mday),
		&(tm1.tm_hour),
		&(tm1.tm_min),
		&(tm1.tm_sec),
		&(tm1.tm_wday),
		&(tm1.tm_yday));

	tm1.tm_year -= 1900;
	tm1.tm_mon--;
	tm1.tm_isdst = -1;
	time1 = mktime(&tm1);

	return time1;

}


