#ifndef SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include <cc++/socket.h>
#include "sms.h"
#include "smschildprivilegechecker.h"
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
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

#include "childprotocol.h"

#include "smslogger.h"

namespace SMS {


const int queueLen=10;


class CChildProtocolTCPSocket : public TCPSocket
{
	CSMSChildPrivilegeChecker* m_privilegeChecker;

protected:
        bool onAccept(const InetHostAddress &ia, tpport_t port);

public:
        CChildProtocolTCPSocket(InetAddress &ia, tpport_t port, CSMSChildPrivilegeChecker* privilegeChecker);
};

class myTcpStream:public tcpstream{
public:
	ssize_t write(const char* buf, ssize_t bufLen){
		return tcpstream::writeData(buf,bufLen);
	}

	ssize_t read(char* buf, ssize_t bufLen){
		return tcpstream::readData(buf,bufLen);
	}

};


CChildProtocolTCPSocket::CChildProtocolTCPSocket(InetAddress &ia, tpport_t port, CSMSChildPrivilegeChecker* privilegeChecker) : 
	TCPSocket(ia, port),m_privilegeChecker(privilegeChecker) {};

bool CChildProtocolTCPSocket::onAccept(const InetHostAddress &ia, tpport_t port){
		std::stringstream st;
		st<<ia;
		return (m_privilegeChecker->isConnectPermitted(st.str().c_str(),port)==TRUE)?true:false;
};

class CSMSChildProtocol: public CSMSProtocol{
	CSMSLogger* m_pSMSLogger;
	int m_pid;
	int m_state;
	myTcpStream *m_pStream;
	enum { ready,headLenghtUnkown, headIncomplete, bodyIncomplete };
	unsigned long int m_serial;
	CChildProtocolTCPSocket *m_pServiceSocket;
	CSMSChildPrivilegeChecker *m_pChildPrivilegeChecker;
private:

unsigned long int getSerial(){
	return m_serial++;
}


int doSendErrorMsg(byte msgType, byte SerialNo[4], byte ErrorCode){
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

	sigset_t sigmask, oldmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask,SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigmask, &oldmask);
	
	m_pStream->write(msg,len);

	sigprocmask(SIG_SETMASK, & oldmask , NULL);
	return 0;
}

int isMsgValid(char* buf, unsigned long int len, SMSMessage** msg, unsigned int * msgLen){
	PSMSChildProtocolSendMessage testMsg=(PSMSChildProtocolSendMessage)buf;

	*msgLen=sizeof(SMSMessage)+sms_byteToLong(testMsg->smsBodyLength);
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , testMsg->senderNo , MOBILENUMBERLENGTH);
	strncpy((*msg)->TargetNumber , testMsg->targetNo , MOBILENUMBERLENGTH);
	strncpy((*msg)->FeeTargetNumber , testMsg->feeTargetNo , MOBILENUMBERLENGTH);
	(*msg)->SMSBodyLength=sms_byteToLong(testMsg->smsBodyLength);
	memcpy((*msg)->SMSBody, testMsg->smsBody, (*msg)->SMSBodyLength);

	(*msg)->sendTime=time(NULL);
	strncpy((*msg)->childCode,"11",SMS_MAXCHILDCODE_LEN);
	(*msg)->childCode[SMS_MAXCHILDCODE_LEN]=0;
/*
	strncpy((*msg)->parentID,"5618",SMS_PARENTID_LEN);
	(*msg)->parentID[SMS_PARENTID_LEN]=0;
	strncpy((*msg)->serviceCode,"-BZHLZC",SMS_FEECODE_LEN);
	(*msg)->serviceCode[SMS_FEECODE_LEN]=0;
*/
	(*msg)->FeeType=6;
	(*msg)->FeeMoney=10;

	if (!m_pChildPrivilegeChecker->isMsgValid(*msg)){
		syslog(LOG_ERR,"message validation failed!");
		return -1;
	}
	return 0;
}

int doMessage(CSMSStorage* pSMSStorage, char* msg, unsigned long int len){
	if ((PSMSChildProtocolCommon(msg))->head.msgTypeID==MSGTYPE_SM ) {
		SMSMessage * formatedMsg=NULL;
		unsigned int msgLen=0;
		int ret;
		if (!(ret=isMsgValid(msg,len, &formatedMsg,&msgLen))){
			pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
			syslog(LOG_ERR,"send reply 1");
			doSendErrorMsg(MSGTYPE_SMR, (PSMSChildProtocolCommon(msg))->head.SMSSerialNo, MSG_OK);
			syslog(LOG_ERR,"send reply 2");
			doSendErrorMsg(MSGTYPE_SMS, (PSMSChildProtocolCommon(msg))->head.SMSSerialNo, MSG_OK);
			delete formatedMsg;
		} else {
			return -1;
		}
	}
	if ((PSMSChildProtocolCommon(msg))->head.msgTypeID==MSGTYPE_CDR ) { //监测信息返回
		//todo : 重设时钟
	}
	return 0;
}


int OnAccept(myTcpStream* pStream,CSMSStorage* pSMSStorage){
	char buf[1000];
	int errCount=0;
	int i,ret,l;
	int len=0;

	byte msgType=0;
	unsigned long int smsSerialNo, msgLen;

	l=sizeof(SMSChildProtocolHead);
redo0:
	i=pStream->read(buf,l);
	if ((i<0) && (errno=EINTR)) {
		goto redo0;
	}
	len+=i;
	if (i<l) {
		syslog(LOG_ERR, "read msg head error %d ", i);
		return -1;
	}
	msgType=(PSMSChildProtocolCommon(buf))->head.msgTypeID;
	msgLen=sms_byteToLong((PSMSChildProtocolCommon(buf))->head.msgLength);
	smsSerialNo=sms_byteToLong((PSMSChildProtocolCommon(buf))->head.SMSSerialNo);
	syslog(LOG_ERR,"msg head length %d",msgLen);
	syslog(LOG_ERR,"msg sn %d",smsSerialNo);
	if ( (msgType!=MSGTYPE_PWD) ) {
		syslog(LOG_ERR, "msg head type error");
		return -1;
	}
redo02:
	i=pStream->read(buf+len,msgLen);
	if ((i<0) && (errno=EINTR)) {
		goto redo02;
	}
	len+=i;
	if (i<msgLen) {
		syslog(LOG_ERR, "read msg head body error");
		return -1;
	}
	
	if (!m_pChildPrivilegeChecker->canUserConnect( (PSMSChildProtocolPassword(buf))->user,(PSMSChildProtocolPassword(buf))->password)  ){
		syslog(LOG_ERR,"connection user & password wrong!");
		return -1;
	}

	m_pStream=pStream;
	pSMSStorage->init();
	pSMSStorage->OnNotify();

	for (;;){
		len=0;
		l=sizeof(SMSChildProtocolHead);
redo1:
		i=pStream->read(buf,l);
		if ((i<0) && (errno=EINTR)) {
			goto redo1;
		}

		len+=i;
		if (i<l) {
			syslog(LOG_ERR, "read msg head error %d ", i);
			break;
		}
		msgType=(PSMSChildProtocolCommon(buf))->head.msgTypeID;
		msgLen=sms_byteToLong((PSMSChildProtocolCommon(buf))->head.msgLength);
		smsSerialNo=sms_byteToLong((PSMSChildProtocolCommon(buf))->head.SMSSerialNo);
		syslog(LOG_ERR,"msg head length %d",msgLen);
		syslog(LOG_ERR,"msg sn %d",smsSerialNo);
		if ( (msgType!=MSGTYPE_SM)  &&	( msgType!=MSGTYPE_CDR) ) {
			doSendErrorMsg(MSGTYPE_SMR, (PSMSChildProtocolCommon(buf))->head.SMSSerialNo, MSGERR_MSGTYPE);
			syslog(LOG_ERR, "msg head type error");
			break;
		}
redo2:
		i=pStream->read(buf+len,msgLen);
		if ((i<0) && (errno=EINTR)) {
			goto redo2;
		}

		len+=i;
		if (i<msgLen) {
			syslog(LOG_ERR, "read msg head body error");
			break;
		}
		if (msgType==MSGTYPE_SM) {
			l=sms_byteToLong((PSMSChildProtocolSendMessage(buf))->smsBodyLength);
			syslog(LOG_ERR, "msg body length %d", l);
redo3:
			i=pStream->read(buf+len,l);
			if ((i<0) && (errno=EINTR)) {
				goto redo3;
			}

			len+=i;
			if (i<l) {
				syslog(LOG_ERR, "read msg head error ");
				break;
			}
		} 
		doMessage(pSMSStorage,buf,len);
	}


	m_pStream=NULL;
	return 0;
}
public:
	CSMSChildProtocol(){
		m_pid=0;
		m_state=ready;
		m_pStream=NULL;
		m_serial=0;
		m_pServiceSocket=NULL;
		m_pChildPrivilegeChecker=NULL;
		m_pSMSLogger=NULL;
	}

	int Run(CSMSStorage* pSMSStorage){
		m_pSMSLogger=new CSMSLogger;
		InetAddress addr;
		m_pChildPrivilegeChecker=new CSMSChildPrivilegeChecker;
		myTcpStream tcp;
        try   {
			m_pServiceSocket=new CChildProtocolTCPSocket(addr,atoi(testport),m_pChildPrivilegeChecker);

			while(m_pServiceSocket->isPendingConnection()){
				tcp.open(*m_pServiceSocket);
				if (m_pid!=0) {
					kill(m_pid,SIGTERM);
				}
				switch(m_pid=fork()){
					case 0:
						delete m_pServiceSocket;
						OnAccept(&tcp,pSMSStorage);
						tcp.close();
						exit(0);
						break;
					case -1:
						syslog(LOG_ERR,"fork error");
						exit(-1);
						break;
					default:
						tcp.close();
				}

			}	
		} catch (Socket *socket)
        {
                tpport_t port;
                InetAddress saddr = (InetAddress)socket->getPeer(&port);
				syslog(LOG_ERR,"socket error %s : %d", saddr.getHostname(), port);

                if(socket->getErrorNumber() == Socket::errResourceFailure)
                {
                        syslog(LOG_ERR, "bind failed; no resources" );
                }
                if(socket->getErrorNumber() == Socket::errBindingFailed)
                {
                        syslog(LOG_ERR, "bind failed; port busy" );
                }
        }
		return 0;
	}

	int Send(PSMSMessage msg){
		PSMSChildProtocolReceivedMessage sms;

		if (m_pStream==NULL){
			return -1;
		}

		if (!m_pChildPrivilegeChecker->isMsgValid(msg)){
			syslog(LOG_ERR,"message validation failed!");
			return -1;
		}
		unsigned long int smsLen=sizeof(SMSChildProtocolReceivedMessage)+msg->SMSBodyLength;
		
		syslog(LOG_ERR,"write msg ....");
		
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

		sigset_t sigmask, oldmask;
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIGUSR1);
		sigprocmask(SIG_BLOCK, &sigmask, &oldmask);

		m_pStream->write((const char*)sms,smsLen);

		sigprocmask(SIG_SETMASK, &oldmask , NULL);

		free(sms);

		m_pSMSLogger->logIt(msg->SenderNumber, msg->TargetNumber,"",0,"11",msg->parentID,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody,msg->SMSBodyLength,0,SMS_TRANSFER_UP);


		return 0;

	}

	~CSMSChildProtocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
