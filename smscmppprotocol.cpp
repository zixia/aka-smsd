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

#include "CMPPAPI.hpp"

#define CONFIGFILE SMSETCDIR "cmppc.ini"

#define WAITTIME	60 //60秒

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
	PacketInfoList m_packetInfoList;
	
UINT32 getSerial(){ 
        return m_serial++;
}


int doCloseConnect() {
	syslog(LOG_ERR,"close connection!");
	m_connected=0;
	return SUCCESS;
}

int isMsgValid(DeliverResp * pDeliverResp, SMSMessage** msg, unsigned int * msgLen){
	*msgLen=sizeof(SMSMessage)+pDeliverResp->nMsgLen;
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , pDeliverResp->sSrcTermID , MOBILENUMBERLENGTH);
	(*msg)->SenderNumber[MOBILENUMBERLENGTH]=0;
	strncpy((*msg)->TargetNumber , (pDeliverResp->sDestTermID)+4 , MOBILENUMBERLENGTH);
	(*msg)->TargetNumber[MOBILENUMBERLENGTH];
	(*msg)->FeeTargetNumber[0]=0;
	(*msg)->SMSBodyLength=pDeliverResp->nMsgLen;
	memcpy((*msg)->SMSBody, buf, len);

	(*msg)->arriveTime=time(NULL);
	strncpy((*msg)->parentID,"cmpp",SMS_PARENTID_LEN);
	(*msg)->parentID[SMS_PARENTID_LEN]=0;
	(*msg)->FeeType=0;

	return 0;
}

int deliverMsg(DeliverResp* pDeliverResp ) {
	if (pDeliverResp->nIsReply==1) {
		//处理发送报告
	}
	PSMSMessage formatedMsg;
	unsigned int msgLen;

	syslog(LOG_ERR,"start convert CMPP sms..");
	
	if (!isMsgValid(&head,buf,head.nLenMsg, &formatedMsg,&msgLen)){
		pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
		delete formatedMsg;
	} else {
		return -1;
	}
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
				if ((retCode=InitCMPPAPI(CONFIGFILE)!=0) {
					syslog(LOG_ERR,"InitCMPPAPI Failed: %d",retCode);
					sleep(10);
				} else {
					time(&m_lastsendtime);
					break;
				}
			}
			m_connected=1;
			m_link_test=0;
			for(;;) {
				DeliverResp* pDeliverResp;
				retCode=CMPPDeliver(WAITTIME, pDeliverResp);
				syslog(LOG_ERR,"waitForMsg return: %d",retCode);
				if (retCode==1) {
					deliverMsg(header,WAITIME);
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
						if ((CMPPActiveTest(&retCode))!=0) {
							syslog(LOG_ERR,"CMPPActiveTest failed!");
							break;
						}
						if (retCode!=ERROR_CODE_OK) {
							syslog(LOG_ERR,"CMPPActiveTest return error!: %d",retCode);
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
		syslog(LOG_ERR,"send msg to cmpp....");
		if (CMPPSendSingle(1, 1,
		const char *sServiceID, const int nMsgFormat,
		const char *sFeeType, const char *sFeeCode,
		const char *sValidTime, const char *sAtTime,
		msg->SenderNumber, msg->TargetNumber,
		msg->SMSBodyLength, msg->SMSBody,
		char *sMsgID, &retCode,
		3, msg->FeeTargetNumber,
		const char cTpPid, const char cTpUdhi)!=0) {
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			syslog(LOG_ERR,"send msg to cmpp failed...");
			return FAILED;
		}

				      
		if (retCode==ERROR_CODE_OK) {
			m_pSMSLogger->logIt(msg->SenderNumber, msg->TargetNumber,msg->FeeTargetNumber,msg->FeeType,msg->childCode,"58181888" ,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody,msg->SMSBodyLength);
		} else {
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			syslog(LOG_ERR,"send msg to cmpp error:%d",retCode);
			switch(retCode) {
			case ERROR_CODE_INVALID_FEECODE:
			case ERROR_CODE_TOO_LONG:
			case ERROR_CODE_INVALID_SERVICEID:
			case ERROR_CODE_INVALID_ICP:
			case ERROR_CODE_INVALID_MSGFORMAT:
			case ERROR_CODE_INVALID_FEETYPE:
			case ERROR_CODE_INVALID_VALIDTIME:
			case ERROR_CODE_INVALID_ATTIME:
			case ERROR_CODE_INVALID_SRCTERMID:
			case ERROR_CODE_INVALID_DESTTERMID:
			case ERROR_CODE_INVALID_DESTTERMIDFILE:
			case ERROR_CODE_INVALID_MSGFILE:
			case ERROR_CODE_INVALID_MSG:
			case ERROR_CODE_INVALID_USER_TYPE:
			case ERROR_CODE_INVALID_FEETERMID:
				return ERROR;
			default
				return FAILED;
			}
		}
		return SUCCESS;

	}
	/* }}} */

	~CSMSCMPP2Protocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
