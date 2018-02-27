#include "SocketAPP.h"
#include "TestTask.h"
#include "stdio.h"
#include "SaveQueue.h"
#include "mcom.h"

#ifdef  WIN_64_DEF
#include <mysql.h>
#endif

#ifdef  WIN_64_DEF
#pragma comment(lib,"libmysql.lib") 
#endif

/**************************字符串转JSON对象******************************************/
int  ParseJsonFromString(char *mstr , Json::Value  *mJson)    
{    
	const char* str1 = "{\"uploadid\": \"UP000000\",\"code\": 100,\"msg\": \"\",\"files\": \"\"}";    
    
	Json::Reader reader;     
	if (!reader.parse(mstr, *mJson))  // reader将Json字符串解析到root，root将包含Json里所有子元素     
	return  -1;	

	return 0;    
}  

/*************************JSON数据拼装************************************/
int ParseJsonFromFile(const char* filename)
{
	Json::Value   json_temp;      // 临时对象，供如下代码使用
	json_temp["name"] = Json::Value("huchao");
	json_temp["age"] = Json::Value(26);

	Json::Value root;             // 表示整个 json 对象
	root["key_string"] = Json::Value("value_string");     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
	root["key_number"] = Json::Value(12345);             // 新建一个 Key（名为：key_number），赋予数值：12345。
	root["key_boolean"] = Json::Value(false);             // 新建一个 Key（名为：key_boolean），赋予bool值：false。
	root["key_double"] = Json::Value(12.345);             // 新建一个 Key（名为：key_double），赋予 double 值：12.345。
	root["key_object"] = json_temp;                       // 新建一个 Key（名为：key_object），赋予 json::Value 对象值。
	root["key_array"].append("array_string");             // 新建一个 Key（名为：key_array），类型为数组，对第一个元素赋值为字符串："array_string"。
	root["key_array"].append(1234);                       // 为数组 key_array 赋值，对第二个元素赋值为：1234。
	
	Json::ValueType type = root.type();                   // 获得 root 的类型，此处为 objectValue 类型。
 
	Json::Reader reader;
	Json::Value json_object;
	const char* json_document = "{\"age\" : 26,\"name\" : \"123\"}";
	if (!reader.parse(json_document, json_object))
	return 0;
	std::cout << json_object["name"] << std::endl;
	std::cout << json_object["age"] << std::endl;

	return 0;
}
/**************
char   RecAPPData
接收解析服务器下发数据
*****/
char   RecAPPData( SOCKET ClientS, unsigned char * src, unsigned  int  len )
{
	unsigned  char decdata[6]={ 0x96,0x69,0x96,0x69};
	char  loop=1;
	unsigned  char *pstr;
	unsigned  char *srcdat=src;
	unsigned  char *pdata = src;
	long  DataLen=0;
	SocketRecUnit   mRECdataS; //数据缓存结构体，包括socket连接
	EnterCriticalSection(&mAPP_RECLock);//加锁
	memcpy( mRECdataS.RecData , src  ,len );
	mRECdataS.DataLen = len;
	LeaveCriticalSection(&mAPP_RECLock);//解锁
	srcdat=mRECdataS.RecData;
	pdata = srcdat;
	DataLen = mRECdataS.DataLen;

	if(DataLen >0 && DataLen < REC_SIZE)
	{	
		while(loop)
		{
			 //注意:APP的请求，没有数据区的独立数据个数和每个独立数据的长度，少了2个字节
			if( (DataLen-(pdata-srcdat)) >= APP_MIN_DATA_LEN )
			{
				

				pstr = FindByteBuff(pdata , (DataLen-(pdata-srcdat)) , decdata , 4);//查找命令头
				if(pstr!=0)//查找到包头
				{		
					pdata=pstr;
					if( DataLen-(pdata-srcdat)- 
						(DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN) >= 0 )//判断剩余数据长度是否够一个整包
					{
						switch (*(pdata+4))//判断命令头
						{								
							case  APP_REQUEST_WEILAN_ALARM:
								//cout<<"查询围栏报警\n"<<endl;
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;
								break;
							case  APP_REQUEST_WEILAN_DATA:
								//cout<<"查询围栏设置\n"<<endl;
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;
								break;
							case  APP_SET_LOCK :	//设置围栏					
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;					
							break;
							case   APP_CONFIG_WEILAN:  //启停围栏
							case   APP_SET_UNLOCK:								
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;	
							break;
							
							case  APP_REGESTER_USER:  //APP注册账号
								responseAPPregester( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );
								
								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;
								break;
								break;
							case  APP_SET_DENG_LU :          //APP登录请求
								responseAPPdenglu( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );
								
								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;
								break;
							case  APP_SET_ZHUXIAO  :         //APP注销请求

								break;
							case  APP_SET_BANGDING :         //APP绑定设备请求
								responseAPPbind( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;	
								break;
							case  APP_SET_SHANCHU  :         //APP删除设备请求
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;	
								
								break;
							case  APP_SET_GET_STATE :        //APP查询设备状态请求
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;	
								
								break;
							case  APP_GET_ALL_DEVICE_STATE:  //APP查询名下所有设备状态
								SaveAPPData( ClientS , *(pdata+4) ,(char*)(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据

								if( DataLen-(pdata-srcdat)- 
									(DATA_HEAD_LEN+ *(pdata+14)*256 + *(pdata+15)+DATA_END_LEN) >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
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

/////////////////////////////服务器解析APP操作数据////////////////////////////////////////////////
char  SaveAPPData(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	unsigned  int  i=0;

	int  ret=0;
	Json::Value  m_json;

	/**由于Jsoncpp解析非法json时，会自动容错成字符类型。对字符类型取下标时，会触发assert终止进程。
解决方法：启用严格模式，让非法的json解析时直接返回false，不自动容错。这样，在调用parse的时候就会返回false。
	**/
	Json::Reader *pJsonParser = new Json::Reader(Json::Features::strictMode());
	
	if(!pJsonParser->parse(src, m_json)) //json合法性检查
    {
		cout << "parse error" << endl;
		delete  pJsonParser;
        return -5;
    }
	delete  pJsonParser;
	string   str_token;
	//Json::FastWriter  fast_writer;//查看json内容对象
	//string str = fast_writer.write( m_json["token"]); //json转string
	//cout<<str<<endl;//这里是带引号的""，当做string内容
	if( GetVaule_CheckStr(&str_token , m_json , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	ret = FindToken(str_token);//比对token

	//cout<<"身份验证结束啦啦:  "<<totalcnt<<endl;

	if(0 == ret)	//身份验证成功
	{
		cout<<"身份验证成功"<<endl;

		switch(Command)
		{
			case   APP_SET_LOCK :	//
				#ifdef  WIN_64_DEF
				APPSetWeiLan(ClientS , m_json );
				#endif
				return 0;
			break;
			case   APP_SET_UNLOCK:
				#ifdef  WIN_64_DEF
				APPUnSetWeiLan(ClientS , m_json );
				#endif
				return 0;
			break;

			case   APP_CONFIG_WEILAN:  //启停围栏
				#ifdef  WIN_64_DEF
				//cout<< "启停围栏"<< endl;

				APPConfigWeiLan(ClientS , m_json );
				#endif
				return 0;
			break;
			
			case  APP_SET_ZHUXIAO  :         //APP注销请求

				break;
			case  APP_SET_BANGDING :         //APP绑定设备请求
			
				break;
			case  APP_SET_SHANCHU  :         //APP删除设备请求
				responseAPPjiebang(ClientS , m_json);
				break;
			case  APP_SET_GET_STATE :        //APP查询设备状态请求
				responseAPPgetstate(ClientS , m_json);
				break;
			case  APP_GET_ALL_DEVICE_STATE:  //APP查询名下所有设备状态
				responseAPPgetalldevicestate(ClientS , m_json);
				break;
			case  APP_REQUEST_WEILAN_ALARM: //查询围栏报警状态
				responseAPPrequest_weilan_alarm(ClientS , m_json);
				break;
			case  APP_REQUEST_WEILAN_DATA: //APP查询围栏设置
				responseAPPrequest_weilan_data(ClientS , m_json);
				break;
			default: 
				return -4;
			break;
		}
		
	}
	else //身份验证失败
	{
		cout<<"身份验证失败！"<<endl;
		responseCheckTokenErr(ClientS);
		return -3;
	}

	return 1;
	
}


//////////////////////回复token验证失败/////////////////////////////////////////
int  responseCheckTokenErr(SOCKET   ClientS)
{
	Json::Value root;             // 表示整个 json 对象
	root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
	root["error"] = Json::Value("token_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
	Json::FastWriter  fast_writer;//查看json内容对象
	string str = fast_writer.write(root); //json转string

	//cout<< str<<endl;

	send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 

	return 0;

}
////////////////////////////////////回复用户注册///////////////////////////////////////////////////
int  responseAPPregester(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	bool  getsucess=false;

	Json::Value  m_json;
	Json::Reader *pJsonParser = new Json::Reader(Json::Features::strictMode());
	
	if(!pJsonParser->parse(src, m_json)) //char* 转json并做合法性检查
    {
		cout << "parse error" << endl;
		delete  pJsonParser;
        return -5;
    }
	delete  pJsonParser;
	string  m_username , m_userpwd ,m_phone ,m_serialnumber , m_card ;
	if( GetVaule_CheckStr(&m_username , m_json , "user") == 0 )
	{
		cout<< "json数据类型错误user应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&m_userpwd , m_json , "pwd") == 0 )
	{
		cout<< "json数据类型错误pwd应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&m_phone , m_json , "phone") == 0 )
	{
		cout<< "json数据类型错误pwd应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&m_serialnumber , m_json , "card") == 0 )
	{
		cout<< "json数据类型错误pwd应该为string"<<endl;
		return  -5;
	}
	//cout<<m_userpwd.data()<<endl;

	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    

	string  mSQLStr="SELECT *  FROM cardinfo WHERE serialnumber = '" + m_serialnumber + "'";
	
	cout<< mSQLStr<<endl;

	unsigned int port = 3306;        
        
    MYSQL   myCont;  //mysql 连接
    MYSQL_RES  *result ;  //存储结果
    MYSQL_ROW  mysql_row; //行内容
	
	my_ulonglong  num_row,num_col; 
	string  mstr_pwd=""; //存储用户密码
    int res;

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
    }
    else
    {
        cout << "connect failed!" << endl;
		mysql_close(&myCont);
		return -2;
    }
	///////////////////////////////////////////////////////////////////////
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	
	if(!res  )
	{	
        //保存查询到的数据到result
        result = mysql_store_result(&myCont);

		num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row ==1)
		{
			getsucess = true ;
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(int f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
				{
					m_card = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
	
		}
		else
		{
			getsucess = false;
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{
		
	}
	else
	{
		cout << "check card is not exist!\n" << endl;
		mysql_close(&myCont);
		return -4;
	}

	string  m_strcard = "SELECT  card  FROM  user_bike  WHERE card = '" + m_card + "'";
	cout<< m_strcard<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strcard.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row ==1)
		{
			getsucess = false;
		}
		else
		{
			getsucess = true;
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		cout << "check card  is  used !\n" << endl;
		mysql_close(&myCont);
		return -4;
	}
////
	
	string  m_checkuser = "SELECT  *  FROM  regester_user  WHERE username = '" + m_username + "'";
	cout<< m_checkuser<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_checkuser.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row ==1)
		{
			getsucess = false;
		}
		else
		{
			getsucess = true;
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "check user  already used!\n" << endl;
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("user_already_used");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		
		return -4;
	}
////
	string  m_reguser = "INSERT  INTO  regester_user ( username , userpwd, token ,phone , regester_time )   VALUES( '" +
		m_username + "','" + m_userpwd + "','9m7u8N7x5AFEE2rvfioikjde215CTT','"+ m_phone + "', NOW(3) ) ";

	cout<<m_reguser<<endl;

	res = mysql_query(&myCont, (const  char *)m_reguser.c_str()); //执行SQL语句,添加一条记录
	
	if(!res  )
	{	
		
		mysql_close(&myCont);	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		 
		cout << "//////////regester user  sucess!/////////////\n" << endl;
		return 0;  		
	}
	else
	{
		mysql_close(&myCont);
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("regester_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    
	mysql_close(&myCont);

	return -1;
}
////////////////////////////////////回复登录///////////////////////////////////////////////////
int  responseAPPdenglu(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	Json::Value  m_json;
	Json::Reader *pJsonParser = new Json::Reader(Json::Features::strictMode());
	
	if(!pJsonParser->parse(src, m_json)) //char* 转json并做合法性检查
    {
		cout << "parse error" << endl;
		delete  pJsonParser;
        return -5;
    }
	delete  pJsonParser;
	string  m_username , m_userpwd;
	if( GetVaule_CheckStr(&m_username , m_json , "user") == 0 )
	{
		cout<< "json数据类型错误user应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&m_userpwd , m_json , "pwd") == 0 )
	{
		cout<< "json数据类型错误pwd应该为string"<<endl;
		return  -5;
	}
	//cout<<m_userpwd.data()<<endl;

	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    

	string  mSQLStr="SELECT userpwd  FROM regester_user WHERE username = '" + m_username + "'";
	
	//cout<< mSQLStr<<endl;

	unsigned int port = 3306;        
        
    MYSQL   myCont;  //mysql 连接
    MYSQL_RES  *result ;  //存储结果
    MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2;
	my_ulonglong  num_row,num_col; 
	string  mstr_pwd=""; //存储用户密码
    int res;

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
        
    }
    else
    {
        cout << "connect failed!" << endl;
		mysql_close(&myCont);
		return -2;
    }
	///////////////////////////////////////////////////////////////////////
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	
	if(!res  )
	{	
        //保存查询到的数据到result
        result = mysql_store_result(&myCont);

		num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "userpwd")) //判断当前列的字段名称
				{
					mstr_pwd = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

		//if (! strcmp(mstr_pwd.c_str(), m_userpwd.c_str()) )
		if (mstr_pwd== m_userpwd )
		{
			
			MakeUUID();//生成UUID
			string  uuid_str  = m_uuid;
			string  UpdateCardStateSQL= "UPDATE regester_user SET  token ='" + uuid_str
				+"' WHERE  username='"+m_username+"'";
			///////////////////////////////////////////////////////////////////////
			res = mysql_query(&myCont, (const  char *)UpdateCardStateSQL.c_str()); //执行SQL语句
			cout<<UpdateCardStateSQL.c_str()<<endl;
			if(!res  )
			{
				mysql_close(&myCont);

				Json::Value root;             // 表示整个 json 对象
				root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
				root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
				root["token"] = uuid_str;
				Json::FastWriter  fast_writer;//查看json内容对象
				string str = fast_writer.write(root); //json转string
				send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
				return 0;
			}
			
			else
			{
				mysql_close(&myCont);
				Json::Value root;             // 表示整个 json 对象
				root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
				root["error"] = Json::Value("pwd_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
				Json::FastWriter  fast_writer;//查看json内容对象
				string str = fast_writer.write(root); //json转string

				send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
			
				return -6;
			}
		}
		else
		{
			mysql_close(&myCont);
			Json::Value root;             // 表示整个 json 对象
			root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
			root["error"] = Json::Value("pwd_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
			Json::FastWriter  fast_writer;//查看json内容对象
			string str = fast_writer.write(root); //json转string

			send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
			
			return -3;
		}    
		
	}

	mysql_close(&myCont);

	return -1;
}

//////////////////////////////////////////////////////////////////////////////////
int  responseAPPbangding(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 

	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  

	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	if( GetVaule_CheckStr(&str_phone , mJsonValue , "phone") == 0 )
	{
		cout<< "json数据类型错误phone应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_bikename , mJsonValue , "bikename") == 0 )
	{
		cout<< "json数据类型错误bikename应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_bikecolor , mJsonValue , "bikecolor") == 0 )
	{
		cout<< "json数据类型错误bikecolor应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_biketype , mJsonValue , "biketype") == 0 )
	{
		cout<< "json数据类型错误biketype应该为string"<<endl;
		return  -5;
	}

	//string  mstr_ziduan = mJsonValue["token"].asString();

	string  m_strToken = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
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

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
				{
					str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		if(getsucess == false)
		{
			mysql_close(&myCont);
			cout << "select username error!\n" << endl;
			return -4;
		}
	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  user_bike(bike_id , username, card,phone,bikename,bikecolor,biketype,regester_time)   VALUES( 1, '"
		+ str_username + "','"+ str_card + "','"+ str_phone + "','"+ str_bikename + "','"+ str_bikecolor+ "','"+ str_biketype
		+ "', NOW(3)) on duplicate key update  bike_id = 1 , username = '" + str_username + "', card = '" + str_card + "', phone = '" + str_phone +"', bikename = '" + str_bikename +
		"', bikecolor = '" + str_bikecolor + "', biketype = '"+str_biketype + "', regester_time =  NOW(3) " ;
	

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。
	
	//string  mSQLStr="UPDATE  user_bike  SET  username = '" + str_username + "', card = '" + str_card + "', phone = '" + str_phone +"', bikename = '" + str_bikename +
	//	"', bikecolor = '" + str_bikecolor + "', biketype = '"+str_biketype + "', regester_time ='2017-5-10-10:02:05' ";

	//cout<<mSQLStr<<endl;
	
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	if(!res  )
	{			
		mysql_close(&myCont);	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		
		GetLocalTime( &sys ); 
		printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		return 0;  		
	}
	else
	{
		mysql_close(&myCont);
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("bangding_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);

	return -1;

}


///////////////////////////////绑定设备/////////////////////////////////////
int  responseAPPbind (SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	Json::Value  mJsonValue;
	Json::Reader *pJsonParser = new Json::Reader(Json::Features::strictMode());
	
	if(!pJsonParser->parse(src, mJsonValue)) //char* 转json并做合法性检查
    {
		cout << "parse error" << endl;
		delete  pJsonParser;
        return -5;
    }
	delete  pJsonParser;


	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
	string  sql_userpwd;
	string   str_pwd,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  
	string  m_serialnumber;

	if( GetVaule_CheckStr(&str_username , mJsonValue , "user") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&str_pwd , mJsonValue , "pwd") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	if( GetVaule_CheckStr(&str_phone , mJsonValue , "phone") == 0 )
	{
		cout<< "json数据类型错误phone应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&m_serialnumber , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_bikename , mJsonValue , "bikename") == 0 )
	{
		cout<< "json数据类型错误bikename应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_bikecolor , mJsonValue , "bikecolor") == 0 )
	{
		cout<< "json数据类型错误bikecolor应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_biketype , mJsonValue , "biketype") == 0 )
	{
		cout<< "json数据类型错误biketype应该为string"<<endl;
		return  -5;
	}
	
	 mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

	string  mSQLStr="SELECT *  FROM  cardinfo  WHERE  serialnumber = '" + m_serialnumber + "'";
	
	cout<< mSQLStr<<endl;

	///////////////////////////////////////////////////////////////////////
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	
	if(!res  )
	{	
        //保存查询到的数据到result
        result = mysql_store_result(&myCont);

		num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row ==1)
		{
			getsucess = true ;
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(int f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
				{
					str_card = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
	
		}
		else
		{
			getsucess = false;
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{
		
	}
	else
	{
		cout << "check card is not exist!\n" << endl;
		mysql_close(&myCont);
		return -4;
	}
	//string  mstr_ziduan = mJsonValue["token"].asString();

	string  m_strToken = "SELECT  userpwd  FROM  regester_user  WHERE username = '" + str_username + "'";
	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
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

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "userpwd")) //判断当前列的字段名称
				{
					sql_userpwd = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					if(str_pwd == sql_userpwd)
					{
						getsucess = true;
						cout<<sql_userpwd.data()<<endl;
						break;
					}
					//
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		if(getsucess == false)
		{
			mysql_close(&myCont);
			cout << "select userpwd error!\n" << endl;
			return -4;
		}
	}
	else
	{
		mysql_close(&myCont);
		cout << "select userpwd error!\n" << endl;
		return -4;
	}
	
	string  m_checkuser = "SELECT  *  FROM  user_bike  WHERE card = '" + str_card + "'";
	cout<< m_checkuser<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_checkuser.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >=1)
		{
			getsucess = false;
		}
		else
		{
			getsucess = true;
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "check card already existed !\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	 mSQLStr = "INSERT  INTO  user_bike(  username, card,phone,bikename,bikecolor,biketype,regester_time)   VALUES( '"
		+ str_username + "','"+ str_card + "','"+ str_phone + "','"+ str_bikename + "','"+ str_bikecolor+ "','"+ str_biketype
		+ "', NOW(3))" ;
	//UPDATE不会新增,是覆盖,有几条就覆盖几条。
	
	//string  mSQLStr="UPDATE  user_bike  SET  username = '" + str_username + "', card = '" + str_card + "', phone = '" + str_phone +"', bikename = '" + str_bikename +
	//	"', bikecolor = '" + str_bikecolor + "', biketype = '"+str_biketype + "', regester_time ='2017-5-10-10:02:05' ";

	//cout<<mSQLStr<<endl;
	
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	if(!res  )
	{			
		mysql_close(&myCont);	
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
		mysql_close(&myCont);
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("bangding_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);

	return -1;

}

/////////////////////////获取状态///////////////////////////////////////////
int  responseAPPgetstate(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	Json::Value  mjson_card;  // 表示整个 json 对象
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 

	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  

	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}

	//string  mstr_ziduan = mJsonValue["token"].asString();
	string  m_strname = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

	//cout<<m_strname<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strname.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
				{
					str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}

	string  m_strcard = "SELECT  card  FROM  user_bike  WHERE username = '" + str_username + "'  and  card = '" +str_card +"'";
	//cout<< m_strcard<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strcard.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
				{				
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "check card error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////////////////
	string  m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "' ORDER BY card_id DESC LIMIT 0,1";
	//cout<<m_strToken<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row > 0)
		{	
			mjson_card["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
			mjson_card["error"] = Json::Value("sucess"); 
			mjson_card["card"] = Json::Value(str_card);	

			for(f1=0;f1<1;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					//if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "card_state")) //判断当前列的字段名称
					{										
						mjson_card[fields[f2].name] = Json::Value( getNullStr(mysql_row[f2]) );
						
						getsucess = true ;
								
					}
					else  if(!strcmp( fields[f2].name , "card_lock"))
					{
						int  lockset = *mysql_row[f2] - '0';
						mjson_card[fields[f2].name] = Json::Value( lockset );
						
					}
					else  if(!strcmp( fields[f2].name , "gps"))
					{
						mjson_card[fields[f2].name] = Json::Value( getNullStr(mysql_row[f2]) );
						
					}
					else if (!strcmp( fields[f2].name , "time"))
					{
						mjson_card[fields[f2].name] = Json::Value( getNullStr(mysql_row[f2]) );
						
					}

					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
	
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长	
	}

	mysql_close(&myCont);//关闭连接，否则return后，函数退出了，mysql连接数被占了

	if(getsucess == true )
	{
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(mjson_card); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		
		GetLocalTime( &sys ); 
		printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		return 0; 
	}
	else  if(getsucess == false)
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("get_state error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "get CARD state error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	
    mysql_close(&myCont);

	return -1;

}

//////////////////////////////////////////////////////////////////////////////////
int  responseAPPjiebang(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 

	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  

	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}


	string  m_strToken = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	
    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
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
		if(num_row > 0)
		{
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
					{
						str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
						//cout<<str_username.data()<<endl;
						getsucess =true;
						break;
						//cout<<mstr_pwd.data()<<endl;
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true )
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "DELETE  FROM  user_bike  WHERE  card = '" + str_card+"'  and  username = '" +str_username + "' " ;

	//cout<<mSQLStr<<endl;
	
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	if(!res  )
	{			
		mysql_close(&myCont);	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 

		return 0;  		
	}
	else
	{
		mysql_close(&myCont);
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("jiebang_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "jie bang error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);

	return -1;

}


/////////////////////////获取所有设备状态///////////////////////////////////////////
int  responseAPPgetalldevicestate(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
	Json::Value mjson_cardstate;       
	
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
	my_ulonglong  card_row=0,card_line=0;
	my_ulonglong  base_station_row = 0, base_station_line = 0;
	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  

	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	//string  mstr_ziduan = mJsonValue["token"].asString();
	string  m_strname = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

	//cout<<m_strname<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strname.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
				{
					str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}

	string  m_strcard = "SELECT  *  FROM  user_bike  WHERE username = '" + str_username +"'";
	//cout<< m_strcard<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strcard.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row>0)
		{
			mjson_cardstate["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
			mjson_cardstate["error"] = Json::Value("sucess");
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				str_bikename.clear();
				str_card.clear();
				mysql_row = mysql_fetch_row(result); //获取每行的内容
				
				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if(!strcmp(fields[f2].name , "bikename"))
					{
						str_bikename =  getNullStr(mysql_row[f2]);		
			
					}
					
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
					{				
						//cout<<str_username.data()<<endl;
						str_card =  getNullStr(mysql_row[f2]);										
					
					}//endif card
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}//endfor 列
			
				if(str_card.length()>0   )
				{
					string  m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "' ORDER BY card_id DESC LIMIT 0,1";
					//cout<<m_strToken<<endl;
					res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
					if(!res  )
					{				
						//保存查询到的数据到result
						MYSQL_RES *card_result = mysql_store_result(&myCont);
						card_row=mysql_num_rows(card_result); //读取行数
						card_line=mysql_num_fields(card_result); //读取列数
						MYSQL_FIELD*  card_files = mysql_fetch_fields(card_result); //返回所有字段结构的数组
						if(card_row > 0)
						{	
							getsucess = true;
							Json::Value  mjson_card;  // 表示整个 json 对象
							mjson_card["card"] = Json::Value(str_card);	
							mjson_card["bikename"] = Json::Value(str_bikename);	
							mysql_row = mysql_fetch_row(card_result); //获取每行的内容

							for(int card_f2 =0;card_f2<card_line;card_f2++) //循环列
							{	
								//if (fields[f2].name!=NULL)  
								if (!strcmp( card_files[card_f2].name , "card_state")) //判断当前列的字段名称
								{										
									mjson_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
									getsucess = true ;
						
								}
								else  if(!strcmp( card_files[card_f2].name , "card_lock"))
								{
									int  lockset = *mysql_row[card_f2] - '0';
									mjson_card[card_files[card_f2].name] = Json::Value( lockset );
				
								}
								else  if(!strcmp( card_files[card_f2].name , "gps"))
								{
									mjson_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
								}
								else if (!strcmp( card_files[card_f2].name , "time"))
								{
									mjson_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
								}

								//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
	
							}//endfor 列

							mysql_free_result(card_result); //释放缓存，特别注意，不释放会导致内存增长	
							//mjson_cardstate["data"] = Json::Value( mjson_card );
							mjson_cardstate["data"].append( Json::Value(mjson_card) );//添加子
						
						}//endif
					}
					//////////////////////////////查询基站上传数据表///////////////////////////
					string  m_sql_base_station = "SELECT  *  FROM  card_base_station_data  WHERE card = '" + str_card + "' ORDER BY card_id DESC LIMIT 0,1";
					//cout<<m_strToken<<endl;
					res = mysql_query(&myCont, (const  char *)m_sql_base_station.c_str()); //执行SQL语句,通过token查找username
					if(!res  )
					{				
						//保存查询到的数据到result
						MYSQL_RES *base_station_result = mysql_store_result(&myCont);
						base_station_row=mysql_num_rows(base_station_result); //读取行数
						base_station_line=mysql_num_fields(base_station_result); //读取列数
						MYSQL_FIELD*  card_files = mysql_fetch_fields(base_station_result); //返回所有字段结构的数组
						if(base_station_row > 0)
						{	
							getsucess = true;
							Json::Value  mbase_json_card;  // 表示整个 json 对象
							mbase_json_card["card"] = Json::Value(str_card);	
							mbase_json_card["bikename"] = Json::Value(str_bikename);	
							mysql_row = mysql_fetch_row(base_station_result); //获取每行的内容

							for(int card_f2 =0;card_f2<card_line;card_f2++) //循环列
							{	
								//if (fields[f2].name!=NULL)  
								if (!strcmp( card_files[card_f2].name , "card_state")) //判断当前列的字段名称
								{										
									mbase_json_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
									getsucess = true ;
						
								}
								else  if(!strcmp( card_files[card_f2].name , "card_lock"))
								{
									int  lockset = *mysql_row[card_f2] - '0';
									mbase_json_card[card_files[card_f2].name] = Json::Value( lockset );
				
								}
								else  if(!strcmp( card_files[card_f2].name , "base_station"))
								{
									mbase_json_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
								}
								else if (!strcmp( card_files[card_f2].name , "time"))
								{
									mbase_json_card[card_files[card_f2].name] = Json::Value( getNullStr(mysql_row[card_f2]) );
				
								}

								//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
	
							}//endfor 列

							mysql_free_result(base_station_result); //释放缓存，特别注意，不释放会导致内存增长	
							//mjson_cardstate["data"] = Json::Value( mbase_json_card );
							mjson_cardstate["data"].append( Json::Value(mbase_json_card) );//添加子
						
						}//endif
					}
				}//endif 卡号长度和车辆名称
			}//endfor 行
			mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		}
		else
		{
			mysql_close(&myCont);
			return -6;
		}
	}
	mysql_close(&myCont);//及时关闭sql连接，否则占用mysql连接数

	if(getsucess == true )
	{
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(mjson_cardstate); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		return 0; 
	}
	else  if(getsucess == false)
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("get_state error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "get CARD state error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	
    mysql_close(&myCont);

	return -1;

}

///////////////////////设置围栏/////////////////////////////////////////////
int  APPSetWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    
	char bike_username[32]="";
	char bike_userpwd[32]="";
	char bike_token[66]="";
	int  bike_app_socket=0;
	int  bike_setlock=0;
	int  bike_update_card =0;
	char bike_card[32]="";
	int  bike_card_socket =1;
	char bike_card_state='I';
	int  bike_card_lock=0;
	char bike_bike_name[64]="";
	char bike_gps[200]=",A,3158.4608,N,11848.3737,E,10.05";
	struct tm  ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string  str_lock;
	string   radius;

	tos = to_string((_ULonglong)ClientS);	
	
	if(mJsonValue["radius"].isInt())
	{
		radius = to_string( (_Longlong)mJsonValue["radius"].asInt());
	}
	else
	{		
		cout<< "json数据value类型错误radius应该为int"<<endl;
		return  -5;

	}

	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}	

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&str_gps , mJsonValue , "gps") == 0 )
	{
		cout<< "json数据类型错误gps应该为string"<<endl;
		return  -5;
	}
	
	
	string  m_strToken = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "' ";

	unsigned int port = 3306;        
        
    MYSQL myCont;
    MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
    int res;

	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

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
		if(num_row > 0)
		{
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
					{
						str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
						//cout<<str_username.data()<<endl;
						f1=num_row;
						f2=num_col;
						getsucess =true;
						break;
						//cout<<mstr_pwd.data()<<endl;
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess ==true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////
	//INSERT 会新增一条
	//string  mSQLStr = "INSERT  INTO user_set(username, token,app_socket,setlock,card,update_card,time)   VALUES('"
	//	+ str_username + "','"+ str_token + "',"+ tos + ","+ str_lock + ",'"+ str_card+ "','1', NOW())" ;

	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	//这里把card设置为主键
	//string  mSQLStr="UPDATE user_set  SET username = '" + str_username + "' , token =  '" + str_token + "' , app_socket = " + tos + ", setlock = " + str_lock + ", card = '"+ str_card  +
	//	"', update_card = 2 , time = NOW() " ;
	string  mSQLStr = "INSERT  INTO  alarm_weilan(  card, card_socket,allow_alarm,radius,gps,time)   VALUES(  '"
		+ str_card + "','"+ tos + "' , 1  ," +  radius + ",'" + str_gps + "' , NOW(3)) on duplicate key update  " + 
		" card = '"  + str_card + "' , card_socket =  '" + tos + "' , allow_alarm = 1 , radius = " + radius + " , gps = '"+ 
		str_gps +"' , time = NOW(3) " ;

	//cout<<mSQLStr<<endl;
	
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	mysql_close(&myCont);
	if(!res  )
	{			
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
	
		GetLocalTime( &sys ); 
		printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("lock_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
			
		return -3;
	}

	cout << "set weilan error!\n" << endl;

    mysql_close(&myCont);

	return -1;

}

///////////////////////取消围栏/////////////////////////////////////////////
int  APPUnSetWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    
	char bike_username[32]="";
	char bike_userpwd[32]="";
	char bike_token[66]="";
	int  bike_app_socket=0;
	int  bike_setlock=0;
	int  bike_update_card =0;
	char bike_card[32]="";
	int  bike_card_socket =1;
	char bike_card_state='I';
	int  bike_card_lock=0;
	char bike_bike_name[64]="";
	char bike_gps[200]=",A,3158.4608,N,11848.3737,E,10.05";
	struct tm  ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string  str_lock;
	string   radius;

	tos = to_string((_ULonglong)ClientS);	

	if(mJsonValue["radius"].isInt())
	{
		radius = mJsonValue["radius"].asString();
	}
	else
	{		
		cout<< "json数据value类型错误radius应该为int"<<endl;
		return  -5;

	}

	
	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}	

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&str_gps , mJsonValue , "gps") == 0 )
	{
		cout<< "json数据类型错误gps应该为string"<<endl;
		return  -5;
	}
	string  m_strToken = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "' ";

	unsigned int port = 3306;        
        
    MYSQL myCont;
    MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
    int res;

	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

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
		if(num_row > 0)
		{
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
					{
						str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
						//cout<<str_username.data()<<endl;
						f1=num_row;
						f2=num_col;
						getsucess =true;
						break;
						//cout<<mstr_pwd.data()<<endl;
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess ==true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////
	//INSERT 会新增一条
	//string  mSQLStr = "INSERT  INTO user_set(username, token,app_socket,setlock,card,update_card,time)   VALUES('"
	//	+ str_username + "','"+ str_token + "',"+ tos + ","+ str_lock + ",'"+ str_card+ "','1', NOW())" ;

	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容

	//string  mSQLStr="UPDATE user_set  SET username = '" + str_username + "' , token =  '" + str_token + "' , app_socket = " + tos + ", setlock = " + str_lock + ", card = '"+ str_card  +
	//	"', update_card = 2 , time = NOW(3) " ;
	string  mSQLStr = "INSERT  INTO  alarm_weilan(  card, card_socket,allow_alarm,radius,gps,time)   VALUES(  '"
		+ str_card + "','"+ tos + "' , 0  ," +  radius + ",'" + str_gps + "' , NOW(3)) on duplicate key update  " + 
		" card = '"  + str_card + "' , card_socket =  '" + tos + "' , allow_alarm = 0 , radius = " + radius + " , gps = '"+ 
		str_gps +"' , time = NOW(3) " ;

	//cout<<mSQLStr<<endl;

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	mysql_close(&myCont);
	if(!res  )
	{			
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
	
		GetLocalTime( &sys ); 
		printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("close_weilan_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
			
		return -3;
	}

	cout << "updateSQL error!\n" << endl;

    mysql_close(&myCont);

	return -1;

}


///////////////////////启停围栏/////////////////////////////////////////////
int  APPConfigWeiLan(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    
	char bike_username[32]="";
	char bike_userpwd[32]="";
	char bike_token[66]="";
	int  bike_app_socket=0;
	int  bike_setlock=0;
	int  bike_update_card =0;
	char bike_card[32]="";
	int  bike_card_socket =1;
	char bike_card_state='I';
	int  bike_card_lock=0;
	char bike_bike_name[64]="";
	char bike_gps[200]=",A,3158.4608,N,11848.3737,E,10.05";
	struct tm  ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string  str_lock;
	string   radius , allow_alarm;

	tos = to_string((_ULonglong)ClientS);	

	//cout<< mJsonValue["allow_alarm"] << endl;

	if( GetVaule_CheckStr(&allow_alarm , mJsonValue , "allow_alarm") == 0 )
	{
		cout<< "json数据类型错误 allow_alarm 应该为string"<<endl;
		return  -5;
	}
	
	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}	

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	
	string  m_strToken = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "' ";

	unsigned int port = 3306;        
        
    MYSQL myCont;
    MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
    int res;

	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

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
		if(num_row > 0)
		{
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
					{
						str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
						//cout<<str_username.data()<<endl;
						f1=num_row;
						f2=num_col;
						getsucess =true;
						break;
						//cout<<mstr_pwd.data()<<endl;
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess ==true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////
	string  mSQLStr="UPDATE alarm_weilan  SET allow_alarm = '" + allow_alarm + "' , time = NOW(3)  WHERE  card  =  '" + str_card +"' ";

	//cout<<mSQLStr<<endl;
	
	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句
	mysql_close(&myCont);
	if(!res  )
	{			
		cout<<"修改围栏成功"<<endl;
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		//cout<<str.data()<<endl;
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("config_weilan_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
			
		return -3;
	}

	cout << "updateSQL error!\n" << endl;

    mysql_close(&myCont);

	return -1;

}


/////////////////////////请求围栏报警状态///////////////////////////////////////////
int  responseAPPrequest_weilan_alarm(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
	Json::Value mjson_cardstate;       
	
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
	my_ulonglong  card_row=0,card_line=0;
	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  
	string  gps_data;
	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	
	//string  mstr_ziduan = mJsonValue["token"].asString();
	string  m_strname = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

	//cout<<m_strname<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strname.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
				{
					str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}

	string  m_strcard = "SELECT  card  FROM  user_bike  WHERE username = '" + str_username +"'";
	//cout<< m_strcard<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strcard.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row>0)
		{
			mjson_cardstate["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
			mjson_cardstate["error"] = Json::Value("sucess");
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
					{				
						//cout<<str_username.data()<<endl;
						str_card =  getNullStr(mysql_row[f2]);										
						
						string  m_strToken = "SELECT  *  FROM  card_data  WHERE card = '" + str_card + "' ORDER BY card_id DESC LIMIT 0,1";
						//cout<<m_strToken<<endl;
						res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
						if(!res  )
						{				
							//保存查询到的数据到result
							MYSQL_RES *card_result = mysql_store_result(&myCont);
							card_row=mysql_num_rows(card_result); //读取行数
							card_line=mysql_num_fields(card_result); //读取列数
							MYSQL_FIELD*  card_files = mysql_fetch_fields(card_result); //返回所有字段结构的数组
							if(card_row > 0)
							{	
								getsucess = true;
								//Json::Value  mjson_card;  // 表示整个 json 对象
								//mjson_card["card"] = Json::Value(str_card);	
								mysql_row = mysql_fetch_row(card_result); //获取每行的内容

								for(int card_f2 =0;card_f2<card_line;card_f2++) //循环列
								{	
									//if (fields[f2].name!=NULL)  
									if(!strcmp( card_files[card_f2].name , "gps"))
									{
										gps_data = getNullStr( mysql_row[card_f2]);
										APPCheckAlarmTable(ClientS , gps_data , str_card);//查找指定card围栏设防表
									}
									//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
								}//endfor 列

								mysql_free_result(card_result); //释放缓存，特别注意，不释放会导致内存增长	
								//mjson_cardstate["data"] = Json::Value( mjson_card );
								
							}//endif
						
						}//endif
					
					}//endif card
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}//endfor 列
			}//endfor 行
			mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		}
		else
		{
			mysql_close(&myCont);
			return -6;
		}
	}
	
	mysql_close(&myCont);//及时关闭sql连接，否则占用mysql连接数

	if(getsucess == true )
	{
		//Json::FastWriter  fast_writer;//查看json内容对象
		//string str = fast_writer.write(mjson_cardstate); //json转string

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		return 0; 
	}
	else  if(getsucess == false)
	{
		//Json::Value root;             // 表示整个 json 对象
		//root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		//root["error"] = Json::Value("get_state error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		//Json::FastWriter  fast_writer;//查看json内容对象
		//string str = fast_writer.write(root); //json转string

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "get CARD state error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	
    mysql_close(&myCont);

	return -1;

}

//////////////////////查找指定card围栏设防表//////////////////////////////////////////
int  APPCheckAlarmTable(SOCKET ClientS , string gps , string  card)
{
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";  
	unsigned int port = 3306;   
    char table[] = "bike";    
	char bike_username[32]="";
	char bike_userpwd[32]="";
	char bike_token[66]="";
	int  bike_app_socket=0;
	int  bike_setlock=0;
	int  bike_update_card =0;
	
	int  bike_card_socket =1;
	char bike_card_state='I';
	int  bike_card_lock=0;
	char bike_bike_name[64]="";
	char bike_gps[200]=",A,3158.4608,N,11848.3737,E,10.05";
	struct tm  ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string   str_lock;
	string   radius;
	string   weilan_gps;
	string   weilan_radius;
	string   allow_alarm;
	string  m_strToken = "SELECT  *  FROM  alarm_weilan  WHERE card = '" + card + "' ORDER BY card_id DESC LIMIT 0,1 ";
	            
    MYSQL myCont;
    MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
    int res;

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式    
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }
	cout<<card <<"查询围栏状态！"<<endl;
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
		if(num_row > 0)
		{
			for(f1=0;f1<1;f1++) //循环行只取了最新的一行
			{		
				mysql_row = mysql_fetch_row(result); //获取每行的内容

				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					{
						if (!strcmp( fields[f2].name , "gps")) //判断当前列的字段名称
						{
							weilan_gps = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
							//cout<<str_username.data()<<endl;
							
							getsucess =true;
							
							//cout<<mstr_pwd.data()<<endl;
						}
						else  if (!strcmp( fields[f2].name , "radius")) //判断当前列的字段名称
						{
							weilan_radius = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
							//cout<<str_username.data()<<endl;
							
							getsucess =true;
							
							//cout<<mstr_pwd.data()<<endl;
						}
						if (!strcmp( fields[f2].name , "allow_alarm")) //判断当前列的字段名称
						{
							allow_alarm = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
							if( 0 == allow_alarm.compare("1"))
							getsucess =true;
							else  if( 0 == allow_alarm.compare("0"))
							{
								getsucess = false;
								cout << "围栏未启用" << endl;
								cout<<"///////////////////////////////////////"<<endl;
								return  -10;
							}
							//cout<<mstr_pwd.data()<<endl;
						}
					}
					
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}
			}
		}
		
		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	
	mysql_close(&myCont);
	
	if(getsucess ==true)
	{

	}
	else
	{
		cout << "select alarm_weilan error!\n" << endl;
		return -4;
	}
	
	//cout << "panduan weilan !\n" << endl;
	
	//cout << weilan_radius << endl;
	
	//cout << weilan_gps << endl;
	
	//cout << gps << endl;
////////////////////////////////判断围栏是否超出///////////////////////////////////////
	if(PanDuanWeiLan( weilan_radius , weilan_gps , gps ) >0 ) //判断围栏超出
	{
		
		cout << "alarm weilan !\n" << endl;
		
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("weilan_alarm"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		return 0;
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("no weilan_alarm");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		return -6;	
	}

	mysql_close(&myCont);
	return -1;

}


#define       PI         3.141592657
#define      EARTH_RADIUS    6378137


static double GPSdufen_to_du(double dufen)
{
	double gps_du;
	int  du = (int) (dufen/100);
	
	double  fen = dufen - du*100;
	gps_du = du + fen /60.00; //转换为度格式
	return  gps_du;
}

static inline double  rad( double degree )
{
    return  PI * degree / 180.0;
}
static inline double haverSin(double x)
{
    double v = sin(x / 2.0);
    return v * v;
}


//计算距离(单位 : m)
static double getDistance(double lon1, double lat1, double lon2, double lat2)
{
    double radlon1 = rad(lon1);
    double radlat1 = rad(lat1);
    double radlon2 = rad(lon2);
    double radlat2 = rad(lat2);
    
    double a = fabs(radlat1 - radlat2);
    double b = fabs(radlon1 - radlon2);
    
    double h = haverSin(b) + cos(lat1)*cos(lat2)*haverSin(a);
    double distance = 2 * EARTH_RADIUS * asin(sqrt(h));
    return  distance;
}

//////////////////////////////////判断移动距离和围栏是否有进出///////////////////////////////////////////////
int  PanDuanWeiLan(string  weilan_radius , string  weilan_gps  , string  gps)
{
	double  weilan_lon , weilan_lat , gps_lon, gps_lat;
	string  str_weilan_lon , str_weilan_lat, str_gps_lon , str_gps_lat;
	int  pos= -1;
	int  edpos = -1;
	int len=0;
	int sub_len=0;
	double  alarm_radius = atof(weilan_radius.c_str());
	double  distance;
/*************
	38.01348959752763,114.47717175856118

	,A,3800.3858,N,11427.8776,E,10.05
***************/

	edpos = weilan_gps.find(",");
	if(edpos>=0)
	{
		len = edpos-0;
		
		str_weilan_lat =  weilan_gps.substr(0,len);
		weilan_lat = atof(str_weilan_lat.c_str());
		
		len = weilan_gps.length() - edpos -1;		
		str_weilan_lon = weilan_gps.substr(edpos+1,len);			
		weilan_lon = atof(str_weilan_lon.c_str());
	}		
		
//////////////////////////////////////////////////////////////////////////	
	pos = gps.find("A,");
	
	if(pos>=0)
	{
		edpos = gps.find(",N,");
		if(edpos>=0)
		{
			sub_len = strlen("A,");
			len = edpos-pos-sub_len;
			
			str_gps_lat =  gps.substr(pos+sub_len,len);
			gps_lat = atof(str_gps_lat.c_str());
			
			sub_len = strlen(",N,");
		
			str_gps_lon = gps.substr(edpos+sub_len);			
			gps_lon = atof(str_gps_lon.c_str());
		
		}		
		
	}

	gps_lon = GPSdufen_to_du(gps_lon);//经纬度转换成度为单位
	gps_lat = GPSdufen_to_du(gps_lat);//经纬度转换成度为单位
	cout << "WEILAN_GPS_LON:"<< weilan_lon << endl;
	cout << "WEILAN_GPS_LAT:"<<weilan_lat << endl;
	cout <<"HARD_GPS_LON:"<<  gps_lon <<endl;
	cout <<"HARD_GPS_LAT:"<< gps_lat <<endl;
	cout << "ALARM_RADIUS(meter):"<<alarm_radius <<endl;
	
	//计算两点经纬度之间直线距离
	distance = getDistance(weilan_lon , weilan_lat , gps_lon , gps_lat);
	cout<<"偏差距离："<< distance <<endl;
	cout << "///////////////////////////////////"<<endl;
	if(distance > alarm_radius) //超出围栏
	{
		return  1;
	}
	
	return  0;
	
	
}

/////////////////////////请求围栏配置数据//////////////////////////////////////////
int  responseAPPrequest_weilan_data(SOCKET   ClientS ,Json::Value  mJsonValue)
{
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
	Json::Value mjson_cardstate;       
	
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	MYSQL_ROW  mysql_row; //行内容
	my_ulonglong  f1,f2,num_row,num_col; 
	my_ulonglong  card_row=0,card_line=0;
	string   str_token,str_username ,str_card,str_phone,str_bikename,str_bikecolor,str_biketype;  
	string  gps_data;
	if( GetVaule_CheckStr(&str_token , mJsonValue , "token") == 0 )
	{
		cout<< "json数据类型错误token应该为string"<<endl;
		return  -5;
	}
	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	//string  mstr_ziduan = mJsonValue["token"].asString();
	string  m_strname = "SELECT  username  FROM  regester_user  WHERE token = '" + str_token + "'";
	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }

	//cout<<m_strname<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strname.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组

		for(f1=0;f1<num_row;f1++) //循环行
		{		
			mysql_row = mysql_fetch_row(result); //获取每行的内容

			for(f2=0;f2<num_col;f2++) //循环列
			{	
				if (fields[f2].name!=NULL)  
				if (!strcmp( fields[f2].name , "username")) //判断当前列的字段名称
				{
					str_username = getNullStr(mysql_row[f2]); //获取字段内容，里面判断非NULL
					//cout<<str_username.data()<<endl;
					getsucess = true;
					break;
					//cout<<mstr_pwd.data()<<endl;
				}
				//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
			}
		}

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
		
	}
	if(getsucess == true)
	{
		
	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}

	string  m_strcard = "SELECT  *  FROM  alarm_weilan  WHERE card = '" + str_card + "' ORDER BY time DESC LIMIT 0,1";
	//cout<< m_strcard<<endl;
	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strcard.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row>0)
		{
			mjson_cardstate["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
			mjson_cardstate["error"] = Json::Value("sucess");
			Json::Value  mjson_card;  // 表示整个 json 对象
			mjson_card["card"] = Json::Value(str_card);	
			mysql_row = mysql_fetch_row(result); //获取查询数据内容
			
			for(f1=0;f1<num_row;f1++) //循环行
			{		
				
				for(f2=0;f2<num_col;f2++) //循环列
				{	
					if (fields[f2].name!=NULL)  
					{
						if (!strcmp( fields[f2].name , "card")) //判断当前列的字段名称
						{				
							//cout<<str_username.data()<<endl;
							str_card =  getNullStr(mysql_row[f2]);										
						}//endif card
						else  if (!strcmp( fields[f2].name , "radius")) //判断当前列的字段名称
						{				
							string  radius =  getNullStr(mysql_row[f2]);
							mjson_card[fields[f2].name] = Json::Value( radius );
							//cout<<"围栏半径："<<radius<<endl;
						}
						else  if (!strcmp( fields[f2].name , "allow_alarm")) //判断当前列的字段名称
						{				
							string  allow =  getNullStr(mysql_row[f2]);
							mjson_card[fields[f2].name] = Json::Value( allow );
							//cout<<"围栏报警允许："<<radius<<endl;
						}
						else  if (!strcmp( fields[f2].name , "gps")) //判断当前列的字段名称
						{				
							mjson_card[fields[f2].name] = Json::Value( getNullStr(mysql_row[f2]) );
														
						}
						else  if (!strcmp( fields[f2].name , "time")) //判断当前列的字段名称
						{				
							mjson_card[fields[f2].name] = Json::Value( getNullStr(mysql_row[f2]) );
														
						}
													
					}
										
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //打印每行的各个列数据
			
				}//endfor 列
			}//endfor 行
			getsucess =true;
			mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长
			mjson_cardstate["data"].append( Json::Value(mjson_card) );//添加子项
		}
		else
		{
			mysql_close(&myCont);
			return -6;
		}
	}
	
	mysql_close(&myCont);//及时关闭sql连接，否则占用mysql连接数

	if(getsucess == true )
	{
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(mjson_cardstate); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		return 0; 
	}
	else  if(getsucess == false)
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("get_weilan_state error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "get_weilan_state error!\n" << endl;
		return -4;
	}
///////////////////////////////////////////////////////////////////////	
	
    mysql_close(&myCont);

	return -1;

}











