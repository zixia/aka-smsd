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

#define GWIP	"210.51.0.210"
#define GWPASSWORD	8001
#define GWUSER	""
#define GWPASSWD	""

#define WAITTIME	100000 //10秒

namespace SMS {

#include "gw5168.h"


class CSMS5168Protocol: public CSMSProtocol{
	CSMSLogger* m_pSMSLogger;
	int m_connected;
	time_t m_lastrcvtime;
	time_t m_lastsendtime;
	unsigned long int m_serial;

unsigned long int getSerial(){ 
        return m_serial++;
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



int process(struct CResp * pResp, CSMSStorage* pSMSStorage){
	syslog(LOG_ERR," recieve gw msg type: %d",pResp->head.dwCmdID);

	switch (pResp->head.dwCmdID)  {
	case SUBMITRESP:	//发送短信的网关回应
		if (pResp->sr.result==0) {
			syslog(LOG_ERR," msg %d 发送成功！",pResp->sr.msg_id1);
		} else {
			syslog(LOG_ERR," msg %d 发送失败: 0x%u！",pResp->sr.msg_id1,pResp->sr.result);
		}
		break;
	case DELIVERY:		//收到网关deliver短信
		SMSMessage* formatedMsg;
		unsigned int msgLen;
		if (!convertMsgFormat(&(pResp->dl), &formatedMsg,&msgLen)){
			pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
			delete formatedMsg;
		} else {
			return -1;
		}
		break;
	case ALIVERESP: //keep alive
	default:
		break;
	}
	return 0;
}
public:
	CSMS5168Protocol() {
		m_pSMSLogger=NULL;
		m_connected=0;
		m_serial=0;
	}

	/* {{{ Run(CSMSStorage* pSMSStorage) */
	int Run(CSMSStorage* pSMSStorage){
		int retCode;
		struct CResp msg;
		time_t now;
		m_pSMSLogger=new CSMSLogger;
		pSMSStorage->init();
		for(;;) {
			m_connected=0;
			for(;;) {
				if ((retCode=apiLogin(GWIP,GWPASSWORD,GWUSER,GWPASSWD))!=0) {
					syslog(LOG_ERR,"apiLogin error: %d",retCode);

				} else {
					retCode=apiRecv(&msg,WAITTIME);
					if (retCode==0) {
						int loginResult=(msg.lr.result & 0x1111);
						if (msg.lr.result==0) { //登录成功！
							m_connected=1;
							break;
						} else {
							if (msg.lr.result & 0x1) {
								syslog(LOG_ERR,"apiLogin failed: user/passwd error!");
							}
							if (msg.lr.result & 0x10) {
								syslog(LOG_ERR,"apiLogin failed: ip error!");
							}
							syslog(LOG_ERR,"apiLogin failed. result: 0x%u", msg.lr.result);
						}
					} else {
						syslog(LOG_ERR,"login -- apiRecv failed: %d", retCode);
					}
				}
				apiStop();
				sleep(10);
			}
			time(&m_lastrcvtime);
			time(&m_lastsendtime);
			pSMSStorage->OnNotify();
			while (retCode=apiRecv(&msg,WAITTIME)) {
				/* retcode:
				0：调用函数成功
				1:  接收数据包失败
				2:  超时
				3:  等候数据包失败
				4:  网络断开
				*/
				if (retCode==0) {
					time(&m_lastrcvtime);
					process(&msg,pSMSStorage);
					continue;
				}
				switch(retCode) {
				case 1:
				case 4:
					m_connected=0;
					break;
				}
				time(&now);
				if (now-m_lastsendtime) {
					if (apiActive()!=0) {
						m_connected=0;
						break;
					}
				}
				time(&m_lastsendtime);
				if (now-m_lastrcvtime>3*WAITTIME) {
					m_connected=0;
					break;
				}
			}			

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

	~CSMS5168Protocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
