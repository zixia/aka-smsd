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
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

#include "smstcpstream.h"

namespace SMS {

#include "18dx.h"


class CSMS18DXProtocol: public CSMSProtocol{
	CSMSLogger* m_pSMSLogger;
	TCPSocket *m_pServiceSocket;


int isMsgValid(POAKSREQTRANSFERMOINFO pHead,char* buf, int len, SMSMessage** msg, unsigned int * msgLen){
	*msgLen=sizeof(SMSMessage)+len;
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , pHead->szMobileNo , MOBILENUMBERLENGTH);
	(*msg)->SenderNumber[MOBILENUMBERLENGTH]=0;
	strncpy((*msg)->TargetNumber , (pHead->szSPCode)+4 , MOBILENUMBERLENGTH);
	(*msg)->TargetNumber[MOBILENUMBERLENGTH];
	(*msg)->FeeTargetNumber[0]=0;
	(*msg)->SMSBodyLength=len;
	memcpy((*msg)->SMSBody, buf, len);

	(*msg)->arriveTime=time(NULL);
	strncpy((*msg)->parentID,"18dx",SMS_MAXCHILDCODE_LEN);
	(*msg)->parentID[SMS_PARENTID_LEN]=0;
	(*msg)->FeeType=0;

	return 0;
}



int OnAccept(CSMSTcpStream* pStream,CSMSStorage* pSMSStorage){
	OAKSREQTRANSFERMOINFO head;
	char buf[1000];
	int errCount=0;
	int i,size;
	int len=0;


	size=sizeof(OAKSREQTRANSFERMOINFO);
	i=pStream->read(&head,size);
	if (i<size) {
		syslog(LOG_ERR, "read msg head error %d ", i);
		return -1;
	}
	i=pStream->read(buf,head.nLenMsg);
	if (i<head.nLenMsg) {
		syslog(LOG_ERR, "read msg head body error");
		return -1;
	}

	PSMSMessage formatedMsg;
	unsigned int msgLen;

	syslog(LOG_ERR,"start convert 18dx sms..");
	
	if (!isMsgValid(&head,buf,head.nLenMsg, &formatedMsg,&msgLen)){
		pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
		delete formatedMsg;
	} else {
		return -1;
	}
	

	return 0;
}
public:
	CSMS18DXProtocol() {
		m_pSMSLogger=NULL;
		m_pServiceSocket=NULL;
		m_pSMSLogger=NULL;
	}

	/* {{{ Run(CSMSStorage* pSMSStorage) */
	int Run(CSMSStorage* pSMSStorage){
		m_pSMSLogger=new CSMSLogger;
		pSMSStorage->init();
		pSMSStorage->OnNotify();
		InetAddress addr;
		CSMSTcpStream tcp;
        try {
		m_pServiceSocket=new TCPSocket(addr,atoi(testport));

			while(m_pServiceSocket->isPendingConnection()){
				tcp.open(*m_pServiceSocket);
				if (!tcp) {
					continue;
				}
				switch(fork()){
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
	/* }}} */

	
	/* {{{ Send(SMSMessage* msg) */
	int Send(SMSMessage* msg){
		CSMSTcpStream tcp;
		char addr[100];
		snprintf(addr,sizeof(addr),"%s:%s",host_18dx,port_18dx);
		syslog(LOG_ERR,"send message to %s",addr);
		tcp.open(addr);
		if (!tcp){
			syslog(LOG_ERR,"can't connect to %s" ,addr);
			return ERROR;
		}
		sigset_t sigmask, oldmask;

	      sigemptyset(&sigmask);
              sigaddset(&sigmask,SIGUSR1);
	      sigprocmask(SIG_BLOCK,&sigmask,&oldmask);
				      

		int lenPack= sizeof(OAKSREQSMZIXIASENDTEXT)+msg->SMSBodyLength;
		char* buffer=new char[lenPack];
		POAKSREQSMZIXIASENDTEXT ps=(POAKSREQSMZIXIASENDTEXT)buffer;
		memset(ps,0,lenPack);
		ps->header.dwType= (OAKSID_SM_ZIXIASENDTEXT | OAKSID_REQ);
		ps->header.dwLength=(lenPack-sizeof(OAKSREQHEADER));
	
		ps->nSerialID=(1213242);
		strcpy(ps->szSrcMobileNo,msg->FeeTargetNumber);
		strcpy(ps->szDstMobileNo,msg->TargetNumber); 

		ps->nFeeID=msg->FeeType;

		strncpy(ps->szMobileID, msg->SenderNumber, MOBILE_ID_LEN);
		ps->szMobileID[MOBILE_ID_LEN]=0;

		syslog(LOG_ERR, "send no %s to 18dx ",ps->szMobileID);

		ps->lenText=msg->SMSBodyLength;
		memcpy(ps+1,msg->SMSBody,msg->SMSBodyLength);

		tcp.write(ps,lenPack);
		char* buf=new char[sizeof(OAKSACKSMZIXIASENDTEXT)];
		tcp.read(buf,sizeof(OAKSACKSMZIXIASENDTEXT));
		syslog(LOG_ERR,"send msg return %d",(POAKSACKSMZIXIASENDTEXT(buf))->header.dwResult);


		tcp.close();

		if ((POAKSACKSMZIXIASENDTEXT(buf))->header.dwResult!=OAKSBIT_SUCCESS) {
			delete[] buf;
			delete[] buffer;
			return ERROR;
		}else {
			m_pSMSLogger->logIt(msg->SenderNumber, msg->TargetNumber,msg->FeeTargetNumber,msg->FeeType,msg->childCode,"58181888" ,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody,msg->SMSBodyLength);
		}
		sigprocmask(SIG_SETMASK, &oldmask, NULL);

		delete[] buf;

		delete[] buffer;

		return SUCCESS;

	}
	/* }}} */

	~CSMS18DXProtocol() {
		delete m_pSMSLogger;
	}
};

}

#endif
