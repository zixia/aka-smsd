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
#include "sms.h"
#include "smslogger.h"
#include "time.h"
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

#include "smstcpstream.h"

//#define GWIP	"210.51.0.210"

#define WAITTIME	60 //100秒

//C、T、N为通路检查参数，具体含义可参考CMPP2 协议6.1
#define LINK_C		180 //3分钟
#define LINK_T		60  //60秒
#define LINK_N		3   //3次

namespace SMS {

#include "cmpp2.h"
#include "md5.h"


class CSMSCMPP2Protocol: public CSMSProtocol{
	CSMSLogger* m_pSMSLogger;
	int m_connected;
	int m_link_test;
	time_t m_lasttesttime;
	time_t m_lastsendtime;
	UINT32 m_serial;
	CSMSTcpStream m_tcp;

UINT32 getSerial(){ 
        return m_serial++;
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
	int len;
	char buf;
	PCMPP_HEADER head;
	PCMPP_CONNECT packet;
	len=sizeof(CMPP_HEADER+CMPP_CONNECT);
	buf=(char*)malloc(len);
	if (buf==NULL) {
		return NOENOUGHMEMORY;
	}
	memset(buf,0,len);
	head=(PCMPP_HEADER)head;
	packet=(PCMPP_CONNECT)(buf+sizeof(head));
	head->Total_Length=len;
	head->Command_Id=CMPP_CMD_CONNECT;
	head->Sequence_Id=getSerial();

	strncpy(packet->Source_Addr,spCode,6);
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
	MD5Final(packet->AuthenticatorSource,&md5);

	packet->Version=version;
	packet->Timestamp=atoi(timeStr);

	int ret=m_tcp.write(buf,len);
	free(buf);
	if (ret<len) {
		syslog(LOG_ERR, "send login packet error: %d ", errno);
		return ERROR;
	}
	return SUCCESS;
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
	switch (pHead->Command_id) {
		case CMPP_CMD_CONNECT_RESP :
			if (msgLen!=sizeof(CMPP_CONNECT_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial, CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
		case CMPP_CMD_TERMINATE :
			doSendCommand(getSerial, CMPP_CMD_TERMINATE_RESP, NULL,0);
		case CMPP_CMD_TERMINATE_RESP :
			syslog(LOG_ERR,"recieved terminate msg from gatway!");
			return CONNECTIONLOST;
		case CMPP_CMD_SUBMIT_RESP:
			if (msgLen!=sizeof(CMPP_SUBMIT_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial, CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
		case CMPP_CMD_DELIVER:
			break;
		case CMPP_CMD_CANCEL_RESP:
			if (msgLen!=sizeof(CMPP_CANCEL_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial, CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
		case CMPP_CMD_ACTIVE_TEST:
			CMPP_ACTIVE_TEST_RESP packet;
			doSendCommand(,pHead->Sequence_Id, CMPP_CMD_ACTIVE_TEST_RESP,&packet,sizeof(CMPP_ACTIVE_TEST_RESP));
			return SUCCESS;
		case CMPP_CMD_ACTIVE_TEST_RESP:
			if (msgLen!=sizeof(CMPP_ACTIVE_TEST_RESP)) {
				syslog(LOG_ERR," login reply packet length error!");
				doSendCommand(getSerial, CMPP_CMD_TERMINATE, NULL,0);
				return ERROR;
			} 
			break;
		default:
			syslog(LOG_ERR,"recieved err command! we have to terminate connectiong!");
			doSendCommand(getSerial, CMPP_CMD_TERMINATE, NULL,0);
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
	switch (pHead->Command_id) {
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
		case CMPP_CMD_CANCEL_RESP:
		case CMPP_CMD_ACTIVE_TEST_RESP:
			m_link_test--;
			if (m_link_test<0) {
				m_link_test=0;
			}
			break;
	}
	free(buf);

	return SUCCESS;
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
							break;
						}
						if (retCode=doCMDActiveTest()!=SUCCESS) {
							syslog(LOG_ERR,"apiActive failed!: %d",retCode);
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
		sigset_t sigmask, oldmask;

		sigemptyset(&sigmask);
		sigaddset(&sigmask,SIGUSR1);
		sigprocmask(SIG_BLOCK,&sigmask,&oldmask);

		int retCode;
		char buf[160];
		int len;
/*
apiSend(  DWORD msg_id1,DWORD	msg_id2,	char mobile[21],char 	service_id[10],char		 src_term[21],	char		 fee_term[21],	char msg[160],char 	udhi,BYTE 		pid,	BYTE	 isReply,	WORD 	msg_len,	BYTE msg_fmt);
  

  参数说明:
msg_id1:   用户信息id号
msg_id2:   保留参数=0
mobile[21]:   接收号码
           service_id:   服务代码. 例如-lsxz
      	 src_term[21]:   发送源号码(在接收手机端显示的发送者号码)比如 '51687001' 
         fee_term[21]:   计费号码(为手机号码,即从哪个手机上收费)
           msg[160]:   发送信息
udhi:   头标示(数据为二进制时可能有意义，文本信息填0)
pid:    协议ID(数据为二进制时可能有意义，文本信息填0)
isReply:   是否需要状态报告，目前API内置1，表明需要状态报告。
msg_len:  发送消息长度
msg_fmt:   信息类型 （0：ASCII串  3：短信写卡操作  4：二进制
 8：UCS2编码15：含GB汉字）
*/     
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
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return SUCCESS;

	}
	/* }}} */

	~CSMSCMPP2Protocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
