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
namespace SMS {

#include "18dx.h"


class CSMS18DXProtocol: public CSMSProtocol{
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
		hostent * phe;
		servent * pse;
		protoent *ppe;
		sockaddr_in sin;
		int s,type;

		memset(&sin,0,sizeof(sin));
		sin.sin_family=AF_INET;


		if (pse=getservbyname(port_18dx,"tcp")){
			sin.sin_port=pse->s_port;
		} else if ( (sin.sin_port=htons((unsigned short)atoi(port_18dx) ))==0)	{
			syslog(LOG_ERR, "get port error.");
			return -1;
		}

		if (phe = gethostbyname(host_18dx) )	{
			memcpy(&sin.sin_addr,phe->h_addr, phe->h_length);
		} else if (( sin.sin_addr.s_addr = inet_addr (host_18dx)) == INADDR_NONE)
		{
			syslog(LOG_ERR, "get host error.");
			return -1;
		}
		if ( ( ppe=getprotobyname("tcp")) == 0)
		{
			syslog(LOG_ERR, "get tcp error.");
			return -1;
		}
		type=SOCK_STREAM;
		s=socket(PF_INET, type,ppe->p_proto);
		if (s<0)
		{
			syslog(LOG_ERR, "get socket error");
			return -1;
		}
		if (connect(s,(struct sockaddr * )&sin,sizeof(sin))<0)
		{
			syslog(LOG_ERR, "connect failed.");
			return -1;
		}

		int lenPack= sizeof(OAKSREQSMZIXIASENDTEXT)+msg->SMSBodyLength;
		char* buffer=new char[lenPack];
		POAKSREQSMZIXIASENDTEXT ps=(POAKSREQSMZIXIASENDTEXT)buffer;
		memset(ps,0,lenPack);
		ps->header.dwType= (OAKSID_SM_ZIXIASENDTEXT | OAKSID_REQ);
		ps->header.dwLength=(lenPack-sizeof(OAKSREQHEADER));
	
		ps->nSerialID=(1213242);
		strcpy(ps->szSrcMobileNo,msg->SenderNumber);
		strcpy(ps->szDstMobileNo,msg->TargetNumber); 

		ps->nFeeID=1;

		ps->lenText=msg->SMSBodyLength;
		memcpy(ps+1,msg->SMSBody,msg->SMSBodyLength);

		write(s,ps,lenPack);

		close(s);

		delete[] buffer;

		return 0;

	}
	/* }}} */

	~CSMS18DXProtocol() {
	}
};

}

#endif
