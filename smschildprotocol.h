#ifndef SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include "sms.h"
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

#include "childprotocol.h"

namespace SMS {


const int queueLen=10;

class CSMSChildProtocol: public CSMSProtocol{
	int m_pid;
	int m_state;
	char * m_msg;
	unsigned long int m_msgLen;
	unsigned long int m_msgBufLen;
	int m_sock;
	enum { ready,headLenghtUnkown, headIncomplete, bodyIncomplete };
	unsigned long int m_serial;
private:

unsigned long int getSerial(){
	return m_serial++;
}


int doSendErrorMsg(int msgType, byte SerialNo[4], byte ErrorCode){
	char * msg;
	unsigned long int len;
	len=sizeof(SMSChildProtocolSendMessageSended);
	msg=(char *)malloc(sizeof(SMSChildProtocolSendMessageSended));
	memset(msg,0,len);
	((PSMSChildProtocolSendMessageSended)(msg))->head.msgTypeID=msgType;
	memcpy(((PSMSChildProtocolSendMessageSended)(msg))->head.SMSSerialNo, SerialNo,4);
	sms_longToByte(((PSMSChildProtocolSendMessageSended)(msg))->head.msgLength,len-sizeof(SMSChildProtocolHead));
	sms_longToByte(((PSMSChildProtocolSendMessageSended)(msg))->SerialNo,getSerial());
	((PSMSChildProtocolSendMessageSended)(msg))->ErrorNo=ErrorCode;
	write(m_sock,msg,len);
	return 0;
}
int isMsgValid(SMSMessage** msg, unsigned int * msgLen){
	PSMSChildProtocolSendMessage testMsg=(PSMSChildProtocolSendMessage)m_msg;
	if (testMsg->feeTypeID!=FEETYPE_1) { //类型错误
		doSendErrorMsg(MSGTYPE_SMR, testMsg->head.SMSSerialNo, MSGERR_FEETYPE);
		return -1;
	}
	*msgLen=sizeof(SMSMessage)+sms_byteToLong(testMsg->smsBodyLength);
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , testMsg->senderNo , MOBILENUMBERLENGTH);
	strncpy((*msg)->TargetNumber , testMsg->targetNo , MOBILENUMBERLENGTH);
	(*msg)->SMSBodyLength=sms_byteToLong(testMsg->smsBodyLength);
	memcpy((*msg)->SMSBody, testMsg->smsBody, (*msg)->SMSBodyLength);
	return 0;
}

int doMessage(CSMSStorage* pSMSStorage){
	if ((PSMSChildProtocolCommon(m_msg))->head.msgTypeID==MSGTYPE_SM ) {
		SMSMessage * formatedMsg=NULL;
		unsigned int msgLen=0;
		int ret;
		if (!(ret=isMsgValid(&formatedMsg,&msgLen))){
			pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
			doSendErrorMsg(MSGTYPE_SMR, (PSMSChildProtocolCommon(m_msg))->head.SMSSerialNo, MSG_OK);
			doSendErrorMsg(MSGTYPE_SMS, (PSMSChildProtocolCommon(m_msg))->head.SMSSerialNo, MSG_OK);
			delete formatedMsg;
		} else {
			return -1;
		}
	}
	if ((PSMSChildProtocolCommon(m_msg))->head.msgTypeID==MSGTYPE_CDR ) { //监测信息返回
		//todo : 重设时钟
	}
	return 0;
}

int processMessage(const char*  buf,int* bufLen) {
	char * tmpBuf=NULL;
	if ( (buf==NULL) || (bufLen<=0) ) {
		return -1;
	}
	if (m_state==ready) {
		m_state=headLenghtUnkown;
		m_msg=(char *)malloc(*bufLen);
		if (m_msg==NULL) {
			syslog(LOG_ERR, "can't alloc memory");
			return -1;
		}
		memcpy(m_msg, buf,*bufLen);
		m_msgLen=*bufLen;
		m_msgBufLen=*bufLen;
	} else if ( (m_msgLen+*bufLen) <=m_msgBufLen) {
		memcpy(m_msg+m_msgLen,buf,*bufLen);
		m_msgLen+=*bufLen;
	} else {
		tmpBuf=(char *)malloc(m_msgLen+*bufLen);
		if (m_msg==NULL) {
			syslog(LOG_ERR, "can't alloc memory");
			return -1;
		}
		m_msgBufLen=m_msgLen+*bufLen;
		memcpy(tmpBuf, m_msg, m_msgLen);
		memcpy(tmpBuf+m_msgLen,buf,*bufLen);
		m_msgLen=m_msgBufLen;
	}
	*bufLen=0;
	if ( ( (PSMSChildProtocolCommon(m_msg))->head.msgTypeID!=MSGTYPE_SM)  &&
		( (PSMSChildProtocolCommon(m_msg))->head.msgTypeID!=MSGTYPE_CDR) ) {
		doSendErrorMsg(MSGTYPE_SMR, (PSMSChildProtocolCommon(m_msg))->head.SMSSerialNo, MSGERR_MSGTYPE);
		return -1;
	}
	if (m_state==headLenghtUnkown) {
		if (m_msgLen>=sizeof(byte)*9) {
			m_state=headIncomplete;
		}
	}
	if (m_state==headIncomplete) {
		if (m_msgLen>=(sms_byteToLong((PSMSChildProtocolCommon(m_msg))->head.msgLength) + sizeof(SMSChildProtocolHead)) ) {
			if ( (PSMSChildProtocolCommon(m_msg))->head.msgTypeID==MSGTYPE_SM) {
				m_state=bodyIncomplete;
			} else {
				*bufLen=m_msgLen-(sms_byteToLong((PSMSChildProtocolCommon(m_msg))->head.msgLength) + sizeof(SMSChildProtocolHead));
				m_msgLen=(sms_byteToLong((PSMSChildProtocolCommon(m_msg))->head.msgLength) + sizeof(SMSChildProtocolHead));
				m_state=ready;
				return 1;
			}
		}
	}		
	if (m_state==bodyIncomplete) {
		if (m_msgLen>=(sms_byteToLong((PSMSChildProtocolSendMessage(m_msg))->smsBodyLength) + sizeof(SMSChildProtocolSendMessage)) ) {
				*bufLen=m_msgLen-(sms_byteToLong((PSMSChildProtocolSendMessage(m_msg))->smsBodyLength) + sizeof(SMSChildProtocolSendMessage));
				m_msgLen=(sms_byteToLong((PSMSChildProtocolSendMessage(m_msg))->smsBodyLength) + sizeof(SMSChildProtocolSendMessage));
				m_state=ready;
				return 1;
		}
	}
	return 0;
}

int OnAccept(int s,CSMSStorage* pSMSStorage){
	char buf[1000];
	int errCount=0;
	int i,ret,l;
	int len=sizeof(buf);
	m_sock=s;

	while (i=read(s,buf,len)){
		if (i<0) {
			syslog(LOG_ERR, "read error");
			errCount++;
			/*
			if (errCount>10) {
				break;
			}
			*/
		}
		l=i;
		while (l) {
			ret=processMessage(buf+i-l,&l);
			if (ret<0) {
				goto quit;
			}
			if (ret==1) {
				doMessage(pSMSStorage);
			}
		}
	}
quit:
	m_sock=-1;
	close(s);
	return 0;
}
public:
	CSMSChildProtocol() {
		m_pid=0;
		m_state=ready;
		m_msg=NULL;
		m_msgLen=0;
		m_msgBufLen=0;
		m_sock=-1;
		m_serial=0;
	}

	int Run(CSMSStorage* pSMSStorage){
//		hostent * phe;
		servent * pse;
		protoent *ppe;
		sockaddr_in sin;
		int s,type;

		memset(&sin,0,sizeof(sin));
		sin.sin_family=AF_INET;


		if (pse=getservbyname(testport,"tcp")){
			sin.sin_port=pse->s_port;
		} else if ( (sin.sin_port=htons((unsigned short)atoi(testport) ))==0)	{
			syslog(LOG_ERR, "get port error.");
			exit(-1);
		}
/*
		if (phe = gethostbyname(host_18dx) )	{
			memcpy(&sin.sin_addr,phe->h_addr, phe->h_length);
		} else if (( sin.sin_addr.s_addr = inet_addr (host_18dx)) == INADDR_NONE)
		{
			syslog(LOG_ERR, "get host error.");
			exit(-1);
		}
*/

		sin.sin_addr.s_addr=INADDR_ANY;

		if ( ( ppe=getprotobyname("tcp")) == 0)
		{
			syslog(LOG_ERR, "get tcp error.");
			exit(-1);
		}
		type=SOCK_STREAM;
		s=socket(PF_INET, type,ppe->p_proto);
		if (s<0)
		{
			syslog(LOG_ERR, "get socket error");
			exit(-1);
		}

		
		if (bind(s,(struct sockaddr *)&sin, sizeof(sin))<0){
			syslog(LOG_ERR, "bind error");
			exit(-1);
		}
		listen(s,queueLen);

		struct sockaddr_in fsin;
		unsigned int alen;
		int ssock;
		for(;;){
			alen=sizeof(fsin);
			ssock=accept(s,(struct sockaddr *)&fsin, &alen);
			if (ssock<0) {
/*
				if (errno=EINTR)
					continue;
*/
				syslog(LOG_ERR,"accept error");
				continue;
			}
			if (m_pid!=0) {
				kill(m_pid,SIGTERM);
			}
			switch(m_pid=fork()){
				case 0:
					close(s);
					pSMSStorage->init();
					OnAccept(ssock,pSMSStorage);
					close(ssock);
					exit(0);
					break;
				case -1:
					syslog(LOG_ERR,"fork error");
					exit(-1);
					break;
				default:
					close(ssock);
			}

		}		
		return 0;
	}

	int Send(SMSMessage* msg){
		PSMSChildProtocolReceivedMessage sms;
		unsigned long int smsLen=sizeof(SMSChildProtocolReceivedMessage)+msg->SMSBodyLength;
		
		if (m_sock==-1) {
			syslog(LOG_ERR,"no valid socket");
			return -1;
		}
		
		sms=(PSMSChildProtocolReceivedMessage)malloc(smsLen);

		if (sms==NULL) {
			syslog(LOG_ERR,"can't alloc enough memory for sms");
		}

		memset(sms,0,smsLen);
		sms->head.msgTypeID=MSGTYPE_RM;
		sms_longToByte( (sms->head.SMSSerialNo), getSerial());
		sms_longToByte( (sms->head.msgLength),sizeof(SMSChildProtocolReceivedMessage)-sizeof(SMSChildProtocolHead) );

		sms->smsTypeID=SMSTYPE_TEXT;
			
		strncpy(sms->senderNo , msg->SenderNumber , MOBILENUMBERLENGTH);
		strncpy(sms->targetNo , msg->TargetNumber , MOBILENUMBERLENGTH);
		sms_longToByte((sms->smsLength) , msg->SMSBodyLength);
		memcpy(sms->smsBody, msg->SMSBody, msg->SMSBodyLength);
		write(m_sock,sms,smsLen);

		return 0;

	}

	~CSMSChildProtocol() {
	}
};

}

#endif
