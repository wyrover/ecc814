 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//六、主程序类：
//    1、数据结构
//		a、一个AlertBaseObjList, 类型为基础报警配置类。
//		b、四个WaitSendAlertObjList, 类型为报警发送对象类。
//		c、一个输入EventList, 类型为输入事件类。
//		d、线程：输入、输出。
//    2、控制流程
//		a、读alert.ini以初始化AlertBaseObjList， 并分解出正确的AlertTargetList。
//		b、循环读取输入Event到EventList。
//		c、循环根据AlertBaseObjList和EventList里的Event匹配，并生成AlertSendObj且推送到WaitSendAlertObjList。
//		d、循环将WaitSendAlertObjList发送出去， 维护WaitSendAlertObjList并记录日志。
//		e
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <typeinfo>
#include "AlertMain.h"
#include "AlertBaseObj.h"
#include "AlertSendObj.h"
#include "AlertEventObj.h"
#include <cc++/numbers.h>
#include "svdb/svapi/svapi.h"
#include "svdb/svapi/svdbapi.h"
#include "SerialPort.h"
//#import  < msxml4.dll > 
#include   <tlhelp32.h>   

#include <algorithm>

int nSendCount = 0;
static CSerialPort m_smsPort;
string CAlertMain::strDisable = "";
string CAlertMain::strNormal = "";
string CAlertMain::strWarning = "";
string CAlertMain::strError = "";
string CAlertMain::strOther = "";

string CAlertMain::strENDisable = "";
string CAlertMain::strENNormal = "";
string CAlertMain::strENWarning = "";
string CAlertMain::strENError = "";
string CAlertMain::strENOther = "";

string CAlertMain::strMontorFreq = "";
string CAlertMain::strHour = "";
string CAlertMain::strMinute = "";
string CAlertMain::strMonitorFazhi = "";

string CAlertMain::strWeiHuUserIdcId = "";
string CAlertMain::strSysWeihuUserIdcId = "";
string CAlertMain::strYeWuIdcId = "";
string CAlertMain::strXiaoshouIdcId = "";
int    CAlertMain::nSMSSendLength=50;//初始化短信发送的长度
string    CAlertMain::strSMSSendURL;//短信发送服务器的地址
string    CAlertMain::strSMSSendPort;//短信发送服务器的端口
//#define AddOnEventSever

//#define DebugToFile

void DebugePrint(string strDebugInfo)
{
	#ifndef DebugToFile
		printf("%s\n",strDebugInfo.c_str());
	#else
		printf(strDebugInfo.c_str());
		FILE *fp;
		//fp=fopen("\\Release\\debug.txt","a+");
		fp=fopen("alertServer.log","a+");

		long   int   fileLength=fseek(fp,0L,SEEK_END); 

		if(fileLength<2)
            fprintf(fp,"%s\n",strDebugInfo.c_str());
		else
		{
			fclose(fp);
			fp=fopen("alertServer.log","w");
		}

		fclose(fp);		
	#endif
	return;
}

char strTemp[1024];


//===============================================================
// 打印日志函数
//===============================================================
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

int WriteLog( const char* str )
{
	char timebuf[128],datebuf[128];

	_tzset();
	_strtime( timebuf );
	_strdate( datebuf );

	char szLogFile[] = "alertServer.log";

	// 判断文件大小：在不打开文件的情况下实现
	struct _stat buf;
	if( _stat( szLogFile, &buf ) == 0 )
	{
		if( buf.st_size > 1000*1024 )
		{
			FILE *log = fopen( szLogFile, "w" );
			if( log != NULL )
				fclose( log );
		}
	}


	FILE *log = fopen( szLogFile,"a+");
	if( log != NULL )
	{
		fprintf( log, "%s %s \t%s\n", datebuf, timebuf, str );
		fclose( log );
	}

	return 1;

}

void WriteErr(const char * str)
{

}

#include "./Base/des.h"
#include "afxinet.h"
#include "windows.h"

typedef unsigned char BYTE;

inline BYTE toHex(const BYTE &x)
{
	return x > 9 ? x + 55: x + 48;
}

string urlEncoding( string &sIn )
{
	string sOut;
	for( int ix = 0; ix < static_cast<int>(sIn.size()); ix++ )
	{
		BYTE buf[4];
		memset( buf, 0, 4 );
		if( isalnum( (BYTE)sIn[ix] ) )
		{
			buf[0] = sIn[ix];
		}
		else if ( isspace( (BYTE)sIn[ix] ) )
		{
			buf[0] = '+';
			cout << "sp" << endl;
		}
		else
		{
			buf[0] = '%';
			buf[1] = toHex( (BYTE)sIn[ix] >> 4 );
			buf[2] = toHex( (BYTE)sIn[ix] % 16);
		}
		sOut += (char *)buf;
	}
	return sOut;
}

BOOL GetSourceHtml(char const * theUrl, char * retState) 
{
	CInternetSession session;
	CInternetFile* file = NULL;
	try
	{
		// 试着连接到指定URL
		file = (CInternetFile*) session.OpenURL(theUrl); 
	}
	catch (...)
	{
		// 如果有错误的话，置内容为空
		strcpy(retState, "error");
		return FALSE;
	}

	if (file)
	{
		CString  somecode;

		bool flagReplace = false;
		int replaceNum = 0;
		while (file->ReadString(somecode) != 0 ) //如果采用LPTSTR类型，读取最大个数nMax置0，使它遇空字符时结束
		{
			strncpy( retState, somecode, 1000 );
		}

	}
	else
	{
		strcpy(retState, "error");
		return FALSE;
	}
	return TRUE;
}

 #include <list>
#import "msxml.dll" //引入类型库 
bool WriteXML(string title,list<string> key,list<string> context,string filename)
{
	AfxOleInit(); 
	MSXML::IXMLDOMDocumentPtr pDoc; 
	// 创建DOMDocument对象 
	HRESULT hr;
	hr   =   CoInitialize(NULL);   

	hr =pDoc.CreateInstance(__uuidof(MSXML::DOMDocument)); 

	if ( ! SUCCEEDED(hr)) 
	{  
		return  false;
	}  


	MSXML::IXMLDOMElementPtr xmlRoot=pDoc->createElement("title"); 
	pDoc->appendChild(xmlRoot);  

	// 根节点的名称为title
	// 创建元素并添加到文档中 
	const _variant_t  var1;

	// 设置属性 
//	xmlRoot -> setAttribute( " id " ,( const   char   * )title.c_str());
//	pDoc -> appendChild(xmlRoot);
	MSXML::IXMLDOMElementPtr pNode;
	std::list<string>::iterator	 itkey,itcontext;
	itcontext=context.begin();
	for(itkey=key.begin();itkey!=key.end();itkey++)
	{
		// 添加相应的元素 
		pNode = pDoc -> createElement((_bstr_t) (*itkey).c_str());
		pNode -> Puttext((_bstr_t)( const   char   * )(*itcontext).c_str());
		xmlRoot -> appendChild(pNode);
		itcontext++;
	}
	// 保存到文件 
	// 如果不存在就建立,存在就覆盖  
	pDoc -> save( filename.c_str());
	CoUninitialize();
	return true;
}


bool WebSmsTest(string phoneNumber, string content)
{
	WriteLog("--------WebSmsTest Start--------");
	char strInfo[1024] = {0};

	bool bRet = true;

	string User("test");
	string Pwd("testpwd123");

	string strWebHead;
	//从短信模板中读出短信头的模板
	strWebHead = GetIniFileString("WebSmsConfige", "WebDefine", "", "TxtTemplate.Ini");

	sprintf(strInfo , "从短信模板中读出短信头的模板:%s" , strWebHead.c_str());
	WriteLog(strInfo);

	size_t nLength = strWebHead.length();
	size_t nPos=strWebHead.find("\\;");
	//如果找到了‘\;’
	if (nPos!=string::npos)
	{
		strWebHead=strWebHead.substr(0,nPos);
	}
	else//如果没找到
		strWebHead = "";//将标题模板设置为空


	User = GetIniFileString("SMSWebConfig", "User", "", "smsconfig.ini");
	Pwd = GetIniFileString("SMSWebConfig", "Pwd", "", "smsconfig.ini");
	Des mydes;
	char dechar[1024]={0};
	if(Pwd.size()>0)
	{
		mydes.Decrypt(Pwd.c_str(),dechar);
		Pwd = dechar;
	}

	string strTmp;
	//构造从短信服务器发送的头
	//替换用户名和密码
	strTmp = CAlertMain::ReplaceStdString(strWebHead, "@UserName@", User);
	strWebHead=strTmp;
	strTmp = CAlertMain::ReplaceStdString(strWebHead, "@Pwd@", Pwd);
	strWebHead=strTmp;
	//替换手机号码
	strTmp = CAlertMain::ReplaceStdString(strWebHead, "@Phone@", phoneNumber);
	strWebHead=strTmp;
	std::list<string> listSms;
	std::list<string>::iterator listSmsItem;

	try
	{
		char buf[1024] = {0};

		string url = urlEncoding( content );
		strTmp = CAlertMain::ReplaceStdString(strWebHead, "@Content@", url );

		//printf( "发送web短信：%s\n", strTmp.c_str() );

		WriteLog(">>>>>>>>发送web短信<<<<<<<<");
		WriteLog(strTmp.c_str());
		GetSourceHtml(strTmp.c_str(), buf);

		//printf( "收到应答：%s\n", buf );

		//判断是否发送正常
		string sRet = buf;
		string::size_type indexBeg,indexEnd;
		static const string::size_type npos = -1;
		indexBeg = sRet.find("smstotal=");
		if (indexBeg != npos)
		{
			indexEnd = sRet.find("&");
			string strNum = sRet.substr(indexBeg+strlen("smstotal="), indexEnd-(indexBeg+strlen("smstotal=")));
			OutputDebugString("sxc");
			OutputDebugString(strNum.c_str());
			OutputDebugString("sxc");
			int num = 0;
			num = atoi(strNum.c_str());
			if (!num)
			{
				bRet = false;
			}
		}
		else
		{
			bRet = false;
		}

		
	}
	catch(...)
	{
		WriteErr( "WebSmsTest发生异常！" );
		bRet = false;
	}

	string testResult;
	if (bRet)
	{
		testResult="成功%发送信息到短信服务器正常\n";
	}
	else
	{
		testResult="失败%连接短信服务器失败\n";
	}

	//往文件中写入返回结果
	list<string> key,context;
	key.push_back("phone");
	key.push_back("result");
	context.push_back(phoneNumber);
	context.push_back(testResult);
	WriteIniFileString("websms","phone",phoneNumber,"smstestresult.ini");
	WriteIniFileString("websms","result",testResult,"smstestresult.ini");
	return bRet;	
}

/*
bool WebSmsTest(string phoneNumber, string content)
{
	bool ret = false;
	string User("test");
	string Pwd("testpwd123");

	User = GetIniFileString("SMSWebConfig", "User", "", "smsconfig.ini");
	Pwd = GetIniFileString("SMSWebConfig", "Pwd", "", "smsconfig.ini");

	Des mydes;
	char dechar[1024]={0};
	if(Pwd.size()>0)
	{
		mydes.Decrypt(Pwd.c_str(),dechar);
		Pwd = dechar;
	}

	string strSMS = content;
	
	char buf[1024] = {0};
	string url = urlEncoding(strSMS);

	string sendUrl = "http://www.smshelper.com:8090/sendsms?user=" + User 
		+ "&pwd=" + Pwd 
		+ "&phone=" + phoneNumber
		+ "&extnum=YL"
		+ "&msg=" + url;
	
	GetSourceHtml(sendUrl.c_str(), buf);
	
	string sRet = buf;
	string::size_type indexBeg,indexEnd;
	static const string::size_type npos = -1;
	indexBeg = sRet.find("smstotal=");
	if (indexBeg != npos)
	{
		indexEnd = sRet.find("&");
		string strNum = sRet.substr(indexBeg+strlen("smstotal="), indexEnd-(indexBeg+strlen("smstotal=")));
		OutputDebugString("sxc");
		OutputDebugString(strNum.c_str());
		OutputDebugString("sxc");
		int num = 0;
		num = atoi(strNum.c_str());
		if (!num)
		{
			ret = false;
		}
	}
	else
	{
		ret = false;
	}

	return ret;	
}
*/

//
CAlertMain::CAlertMain()
{
	receivethread = NULL;
	processthread = NULL;
	//sendthread = NULL;
	
	sendemailthread = NULL;
	sendsmsthread = NULL;
	sendscriptthread = NULL;
	sendsoundthread = NULL;
	waitlistmutex = NULL;
	//waitlistmutexreceive = NULL;
	//2008-12-117 sxf
	waitItsmMutex = NULL;
	event = NULL;

	::CoInitialize(NULL);

}

//
CAlertMain::~CAlertMain()
{
	if(receivethread != NULL)
		delete receivethread;
	if(processthread != NULL)
		delete processthread;
	
	//if(sendthread != NULL)
	//	delete sendthread;

	if(sendemailthread != NULL)
		delete sendemailthread;
	if(sendsmsthread != NULL)
		delete sendsmsthread;
	if(sendscriptthread != NULL)
		delete sendscriptthread;
	if(sendsoundthread != NULL)
		delete sendsoundthread;

	if (senditsmthread!=NULL)
		delete senditsmthread;


	if(waitlistmutex != NULL)
		delete waitlistmutex;

	//if(waitlistmutexreceive != NULL)
	//	delete waitlistmutexreceive;

	if (waitItsmMutex != NULL)
		delete waitItsmMutex;
	 
	if(event != NULL)
		delete event;
	
	CAlertMain::UnloadSendApi();
	CAlertMain::UnloadSoundAlertCom();
	CAlertMain::UnloadScriptAlertCom();
	CAlertMain::UnloadWebSmsAlertCom();

	CoUninitialize();
}

//初始化
bool CAlertMain::Init()
{

	OBJECT objRes=LoadResource("default", "localhost");  
	
	//等svdb启动完了才往下走...
	while(objRes == INVALID_VALUE)
	{
		::Sleep(60000);		
		objRes=LoadResource("default", "localhost");  
		//OutputDebugString(" wait");
		DebugePrint("----------wait for svdb---------------\r\n");
	}
    
	SetSvdbAddrByFile("svapi.ini");
	
	if( objRes !=INVALID_VALUE )
	{
		
		MAPNODE ResNode=GetResourceNode(objRes);
		if( ResNode != INVALID_VALUE )
		{
			//FindNodeValue(ResNode,"IDS_Disable",CAlertMain::strDisable);
			//FindNodeValue(ResNode,"IDS_Normal",CAlertMain::strNormal);
			//FindNodeValue(ResNode,"IDS_Warnning",CAlertMain::strWarning);
			//FindNodeValue(ResNode,"IDS_Error",CAlertMain::strError);
			//FindNodeValue(ResNode,"IDS_Other",CAlertMain::strOther);

			FindNodeValue(ResNode,"IDS_MontioFreq",CAlertMain::strMontorFreq);
			FindNodeValue(ResNode,"IDS_Hour",CAlertMain::strHour);
			FindNodeValue(ResNode,"IDS_Minute",CAlertMain::strMinute);
#ifndef _DEBUG
			FindNodeValue(ResNode,"IDS_Clique_Value",CAlertMain::strMonitorFazhi);
#endif
		}		

		CloseResource(objRes);

		objRes = LoadResource( "chinese", "localhost" );
		if( objRes !=INVALID_VALUE )
		{
			MAPNODE ResNode=GetResourceNode(objRes);
			if( ResNode != INVALID_VALUE )
			{
				FindNodeValue(ResNode,"IDS_Disable",CAlertMain::strDisable);
				FindNodeValue(ResNode,"IDS_Normal",CAlertMain::strNormal);
				FindNodeValue(ResNode,"IDS_Warnning",CAlertMain::strWarning);
				FindNodeValue(ResNode,"IDS_Error",CAlertMain::strError);
				FindNodeValue(ResNode,"IDS_Other",CAlertMain::strOther);
			}

			CloseResource(objRes);
		}

		objRes = LoadResource( "english", "localhost" );
		if( objRes !=INVALID_VALUE )
		{
			MAPNODE ResNode=GetResourceNode(objRes);
			if( ResNode != INVALID_VALUE )
			{
				FindNodeValue(ResNode,"IDS_Disable",CAlertMain::strENDisable);
				FindNodeValue(ResNode,"IDS_Normal",CAlertMain::strENNormal);
				FindNodeValue(ResNode,"IDS_Warnning",CAlertMain::strENWarning);
				FindNodeValue(ResNode,"IDS_Error",CAlertMain::strENError);
				FindNodeValue(ResNode,"IDS_Other",CAlertMain::strENOther);
			}

			CloseResource(objRes);
		}
	}

	//读取短信发送的配置参数
	ifstream input("smspara.ini", ios::in);
	string line; 
    if (input.is_open())
    {
		while(1) 
		{ 
			getline(input,line);
			if(!line.empty())
			{
				string beFindStr="smslength=";
				string::size_type nPos=line.find(beFindStr,0);
				string::size_type nLength=beFindStr.size();
				if(nPos!= string::npos)//如果找到了smslength=，则读出这个值
				{
					line.erase(0,nPos+nLength);
					sscanf(line.c_str(),"%d",&nSMSSendLength);
				}

				//读入短信服务器的URL地址
				beFindStr="smsurl=";
				nPos=line.find(beFindStr,0);
				nLength=beFindStr.size();
				if(nPos!= string::npos)//如果找到了smslength=，则读出这个值
				{
					line.erase(0,nPos+nLength);
					strSMSSendURL=line;
				}

				//读入短信服务器的端口号
				beFindStr="smsport=";
				nPos=line.find(beFindStr,0);
				nLength=beFindStr.size();
				if(nPos!= string::npos)//如果找到了smslength=，则读出这个值
				{
					line.erase(0,nPos+nLength);
					strSMSSendPort=line;
				}
			}
			if (input.eof())
				break;
			
		}
		input.close();
	}
	
	//测试
//	CAlertSmsSendObj alert;
//	alert.SendSmsFromWeb();
	//string strTmp = "警报来自SiteView。\r\n监测器:		@Group@ : @monitor@   \r\n组:		@AllGroup@  \r\n状态:		@Status@ \r\n时间:		@Time@\r\nLogFile内容：  \r\n　@Log@　\r\n";
	//WriteIniFileString("Email", "Default", strTmp, "TxtTemplate.Ini");
	//WriteIniFileString("Email", "SelfDefine", strTmp, "TxtTemplate.Ini");
	//WriteIniFileString("Email", "LogTemplate", strTmp.c_str(), "TXTTemplate.Ini");

	//
	//string strTmp1 = "警报来自SiteView。监测器:@AllGroup@:@Group@:@monitor@ 状态: @Status@  时间: @Time@ \r\n";
	//string strTmp1 = "警报:@monitor@ 状态: @Status@  时间: @Time@ \r\n";
	//WriteIniFileString("SMS", "SelfDefine", strTmp1, "TxtTemplate.Ini");
	//WriteIniFileString("SMS", "Default", strTmp1, "TxtTemplate.Ini");
	//WriteIniFileString("SMS", "JKLSMS", strTmp1, "TxtTemplate.Ini");

	//std::list<string> listSms;
	//std::list<string>::iterator listSmsItem;

	//string strTest = "警报来自SiteView。监测器:		SiteView7.0 : :localhost : Memory:localhost  状态: Memory使用率()=26.09, 剩余空间(MB)=1135.23,  错误页/秒(页/秒)=0.00, 内存总量(MB)=1536.00,   时间: 2006-8-22 11:40:01 ";
	// 
	//CAlertMain::ParserToLength(listSms, strTest, 140);
	//for(listSmsItem = listSms.begin(); listSmsItem!=listSms.end(); listSmsItem++)
	//{
	//	DebugePrint((*listSmsItem));
	//	DebugePrint("ParserToLength\r\n");
	//}

	//return false;
	//bool CreateQueue(string queuename,int type=1,string user="default",string addr="localhost");


	#ifdef AddOnEventSever
		CreateQueue("EventSeverInputQueue");//创建导出事件到EventSever记录队列 
	#endif

	CreateQueue("ExportQueue");//创建导出事件记录队列 JIANG 10-16


	bool bInsert = InsertTable("alertlogs", 801);
	if(bInsert)
	{
		OutputDebugString(" InsertTable ok");	
	}
	else
	{
		OutputDebugString(" InsertTable failed");			
	}

	//string strTmp = GetIniFileString("Email", "Default", "", "TxtTemplate.Ini");
	//printf(strTmp.c_str());

	//////////////////////begin to modify at 07/07/31 /////////////////////////////
	//#ifdef IDC_Version
	//	strWeiHuUserIdcId = GetIniFileString("IdcUserCfg", "WeiHuUserIdcId", "", "general.ini");
	//	strSysWeihuUserIdcId = GetIniFileString("IdcUserCfg", "SysWeihuUserIdcId", "", "general.ini");
	//	strYeWuIdcId = GetIniFileString("IdcUserCfg", "YeWuIdcId", "", "general.ini");
	//	strXiaoshouIdcId = GetIniFileString("IdcUserCfg", "XiaoshouIdcId", "", "general.ini");
	//#endif

	if(GetCgiVersion().compare("IDC") == 0)
	{
		strWeiHuUserIdcId = GetIniFileString("IdcUserCfg", "WeiHuUserIdcId", "", "general.ini");
		strSysWeihuUserIdcId = GetIniFileString("IdcUserCfg", "SysWeihuUserIdcId", "", "general.ini");
		strYeWuIdcId = GetIniFileString("IdcUserCfg", "YeWuIdcId", "", "general.ini");
		strXiaoshouIdcId = GetIniFileString("IdcUserCfg", "XiaoshouIdcId", "", "general.ini");
	}
	//////////////////////modify end at 07/07/31 //////////////////////////////////

	CAlertMain::InitSendApi();
	
	//CAlertMain::InitWebSmsAlertCom();
	InitAlertObjList();
	
	bInitSerialPort = CAlertMain::InitSerialPort();

	//ReadCfgFromWatchIni();

	//数据同步
	waitlistmutex = new Mutex("");
	//waitlistmutexreceive = new Mutex("");
	waitItsmMutex = new Mutex("");

#if 0 //立刻处理方式，消息不入队列，直接处理

	queueMutex = new Mutex("");
	AlertObjListMutex = new Mutex("");
	iniMutex = new Mutex("");
	poolMutex = new Mutex("");

	m_dwWorktime = GetTickCount();
	mainThreadId = ::GetCurrentThreadId(); //取得主线程ID

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	m_nBaseThreadCount = (sysInfo.dwNumberOfProcessors+1) * 3;

	for (int j = 0; j < 60-m_nBaseThreadCount; j++)
	{
		DirectProcessThread *workthread = new DirectProcessThread(this);
		if (workthread)
			threadspool.push_back(workthread);
	}
	for (int i=0; i < m_nBaseThreadCount; i++)
	{
		DirectProcessThread *workthread = new DirectProcessThread(this);

		if (workthread)
		{
			workthreads.push_back(workthread);
			workthread->start();
		}
	}
	//SetTimer(NULL,110,20*1000,CAlertMain::TimerCheckProcess);
	ProcessCheckThread *checkThread = new ProcessCheckThread(this);
	checkThread->start();
#else
	
	//启动线程循环处理数据
	receivethread = new ReceiveThread(this);
	receivethread->start();
	
	processthread = new ProcessThread(this);
	processthread->start();

	//sendthread = new SendThread(this);
	//sendthread->start();	

	sendemailthread = new SendEmailAlertThread(this);
	sendemailthread->start();
	sendsmsthread = new SendSmsAlertThread(this);
	sendsmsthread->start();
	sendscriptthread = new SendScriptAlertThread(this);
	sendscriptthread->start();
	sendsoundthread = new SendSoundAlertThread(this);
	sendsoundthread->start();


	//自动创建工单线程
	//senditsmthread = new SendItsmAlertThread(this);
	//senditsmthread->start();
#endif

	//Datetime tm;
	//int nDay = tm.getDayOfWeek();
	//printf("dddddd");
	//printf("day of week %d", nDay);

	//sendpythonthread = new SendPythonAlertThread(this);
	//sendpythonthread->start();	
	return true;
}

//
void CAlertMain::DoReceive()
{
	WriteLog("--------DoReceive Start--------");
	bool bStop = false;
	CAlertEventObj * lastEventobj = NULL;

	while(!bStop)
	{
		//printf("DoReceive:");
		::Sleep(50);
		//接收到m_AlertEventList

		//if(!waitlistmutex->test())
		//{
		//	continue;
		//}

		MQRECORD mrd;

		mrd=::BlockPopMessage("SiteView70-Alert");

		
		if(mrd!=INVALID_VALUE)
		{
			//puts("Pop message failed");

			string label;
			svutil::TTime ct;
			unsigned int len=0;

			if(!::GetMessageData(mrd, label, ct, NULL, len))
			{
				OutputDebugString("Get message data failed");
				//::CloseMQRecord(mrd);
				//waitlistmutex->leave();
				//continue;
			}

			//printf("Data len is :%d\n",len);
			char * buf = NULL;
			buf = new char[len+2];

			memset(buf, 0 , len+2);
			char strTempInfo[1024] = {0};

			if(!::GetMessageData(mrd, label, ct, buf, len))
			{
				OutputDebugString("Get message data failed");
				//::CloseMQRecord(mrd);
				//waitlistmutex->leave();
				//continue;
			}
			sprintf(strTempInfo , "label=%s\tbuf=%s\tbufsize=%d",label.c_str() , buf , (int)(sizeof(buf)/sizeof(char)));
			WriteLog(strTempInfo);		
			::CloseMQRecord(mrd);

			printf( "消息标签：%s\n", label.c_str() );
			printf("waitSendEmailList size=%d",m_WaitSendEmailList.size());

			sprintf(strTempInfo , "消息标签：%s\n" , label.c_str());
			WriteLog(strTempInfo);
			sprintf(strTempInfo , "waitSendEmailList size=%d" , m_WaitSendEmailList.size());
			WriteLog(strTempInfo);
			
			//OutputDebugString(label.c_str());
			//OutputDebugString("\n");
			if(label == "WebSmsTest")
			{
				//测试web短信发送
				//OutputDebugString("Get message From SmsTest:\n");
				//OutputDebugString(buf);

				//printf( "发送目的：%s\n", buf );

				string szSmsTo = "";
				szSmsTo += buf;

				string::size_type pos(0);

				if ((pos=szSmsTo.find( "DataBase")) == string.npos )
				{
					std::list<string> listSms;
					std::list<string>::iterator listSmsItem;
					
					CAlertMain::ParserToken(listSms, szSmsTo.c_str(), ",");
					bool bSucess = false;
					bool bAllSucess = true;
					for(listSmsItem = listSms.begin(); listSmsItem!=listSms.end(); listSmsItem++)
						WebSmsTest((*listSmsItem), " This is a test!");	
				}else
				{
					szSmsTo.replace(pos,8,"");
					CAlertSmsSendObj obj;
					obj.szSmsTo = szSmsTo;
					obj.strAlertContent = "this is atest!";
					string testResult = "";
					if (obj.SendSmsFromDatabase(testResult))
					{
						WriteIniFileString("websms","phone",szSmsTo,"smstestresult.ini");
						WriteIniFileString("websms","result",testResult,"smstestresult.ini");
					}else
					{
						WriteIniFileString("websms","phone",szSmsTo,"smstestresult.ini");
						WriteIniFileString("websms","result",testResult,"smstestresult.ini");
					}
					
				}

			}
			else if(label == "SmsTest")
			{
				//测试短信发送
				//OutputDebugString("Get message From SmsTest:\n");
				//OutputDebugString(buf);

				string szSmsTo = "";
				szSmsTo += buf;
				std::list<string> listSms;
				std::list<string>::iterator listSmsItem;
				
				CAlertMain::ParserToken(listSms, szSmsTo.c_str(), ",");
				bool bSucess = false;
				bool bAllSucess = true;
				for(listSmsItem = listSms.begin(); listSmsItem!=listSms.end(); listSmsItem++)
				{
					CString strSmsTo = (*listSmsItem).c_str();

					CAlertMain::TestSmsFromComm(strSmsTo, " This is a Test!");	
				}

				//CAlertMain::SendSmsFromComm(buf, " This is a Test2");
				//continue;
			}
			else if(label == "IniChange")
			{
				#ifdef AddOnEventSever				
				::PushMessage("EventSeverInputQueue", label, buf, len); //导出事件到EventSever记录队列 
				#endif

				//OutputDebugString(label.c_str());
				//OutputDebugString("\n");

				//配置文件发生变化
				string strIniName, strSection, strOperate;
				std::list<string> listIniParam;
				std::list<string>::iterator listIniParamItem;
				
				CAlertMain::ParserToken(listIniParam, buf, ",");
				
				strIniName = listIniParam.front();
				listIniParam.pop_front();
				strSection = listIniParam.front();
				listIniParam.pop_front();
				strOperate = listIniParam.front();				
				listIniParam.pop_front();

				OutputDebugString(strIniName.c_str());
				OutputDebugString("\n");
				OutputDebugString(strSection.c_str());
				OutputDebugString("\n");
				OutputDebugString(strOperate.c_str());
				OutputDebugString("\n");

				printf( "配置文件名：%s\n", strIniName.c_str() );
				printf( "配置参数名：%s\n", strSection.c_str() );
				printf( "配置操作名：%s\n", strOperate.c_str() );
				printf("waitSendEmailList size=%d",m_WaitSendEmailList.size());

				strTempSection = strSection;

				
				//进入临界区
				if(!waitlistmutex->test())
				{
					//2008-12-16 sxf
					delete buf;
					continue;
				}

				//////////////////////begin to modify at 07/07/31 /////////////////////////////
				//#ifdef IDC_Version
				if(GetCgiVersion().compare("IDC") == 0)
				{
				//////////////////////modify end at 07/07/31 //////////////////////////////////

					string strIniIdcId = listIniParam.front();
					listIniParam.pop_front();

					if(strIniName == "alert.ini")
					{
						//alert.ini	
						if(strOperate == "ADD")
						{
							//ADD
							ReadAlertObjFromIni(strSection, strIniIdcId);
						}
						else if(strOperate == "DELETE")
						{
							//DELETE
							DeleteAlertObjFromSection(strSection, strIniIdcId);
						}
						else
						{
							//EDIT UPDATE
							DeleteAlertObjFromSection(strSection, strIniIdcId);
							ReadAlertObjFromIni(strSection, strIniIdcId);
						}
					}
					else if(strIniName == "smsconfig.ini")
					{
						//smsconfig.ini
						//是否比较一下？．．．
						CAlertMain::CloseSerialPort();
						bInitSerialPort = CAlertMain::InitSerialPort();
					}
					else
					{
						
					}

				//////////////////////begin to modify at 07/07/31 /////////////////////////////
				//#else
				}
				else
				{
				//////////////////////modify end at 07/07/31 //////////////////////////////////

					if(strIniName == "alert.ini")
					{
						//alert.ini	
						if(strOperate == "ADD")
						{
							//ADD
							ReadAlertObjFromIni(strSection);
						}
						else if(strOperate == "DELETE")
						{
							//DELETE
							DeleteAlertObjFromSection(strSection);
						}
						else
						{
							//EDIT UPDATE
							DeleteAlertObjFromSection(strSection);
							ReadAlertObjFromIni(strSection);
						}
					}
					else if(strIniName == "smsconfig.ini")
					{
						//smsconfig.ini
						//是否比较一下？．．．
						CAlertMain::CloseSerialPort();
						bInitSerialPort = CAlertMain::InitSerialPort();
					}
					else if(strIniName == "watchsheetcfg.ini")
					{
						//值班 配置改变 watchsheetcfg.ini --> 暂时不用 
						if(strOperate == "ADD")
						{
							//ADD
							ReadCfgFromWatchIniSection(strSection);
						}
						else if(strOperate == "DELETE")
						{
							//DELETE
							DeleteCfgFromWatchIniSection(strSection);
						}
						else
						{
							//EDIT UPDATE
							DeleteCfgFromWatchIniSection(strSection);
							ReadCfgFromWatchIniSection(strSection);
						}					
					}
					else
					{

					}

				//////////////////////begin to modify at 07/07/31 /////////////////////////////
				//#endif
				}
				//////////////////////modify end at 07/07/31 //////////////////////////////////
				//退出临界区
				waitlistmutex->leave();
			}
			else
			{
				if(label != "alertlogs")
				{
					//事件
					SVDYN dyn;
					if(!BuildDynByData(buf, len, dyn))
					{
						waitlistmutex->leave();
						delete buf;
						continue;			
					}

					event = new CAlertEventObj();

					event->strMonitorId = label;
					if (dyn.m_displaystr!=NULL)
						event->strEventDes = dyn.m_displaystr;
					event->nEventType = dyn.m_state;
					event->nEventCount = dyn.m_laststatekeeptimes;
					event->strTime = ct.Format();
					//2008-12-2 消息过期时间为接受到的消息时间加2分钟
					event->m_dtExpireTime =  ct + svutil::TTimeSpan(0,0,2,0);
					
					//
					string strEvent;
					char tmpBuf[1024];
					sprintf(tmpBuf, "Id:%s  Type:%d  Count:%d", label.c_str(), dyn.m_state, dyn.m_laststatekeeptimes);
					strEvent = tmpBuf;
					//OutputDebugString(strEvent.c_str());
					//OutputDebugString("\n");
					
					//bool PushMessage(string queuename,string label,const char *data,unsigned int datalen,string user="default",string addr="localhost");
					std::string defaultret = "error";
					std::string alertexport = GetIniFileString("ExportQueue", "Enable",  defaultret, "general.ini");
					if(strcmp(alertexport.c_str(), "error") == 0)
					{					
					}
					else if(strcmp(alertexport.c_str(), "1") == 0)
					{						
						::PushMessage("ExportQueue", label, buf, len); 
					}
					else
					{
						#ifdef AddOnEventSever				
						::PushMessage("EventSeverInputQueue", label, buf, len); //导出事件到EventSever记录队列 
						#endif
					}

					//进入临界区
					if(waitlistmutex->test())
					{
						
						m_AlertEventList.push_back(event);
						printf("alerteventlist size=%d",m_AlertEventList.size());

						//退出临界区
						waitlistmutex->leave();
					}else
					{
						delete event;
					}
				}
			}

			if(buf != NULL)
			{
				delete [] buf;
				buf=NULL;
			}
		}
		else
		{
			DebugePrint("BlockPopMessage Error!!!");
			continue;
		}

		//waitlistmutex->leave();
	}	
}

//
void CAlertMain::DoProcess()
{
	WriteLog("--------DoProcess Start--------");
	bool bStop = false;
	CAlertEventObj * lastEventobj = NULL;
	while(!bStop)
	{
		::Sleep(50);
		//接收到m_AlertEventList

		//进入临界区
		if(!waitlistmutex->test())
		{
			continue;
		}

		//printf("label:%s,ct:%s,data:%s\n",label.c_str(),ct.Format().c_str(),buf.getbuffer());

		//生成AlertSendObj并推送到m_WaitSendEmailList等队列．
		if(!m_AlertEventList.empty())
		{
			lastEventobj = NULL;
			lastEventobj = m_AlertEventList.front();	
			//printf("lastEventobj:%d\n", lastEventobj->nEventCount);
			if(lastEventobj != NULL)
			{
				WriteLog(lastEventobj->GetDebugInfo().c_str());
				//DebugePrint(lastEventobj->GetDebugInfo());
				for(m_AlertObjListItem = m_AlertObjList.begin(); m_AlertObjListItem != m_AlertObjList.end(); m_AlertObjListItem++)
				{
					WriteLog((*m_AlertObjListItem)->GetDebugInfo().c_str());
					//升级匹配
					if((*m_AlertObjListItem)->IsUpgradeMatching(lastEventobj))
					{		
						WriteLog("升级匹配成功！");
						CAlertSendObj * sendobj = (*m_AlertObjListItem)->MakeSendObj(lastEventobj);
						if(sendobj != NULL)
						{
							sendobj->SetUpgradeTrue();

							if(sendobj->nType == 1)
							{
								m_WaitSendEmailList.push_back(sendobj);
								nSendCount ++;
							}
							else if(sendobj->nType == 2)
							{
								m_WaitSendSmsList.push_back(sendobj);
								nSendCount ++;
							}
							else
							{
								delete sendobj;
							}
							//DebugePrint("------------------------------ 升级匹配 etc-------------------\r\n");
							//DebugePrint(sendobj->GetDebugInfo());
							//if(typeid(sendobj) == typeid(CAlertEmailSendObj))
							//{
							//	m_WaitSendEmailList.push_back(sendobj);
							//}
							//else if(typeid(sendobj) == typeid(CAlertSmsSendObj))
							//{
							//	m_WaitSendSmsList.push_back(sendobj);
							//}
							//else
							//{
							//
							//}
							//m_WaitSendList.push_back(sendobj);
						}

					}

					//普通匹配
					if((*m_AlertObjListItem)->IsMatching(lastEventobj))
					{
						WriteLog("条件匹配成功！");
						CAlertSendObj * sendobj = (*m_AlertObjListItem)->MakeSendObj(lastEventobj);
						if(sendobj != NULL)
						{
							WriteLog("成功生成发送对象");
							//OutputDebugString(typeid(sendobj).name());
							//OutputDebugString(typeid(CAlertEmailSendObj *).name());

							if(sendobj->nType == 1)
							{
								m_WaitSendEmailList.push_back(sendobj);
								nSendCount ++;
							}
							else if(sendobj->nType == 2)
							{
								DebugePrint("------------------------------ 普通匹配 SMS etc-------------------\r\n");
								m_WaitSendSmsList.push_back(sendobj);								
								nSendCount ++;
							}
							else if(sendobj->nType == 3)
							{
								m_WaitSendScriptList.push_back(sendobj);
								nSendCount ++;
							}
							else if(sendobj->nType == 4)
							{
								m_WaitSendSoundList.push_back(sendobj);
								nSendCount ++;
							}
							else if(sendobj->nType == 5)
							{
								m_WaitSendPythonList.push_back(sendobj);
								nSendCount ++;
							}
							else if(sendobj->nType == 6)
							{
								m_WaitSendWebList.push_back(sendobj);
								nSendCount ++;
							}
							else
							{
								delete 	sendobj;
							}
							
							
							//DebugePrint(sendobj->GetDebugInfo());

							//m_WaitSendEmailList.push_back(sendobj);
							//CAlertSendObj * lastSendObj = NULL;
							//lastSendObj = m_WaitSendEmailList.front();

							////printf("lastSendObj:%d\n", lastSendObj->nSendId);
							//if(lastSendObj != NULL)
							//{
							//	m_WaitSendEmailList.pop_front();
							//	delete lastSendObj;
							//}
							//else
							//{
							//	m_WaitSendEmailList.pop_front();
							//	DebugePrint("lastSendObj Error\r\n");
							//}


						//	//if(typeid(sendobj) == typeid(CAlertEmailSendObj *))
						//	//{
						//	//	m_WaitSendEmailList.push_back(sendobj);
						//	//}
						//	//else if(typeid(sendobj) == typeid(CAlertSmsSendObj *))
						//	//{
						//	//	m_WaitSendSmsList.push_back(sendobj);
						//	//}
						//	//else if(typeid(sendobj) == typeid(CAlertSoundSendObj *))
						//	//{
						//	//	m_WaitSendScriptList.push_back(sendobj);
						//	//}
						//	//else if(typeid(sendobj) == typeid(CAlertScriptSendObj *))
						//	//{
						//	//	m_WaitSendSoundList.push_back(sendobj);
						//	//}
						//	//else
						//	//{
						//	//
						//	//}
						//	
						//	//m_WaitSendList.push_back(sendobj);
						}
					}					
				}

				//
				//CreateItsmTaskFromEvent(lastEventobj);
				
			}

			//DebugePrint("m_AlertEventList pop_front");
			m_AlertEventList.pop_front();

			if(lastEventobj != NULL)
				delete lastEventobj;

			//退出临界区
			waitlistmutex->leave();
		}
		else
		{
			//退出临界区
			waitlistmutex->leave();		
		}
	}
}

void CAlertMain::DoDirectProcess(Thread *pThread)
{
	WriteLog("--------DoDirectProcess Start--------");
	char strTMP[1024] = {0};
	int NoMsgCount = 0;

	while(1)
	{
		if (!queueMutex->test())
		{

			Sleep(100);
			continue;
		}

		//刷新线程工作时间
		DWORD now = GetTickCount();
		InterlockedExchange((LONG *)&m_dwWorktime,now);
		//

		MQRECORD mrd;

		mrd=::PopMessage("SiteView70-Alert",1);

		if (mrd == INVALID_VALUE)
		{
			queueMutex->leave();
			Sleep(10);
			++NoMsgCount;

			if (NoMsgCount > 160) //线程60次没有没有取到消息
			{
				if (poolMutex->test())
				{
					if (static_cast<int>(workthreads.size()) > m_nBaseThreadCount)
					{
						printf("@@@@@@@@@@@线程太多，退出一个 @@@@@\n"); 
						workthreads.remove(pThread);
						threadspool.push_back(pThread);
						poolMutex->leave();	
						break;
					}
					poolMutex->leave();
				}
			}

			continue;
		}

		NoMsgCount = 0; //计数置0

		string label;
		svutil::TTime ct;
		unsigned int len=0;

		if(!::GetMessageData(mrd, label, ct, NULL, len))
		{
			printf("Get message data failed");
			::CloseMQRecord(mrd);
			queueMutex->leave();
			Sleep(10);
			continue;
		}

		char *buf = new char[len+2];

		memset(buf, 0 , len+2);

		if(!::GetMessageData(mrd, label, ct, buf, len))
		{
			printf("Get message data failed");
			::CloseMQRecord(mrd);
			queueMutex->leave();
			delete buf;
			Sleep(10);
			continue;
		}
		
		::CloseMQRecord(mrd);

		queueMutex->leave();
				
		if(label == "WebSmsTest")
		{
			//测试web短信发送
			string szSmsTo = buf;

			string::size_type pos(0);

			if ((pos=szSmsTo.find( "DataBase")) == string.npos)
			{
				std::list<string> listSms;
				std::list<string>::iterator listSmsItem;
				
				CAlertMain::ParserToken(listSms, szSmsTo.c_str(), ",");
				//bool bSucess = false;
				//bool bAllSucess = true;
				for(listSmsItem = listSms.begin(); listSmsItem!=listSms.end(); listSmsItem++)
					WebSmsTest((*listSmsItem), " This is a test!");	
			}else
			{
				szSmsTo.replace(pos,8,"");
				CAlertSmsSendObj obj;
				obj.szSmsTo = szSmsTo;
				obj.strAlertContent = "this is atest!";
				string testResult = "";
				if (obj.SendSmsFromDatabase(testResult))
				{
					WriteIniFileString("websms","phone",szSmsTo,"smstestresult.ini");
					WriteIniFileString("websms","result",testResult,"smstestresult.ini");
				}else
				{
					WriteIniFileString("websms","phone",szSmsTo,"smstestresult.ini");
					WriteIniFileString("websms","result",testResult,"smstestresult.ini");
				}
				
			}

		}
		else if(label == "SmsTest")
		{
			//测试短信发送
			string szSmsTo = buf;

			std::list<string> listSms;
			std::list<string>::iterator listSmsItem;
			
			CAlertMain::ParserToken(listSms, szSmsTo.c_str(), ",");
			bool bSucess = false;
			bool bAllSucess = true;
			for(listSmsItem = listSms.begin(); listSmsItem!=listSms.end(); listSmsItem++)
			{
				CString strSmsTo = (*listSmsItem).c_str();

				CAlertMain::TestSmsFromComm(strSmsTo, " This is a Test3");	
			}
		}
		else if(label == "IniChange")
		{
			#ifdef AddOnEventSever				
			::PushMessage("EventSeverInputQueue", label, buf, len); //导出事件到EventSever记录队列 
			#endif

			//配置文件发生变化
			string strIniName, strSection, strOperate;
			std::list<string> listIniParam;
			std::list<string>::iterator listIniParamItem;
			
			CAlertMain::ParserToken(listIniParam, buf, ",");
			
			strIniName = listIniParam.front();
			listIniParam.pop_front();
			strSection = listIniParam.front();
			listIniParam.pop_front();
			strOperate = listIniParam.front();				
			listIniParam.pop_front();

			//////////////////////begin to modify at 07/07/31 /////////////////////////////
			//#ifdef IDC_Version
			if(GetCgiVersion().compare("IDC") == 0)
			{
			//////////////////////modify end at 07/07/31 //////////////////////////////////

				string strIniIdcId = listIniParam.front();
				listIniParam.pop_front();

				if(strIniName == "alert.ini")
				{
					AlertObjListMutex->enter();
					//alert.ini	
					if(strOperate == "ADD")
					{
						//ADD
						ReadAlertObjFromIni(strSection, strIniIdcId);
					}
					else if(strOperate == "DELETE")
					{
						//DELETE
						DeleteAlertObjFromSection(strSection, strIniIdcId);
					}
					else
					{
						//EDIT UPDATE
						DeleteAlertObjFromSection(strSection, strIniIdcId);
						ReadAlertObjFromIni(strSection, strIniIdcId);
					}
					AlertObjListMutex->leave();
				}
				else if(strIniName == "smsconfig.ini")
				{
					//smsconfig.ini
					//是否比较一下？．．．
					CAlertMain::CloseSerialPort();
					bInitSerialPort = CAlertMain::InitSerialPort();
				}
				else
				{
					
				}

			//////////////////////begin to modify at 07/07/31 /////////////////////////////
			//#else
			}
			else
			{
			//////////////////////modify end at 07/07/31 //////////////////////////////////

				if(strIniName == "alert.ini")
				{
					AlertObjListMutex->enter();

					//alert.ini	
					if(strOperate == "ADD")
					{
						//ADD
						ReadAlertObjFromIni(strSection);
					}
					else if(strOperate == "DELETE")
					{
						//DELETE
						DeleteAlertObjFromSection(strSection);
					}
					else
					{
						//EDIT UPDATE
						DeleteAlertObjFromSection(strSection);
						ReadAlertObjFromIni(strSection);
					}
					AlertObjListMutex->leave();
				}
				else if(strIniName == "smsconfig.ini")
				{
					//smsconfig.ini
					//是否比较一下？．．．
					CAlertMain::CloseSerialPort();
					bInitSerialPort = CAlertMain::InitSerialPort();
				}
				else if(strIniName == "watchsheetcfg.ini")
				{
					AlertObjListMutex->enter();
					//值班 配置改变 watchsheetcfg.ini --> 暂时不用 
					if(strOperate == "ADD")
					{
						//ADD
						ReadCfgFromWatchIniSection(strSection);
					}
					else if(strOperate == "DELETE")
					{
						//DELETE
						DeleteCfgFromWatchIniSection(strSection);
					}
					else
					{
						//EDIT UPDATE
						DeleteCfgFromWatchIniSection(strSection);
						ReadCfgFromWatchIniSection(strSection);
					}
					AlertObjListMutex->leave();
				}
				else
				{

				}

			//////////////////////begin to modify at 07/07/31 /////////////////////////////
			//#endif
			}
			//////////////////////modify end at 07/07/31 //////////////////////////////////

		}
		else
		{
			if(label != "alertlogs")
			{
				//事件
				SVDYN dyn;
				if(!BuildDynByData(buf, len, dyn))
				{
					delete buf;
					Sleep(10);
					continue;			
				}

				svutil::TTime now = svutil::TTime::GetCurrentTimeEx();

				CAlertEventObj event;

				event.strMonitorId = label;
				if (dyn.m_displaystr!=NULL)
					event.strEventDes = dyn.m_displaystr;
				event.nEventType = dyn.m_state;
				event.nEventCount = dyn.m_laststatekeeptimes;
				sprintf(strTMP , "dyn.m_laststatekeeptimes=%d" , dyn.m_laststatekeeptimes);
				WriteLog(strTMP);
				event.strTime = ct.Format();
				
				std::string defaultret = "error";
				std::string alertexport = GetIniFileString("ExportQueue", "Enable",  defaultret, "general.ini");
				if(strcmp(alertexport.c_str(), "error") == 0)
				{					
				}
				else if(strcmp(alertexport.c_str(), "1") == 0)
				{						
					::PushMessage("ExportQueue", label, buf, len); 
				}
				else
				{
					#ifdef AddOnEventSever				
					::PushMessage("EventSeverInputQueue", label, buf, len); //导出事件到EventSever记录队列 
					#endif
				}


				//检查是否需要创建线程
				if (now - ct > svutil::TTimeSpan(0,0,0,2))
				{
					if (poolMutex->test())
					{
						if (!threadspool.empty())
						{
							printf("@@@@@创建更多的线程..........@@@@\n");

							Thread *p = threadspool.front();
							threadspool.pop_front();
							if (p)
							{	p->start();
								workthreads.push_back(p);
							}
						}

						poolMutex->leave();
					}
				}


				//处理事件
				DebugePrint(event.GetDebugInfo());

				std::list<CAlertSendObj *> listSend;

				//消息有效期限
				if (now - ct < svutil::TTimeSpan(0,0,0,20))
				{

					AlertObjListMutex->enter();
					try
					{

						for(std::list<CAlertBaseObj*>::iterator objIt = m_AlertObjList.begin(); objIt != m_AlertObjList.end(); objIt++)
						{
							if (!(*objIt))
							{
								Sleep(10);
								continue;
							}
							//升级匹配
							if((*objIt)->IsUpgradeMatching(&event))
							{					
								CAlertSendObj * sendobj = (*objIt)->MakeSendObj(&event);
								if(sendobj != NULL)
								{
									sendobj->SetUpgradeTrue();

									if(sendobj->nType == 1 || sendobj->nType == 2)
									{
										listSend.push_back(sendobj);
									}else
									{
										delete sendobj;
									}
									
								}

							}

							//普通匹配
							if((*objIt)->IsMatching(&event))
							{
								CAlertSendObj * sendobj = (*objIt)->MakeSendObj(&event);
								if(sendobj != NULL)
								{
									listSend.push_back(sendobj);
								}
							}
							
							Sleep(10);
						}
					}catch(...)
					{
						printf("事件匹配发生异常。\n");
					}

					AlertObjListMutex->leave();


					while(!listSend.empty())
					{
						CAlertSendObj * sendobj  = listSend.front();
						listSend.pop_front();
						if (sendobj)
						{
							try
							{
								sendobj->SendAlert();
								delete sendobj;

							}catch(...)
							{
								printf("发送报警出现异常:%u\n",GetLastError());
								return;
							}
							
						}
						Sleep(10);
					}
					
					//处理自动创建工单
					AutoCreateIssue(&event);
				}
				else
				{
					printf("@@@@@@[告警过多，系统丢弃一条消息.]@@@@@@@\n");

				}

				
			}
		}

		if(buf != NULL)
		{
			delete buf;
			buf=NULL;
		}
	}

}

void CAlertMain::DoCheckProcess()
{
	while(1)
	{
		

		DWORD			curThreadCount = 0;
	
		HANDLE			hProcessSnap=NULL;     
		PROCESSENTRY32  pe32={0};


		Sleep(20*1000);

	/*	poolMutex->enter();

		hProcessSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		if(hProcessSnap == INVALID_HANDLE_VALUE)
		{
			printf("CreateToolhelp32Snapshot fail.\n");
			CloseHandle   (hProcessSnap);  
			poolMutex->leave();
			continue;
		}

		DWORD dwProcessID = ::GetCurrentProcessId();

		pe32.dwSize   =   sizeof(PROCESSENTRY32);

		if(Process32First(hProcessSnap, &pe32))
		{
			do
			{
				if (pe32.th32ProcessID == dwProcessID)
				{
					printf("current process thread count:%d\n",pe32.cntThreads);
					curThreadCount = pe32.cntThreads;
				}
			}while(Process32Next(hProcessSnap,&pe32));

		}

		CloseHandle   (hProcessSnap);

		if (curThreadCount - 2 < workthreads.size())
		{
			poolMutex->leave();
			goto EXIT;
		}

		poolMutex->leave();*/
		

		DWORD worktime = 0;

		::InterlockedExchange((LONG *)&worktime, m_dwWorktime);
		DWORD now = GetTickCount();

		if (now - worktime > 90*1000)
		{
			printf("工作线程最后活动时间已经超过了1分半钟了,进程将会被结束\n");
			goto EXIT;
		}

		printf("check process ok.\n");
	}

EXIT:

	PostThreadMessage(mainThreadId,WM_QUIT,0,0);
	
}

//
//void CAlertMain::DoSend()
//{
//	CAlertSendObj * lastSendObj = NULL;
//	bool bStop = false;
//	while(!bStop)
//	{	
//		//printf("DoSend:");
//		::Sleep(50);
//		if(!waitlistmutex->test())
//		{
//			continue;
//		}
//		//进入临界区
//
//		//从m_WaitSendList发送出去
//		if(!m_WaitSendList.empty())
//		{
//			lastSendObj = NULL;
//			lastSendObj = m_WaitSendList.front();
//
//			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
//			if(lastSendObj != NULL)
//			{
//				m_WaitSendList.pop_front();
//				waitlistmutex->leave();
//				//printf(lastSendObj->GetDebugInfo().c_str());
//				DebugePrint(lastSendObj->GetDebugInfo().c_str());
//				lastSendObj->SendAlert();
//			}
//			else
//			{
//				m_WaitSendList.pop_front();
//				waitlistmutex->leave();
//			}
//		}
//
//		//逻辑可能有问题...
//		waitlistmutex->leave();
//
//		//退出临界区
//	}
//}

//发送Email报警
void CAlertMain::DoSendEmailAlert()
{	
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendEmailList发送出去
		if(!m_WaitSendEmailList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendEmailList.size() > 1000)
			{
				//m_WaitSendEmailList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendEmailList.begin(); item != m_WaitSendEmailList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendEmailList.erase(m_WaitSendEmailList.begin(), m_WaitSendEmailList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendEmailList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendEmailList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendEmailList.pop_front();
				waitlistmutex->leave();

				//DebugePrint("----------------------------DoSendEmailAlert Normal  Delete-------------------\r\n");
				DebugePrint(lastSendObj->GetDebugInfo());
				
				
				try
				{
					//2008-12-2 sxf 添加判断消息是否过期，原因有可能是邮件发送超时太长，导致消息累积
					svutil::TTime ct = svutil::TTime::GetCurrentTimeEx();

					if (lastSendObj->m_dtExpireTime >= ct)
					{
						lastSendObj->SendAlert();
						//CreateItsmTask(lastSendObj,1);
					}
					else
					{
						printf("消息已经过期:%s\n",lastSendObj->strTime.c_str());

						DebugePrint("消息已经过期,"+lastSendObj->strTime);
					}
				}
				catch(...)
				{
					//DebugePrint("\r\n------------------发送邮件异常开始-----------------------------\r\n");
					DebugePrint(" DoSendEmailAlert SendAlert Error!\r\n");
					//DebugePrint("------------------发送邮件异常结束------------------------------\r\n");
				}

				delete lastSendObj;	
				//nSendCount --;

				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendEmailAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);
			}
			else
			{
				DebugePrint("------------------------------DoSendEmailAlert Error-------------------\r\n");
				m_WaitSendEmailList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}

		//退出临界区
	}
}

//短信
void CAlertMain::DoSendSmsAlert()
{
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendSmsList发送出去
		if(!m_WaitSendSmsList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendSmsList.size() > 1000)
			{
				//m_WaitSendEmailList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendSmsList.begin(); item != m_WaitSendSmsList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendSmsList.erase(m_WaitSendSmsList.begin(), m_WaitSendSmsList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendSmsList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendSmsList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendSmsList.pop_front();
				waitlistmutex->leave();
				//printf(lastSendObj->GetDebugInfo().c_str());
				DebugePrint(lastSendObj->GetDebugInfo());
				

				try
				{
					lastSendObj->SendAlert();
					//CreateItsmTask(lastSendObj,2);
				}
				catch(...)
				{
					//DebugePrint("------------------发送短信异常开始-----------------------------\r\n");
					DebugePrint(" DoSendSmsAlert SendAlert Error!\r\n");					
					//DebugePrint("------------------发送短信异常结束------------------------------\r\n")	;
					
					//如果是
					//CAlertMain::UnloadWebSmsAlertCom();
					//CAlertMain::InitWebSmsAlertCom();
				}

				delete lastSendObj;				
				nSendCount --;
				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendSmsAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);

			}
			else
			{
				m_WaitSendSmsList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}

	}

	//CAlertMain::UnloadWebSmsAlertCom();
}


//
void CAlertMain::DoSendScriptAlert()
{	
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendScriptList发送出去
		if(!m_WaitSendScriptList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendScriptList.size() > 1000)
			{
				//m_WaitSendScriptList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendScriptList.begin(); item != m_WaitSendScriptList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendScriptList.erase(m_WaitSendScriptList.begin(), m_WaitSendScriptList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendScriptList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendScriptList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendScriptList.pop_front();
				waitlistmutex->leave();
				DebugePrint(lastSendObj->GetDebugInfo());
				

				try
				{					
					lastSendObj->SendAlert();
					//CreateItsmTask(lastSendObj,3);
				}
				catch(...)
				{
					//DebugePrint("\r\n------------------发送脚本异常开始-----------------------------\r\n");
					DebugePrint(" DoSendScriptAlert SendAlert Error!\r\n");
					//DebugePrint("------------------发送脚本异常结束------------------------------\r\n");
					//CAlertMain::UnloadScriptAlertCom();
					//CAlertMain::InitScriptAlertCom();
				}

				delete lastSendObj;	
				//waitlistmutex->leave();
				nSendCount --;
				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendScriptAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);
			}
			else
			{
				m_WaitSendScriptList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}
	}
}

//
void CAlertMain::DoSendSoundAlert()
{	
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendSoundList发送出去
		if(!m_WaitSendSoundList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendSoundList.size() > 1000)
			{
				//m_WaitSendSoundList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendSoundList.begin(); item != m_WaitSendSoundList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendSoundList.erase(m_WaitSendSoundList.begin(), m_WaitSendSoundList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendSoundList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendSoundList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendSoundList.pop_front();
				waitlistmutex->leave();
				//printf(lastSendObj->GetDebugInfo().c_str());
				DebugePrint(lastSendObj->GetDebugInfo());				

				try
				{
					
					lastSendObj->SendAlert();
					//CreateItsmTask(lastSendObj,4);

				}
				catch(...)
				{
					//DebugePrint("\r\n------------------发送声音异常开始-----------------------------\r\n");
					DebugePrint(" DoSendSoundAlert SendAlert Error!\r\n");
					//DebugePrint("------------------发送声音异常结束------------------------------\r\n");		
					//CAlertMain::UnloadSoundAlertCom();
					//CAlertMain::InitSoundAlertCom();
				}
				
				delete lastSendObj;				
				//waitlistmutex->leave();
				
				nSendCount --;
				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendSoundAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);

			}
			else
			{
				m_WaitSendSoundList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}
	}
}

//
void CAlertMain::DoSendPythonAlert()
{	
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendSoundList发送出去
		if(!m_WaitSendPythonList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendPythonList.size() > 1000)
			{
				//m_WaitSendSoundList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendPythonList.begin(); item != m_WaitSendPythonList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendPythonList.erase(m_WaitSendPythonList.begin(), m_WaitSendPythonList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendPythonList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendPythonList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendPythonList.pop_front();
				waitlistmutex->leave();
				//printf(lastSendObj->GetDebugInfo().c_str());
				DebugePrint(lastSendObj->GetDebugInfo());				

				try
				{

					lastSendObj->SendAlert();

					//CreateItsmTask(lastSendObj,5);
				}
				catch(...)
				{
					//DebugePrint("\r\n------------------发送声音异常开始-----------------------------\r\n");
					DebugePrint(" DoSendSoundAlert SendAlert Error!\r\n");
					//DebugePrint("------------------发送声音异常结束------------------------------\r\n");		
					//CAlertMain::UnloadSoundAlertCom();
					//CAlertMain::InitSoundAlertCom();
				}
				
				delete lastSendObj;				
				//waitlistmutex->leave();
				
				nSendCount --;
				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendPythonAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);

			}
			else
			{
				m_WaitSendPythonList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}
	}
}

void CAlertMain::DoSendWebAlert()
{	
	bool bStop = false;
	while(!bStop)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitlistmutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendWebList发送出去
		if(!m_WaitSendWebList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendWebList.size() > 1000)
			{
				//m_WaitSendWebList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendWebList.begin(); item != m_WaitSendWebList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendWebList.erase(m_WaitSendWebList.begin(), m_WaitSendWebList.end());

				waitlistmutex->leave();
				DebugePrint("Error, m_WaitSendWebList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendWebList.front();

			//printf("lastSendObj:%d\n", lastSendObj->nSendId);
			if(lastSendObj != NULL)
			{
				m_WaitSendWebList.pop_front();
				waitlistmutex->leave();
				//printf(lastSendObj->GetDebugInfo().c_str());
				DebugePrint(lastSendObj->GetDebugInfo());				

				try
				{

					lastSendObj->SendAlert();
					//CreateItsmTask(lastSendObj,6);
				}
				catch(...)
				{
					//DebugePrint("\r\n------------------发送声音异常开始-----------------------------\r\n");
					DebugePrint(" DoSendSoundAlert SendAlert Error!\r\n");
					//DebugePrint("------------------发送声音异常结束------------------------------\r\n");		
					//CAlertMain::UnloadSoundAlertCom();
					//CAlertMain::InitSoundAlertCom();
				}
				
				delete lastSendObj;				
				//waitlistmutex->leave();
				
				nSendCount --;
				char chItem[32]  = {0};	
				sprintf(chItem, "%d", nSendCount);
				string strOutput = "DoSendWebAlert : The SendCount is : ";
				strOutput += chItem;
				strOutput += "\r\n";
				DebugePrint(strOutput);

			}
			else
			{
				m_WaitSendWebList.pop_front();
				waitlistmutex->leave();
			}
		}
		else
		{
			//逻辑可能有问题...
			waitlistmutex->leave();
		}
	}

}

//创建建工单的任务
void CAlertMain::CreateItsmTask(CAlertSendObj *pSendObj,int iType)
{

	CAlertItsmSendObj *pItsmSendObj = new CAlertItsmSendObj();

	string strTitle;
	string strContent;

	ASSERT(pItsmSendObj != NULL);

	switch(iType)
	{
	case 1:
		strTitle = ((CAlertEmailSendObj *)pSendObj)->strAlertTitle;
		strContent =((CAlertEmailSendObj *)pSendObj)->strAlertContent;
		break;
	case 2:
		strTitle = ((CAlertSmsSendObj *)pSendObj)->strAlertTitle;
		strContent = ((CAlertSmsSendObj *)pSendObj)->strAlertContent;
		break;
	case 3:

		strTitle = "";//((CAlertScriptSendObj *)pSendObj)->strAlertTitle;
		strContent ="";// ((CAlertScriptSendObj *)pSendObj)->strAlertContent;
		break;
	case 4:

		strTitle =  "";//((CAlertSoundSendObj *)pSendObj)->strAlertTitle;
		strContent = "";// ((CAlertSoundSendObj *)pSendObj)->strAlertContent;
		break;
	case 5:

		strTitle ="";// ((CAlertPythonSendObj *)pSendObj)->strAlertTitle;
		strContent = "";//((CAlertPythonSendObj *)pSendObj)->strAlertContent;
		break;
	/*case 6:
		CAlertPythonSendObj *pObj = (CAlertPythonSendObj *)pSendObj;
		strTitle = pObj->strAlertTitle;
		strContent = pObj->strAlertContent;
		break;*/
	default:
		strTitle ="";
		strContent = "";
	}

	if (pItsmSendObj != NULL)
	{
		pItsmSendObj->strAlertMonitorId = pSendObj->strAlertMonitorId;
		pItsmSendObj->strAlertIndex = pSendObj->strAlertIndex;
		pItsmSendObj->strAlertName = pSendObj->strAlertName;
		pItsmSendObj->strEventDes = pSendObj->strEventDes;
		pItsmSendObj->bUpgrade = pSendObj->bUpgrade;
		pItsmSendObj->m_dtExpireTime = pSendObj->m_dtExpireTime;
		pItsmSendObj->nEventCount = pSendObj->nEventCount;
		pItsmSendObj->nEventType = pSendObj->nEventType;
		pItsmSendObj->nSendId = pSendObj->nSendId;
		pItsmSendObj->nType = pSendObj->nType;
		pItsmSendObj->strAlertContent = strContent;
		pItsmSendObj->strAlertTitle = strTitle;
		pItsmSendObj->strTime = pSendObj->strTime;
		waitItsmMutex->enter();
		m_WaitSendItsmList.push_back(pItsmSendObj);
		waitItsmMutex->leave();
		
	}
}

CAlertSendObj * CAlertMain::CreateItsmTaskFromEvent(CAlertEventObj *pEvent)
{

	if (!pEvent)
		return NULL;

	//不是错误不启动自动创建工单
	if (pEvent->GetEventTypeString().compare(CAlertMain::strError) != 0)
		return NULL;

	string ip = GetIniFileString("itsm_server","itsmServer", "" , "itsmConfig.ini");
	if (ip.size()<= 0) //没有设置itsm服务的地址
		return NULL;

	CAlertItsmSendObj *pItsmSendObj = new CAlertItsmSendObj();

	ASSERT(pItsmSendObj != NULL);
	if (pItsmSendObj != NULL)
	{
		pItsmSendObj->strAlertMonitorId = pEvent->strMonitorId;
		pItsmSendObj->strEventDes = pEvent->strEventDes;

		pItsmSendObj->m_dtExpireTime = pEvent->m_dtExpireTime;
		pItsmSendObj->nEventCount = pEvent->nEventCount;
		pItsmSendObj->nEventType = pEvent->nEventType;

		pItsmSendObj->strTime = pEvent->strTime;
		waitItsmMutex->enter();
		m_WaitSendItsmList.push_back(pItsmSendObj);
		waitItsmMutex->leave();
	}

	return pItsmSendObj;
}

int CAlertMain::AutoCreateIssue(CAlertEventObj *pEvent)
{
	if (!pEvent)
		return -1;

	//不是错误不启动自动创建工单
	if (pEvent->GetEventTypeString().compare(CAlertMain::strError) != 0)
		return -2;

	string ip = GetIniFileString("itsm_server","itsmServer", "" , "itsmConfig.ini");
	if (ip.size()<= 0) //没有设置itsm服务的地址
		return -3;

	CAlertItsmSendObj *pItsmSendObj = new CAlertItsmSendObj();

	ASSERT(pItsmSendObj != NULL);
	if (pItsmSendObj != NULL)
	{
		pItsmSendObj->strAlertMonitorId = pEvent->strMonitorId;
		pItsmSendObj->strEventDes = pEvent->strEventDes;

		pItsmSendObj->m_dtExpireTime = pEvent->m_dtExpireTime;
		pItsmSendObj->nEventCount = pEvent->nEventCount;
		pItsmSendObj->nEventType = pEvent->nEventType;
		pItsmSendObj->strTime = pEvent->strTime;

		try
		{
			printf("***************发送Itsm自动创建工单请求************\n");
			pItsmSendObj->SendAlert();

		}catch(...)
		{
			printf("***自动创建工单发生异常\n");
			delete pItsmSendObj;
			return -9;
		}
		delete pItsmSendObj;
	}


	return 0;
}



//处理Itsm系统接口
void CAlertMain::DoSendItsmAlert()
{
	while(1)
	{	
		//printf("DoSend:");
		::Sleep(10);
		if(!waitItsmMutex->test())
		{
			continue;
		}
		//进入临界区

		//从m_WaitSendWebList发送出去
		if(!m_WaitSendItsmList.empty())
		{
			//发送等待队列大小超过1000则清空
			if(m_WaitSendItsmList.size() > 100)
			{
				//m_WaitSendWebList.clear();
				
				list <CAlertSendObj *>::iterator  item;

				for(item = m_WaitSendItsmList.begin(); item != m_WaitSendItsmList.end(); item ++)
				{
					delete (*item);
				}

				m_WaitSendItsmList.erase(m_WaitSendItsmList.begin(), m_WaitSendItsmList.end());

				waitItsmMutex->leave();
				DebugePrint("Error, m_WaitSendWebList.size() > 1000, Clear Ok!");
				continue;
			}

			CAlertSendObj * lastSendObj = NULL;
			lastSendObj = m_WaitSendItsmList.front();

			if(lastSendObj != NULL)
			{
				m_WaitSendItsmList.pop_front();
				waitItsmMutex->leave();

				printf("***************发送Itsm自动创建工单请求************\n");	

				try
				{
					lastSendObj->SendAlert();
				}
				catch(...)
				{
					DebugePrint(" DoSendItsmAlert SendAlert Error!\r\n");
				}
				
				delete lastSendObj;				
				

			}
			else
			{
				m_WaitSendItsmList.pop_front();
				waitItsmMutex->leave();
			}
		}
		else
		{
			waitItsmMutex->leave();
		}
	}
}


//从alert.ini初始化报警对象列表
void CAlertMain::InitAlertObjList()
{	
	//
	std::list<string> keylist;
	std::list<string>::iterator keyitem;

	//////////////////////begin to modify at 07/07/31 /////////////////////////////
	//#ifdef IDC_Version
	if(GetCgiVersion().compare("IDC") == 0)
	{
	//////////////////////modify end at 07/07/31 //////////////////////////////////

		//如果是IDC版本则需要获取所有IDC用户子目录下的alert.ini的值合成一个列表供下面的代码使用。

		//需要存储IDC用户的子用户ID到一个列表供后边读alert.ini用。
		std::list<string> idcIdlist;
		std::list<string>::iterator m_idcItem;

		PAIRLIST retlist;
		std::list<struct sv_pair>::iterator svitem;		
		string strStrIdcId = "";
		GetAllGroupsInfo(retlist, "sv_name");
		OutputDebugString("---------------alert idcinfo  load----------------------\n");
		for(svitem = retlist.begin(); svitem != retlist.end(); svitem++)
		{
			strStrIdcId = (*svitem).name;

			//不是机房管理员等
			if(strStrIdcId.compare(strWeiHuUserIdcId) != 0 && strStrIdcId.compare(strSysWeihuUserIdcId) != 0 \
				&& strStrIdcId.compare(strYeWuIdcId) != 0 && strStrIdcId.compare(strXiaoshouIdcId) != 0)
			{			
				//只有一个分隔点
				if(IsIdcGroup(strStrIdcId.c_str()))
				{
					OutputDebugString(strStrIdcId.c_str());
					OutputDebugString("\n");
					idcIdlist.push_back(strStrIdcId);
				}
			}
		}

		//根据IdcId读配置
		for(m_idcItem = idcIdlist.begin(); m_idcItem != idcIdlist.end(); m_idcItem++)
		{
			//从ini获取报警列表
			if(GetIniFileSections(keylist, "alert.ini", "localhost", (*m_idcItem)))
			{
				//从ini初始化报警列表
				for(keyitem = keylist.begin(); keyitem != keylist.end(); keyitem ++)	
				{
					ReadAlertObjFromIni((*keyitem), (*m_idcItem));
				}
			}
		}

	//////////////////////begin to modify at 07/07/31 /////////////////////////////
	//#else
	}
	else
	{
	//////////////////////modify end at 07/07/31 //////////////////////////////////

		//从ini获取报警列表
		if(GetIniFileSections(keylist, "alert.ini"))
		{
			//从ini初始化报警列表
			for(keyitem = keylist.begin(); keyitem != keylist.end(); keyitem ++)
			{
				ReadAlertObjFromIni((*keyitem));
			}
		}	

	//////////////////////begin to modify at 07/07/31 /////////////////////////////
	//#endif
	}
	//////////////////////modify end at 07/07/31 //////////////////////////////////


}

//
void CAlertMain::ReadAlertObjFromIni(string strSection)
{
	//基础参数
	string strIndex, strAlertName, strAlertType, strAlertCategory, strAlertState, strTmp;
	string strAlertTargerList;
	string strTempCountList;

	//报警参数	
	string strEmailAdressValue, strOtherAdressValue, strEmailTemplateValue;
	string strSmsNumberValue, strOtherNumberValue, strSmsSendMode, strSmsTemplateValue;	
	string strServerTextValue, strScriptFileValue, strScriptParamValue, strScriptServerId;
	string strServerValue, strLoginNameValue, strLoginPwdValue;
	string strAlertUpgradeValue, strAlertUpgradeToValue, strAlertStopValue;
	string strReceive, strLevel, strContent;
	int nCond = 0, nAlertType = 0;
	size_t index = 0;
	string strAlwaysTimesValue, strOnlyTimesValue,strSelTimes1Value,strSelTimes2Value;
	
	CAlertBaseObj * alertbaseobj = NULL;
	CAlertEmailObj * alertemailobj = NULL;
	CAlertSmsObj * alertsmsobj = NULL;
	CAlertScriptObj * alertscriptobj = NULL;
	CAlertSoundObj * alertsoundobj = NULL;
	CAlertPythonObj * alertpythonobj = NULL;

	//从ini读数据
	strIndex = GetIniFileString(strSection, "nIndex", "", "alert.ini");
	strAlertName = GetIniFileString(strSection, "AlertName", "", "alert.ini");
	strAlertType = GetIniFileString(strSection, "AlertType", "" , "alert.ini");
	strAlertCategory = GetIniFileString(strSection, "AlertCategory", "", "alert.ini");
	strAlertState = GetIniFileString(strSection, "AlertState", "", "alert.ini");
	
	//获取报警依赖目标
	strAlertTargerList = GetIniFileString(strIndex, "AlertTarget", "", "alert.ini");
	nAlertType = GetIntFromAlertType(strAlertType);
	
	//报警条件
	nCond = GetIniFileInt(strIndex, "AlertCond", 0, "alert.ini");
	if(nCond == 0)		//如果按整形没取出AlertCond的值，按字符串型再取一次
	{
		WriteLog("按整型未取出AlertCond!");
		string strNCond = GetIniFileString(strIndex , "AlertCond", "3" , "alert.ini");
		nCond = atoi(strNCond.c_str());
	}

//临时添加，用以查询从配置文件里面读取的值
	char strTempCond[128] = {0};
	sprintf(strTempCond , "从INI里面获取的nIndex为%s的nCond=%d", strIndex.c_str(),nCond);
	WriteLog(strTempCond);	
//临时添加结束
	strAlwaysTimesValue = GetIniFileString(strIndex, "AlwaysTimes", "", "alert.ini");
	strOnlyTimesValue = GetIniFileString(strIndex, "OnlyTimes", "", "alert.ini");
	strSelTimes1Value = GetIniFileString(strIndex, "SelTimes1", "", "alert.ini");
	strSelTimes2Value = GetIniFileString(strIndex, "SelTimes2", "", "alert.ini");			


	switch(nAlertType)
	{
		case 1:
			//email报警
			strEmailAdressValue = GetIniFileString(strIndex, "EmailAdress", "", "alert.ini");
			strOtherAdressValue = GetIniFileString(strIndex, "OtherAdress", "", "alert.ini");
			strEmailTemplateValue = GetIniFileString(strIndex, "EmailTemplate", "", "alert.ini");
			
			strAlertUpgradeValue = GetIniFileString(strIndex, "Upgrade", "", "alert.ini");
			if(strAlertUpgradeValue == "")
				strAlertUpgradeValue = "0";
			
			strAlertUpgradeToValue = GetIniFileString(strIndex, "UpgradeTo", "", "alert.ini");
			
			strAlertStopValue = GetIniFileString(strIndex, "Stop", "", "alert.ini");
			if(strAlertStopValue == "")
				strAlertStopValue = "0";

			//index = strEmailAdressValue.find("禁止");
			/*
			index =strEmailAdressValue.find(CAlertMain::strDisable.c_str());
			if(index != -1)
			*/
			if( ((index=strEmailAdressValue.find(CAlertMain::strDisable.c_str())) != -1) ||
				((index=strEmailAdressValue.find(CAlertMain::strENDisable.c_str())) != -1) )
			{
				//OutputDebugString(strEmailAdressValue.c_str());
				strTmp = strEmailAdressValue.substr(0, index - 1);
				//OutputDebugString(strTmp.c_str());
				strEmailAdressValue = strTmp;
			}

			DebugePrint(strEmailAdressValue.c_str());

			alertemailobj = new CAlertEmailObj();					

			alertemailobj->strEmailAdressValue = strEmailAdressValue;
			alertemailobj->strOtherAdressValue = strOtherAdressValue;
			alertemailobj->strEmailTemplateValue = strEmailTemplateValue;
			
			alertemailobj->strAlertUpgradeValue = strAlertUpgradeValue;
			alertemailobj->strAlertUpgradeToValue = strAlertUpgradeToValue;
			alertemailobj->strAlertStopValue = strAlertStopValue;
			
			alertemailobj->RefreshData();
			alertbaseobj = (CAlertBaseObj *)alertemailobj;
			break;
		case 2:
			//短信报警
			strSmsNumberValue = GetIniFileString(strIndex, "SmsNumber", "", "alert.ini");
			strOtherNumberValue = GetIniFileString(strIndex, "OtherNumber", "", "alert.ini");
			strSmsSendMode = GetIniFileString(strIndex, "SmsSendMode", "", "alert.ini");
			strSmsTemplateValue = GetIniFileString(strIndex, "SmsTemplate", "", "alert.ini");

			strAlertUpgradeValue = GetIniFileString(strIndex, "Upgrade", "", "alert.ini");
			if(strAlertUpgradeValue == "")
				strAlertUpgradeValue = "0";

			strAlertUpgradeToValue = GetIniFileString(strIndex, "UpgradeTo", "", "alert.ini");

			strAlertStopValue = GetIniFileString(strIndex, "Stop", "", "alert.ini");
			if(strAlertStopValue == "")
				strAlertStopValue = "0";

			//index = strSmsNumberValue.find("禁止");
			/*
			index = strSmsNumberValue.find(CAlertMain::strDisable.c_str());
			if(index != -1)
			*/
			if( ((index=strEmailAdressValue.find(CAlertMain::strDisable.c_str())) != -1) ||
				((index=strEmailAdressValue.find(CAlertMain::strENDisable.c_str())) != -1) )
			{
				//OutputDebugString(strEmailAdressValue.c_str());
				strTmp = strSmsNumberValue.substr(0, index - 1);
				//OutputDebugString(strTmp.c_str());
				strSmsNumberValue = strTmp;
			}
			
			DebugePrint(strSmsNumberValue.c_str());
			//alertsmsobj = (CAlertSmsObj *)alertbaseobj;
			alertsmsobj = new CAlertSmsObj();
			alertsmsobj->strSmsNumberValue = strSmsNumberValue;
			alertsmsobj->strOtherNumberValue = strOtherNumberValue;
			alertsmsobj->strSmsSendMode = strSmsSendMode;
			alertsmsobj->strSmsTemplateValue = strSmsTemplateValue;

			alertsmsobj->strAlertUpgradeValue = strAlertUpgradeValue;
			alertsmsobj->strAlertUpgradeToValue = strAlertUpgradeToValue;
			alertsmsobj->strAlertStopValue = strAlertStopValue;

			alertsmsobj->RefreshData();
			alertbaseobj = (CAlertBaseObj *)alertsmsobj;
			break;
		case 3:
			//脚本报警
			strServerTextValue = GetIniFileString(strIndex, "ScriptServer", "", "alert.ini");
			strScriptServerId = GetIniFileString(strIndex, "ScriptServerId", "", "alert.ini");
			strScriptFileValue = GetIniFileString(strIndex, "ScriptFile", "", "alert.ini");
			strScriptParamValue = GetIniFileString(strIndex, "ScriptParam", "", "alert.ini");

			//alertscriptobj = (CAlertScriptObj *)alertbaseobj;
			alertscriptobj = new CAlertScriptObj();					
			alertscriptobj->strServerTextValue = strServerTextValue;
			alertscriptobj->strScriptServerId = strScriptServerId;
			alertscriptobj->strScriptFileValue = strScriptFileValue;
			alertscriptobj->strScriptParamValue = strScriptParamValue;
			
			alertbaseobj = (CAlertBaseObj *)alertscriptobj;
			break;
		case 4:
			//声音报警
			strServerValue = GetIniFileString(strIndex, "Server", "", "alert.ini");
			strLoginNameValue = GetIniFileString(strIndex, "LoginName", "", "alert.ini");
			strLoginPwdValue = GetIniFileString(strIndex, "LoginPwd", "", "alert.ini");
			
			alertsoundobj = new CAlertSoundObj();					
			alertsoundobj->strServerValue = strServerValue;
			alertsoundobj->strLoginNameValue = strLoginNameValue;
			alertsoundobj->strLoginPwdValue = strLoginPwdValue;
			
			alertbaseobj = (CAlertBaseObj *)alertsoundobj;
			break;
		case 5:
			//工单报警
			strReceive = GetIniFileString(strIndex, "PythonReceive", "", "alert.ini");
			strLevel = GetIniFileString(strIndex, "PythonLevel", "", "alert.ini");
			strContent = GetIniFileString(strIndex, "Content", "", "alert.ini");
			
			alertpythonobj = new CAlertPythonObj();
			alertpythonobj->strReceive = strReceive;
			alertpythonobj->strLevel = strLevel;
			alertpythonobj->strContent = strContent;
			
			alertbaseobj = (CAlertBaseObj *)alertpythonobj;
			break;
		default:
			break;
	}

	if(alertbaseobj != NULL)
	{
		alertbaseobj->strIndex = strIndex;
		alertbaseobj->strAlertName = strAlertName;
		alertbaseobj->strAlertType = strAlertType;
		alertbaseobj->strAlertCategory = strAlertCategory;
		alertbaseobj->strAlertState = strAlertState;
		alertbaseobj->strAlertTargerList = strAlertTargerList;
		alertbaseobj->nAlertType = nAlertType;
		alertbaseobj->nCond = nCond;
		alertbaseobj->strAlwaysTimesValue = strAlwaysTimesValue;
		alertbaseobj->strOnlyTimesValue = strOnlyTimesValue;
		alertbaseobj->strSelTimes1Value = strSelTimes1Value;
		alertbaseobj->strSelTimes2Value = strSelTimes2Value;			
		
		//printf(alertbaseobj->GetDebugInfo().c_str());
		//DebugePrint(alertbaseobj->GetDebugInfo().c_str());
		
		//进入临界区
		
		//if(!waitlistmutex->test())
		{	
			m_AlertObjList.push_back(alertbaseobj);	
			alertbaseobj->AnalysisAlertTarget();
			//waitlistmutex->leave();
		}
		//else
		{
			//DebugePrint("--------------------------读取ini记录因数据冲突而放弃!!!!!!!!----------------------------------\r\n");
		}		

		DebugePrint(alertbaseobj->GetDebugInfo());
	}

	//alertbaseobj = NULL;
}

//
void CAlertMain::DeleteAlertObjFromSection(string strSection)
{
	bool bExist = false;
	//分解出正确的pAlertTargetList
	for(m_AlertObjListItem = m_AlertObjList.begin(); m_AlertObjListItem != m_AlertObjList.end(); m_AlertObjListItem++)
	{
		if((*m_AlertObjListItem)->strIndex == strSection)
		{
			bExist = true;
			break;
		}		
	}

	if(bExist)
	{
		//进入临界区
		//if(!waitlistmutex->test())
		{	
			m_AlertObjList.erase(m_AlertObjListItem);			
			//waitlistmutex->leave();
		}
		//else
		{
			//DebugePrint("--------------------------删除ini记录因数据冲突而放弃!!!!!!!!----------------------------------\r\n");
		}
		//delete (*m_AlertObjListItem);

	}
}

//
void CAlertMain::ReadAlertObjFromIni(string strSection, string strIdcId)
{
	//基础参数
	string strIndex, strAlertName, strAlertType, strAlertCategory, strAlertState, strTmp;
	string strAlertTargerList;
	string strTempCountList;

	//报警参数	
	string strEmailAdressValue, strOtherAdressValue, strEmailTemplateValue;
	string strSmsNumberValue, strOtherNumberValue, strSmsSendMode, strSmsTemplateValue;	
	string strServerTextValue, strScriptFileValue, strScriptParamValue, strScriptServerId;
	string strServerValue, strLoginNameValue, strLoginPwdValue;
	string strAlertUpgradeValue, strAlertUpgradeToValue, strAlertStopValue;
	int nCond = 0, nAlertType = 0;
	size_t index = 0;
	string strAlwaysTimesValue, strOnlyTimesValue,strSelTimes1Value,strSelTimes2Value;
	
	CAlertBaseObj * alertbaseobj = NULL;
	CAlertEmailObj * alertemailobj = NULL;
	CAlertSmsObj * alertsmsobj = NULL;
	CAlertScriptObj * alertscriptobj = NULL;
	CAlertSoundObj * alertsoundobj = NULL;

	//从ini读数据
	strIndex = GetIniFileString(strSection, "nIndex", "", "alert.ini", "localhost", strIdcId);
	strAlertName = GetIniFileString(strSection, "AlertName", "", "alert.ini", "localhost", strIdcId);
	strAlertType = GetIniFileString(strSection, "AlertType", "" , "alert.ini", "localhost", strIdcId);
	strAlertCategory = GetIniFileString(strSection, "AlertCategory", "", "alert.ini", "localhost", strIdcId);
	strAlertState = GetIniFileString(strSection, "AlertState", "", "alert.ini", "localhost", strIdcId);

	
	//获取报警依赖目标
	strAlertTargerList = GetIniFileString(strIndex, "AlertTarget", "", "alert.ini", "localhost", strIdcId);
	nAlertType = GetIntFromAlertType(strAlertType);
	
	//报警条件
	nCond = GetIniFileInt(strIndex, "AlertCond", 0, "alert.ini", "localhost", strIdcId);
	if(nCond == 0)		//如果按整形没取出AlertCond的值，按字符串型再取一次
	{
		string strNCond = GetIniFileString(strIndex , "AlertCond", "3" , "alert.ini");
		nCond = atoi(strNCond.c_str());
	}

	strAlwaysTimesValue = GetIniFileString(strIndex, "AlwaysTimes", "", "alert.ini", "localhost", strIdcId);
	strOnlyTimesValue = GetIniFileString(strIndex, "OnlyTimes", "", "alert.ini", "localhost", strIdcId);
	strSelTimes1Value = GetIniFileString(strIndex, "SelTimes1", "", "alert.ini", "localhost", strIdcId);
	strSelTimes2Value = GetIniFileString(strIndex, "SelTimes2", "", "alert.ini", "localhost", strIdcId);			
	strTempCountList = GetIniFileString(strSection , "TempCount" , "" , "alert.ini" , "localhost" , strIdcId);

	switch(nAlertType)
	{
		case 1:
			//email报警
			strEmailAdressValue = GetIniFileString(strIndex, "EmailAdress", "", "alert.ini", "localhost", strIdcId);
			strOtherAdressValue = GetIniFileString(strIndex, "OtherAdress", "", "alert.ini", "localhost", strIdcId);
			strEmailTemplateValue = GetIniFileString(strIndex, "EmailTemplate", "", "alert.ini", "localhost", strIdcId);
			
			strAlertUpgradeValue = GetIniFileString(strIndex, "Upgrade", "", "alert.ini", "localhost", strIdcId);
			if(strAlertUpgradeValue == "")
				strAlertUpgradeValue = "0";
			
			strAlertUpgradeToValue = GetIniFileString(strIndex, "UpgradeTo", "", "alert.ini", "localhost", strIdcId);
			
			strAlertStopValue = GetIniFileString(strIndex, "Stop", "", "alert.ini", "localhost", strIdcId);
			if(strAlertStopValue == "")
				strAlertStopValue = "0";

			//index = strEmailAdressValue.find("禁止");
			/*
			index = strEmailAdressValue.find(CAlertMain::strDisable.c_str());
			if(index != -1)	
			*/
			if( ((index=strEmailAdressValue.find(CAlertMain::strDisable.c_str())) != -1) ||
				((index=strEmailAdressValue.find(CAlertMain::strENDisable.c_str())) != -1) )
			{
				//OutputDebugString(strEmailAdressValue.c_str());
				strTmp = strEmailAdressValue.substr(0, index - 1);
				//OutputDebugString(strTmp.c_str());
				strEmailAdressValue = strTmp;
			}

			DebugePrint(strEmailAdressValue.c_str());

			alertemailobj = new CAlertEmailObj();					

			alertemailobj->strEmailAdressValue = strEmailAdressValue;
			alertemailobj->strOtherAdressValue = strOtherAdressValue;
			alertemailobj->strEmailTemplateValue = strEmailTemplateValue;
			
			alertemailobj->strAlertUpgradeValue = strAlertUpgradeValue;
			alertemailobj->strAlertUpgradeToValue = strAlertUpgradeToValue;
			alertemailobj->strAlertStopValue = strAlertStopValue;
			
			alertemailobj->RefreshData();
			alertbaseobj = (CAlertBaseObj *)alertemailobj;
			break;
		case 2:
			//短信报警
			strSmsNumberValue = GetIniFileString(strIndex, "SmsNumber", "", "alert.ini", "localhost", strIdcId);
			strOtherNumberValue = GetIniFileString(strIndex, "OtherNumber", "", "alert.ini", "localhost", strIdcId);
			strSmsSendMode = GetIniFileString(strIndex, "SmsSendMode", "", "alert.ini", "localhost", strIdcId);
			strSmsTemplateValue = GetIniFileString(strIndex, "SmsTemplate", "", "alert.ini", "localhost", strIdcId);

			strAlertUpgradeValue = GetIniFileString(strIndex, "Upgrade", "", "alert.ini", "localhost", strIdcId);
			if(strAlertUpgradeValue == "")
				strAlertUpgradeValue = "0";

			strAlertUpgradeToValue = GetIniFileString(strIndex, "UpgradeTo", "", "alert.ini", "localhost", strIdcId);

			strAlertStopValue = GetIniFileString(strIndex, "Stop", "", "alert.ini", "localhost", strIdcId);
			if(strAlertStopValue == "")
				strAlertStopValue = "0";

			//index = strSmsNumberValue.find("禁止");
			/*
			index = strSmsNumberValue.find(CAlertMain::strDisable.c_str());
			if(index != -1)	
			*/
			if( ((index=strEmailAdressValue.find(CAlertMain::strDisable.c_str())) != -1) ||
				((index=strEmailAdressValue.find(CAlertMain::strENDisable.c_str())) != -1) )
			{
				//OutputDebugString(strEmailAdressValue.c_str());
				strTmp = strSmsNumberValue.substr(0, index - 1);
				//OutputDebugString(strTmp.c_str());
				strSmsNumberValue = strTmp;
			}
			
			DebugePrint(strSmsNumberValue.c_str());
			//alertsmsobj = (CAlertSmsObj *)alertbaseobj;
			alertsmsobj = new CAlertSmsObj();
			alertsmsobj->strSmsNumberValue = strSmsNumberValue;
			alertsmsobj->strOtherNumberValue = strOtherNumberValue;
			alertsmsobj->strSmsSendMode = strSmsSendMode;
			alertsmsobj->strSmsTemplateValue = strSmsTemplateValue;

			alertsmsobj->strAlertUpgradeValue = strAlertUpgradeValue;
			alertsmsobj->strAlertUpgradeToValue = strAlertUpgradeToValue;
			alertsmsobj->strAlertStopValue = strAlertStopValue;

			alertsmsobj->RefreshData();
			alertbaseobj = (CAlertBaseObj *)alertsmsobj;
			break;
		case 3:
			//脚本报警
			strServerTextValue = GetIniFileString(strIndex, "ScriptServer", "", "alert.ini", "localhost", strIdcId);
			strScriptServerId = GetIniFileString(strIndex, "ScriptServerId", "", "alert.ini", "localhost", strIdcId);
			strScriptFileValue = GetIniFileString(strIndex, "ScriptFile", "", "alert.ini", "localhost", strIdcId);
			strScriptParamValue = GetIniFileString(strIndex, "ScriptParam", "", "alert.ini", "localhost", strIdcId);

			//alertscriptobj = (CAlertScriptObj *)alertbaseobj;
			alertscriptobj = new CAlertScriptObj();					
			alertscriptobj->strServerTextValue = strServerTextValue;
			alertscriptobj->strScriptServerId = strScriptServerId;
			alertscriptobj->strScriptFileValue = strScriptFileValue;
			alertscriptobj->strScriptParamValue = strScriptParamValue;
			
			alertbaseobj = (CAlertBaseObj *)alertscriptobj;
			break;
		case 4:
			//声音报警
			strServerValue = GetIniFileString(strIndex, "Server", "", "alert.ini", "localhost", strIdcId);
			strLoginNameValue = GetIniFileString(strIndex, "LoginName", "", "alert.ini", "localhost", strIdcId);
			strLoginPwdValue = GetIniFileString(strIndex, "LoginPwd", "", "alert.ini", "localhost", strIdcId);
			
			alertsoundobj = new CAlertSoundObj();					
			alertsoundobj->strServerValue = strServerValue;
			alertsoundobj->strLoginNameValue = strLoginNameValue;
			alertsoundobj->strLoginPwdValue = strLoginPwdValue;
			
			alertbaseobj = (CAlertBaseObj *)alertsoundobj;
			break;
		case 5:
			//
			break;
		default:
			break;
	}

	if(alertbaseobj != NULL)
	{
		alertbaseobj->strIndex = strIndex;
		alertbaseobj->strAlertName = strAlertName;
		alertbaseobj->strAlertType = strAlertType;
		alertbaseobj->strAlertCategory = strAlertCategory;
		alertbaseobj->strAlertState = strAlertState;
		alertbaseobj->strAlertTargerList = strAlertTargerList;
		alertbaseobj->nAlertType = nAlertType;
		alertbaseobj->nCond = nCond;
		alertbaseobj->strAlwaysTimesValue = strAlwaysTimesValue;
		alertbaseobj->strOnlyTimesValue = strOnlyTimesValue;
		alertbaseobj->strSelTimes1Value = strSelTimes1Value;
		alertbaseobj->strSelTimes2Value = strSelTimes2Value;			
		
		alertbaseobj->strIdcId = strIdcId;
		
		//printf(alertbaseobj->GetDebugInfo().c_str());
		//DebugePrint(alertbaseobj->GetDebugInfo().c_str());
		
		//进入临界区
		
		//if(!waitlistmutex->test())
		{	
			m_AlertObjList.push_back(alertbaseobj);	
			alertbaseobj->AnalysisAlertTarget();
			//waitlistmutex->leave();
		}
		//else
		{
			//DebugePrint("--------------------------读取ini记录因数据冲突而放弃!!!!!!!!----------------------------------\r\n");
		}		

		DebugePrint(alertbaseobj->GetDebugInfo());
	}

	//alertbaseobj = NULL;
}

//
void CAlertMain::DeleteAlertObjFromSection(string strSection, string strIdcId)
{
	bool bExist = false;
	//分解出正确的pAlertTargetList
	for(m_AlertObjListItem = m_AlertObjList.begin(); m_AlertObjListItem != m_AlertObjList.end(); m_AlertObjListItem++)
	{
		if((*m_AlertObjListItem)->strIndex == strSection && (*m_AlertObjListItem)->strIdcId == strIdcId)
		{
			bExist = true;
			break;
		}		
	}

	if(bExist)
	{
		//进入临界区
		//if(!waitlistmutex->test())
		{	
			m_AlertObjList.erase(m_AlertObjListItem);			
			//waitlistmutex->leave();
		}
		//else
		{
			//DebugePrint("--------------------------删除ini记录因数据冲突而放弃!!!!!!!!----------------------------------\r\n");
		}
		//delete (*m_AlertObjListItem);

	}
}


//alert.ini发生变化时刷新警对象列表
void CAlertMain::RefreshAlertObjList()
{
	//
}

//不需要了
void  CAlertMain::IniChangeProc()
{
	DebugePrint("IniChangeProc");
	
	//DoReceive  先挂起一下
	receivethread->suspend();
	
	//
	m_AlertObjList.clear();
	InitAlertObjList();

	//分解出正确的pAlertTargetList
	for(m_AlertObjListItem = m_AlertObjList.begin(); m_AlertObjListItem != m_AlertObjList.end(); m_AlertObjListItem++)
	{
		(*m_AlertObjListItem)->AnalysisAlertTarget();
		DebugePrint((*m_AlertObjListItem)->GetDebugInfo());
	}

	//DoReceive  重新启动
	receivethread->resume();
}

//不需要了
void  CAlertMain::SmsPortChangeProc()
{
	//接收队列和插事件队列的函数。。。
	//

	//关闭串口
	CAlertMain::CloseSerialPort();
	
	bInitSerialPort = CAlertMain::InitSerialPort();	
}

//从AlertType获取int
int CAlertMain::GetIntFromAlertType(string strType)
{
	int nType = 0;
	if(strType == "EmailAlert")
		nType = 1;
	else if(strType == "SmsAlert")
		nType = 2;
	else if(strType == "ScriptAlert")
		nType = 3;
	else if(strType == "SoundAlert")
		nType = 4;
	else if(strType == "PythonAlert")
		nType = 5;
	else
		nType = 0;
	return nType;
}

//////////////////////////////////////////////////////


////////////////静态函数开始//////////////////////////

HINSTANCE CAlertMain::hEmailDll = NULL;//静态成员的初始化
HINSTANCE CAlertMain::hMonitorDll = NULL;//静态成员的初始化
SendEmail * CAlertMain::pSendEmail = NULL;//静态成员的初始化
EXECUTESCRIPT * CAlertMain::pExcuteScript = NULL;//静态成员的初始化
_Alert * CAlertMain::mySoundRef = NULL;
_Alert * CAlertMain::myScriptRef = NULL;
IUMSmSendPtr CAlertMain::pSender = NULL;
static const basic_string <char>::size_type npos = -1;
bool CAlertMain::bInitSerialPort = false;

//值班配置列表
list<WATCH_LIST> CAlertMain::m_pListWatch;
//值班配置列表的表项
list<WATCH_LIST>::iterator CAlertMain::m_pListWatchItem;

//static jwsmtp::mailer sendmail(false, 25);

//初始化各种SendIpi
bool CAlertMain::InitSendApi()
{
	//AlertEmail.dll
#if WIN32
    //hDll = LoadLibrary(strPath.c_str());
	//hDll = LoadLibrary("AlertEmail.dll");
	//
 //   if (hDll)
 //   {
 //       pSendEmail = (SendEmailAlert*)::GetProcAddress(hDll, "EmailSendMessage");
 //       if(pSendEmail)
 //       {  
	//		int iEmailRet = 0;
	//		printf("load dll sucess");
	//		CAlertMain::pSendEmail("mail.dragonflow.com", "xingyu.cheng@dragonflow.com","test", "cxytgf@sina.com",
	//		"ffffffff", "xingyu.cheng",	"xingyu.cheng@dragonflow.com", iEmailRet);

	//	}
 //   }
    hEmailDll = LoadLibrary("emailalert.dll");
    if (hEmailDll)
    {
        pSendEmail = (SendEmail*)::GetProcAddress(hEmailDll, "SendEmail");
        if (pSendEmail)
        {  
   //         bool bRet = pSendEmail("mail.dragonflow.com", "xingyu.cheng@dragonflow.com",
   //             "xingyu.cheng@dragonflow.com", "ffffffff",
   //             "ffffffffffffffff", "xingyu.cheng@dragonflow.com", "xingyu.cheng");
   ////         //bool bRet = (*func)("smtp.sina.com.cn", "cxytgf@sina.com",
   ////         //    "cxytgf@sina.com", "ffffffff",
   ////         //    "ffffffffffffffff", "cxytgf@sina.com", "cxytgf");
			//if(bRet)
			//	printf("SendEmail sucess");
			//else
			//	printf("SendEmail fail");
        }
        /*FreeLibrary(hDll);*/
    }
	
    hMonitorDll = LoadLibrary("Monitor.dll");
    if (hMonitorDll)
    {
        pExcuteScript = (EXECUTESCRIPT*)::GetProcAddress(hMonitorDll, "SCRIPT");
        if (pExcuteScript)
        {  

		}
    }

#endif
	return true;
}

//卸载各种SendIpi
void CAlertMain::UnloadSendApi()
{
	#if WIN32
		if (hEmailDll != NULL)
		{
			FreeLibrary(hEmailDll);
		}
		if(hMonitorDll != NULL)
		{
			FreeLibrary(hMonitorDll);
		}
	#endif
}

//
bool CAlertMain::SendMail(string strServer, string strFrom, string strTo, string strSubject, string strContent, string strUser, string strPwd)
{
	//sendmail.setserver(strServer.c_str());
	//sendmail.setsender(strFrom.c_str());
	//sendmail.addrecipient(strTo.c_str());
	//sendmail.setsubject(strSubject.c_str());
	//sendmail.setmessage(strContent.c_str());
 //   sendmail.username(strUser.c_str());
 //   sendmail.password(strPwd.c_str());   

	//sendmail.send();

	return true;
}
//
bool CAlertMain::InitSoundAlertCom()
{
	CoInitialize(NULL);
	HRESULT hr=CoCreateInstance(CLSID_Alert,NULL,
							CLSCTX_ALL,
							IID__Alert,(void **)&CAlertMain::mySoundRef);
	if(SUCCEEDED(hr))
	{
		DebugePrint("CAlertSoundSendObj creat com success");
		//CoUninitialize();
		return true;	
	}
	else 
	{
		DebugePrint("CAlertSoundSendObj creat com failed");
		//CoUninitialize();
		return false;
	}
	//CoUninitialize();
}

//
void CAlertMain::UnloadSoundAlertCom()
{
	if(CAlertMain::mySoundRef != NULL)
	{
		//CoInitialize(NULL);
		CAlertMain::mySoundRef->Release();
		CAlertMain::mySoundRef = NULL;
	}
	//CoUninitialize();	
}

//
bool CAlertMain::InitScriptAlertCom()
{
	
	HRESULT hr=CoCreateInstance(CLSID_Alert,NULL,
							CLSCTX_ALL,
							IID__Alert,(void **)&CAlertMain::myScriptRef);
	if(SUCCEEDED(hr))
	{
		DebugePrint("CAlertScriptSendObj creat com success");
		//CoUninitialize();
		return true;	
	}
	else 
	{
		DebugePrint("CAlertScriptSendObj creat com failed");
		//CoUninitialize();
		return false;
	}
	//CoUninitialize();
}

//
void CAlertMain::UnloadScriptAlertCom()
{
	if(CAlertMain::myScriptRef != NULL)
	{
		CAlertMain::myScriptRef->Release();
		CAlertMain::myScriptRef = NULL;
	}

	//CoUninitialize();	
}

//
bool CAlertMain::InitWebSmsAlertCom()
{
	HRESULT hr = pSender.CreateInstance("SMSend.UMSmSend");

	if(SUCCEEDED(hr))
	{
		DebugePrint("CAlertWebSmsSendObj creat com success");
		//CoUninitialize();
		return true;	
	}
	else 
	{
		DebugePrint("CAlertWebSmsSendObj creat com failed");
		//CoUninitialize();
		return false;
	}
}

//
void CAlertMain::UnloadWebSmsAlertCom()
{
	//CoInitialize(NULL);
	//CAlertMain::myRef->Release();
	//CAlertMain::myRef = NULL;
	//CAlertMain::pSender.Release();
	//CoUninitialize();	
}



//
bool CAlertMain::ParserToLength(list<string >&pTokenList, string  strQueryString, int nLength)
{
	size_t nCount = strQueryString.length() / nLength;
	for(int i = 0; i <= (int)nCount; i++)
	{
		string strTmp = strQueryString.substr(0, nLength);
		pTokenList.push_back(strTmp);
		strQueryString.erase(0, nLength);
	}

	return true;
}

//分解字符串
bool CAlertMain::ParserToken(list<string >&pTokenList, const char * pQueryString, char *pSVSeps)
{
    char * token = NULL;
    // duplicate string
	char *cp = new char[strlen(pQueryString) +1];
	if (!cp)
		return false;

	strcpy(cp,pQueryString);
	//char * cp = _strdup(pQueryString);
    if (cp)
    {
        char * pTmp = cp;
        if (pSVSeps) // using separators
            token = strtok( pTmp , pSVSeps);
        else // using separators
		{
			delete cp;
			return false;
		}
            //token = strtok( pTmp, chDefSeps);
        // every field
        while( token != NULL )
        {
            //triml(token);
            //AddListItem(token);
			pTokenList.push_back(token);

			//printf( "token=%s\n", token );
            // next field
            if (pSVSeps)
                token = strtok( NULL , pSVSeps);
            else
			{
				delete cp;
               return false;
			}
				//token = strtok( NULL, chDefSeps);
        }
        // free memory
        delete cp;
    }
    return true;
}

//字符串替换
string CAlertMain::ReplaceStdString(string strIn, string strFrom, string strTo)
{
	string strTmp = strIn;
	size_t nPos=0;
	while(1)
	{
		nPos= strTmp.find(strFrom, nPos);
		size_t nLength = strFrom.length();

		if(nPos != npos)
		{
			strTmp = strTmp.replace(nPos, nLength, strTo);
		}
		else
		{
			break;
		}
	}
	
	return strTmp;
}

//从monitorid获取IdcUserId
string CAlertMain::TruncateToUId(string id)
{
	string uid=id;
	string::size_type pos1=id.find(".");
	if(pos1 != string::npos)
	{
		string::size_type pos2=id.find(".",pos1+1);
		if(pos2 != string::npos)
			uid= id.erase(pos2);
	}
	return uid;
}

//是否是idcuserid
bool CAlertMain::IsIdcGroup(const char * pQueryString)
{	
    char * token = NULL;
	int nCount = 0;   
	// duplicate string
	char * cp = ::strdup(pQueryString);
    if(cp)
    {
        char * pTmp = cp;        
        token = strtok( pTmp , ".");

         //token = strtok( pTmp, chDefSeps);
        // every field
        while( token != NULL )
        {
            nCount ++;
			// next field
            token = strtok( NULL , ".");
        }

        free(cp);
    }
	
	//return (nCount == 1) ? true : false;
	if(nCount == 2)
		return true;
	else
		return false;
}

//根据调度判断是否允许发送
bool CAlertMain::IsScheduleMatch(string strSchedule)
{
	if(strSchedule == "")
		return true;

	bool bReturn = false;	
	Datetime tm;
	int nDay;
	int nHour;	
	int nMin;  	
	nDay = tm.getDayOfWeek();
	nHour = tm.getHour();
	nMin = tm.getMinute();
	//printf("%d:%d:%d ",nDay,nHour,nMin);

	//"Type"  "绝对任务计划"	"相对任务计划" 不存在"绝对任务计划"的情况， 应该在emailset.exe里屏蔽掉
	OBJECT hTask = GetTask(strSchedule);
	
	//if(GetTaskValue("Type", hTask) == "相对任务计划")
	if (GetTaskValue("Type", hTask) == "2") //时间段任务计划
	{
		//相对调度条件参数
			
		char buf[256];
		string temp1 = "Allow";
		itoa(nDay, buf, 10);
		temp1 += buf;
		string temp = GetTaskValue(temp1, hTask);
		
		if(strcmp(temp.c_str(), "1") == 0)
		{				
			//允许	
			int nTaskStartHour = 0, nTaskEndHour = 0, nTaskStartMin = 0, nTaskEndMin = 0;
			int nTaskStart = 0, nTaskEnd = 0, nInput = 0;

			//开始
			temp1 = "start";
			temp1 += buf;
			std::list<string> pTmpList;	
			temp = GetTaskValue(temp1, hTask);
			CAlertMain::ParserToken(pTmpList, temp.c_str(), ":");

			sscanf(pTmpList.front().c_str(), "%d", &nTaskStartHour);
			sscanf(pTmpList.back().c_str(), "%d", &nTaskStartMin);
			
			//结束
			pTmpList.clear();
			temp1 = "end";
			temp1 += buf;
			temp = GetTaskValue(temp1, hTask);
			CAlertMain::ParserToken(pTmpList, temp.c_str(), ":");

			sscanf(pTmpList.front().c_str(), "%d", &nTaskEndHour);
			sscanf(pTmpList.back().c_str(), "%d", &nTaskEndMin);

			printf("时间信息：\n");
			printf( "nTaskEndHour=%d, nTaskEndMin=%d, nHour=%d", nTaskEndHour, nTaskEndMin, nHour );

			//比较
			nTaskStart = nTaskStartHour * 60 + nTaskStartMin;
			nTaskEnd = nTaskEndHour * 60 + nTaskEndMin;
			nInput = nHour * 60 +  nMin;

			if(nInput >= nTaskStart && nInput <= nTaskEnd)
				bReturn = true;
		}else
		{
			//禁止
		}
		
	}else if(GetTaskValue("Type", hTask) == "1") //绝对时间任务计划
	{
		char buf[256];
		string temp1 = "Allow";
		itoa(nDay, buf, 10);
		temp1 += buf;
		//printf( "temp=%s\n", temp1.c_str() );
		string temp = GetTaskValue(temp1, hTask);
		
		if(strcmp(temp.c_str(), "1") == 0)
		{
			int nTaskStartHour=0;

			//开始
			temp1 = "start";
			temp1 += buf;
			//printf( "temp=%s\n", temp1.c_str() );
			std::list<string> pTmpList;	
			temp = GetTaskValue(temp1, hTask);
			
			string tmp = temp;

			std::basic_string<char>::size_type nPos = temp.find( " " );

			if( nPos != std::basic_string<char>::npos )
			{
				tmp = temp.substr( nPos+1 );
				//printf( "nPos=%d,tmp=%s\n", nPos, tmp.c_str() );
			}

			CAlertMain::ParserToken(pTmpList, tmp.c_str(), ":");

			//sscanf((pTmpList.front().c_str()), "%d", &nTaskStartHour);

			nTaskStartHour = atoi( pTmpList.front().c_str() );

			//printf( "TaskStartHour=%d,Hour=%d\n", nTaskStartHour, nHour );

			if (nTaskStartHour == nHour)
				bReturn = true;

		}else
		{
			//禁止
		}
	}else if(GetTaskValue("Type", hTask) == "3") //相对时间任务计划
	{
		string strDay ="start";
		string strTaskVal;

		strDay += '0'+nDay;
		
		strTaskVal = GetTaskValue(strDay,hTask);

		printf( "TaskVal=%s, size=%d, Hour=%d\n", strTaskVal.c_str(), strTaskVal.size(), nHour );

		if (static_cast<int>(strTaskVal.size()) > nHour) //长度是否够？
		{
			if (strTaskVal[nHour] == '1')
				bReturn = true;
		}

	}

	CloseTask(hTask);

	return bReturn;
}

//删除某个值班配置表
void CAlertMain::DeleteCfgFromWatchIniSection(string strSection)
{
	for(m_pListWatchItem = m_pListWatch.begin(); m_pListWatchItem != m_pListWatch.end(); m_pListWatchItem++)
	{
		if(m_pListWatchItem->strItemindex == strSection)
		{
			//bExist = true;
			//break;
			m_pListWatch.erase(m_pListWatchItem);
		}		
	}
}

//读某个值班配置表
void CAlertMain::ReadCfgFromWatchIniSection(string strSection)
{
	string strCount = GetIniFileString(strSection, "count", "", "watchsheetcfg.ini");
	//string strAlertindex = GetIniFileString(strSection, "alertindex", "", "watchsheetcfg.ini");
	string strWatchType = GetIniFileString(strSection, "type", "", "watchsheetcfg.ini");
	
	int nCount = 0;

	DebugePrint("strWatchType=" + strWatchType);
	DebugePrint("strCount=" + strCount);
	
	sscanf(strCount.c_str(), "%d", &nCount);
	for(int i = 1 ; i <= 100; i++)
	{
		WATCH_LIST watchItem;
		
		char chItem[32]  = {0};	
		string strItemDes = "item";
		sprintf(chItem, "%d", i);
		strItemDes.append(chItem);

		string strItem = GetIniFileString(strSection, strItemDes, "", "watchsheetcfg.ini");
		
		DebugePrint("strItem=" + strItem);

		if (strItem.size() <=0)
			continue;


		std::list<string> listWatchItem;
		
		CAlertMain::ParserToken(listWatchItem, strItem.c_str(), ",");
		
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nDayOfWeek);
		//printf( "DayOfWeek=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nYear);
		//printf( "Year=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nMonth);
		//printf( "Month=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();

		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nDay);
		//printf( "Day=%d\n", listWatchItem.front().c_str() );
		watchItem.nDayOfMonth = watchItem.nDay;
		listWatchItem.pop_front();

		//读取结束日期
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nEndDayOfWeek);
		//printf( "EndDayOfWeek=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nEndYear);
		//printf( "EndYear=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		DebugePrint("2");
		
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nEndMonth);
		//printf( "EndMonth=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();

		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nEndDay);
		//printf( "EndDay=%d\n", listWatchItem.front().c_str() );
		watchItem.nEndDayOfMonth = watchItem.nEndDay;
		listWatchItem.pop_front();
		//

		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nItemStartHour);
		//printf( "ItemStartHour=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nItemStartMin);
		//printf( "ItemStartMin=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nItemEndHour);
		//printf( "ItemEndHour=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();				
		sscanf(listWatchItem.front().c_str(), "%d", &watchItem.nItemEndMin);
		//printf( "ItemEndMin=%d\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		
		watchItem.strPhoneNumber = listWatchItem.front();
		//printf( "PhoneNumber=%s\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();

		watchItem.strEmailAddress = listWatchItem.front();
		//printf( "EmailAddress=%s\n", listWatchItem.front().c_str() );
		listWatchItem.pop_front();
		
		//watchItem.strAlertindex = strAlertindex;
		watchItem.strItemindex = strSection;
		watchItem.strType = strWatchType;
		
		m_pListWatch.push_back(watchItem);

		--nCount;
		if (nCount <=0)
			break;
	}
}

//初始化所有值班配置列表
void CAlertMain::ReadCfgFromWatchIni()
{
	std::list<string> keylist;
	std::list<string>::iterator keyitem;

	//从ini读取所有值班配置列表
	//if(GetIniFileSections(keylist, "watchsmscfg.ini"))
	if(GetIniFileSections(keylist, "watchsheetcfg.ini"))
	{
		//从ini初始化报警列表
		for(keyitem = keylist.begin(); keyitem != keylist.end(); keyitem ++)
		{
			ReadCfgFromWatchIniSection((*keyitem));
		}
	}
	//OutputDebugString(CAlertMain::GetCurPhoneNumberFromWatchList("20024").c_str());
}

//根据报警索引和值班表配置取出应该发送的手机号码
string CAlertMain::GetCfgFromWatchList(string strAlertindex, bool bEmail)
{
	OutputDebugString("GetCfgFromWatchList Start:\n");
	OutputDebugString(strAlertindex.c_str());

	//根据strAlertindex获取 值班配置项名称 cfg	
	string strWatchSheet = GetIniFileString(strAlertindex, "WatchSheet", "", "alert.ini");

	if(strWatchSheet == "空" || strWatchSheet == "default" || strWatchSheet == "")
		return "";
	
	DebugePrint("strWatchSheet="+strWatchSheet);

	//临时重新读取相应的配置数据
	m_pListWatch.clear();
	ReadCfgFromWatchIniSection(strWatchSheet);

	DebugePrint("ReadCfgFromWatchIniSection 0k");

	//三种方式	
	Datetime tm;
	//printf( "tmYear=%d,tmMonth=%d,tmDay=%d\n", tm.getYear(), tm.getMonth(), tm.getDay() );
	int nYear, nMonth, nDayOfWeek, nDayOfMonth, nDay;	
	int nHour, nMin, nCurTmStat, nItemStartStat, nItemEndStat;	
	
	//获取当前时间 格式 星期X XX点 XX分 等	

	nYear = tm.getYear();
	nMonth = tm.getMonth();
	
	nDayOfWeek = tm.getDayOfWeek();	
	nDayOfMonth = tm.getDay();
	nDay = tm.getDay();

	nHour = tm.getHour();
	nMin = tm.getMinute();
	
	nCurTmStat = nHour * 60 +  nMin;

	bool bWatchMatch = false;
	
	for(m_pListWatchItem = m_pListWatch.begin(); m_pListWatchItem != m_pListWatch.end(); m_pListWatchItem++)
	{
		bWatchMatch = false;

		//比较日期
		if(m_pListWatchItem->strType == "day")
		{
			
			Datetime st(m_pListWatchItem->nYear,m_pListWatchItem->nMonth,m_pListWatchItem->nDay,0,0,0);
			Datetime ed(m_pListWatchItem->nEndYear,m_pListWatchItem->nEndMonth,m_pListWatchItem->nEndDay,0,0,0);
			Datetime td( tm.getYear(), tm.getMonth(), tm.getDay(), 0, 0, 0 );
			//printf( "Year=%d,Month=%d,Day=%d\n", m_pListWatchItem->nYear, m_pListWatchItem->nMonth, m_pListWatchItem->nDay );
			//printf( "EndYear=%d,EndMonth=%d,EndDay=%d\n", m_pListWatchItem->nEndYear, m_pListWatchItem->nEndMonth, m_pListWatchItem->nEndDay );

			//printf( "td=%d,st=%d,ed=%d\n", td.getDatetime(), st.getDatetime(), ed.getDatetime() );

			//指定日期
			//if(strWatchSheet == m_pListWatchItem->strItemindex && (nYear >= m_pListWatchItem->nYear && 
			//	nMonth >= m_pListWatchItem->nMonth && nDay >= m_pListWatchItem->nDay))
			if (td>=st && td<=ed)
			{
				bWatchMatch = true;
			}		
		}
		else if(m_pListWatchItem->strType == "dayofmonth")
		{
			//日期
			if(strWatchSheet == m_pListWatchItem->strItemindex && nDayOfMonth == m_pListWatchItem->nDayOfMonth)
			{
				bWatchMatch = true;
			}		
		}
		else if(m_pListWatchItem->strType == "dayofweek")
		{
			//星期
			if(strWatchSheet == m_pListWatchItem->strItemindex && nDayOfWeek == m_pListWatchItem->nDayOfWeek)
			{
				bWatchMatch = true;
			}
		}
		else
		{
		
		}

		if(bWatchMatch)
		{
			//比较时间
			nItemStartStat = m_pListWatchItem->nItemStartHour * 60 + m_pListWatchItem->nItemStartMin;
			nItemEndStat = m_pListWatchItem->nItemEndHour * 60 + m_pListWatchItem->nItemEndMin;

			//printf( "ItemStartHour=%d,ItemStartMin=%d,ItemEndHour=%d,ItemEndMin=%d\n", m_pListWatchItem->nItemStartHour, m_pListWatchItem->nItemStartMin, m_pListWatchItem->nItemEndHour, m_pListWatchItem->nItemEndMin );

			//printf( "CurTmStat=%d,ItemStartStat=%d,ItemEndStat=%d\n", nCurTmStat, nItemStartStat, nItemEndStat );
			if(nCurTmStat >= nItemStartStat && nCurTmStat <= nItemEndStat)
			{
				if(bEmail)
					return m_pListWatchItem->strEmailAddress;
				else
					return m_pListWatchItem->strPhoneNumber;
			}
			
		}
	}

	OutputDebugString("GetCfgFromWatchList End:\n");
	return "";
}

//根据monitorid查询指定属性的值
string  CAlertMain::GetMonitorPropValue(string strId, string strPropName)
{
	string strTmp = "";

	//监测器名称
	OBJECT objMonitor = GetMonitor(strId);
	if(objMonitor != INVALID_VALUE)
    {
        MAPNODE motnitornode = GetMonitorMainAttribNode(objMonitor);
        if(motnitornode != INVALID_VALUE)
        {
			FindNodeValue(motnitornode, strPropName, strTmp);
		}

		CloseMonitor(objMonitor);
	}

	return strTmp;
}

//
//取得监测器参数值 sxf 2008-12-4
string CAlertMain::GetMonitorParameterValue(string strId, string strPropName)
{
	string strTmp = "";

	//监测器名称
	OBJECT objMonitor = GetMonitor(strId);
	if(objMonitor != INVALID_VALUE)
    {
        MAPNODE motnitornode = GetMonitorParameter(objMonitor);
        if(motnitornode != INVALID_VALUE)
        {
			FindNodeValue(motnitornode, strPropName, strTmp);
		}

		CloseMonitor(objMonitor);
	}

	return strTmp;
}

//取得文件的最后一行 sxf 2008-12-4
string CAlertMain::GetFileLastLine(string strFilePath, string strLineTag)
{
	FILE *f;
	string strTxt;
	char szBuf[2];
	int retCount = 0;

	f = fopen(strFilePath.c_str(),"r");

	if (!f)
		return strTxt;

	fseek(f,-1,SEEK_END);

	for(;;)
	{
		memset(szBuf, 0 ,2);

		fread(szBuf,1,1,f);

		//if (szBuf[0] == 0)
		//	break;

		if (szBuf[0] == 0x0a)
		{
			if (strTxt.size() > 0)
				break;
			else 
			{
				if (retCount > 0)
					break;
				else
					++retCount;
			}
			fseek(f,-1,SEEK_CUR);
		}
		else
		{
			strTxt+= szBuf;
		}

		if (0 !=fseek(f,-2,SEEK_CUR))
			break;

		
	}

	fclose(f);

	if (strTxt.size() > 0)
		reverse(strTxt.begin(),strTxt.end());

	return strTxt;

	
}

//根据monitorid获取监测器的名称
string  CAlertMain::GetMonitorTitle(string strId)
{
	string strTmp = "";

	//---------------------------------------------------------------------------------------------
	//_zouxiao_2008.7.22
	//修改进程和服务的标题
	string strMonitorType = GetMonitorPropValue(strId, "sv_monitortype");	
		
	/*
	if(strMonitorType == "14" || strMonitorType == "33" || strMonitorType == "41"
		|| strMonitorType == "111" || strMonitorType == "174"  || strMonitorType == "175")
	{
		// 14 Service  33 Nt4.0Process  41 Process  111 UnixProcess  174 SNMP_Process  175 SNMP_Service
	}
	*/

	if(strMonitorType == "14")
		strTmp="Service";

	else if(strMonitorType == "33")
		strTmp="Process";

	else if(strMonitorType == "41")
		strTmp="Process";

	else if(strMonitorType == "111")
		strTmp="Process";

	else if(strMonitorType == "174")
		strTmp="Process";

	else if(strMonitorType == "175")
		strTmp="Service";

	//---------------------------------------------------------------------------------------------

	else
	{
		//监测器名称
		OBJECT objMonitor = GetMonitor(strId);
		if(objMonitor != INVALID_VALUE)
		{
			MAPNODE motnitornode = GetMonitorMainAttribNode(objMonitor);

			if(motnitornode != INVALID_VALUE)
			{
				FindNodeValue(motnitornode, "sv_name", strTmp);
				//CloseResource(motnitornode);
			}

			CloseMonitor(objMonitor);
		}
    }


	//加进程名	
	if(strMonitorType == "14" || strMonitorType == "33" || strMonitorType == "41"
		|| strMonitorType == "111" || strMonitorType == "174"  || strMonitorType == "175")
	{
		// 14 Service  33 Nt4.0Process  41 Process  111 UnixProcess  174 SNMP_Process  175 SNMP_Service
		//strAlertTitle += " ";		

		string strProcName = "";
		OBJECT hMon = GetMonitor(strId);
		MAPNODE paramNode = GetMonitorParameter(hMon);		
		if(strMonitorType == "14")
		{
			FindNodeValue(paramNode, "_Service", strProcName);
		}
		else if(strMonitorType == "33")
		{
			FindNodeValue(paramNode, "_monitorProcessList", strProcName);
		}
		else if(strMonitorType == "41")
		{
			FindNodeValue(paramNode, "_monitorProcessList", strProcName);
		}
		else if(strMonitorType == "111")
		{
			FindNodeValue(paramNode, "_Service", strProcName);
		}
		else if(strMonitorType == "174")
		{
			FindNodeValue(paramNode, "_SelValue", strProcName);
		}
		else if(strMonitorType == "175")
		{
			FindNodeValue(paramNode, "_InterfaceIndex", strProcName);
		}
		else
		{
			
		}

		//CloseResource(paramNode);
		CloseMonitor(hMon);

		strTmp += ":";
		strTmp += strProcName;
		
	}

	return strTmp;
}

//根据monitorid获取父设备的名称
string  CAlertMain::GetDeviceTitle(string strId)
{
	string strTmp = "";
	string strParentId = FindParentID(strId);

	//设备名称
	OBJECT objDevice = GetEntity(strParentId);
    if(objDevice != INVALID_VALUE)
    {
        MAPNODE devicenode = GetEntityMainAttribNode(objDevice);
        if(devicenode != INVALID_VALUE)
        {
			FindNodeValue(devicenode, "sv_name", strTmp);
		}
		
		CloseEntity(objDevice);
	}

	return strTmp;
}

string CAlertMain::GetGroupTitle(string strId)
{
	OBJECT objGroup;
	string szName = "";
    objGroup = GetGroup(strId);
    if(objGroup != INVALID_VALUE)
    {

        MAPNODE node = GetGroupMainAttribNode(objGroup);
        if(node != INVALID_VALUE)
        {
            FindNodeValue(node, "sv_name", szName);  
        }
        CloseGroup(objGroup);
    }

	return szName;

}

//根据monitorid获取全路径
string CAlertMain::GetAllGroupTitle(string strId)
{
    OBJECT objGroup;
    size_t nPos = strId.find(".");
    if(nPos < 0)
    {
        objGroup = GetSVSE(strId);
        string szName = "";
        if(objGroup != INVALID_VALUE)
            szName = GetSVSELabel(objGroup);
        else
            szName = "localhost";
		
		if(objGroup != INVALID_VALUE)
			CloseSVSE(objGroup);
        return szName;        
    }
    else
    {
        string szParent = FindParentID(strId);
        string szName = "";
        objGroup = GetGroup(strId);
        if(objGroup != INVALID_VALUE)
        {

            MAPNODE node = GetGroupMainAttribNode(objGroup);
            if(node != INVALID_VALUE)
            {
                FindNodeValue(node, "sv_name", szName);  
            }
            CloseGroup(objGroup);
        }
        return GetAllGroupTitle(szParent) + " : " + szName;
    }
}

//打开发短信串口
bool CAlertMain::InitSerialPort()
{
    //串口名称
	string sret;
	string Value = GetIniFileString("SMSCommConfig", "Port", sret, "smsconfig.ini");

    CString strCOM = "COM";
	strCOM += Value.c_str();

    //初始化串口
	int nErr = m_smsPort.InitPort(strCOM);
    if (nErr == 0)
		return TRUE;//初始化成功
    else
    {
        switch(nErr)
        {
        case CSerialPort::OpenPortFailed://打开端口失败
			puts( "打开端口失败\n" );
            break;
        case CSerialPort::NoSetCenter://没有设置短信中心
			puts( "没有设置短信中心\n" );
            break;
        }
		m_smsPort.CloseCom();
        return FALSE;//初始化失败
    }

	return true;
}

//关闭串口
void CAlertMain::CloseSerialPort()
{
	m_smsPort.CloseCom();
}

//通过串口发短信
int CAlertMain::SendSmsFromComm( CString strSmsTo, CString strContent, int nSMSMaxLength )
{
	string testResult;
	int bRet=false;
	if(bInitSerialPort)
	{
		bRet=m_smsPort.SendMsg( strSmsTo, strContent, nSMSMaxLength );
	}

	return bRet;
}

//通过串口测试发短信
int CAlertMain::TestSmsFromComm(CString strSmsTo, CString strContent)
{
	string testResult;
	if(bInitSerialPort)
	{
		string strSMSMaxLength = GetIniFileString( "SMSCommConfig", "length", "70", "smsconfig.ini" );
		int nSMSMaxLength(70);
		if( !strSMSMaxLength.empty() )
		{
			nSMSMaxLength = atoi( strSMSMaxLength.c_str() );
		}

		int bRet;
		bRet=m_smsPort.SendMsg(strSmsTo, strContent, nSMSMaxLength );

		//往文件中写入返回结果
		if (bRet == 0)
			testResult="成功%发送短信到串口正常\n";
		else
			testResult="失败%发送短信到串口失败\n";

	}
	else
		testResult="失败%初始化串口失败\n";

	string strto=strSmsTo.GetBuffer(0);
	WriteIniFileString("comsms","phone",strto,"smstestresult.ini");
	WriteIniFileString("comsms","result",testResult,"smstestresult.ini");

	return -1;
}



////////////////静态函数结束//////////////////////////

///////////////CommonC++线程开始/////////////////////

ReceiveThread::ReceiveThread()
{
	
}

//
ReceiveThread::~ReceiveThread()
{
	
}

//
ReceiveThread::ReceiveThread(CAlertMain* obj)
{
	parentObj = obj;
}


//
void ReceiveThread::run()
{
	parentObj->DoReceive();
}



//
ProcessThread::ProcessThread()
{
	
}

//
ProcessThread::~ProcessThread()
{
	
}

//
ProcessThread::ProcessThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void ProcessThread::run()
{
	parentObj->DoProcess();
}


////
//SendThread::SendThread()
//{
//	
//}
//
////
//SendThread::~SendThread()
//{
//	
//}
//
////
//SendThread::SendThread(CAlertMain* obj)
//{
//	parentObj = obj;
//}
//
////
//void SendThread::run()
//{
//	//parentObj->DoSend();
//}

//
SendEmailAlertThread::SendEmailAlertThread()
{
	
}

//
SendEmailAlertThread::~SendEmailAlertThread()
{
	
}

//
SendEmailAlertThread::SendEmailAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void SendEmailAlertThread::run()
{
	parentObj->DoSendEmailAlert();
}

//
SendSmsAlertThread::SendSmsAlertThread()
{
	
}

//
SendSmsAlertThread::~SendSmsAlertThread()
{
	//CAlertMain::UnloadWebSmsAlertCom();	
}

//
SendSmsAlertThread::SendSmsAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void SendSmsAlertThread::run()
{	
	::CoInitialize(NULL);
	//CAlertMain::InitWebSmsAlertCom();
	parentObj->DoSendSmsAlert();
	//CAlertMain::pSender.Release();
	::CoUninitialize();
}

//
SendScriptAlertThread::SendScriptAlertThread()
{
	
}

//
SendScriptAlertThread::~SendScriptAlertThread()
{
	//CAlertMain::UnloadScriptAlertCom();
}

//
SendScriptAlertThread::SendScriptAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void SendScriptAlertThread::run()
{
	::CoInitialize(NULL);
	CAlertMain::InitScriptAlertCom();
	parentObj->DoSendScriptAlert();
	CAlertMain::UnloadScriptAlertCom();
	::CoUninitialize();
}

//
SendSoundAlertThread::SendSoundAlertThread()
{
	
}

//
SendSoundAlertThread::~SendSoundAlertThread()
{
	//CAlertMain::UnloadSoundAlertCom();
}

//
SendSoundAlertThread::SendSoundAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void SendSoundAlertThread::run()
{
	::CoInitialize(NULL);
	CAlertMain::InitSoundAlertCom();
	parentObj->DoSendSoundAlert();
	CAlertMain::UnloadSoundAlertCom();
	::CoUninitialize();
}

//
SendPythonAlertThread::SendPythonAlertThread()
{
	
}

//
SendPythonAlertThread::~SendPythonAlertThread()
{

}

//
SendPythonAlertThread::SendPythonAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

//
void SendPythonAlertThread::run()
{
	parentObj->DoSendPythonAlert();
}

//itsm发送处理线程
SendItsmAlertThread::SendItsmAlertThread()
{
}

SendItsmAlertThread::SendItsmAlertThread(CAlertMain* obj)
{
	parentObj = obj;
}

SendItsmAlertThread::~SendItsmAlertThread()
{

}

//
void SendItsmAlertThread::run()
{
	parentObj->DoSendItsmAlert();
}

//
DirectProcessThread::DirectProcessThread(CAlertMain *obj)
{
	parentObj = obj;
}

void DirectProcessThread::run()
{
	parentObj->DoDirectProcess(this);
}

//

ProcessCheckThread::ProcessCheckThread(CAlertMain *obj)
{
	parentObj = obj;
}

void ProcessCheckThread::run()
{
	parentObj->DoCheckProcess();
}

///////////////CommonC++线程结束/////////////////////