#ifndef SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include <cc++/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sms.h"
#include "smslogger.h"
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

namespace SMS {

#include "18dx.h"


class myTcpStream:public tcpstream{

public:
	ssize_t write(const void* buf, ssize_t bufLen){
		return tcpstream::writeData(buf,bufLen);
	}

	ssize_t read(char* buf, ssize_t bufLen){
		return tcpstream::readData(buf,bufLen);
	}

};

class CSMS18DXProtocol: public CSMSProtocol{
	CSMSLogger m_SMSLogger;
public:
	CSMS18DXProtocol() {
	}

	/* {{{ Run(CSMSStorage* pSMSStorage) */
	int Run(CSMSStorage* pSMSStorage){
		for(;;){
		    pause();
		}
		return 0;
	}
	/* }}} */

	
	/* {{{ Send(SMSMessage* msg) */
	int Send(SMSMessage* msg){
		myTcpStream tcp;
		char addr[100];
		snprintf(addr,sizeof(addr),"%s:%s",host_18dx,port_18dx);
		syslog(LOG_ERR,"send message to %s",addr);
		tcp.open(addr);
		if (!tcp){
			syslog(LOG_ERR,"can't connect to %s" ,addr);
			return ERROR;
		}

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

		ps->nMobileID=atoi(msg->SenderNumber);

		syslog(LOG_ERR, "send no %d to 18dx ",ps->nMobileID);

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
			m_SMSLogger.logIt(msg->SenderNumber, msg->TargetNumber,msg->FeeTargetNumber,msg->FeeType,msg->childCode,"58181888" ,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody);
		}

		delete[] buf;

		delete[] buffer;

		return SUCCESS;

	}
	/* }}} */

	~CSMS18DXProtocol() {
	}
};

}

#endif
