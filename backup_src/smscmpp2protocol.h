#ifndef SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include <cc++/socket.h>
#include <sqlplus.hh>
#include <asm/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include <string>
#include "sms.h"
#include "smslogger.h"
#include "time.h"
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

#include "smstcpstream.h"

//#define GWIP	"210.51.0.210"

#define WAITTIME	60000 //60秒

//C、T、N为通路检查参数，具体含义可参考CMPP2 协议6.1
#define LINK_C		180 //3分钟
#define LINK_T		60  //60秒
#define LINK_N		3   //3次

//CMPP通信窗口大小，具体含义参考CMPP2协议6.1
#define WINDOWSIZE	8

namespace SMS {

#include "cmpp2.h"
#include "md5.h"

struct packetInfo{
	UINT32 serial;
	UINT32 command;
	time_t sendTIme;
	void* pData;
};

struct sendData{
	std::string smsName;
};

typedef std::vector<packetInfo> PacketInfoList;


class CSMSCMPP2Protocol: public CSMSProtocol{
	CSMSLogger* m_pSMSLogger;
	int m_connected; //是否已与上游成功连接
	int m_link_test; //已发送且未收到回应的测试包个数
	time_t m_lasttesttime; //上次测试包发送时间
	time_t m_lastsendtime; //最近一次向上游发送非测试包的时间
	UINT32 m_serial;
	CSMSTcpStream m_tcp;
	PacketInfoList m_packetInfoList;
	
UINT32 getSerial(){ 
        return m_serial++;
}

//统一向上游发送信息
int doSendCommand(UINT32 sn, UINT32 commandID, const char* buf, UINT32 len){
		sigset_t sigmask, oldmask;

		sigemptyset(&sigmask);
		sigaddset(&sigmask,SIGUSR1);
		sigprocmask(SIG_BLOCK,&sigmask,&oldmask);	

		CMPP_HEADER head;
		int retCode;
		head.Command_Id=commandID;
		head.Sequence_Id=sn;
		head.Total_Length=sizeof(CMPP_HEADER) + len;
		
		//包头
		retCode=m_tcp.write(head,sizeof(CMPP_HEADER),WAITTIME);
		if (retCode<0) {
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			return ERROR;
		}
		//包内容
		if (len>0) {
			retCode=m_tcp.write(buf,len,WAITTIME);
			if (retCode<0) {
				sigprocmask(SIG_SETMASK, &oldmask, NULL);
				return ERROR;
			}
		}

		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return SUCCESS;
}

int doCMDLogin(const OCTETSTRING* gatewayIP, const OCTETSTRING* spCode, const OCTETSTRING * loginSecret, UINT8 version, time_t timestamp) {
	char addr[100];
	snprintf(addr,sizeof(addr),"%s:%d",gatewayIP,CMPP_LONGCONNECTION_PORT);
	syslog(LOG_ERR,"try to connect to %s",addr);
	m_tcp.open(addr);
	if (!m_tcp){
		syslog(LOG_ERR,"can't connect to %s" ,addr);
		return FAILED;
	}
	CMPP_CONNECT packet;
	memset(&packet, 0 ,sizeof(CMPP_CONNECT));

	strncpy(packet.Source_Addr,spCode,6);
	char timeStr[20];
	struct tm t;
	gmtime_r(&timestamp,&t);
	snprintf(timeStr,"%02d%02d%02d%02d%02d",t.tm_mon+1,tm.tm_mday,tm.tm_hour, tm.tm_min, tm.tm_sec);

	struct MD5Context md5;
	int i;
	MD5Init(&md5);
	MD5Update(&md5, spCode,strlen(spCode));
	for (i=0;i<9+6-strlen(spCode;i++)
		MD5Update(&md5,"\0",1);
	MD5Update(&md5,loginSecret,strlen(loginSecret));
	MD5Update(&md5,timeStr,10);
	MD5Final(packet.AuthenticatorSource,&md5);

	packet.Version=version;
	packet.Timestamp=atoi(timeStr);

	return doSendCommand(getSerial(),CMPP_CMD_CONNECT,&packet,sizeof(CMPP_CMD_CONNECT));
}

int doCloseConnect() {
	syslog(LOG_ERR,"close connection!");
	m_connected=0;
	m_tcp.close();
	return SUCCESS;
}

int receiveMsg(char* buf, int bufLen, int waitTime) {
	int len=sizeof(PCMPP_HEADER);
	int rcv=m_tcp.read(buf,Len);
	if (rcv<0) {
		syslog(LOG_ERR, "receive msg  error!: %d",errno);
		return rcv;
	}
	return SUCCESS;
}

int waitForMsgHeader(PCMPP_HEADER pHead, int waitTime ) {
	syslog(LOG_ERR," waiting for msg head...");
	return receiveMsg(pHead, sizeof(CMPP_HEADER) , waitTime);
}

int dispatchMsg(PCMPP_HEADER pHead, int waitTime ) {
	char * buf;
	int msgLen=pHead->Total_Length-sizeof(CMMP_HEADER);
	syslog(LOG_ERR,"recieving  & dispatching msg...");
	syslog(LOG_ERR,"msg sn: %d type :0x%X length: %d",pHead->Sequence_Id,pHead->Command_Id, msgLen);
//检查msg类型和长度是否合法
	switch (pHead->Command_Id) {
		case CMPP_CMD_CONNECT_RESP :
			if (msgLen!=sizeof(CMPP_CONNECT_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial(), CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
			break;
		case CMPP_CMD_TERMINATE :
			doSendCommand(getSerial(), CMPP_CMD_TERMINATE_RESP, NULL,0);
		case CMPP_CMD_TERMINATE_RESP :
			syslog(LOG_ERR,"recieved terminate msg from gatway!");
			return CONNECTIONLOST;
		case CMPP_CMD_SUBMIT_RESP:
			if (msgLen!=sizeof(CMPP_SUBMIT_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial(), CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
			break;
		case CMPP_CMD_DELIVER:
			break;
		case CMPP_CMD_CANCEL_RESP:
			if (msgLen!=sizeof(CMPP_CANCEL_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial(), CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
			break;
		case CMPP_CMD_ACTIVE_TEST:
			CMPP_ACTIVE_TEST_RESP packet;
			return doSendCommand(pHead->Sequence_Id, CMPP_CMD_ACTIVE_TEST_RESP,&packet,sizeof(CMPP_ACTIVE_TEST_RESP));
		case CMPP_CMD_ACTIVE_TEST_RESP:
			if (msgLen!=sizeof(CMPP_ACTIVE_TEST_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial(), CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
			break;
		default:
			syslog(LOG_ERR,"recieved err command! we have to terminate connectiong!");
			doSendCommand(getSerial(), CMPP_CMD_TERMINATE, NULL,0);
			return CONNECTIONLOST;
	}
	buf=(char*)malloc(msgLen);
	if (buf==NULL) {
		syslog(LOG_ERR,"no NOMERY for msg buffer!");
		return NOENOUGHMEMORY;
	}

	if (receiveMsg(buf,msgLen)!=SUCCESS) {
		free(buf);
		return ERROR;
	}
	switch (pHead->Command_Id) {
		case CMPP_CMD_CONNECT_RESP:
			PCMPP_CONNECT_RESP p=(PCMPP_CONNECT_RESP)buf;
			if (p->Status==0) {
				syslog(LOG_ERR, "login ok!");
				m_connected=1;
				pSMSStorage->OnNotify();
			} else {
				syslog(LOG_ERR,"login failed! retCode: %d Version: %0x%X",p->Status,p->Version);
			}
			break;
		case CMPP_CMD_SUBMIT_RESP:
			PCMPP_SUBMIT_RESP:

		case CMPP_CMD_DELIVER:
			doDeliverMsg(buf,msgLen);
			break;
		case CMPP_CMD_CANCEL_RESP:
		case CMPP_CMD_ACTIVE_TEST_RESP:
			m_link_test=0;
			break;
	}
	free(buf);

	return SUCCESS;
}

int doCMDActiveTest(){
	UINT32 serial=getSerial();
	int retCode=doSendCommand(serial,CMPP_CMD_ACTIVE_TEST,NULL,0);
	if (retCode==SUCCESS) {
		struct packetInfo* p;
		p=new struct packetInfo;
		p->serial=serial;
		p->command=CMPP_CMD_ACTIVE_TEST;
		time(&(p->sendTime));
		p->pData=NULL;
		m_packetInfoList.push_back(p);
	}
	return retCode;
}

int convertMsgFormat(struct CDeliver* pSMS,  SMSMessage** msg, unsigned int * msgLen){
	*msgLen=sizeof(SMSMessage)+strlen(pSMS->msg);
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , pSMS->mobile , MOBILENUMBERLENGTH);
	(*msg)->SenderNumber[MOBILENUMBERLENGTH]=0;
	strncpy((*msg)->TargetNumber , pSMS->dst_num , MOBILENUMBERLENGTH);
	(*msg)->TargetNumber[MOBILENUMBERLENGTH];
	(*msg)->FeeTargetNumber[0]=0;
	(*msg)->SMSBodyLength=strlen(pSMS->msg);
	memcpy((*msg)->SMSBody, pSMS->msg, strlen(pSMS->msg));

	(*msg)->arriveTime=time(NULL);
	strncpy((*msg)->parentID,"5168",SMS_MAXCHILDCODE_LEN);
	(*msg)->parentID[SMS_PARENTID_LEN]=0;
	(*msg)->FeeType=0;

	return 0;
}




public:
	CSMSCMPP2Protocol() {
		m_pSMSLogger=NULL;
		m_connected=0;
		m_serial=0;
		m_link_test=0;
	}

	/* {{{ Run(CSMSStorage* pSMSStorage) */
	int Run(CSMSStorage* pSMSStorage){
		int retCode;
		CMPP_HEADER header;
		time_t now;
		m_pSMSLogger=new CSMSLogger;
		pSMSStorage->init();
		for(;;) {
			m_connected=0;

			for(;;) {
				time(&now);
				if ((retCode=doCMDLogin(CMPP_GWIP, CMPP_SPCODE , CMPPVERSION(2,0) , now))!=SUCCESS) {
					syslog(LOG_ERR,"doCMDLogin error: %d",retCode);
					sleep(10);
				} else {
					time(&m_lastsendtime);
					break;
				}
			}
			m_link_test=0;
			for(;;) {
				retCode=waitForMsgHeader(&header,WAITTIME);
				syslog(LOG_ERR,"waitForMsg return: %d",retCode);
				if (retCode==SUCCESS) {
					if ((retCode=dispatchMsg(header,WAITIME))==SUCCESS) 
						continue;
				}
				if (retCode!=TIMEOUT) {
					break;
				}
				time(&now);
				if (m_connected) {
					if ( ( (now-m_lastsendtime)>LINK_C ) || ( m_link_test && (now-m_lasttesttime>LINK_T) ) ){
						syslog(LOG_ERR,"try to keep connection!");
						now(&m_lasttesttime);
						now(&m_lastsendtime);
						m_link_test++;
						if (m_lint_test>LINK_N) {
							syslog(LOG_ERR,"%d test packets sended but no reply!"
							break;
						}
						if (retCode=doCMDActiveTest()!=SUCCESS) {
							syslog(LOG_ERR,"doCMDActiveTest failed!: %d",retCode);
							break;
						}
					} 
				} else {
					syslog(LOG_ERR, "no reply for login request!");
					break;
				}
			}		
			doCloseConnection();
			syslog(LOG_ERR," connection lost!");	
		}
		return 0;
	}
	/* }}} */

	
	/* {{{ Send(SMSMessage* msg) */
	int Send(SMSMessage* msg){
		if (m_connected==0) {
			return FAILED;
		}

		int retCode;
		char buf[160];
		int len;
		char* buf;

		len=msg->SMSBodyLength;
		if (len>159) 
			len=159;
		memcpy(buf,msg->SMSBody,len);
		buf[len]=0;
		retCode= apiSend(getSerial(),  0,msg->TargetNumber,msg->serviceCode,msg->SenderNumber,msg->FeeTargetNumber, buf,0,0,1,len,15);
		syslog(LOG_ERR,"send msg to 5618....");
				      
		if (retCode==0) {
			m_pSMSLogger->logIt(msg->SenderNumber, msg->TargetNumber,msg->FeeTargetNumber,msg->FeeType,msg->childCode,"58181888" ,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody,msg->SMSBodyLength);
		} else {
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			return FAILED;
		}
		return NODELETE;

	}
	/* }}} */

	~CSMSCMPP2Protocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
