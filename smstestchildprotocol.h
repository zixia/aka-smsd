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
#include <sstream>

namespace SMS {

#include "18dx.h"

const int queueLen=10;

class CSMSTestProtocol: public CSMSProtocol{
private:

int isMsgValid(char* buffer, unsigned int len, SMSMessage** msg, unsigned int * msgLen){
	POAKSREQTRANSFERMOINFO testMsg=(POAKSREQTRANSFERMOINFO)buffer;
	/*
	if (OAKSID_SM_SVRMOINFO!=testMsg->h.dwType) 
		return -1;
	if ((len-sizeof(testMsg->length)-sizeof(testMsg->SenderNumber)-sizeof(testMsg->TargetNumber)-sizeof(testMsg->SMSBodyLength))!=testMsg->SMSBodyLength) {
		return -1;
	}
	*/
	*msgLen=sizeof(SMSMessage)+testMsg->nLenMsg;
	*msg=(SMSMessage*) new char[*msgLen];

	memset(*msg,0,*msgLen);
	(*msg)->length=*msgLen;
	strncpy((*msg)->SenderNumber , testMsg->szMobileNo , MOBILENUMBERLENGTH);
	strncpy((*msg)->TargetNumber , testMsg->szSPCode , MOBILENUMBERLENGTH);
	(*msg)->SMSBodyLength=testMsg->nLenMsg;
	memcpy((*msg)->SMSBody, testMsg+sizeof(OAKSREQTRANSFERMOINFO), testMsg->nLenMsg);
	return 0;
}

int OnAccept(int s,CSMSStorage* pSMSStorage){
	char buf[100];
	char* msg=new char[10000];
	unsigned int len=0,i;
	while (i=read(s,buf,sizeof(buf))){
		if (i<0) {
			syslog(LOG_ERR, "read error");
			exit(-1);
		}
		memcpy(msg+len,buf,i);
		len+=i;
	}
	SMSMessage * formatedMsg=NULL;
	unsigned int msgLen=0;
	if (!isMsgValid(msg,len,&formatedMsg,&msgLen)){
		syslog(LOG_ERR, "write new msg");
		pSMSStorage->writeSMStoStorage(formatedMsg->SenderNumber,formatedMsg->TargetNumber,(char *)formatedMsg,msgLen);
		delete formatedMsg;
	} else {
		syslog(LOG_ERR,"SMS error...");
	}
	return 0;
}
public:
	CSMSTestProtocol() {
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
		int on=1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		
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
			switch(fork()){
				case 0:
					close(s);
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
/*
		if (connect(s,(struct sockaddr * )&sin,sizeof(sin))<0)
		{
			syslog(LOG_ERR, "connect failed.");
			exit(-1);
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
*/
		return 0;

	}

	~CSMSTestProtocol() {
	}
};

}

#endif
