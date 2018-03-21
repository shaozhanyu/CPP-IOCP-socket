
#include "mcom.h"
#include "string.h"


#ifdef  WIN_64_DEF
#include <mysql.h>
#endif

#ifdef  WIN_64_DEF
#pragma comment(lib,"libmysql.lib") 
#endif


#ifdef  WIN_64_DEF
//////////////////////////更新卡状态/////////////////////////////////////////////
int  APPUpdateUserState(SOCKET   ClientS ,Json::Value  mJsonValue)
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
	char bike_gps[150]=",A,3158.4608,N,11848.3737,E,10.05";
	struct tm  ;  

	string   tos ,str_token,str_card , str_username;  
	string  str_lock;

	tos = to_string((long)ClientS);	

	if(mJsonValue["setlock"] == 1)
		str_lock = "1";
	else if(mJsonValue["setlock"] == 0)
		str_lock = "0";

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
	
	string  m_strToken = "SELECT  username  FROM  register_user  WHERE token = '" + str_token + "' ";

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
	string  mSQLStr = "INSERT  INTO user_set(user_id , username, token,app_socket,setlock,card,update_card,time)   VALUES( 1 , '"
		+ str_username + "','"+ str_token + "',"+ tos + ","+ str_lock + ",'"+ str_card+ "',1, NOW(3)) on duplicate key update  user_id = " + 
		"1 ,username = '"  + str_username + "' , token =  '" + str_token + "' , app_socket = " + tos + ", setlock = " + str_lock +", card = '"+ 
		str_card  + "', update_card = 2 , time = NOW(3) " ;


	//string  mSQLStr="UPDATE user_set  SET username = '" + str_username + "' , token =  '" + str_token + "' , app_socket = " + tos + ", setlock = " + str_lock + ", card = '"+ str_card  +
	//	"', update_card = 2 , time = NOW() " ;

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

	cout << "updateSQL error!\n" << endl;

    mysql_close(&myCont);

	return -1;

}

////////////////////APP身份验证-token验证//////////////////////////////
/////////////////////token查询比对///////////////////////////////////
int  FindToken(string  token)
{
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";    

	string  mSQLStr="SELECT count(*) FROM register_user WHERE token = '" + token + "'";
	
	//cout<< mSQLStr<<endl;

	unsigned int port = 3306;        
        
    MYSQL myCont;
    MYSQL_RES  *result ;
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
        my_ulonglong  rowcount = mysql_num_rows(result);

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

		if ( rowcount >0 )
		{
			mysql_close(&myCont);		
			return 0;
		}
		else
		{
			mysql_close(&myCont);
			return -3;
		}    
		
	}

	mysql_close(&myCont);

	return -1;

}

#endif
//////////////////////////////////////////////////////////////////
//数据库null值转换"""
string  getNullStr(const char* str ) 
{
    string res="";
    if (str!=NULL)
    {    res=str;
     }
    return res;
} 

////////////////////////////////////////////////////////////////////

int  GetVaule_CheckStr(string *str , Json::Value  mJsonValue , string  str_value)
{
	string  str_key =  str_value ;

	if(mJsonValue[str_value].isString())
	{
		*str = mJsonValue[str_value].asString();	
	}
	else
	{		
		return  0;

	}
	return 1;
}





