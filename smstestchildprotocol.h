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
#include <cc++/socket.h>
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

namespace SMS {

#include "18dx.h"

class C18dxTCPSocket : public TCPSocket
{
protected:
        bool onAccept(const InetHostAddress &ia, tpport_t port);

public:
        C18dxTCPSocket(InetAddress &ia, tpport_t port);
};

class myTcpStream:public tcpstream{
public:
	ssize_t write(const void* buf, ssize_t bufLen){
		return tcpstream::writeData(buf,bufLen);
	}

	ssize_t read(void* buf, ssize_t bufLen){
		return tcpstream::readData(buf,bufLen);
	}

};


C18dxTCPSocket::C18dxTCPSocket(InetAddress &ia, tpport_t port) : 
	TCPSocket(ia, port) {};

bool C18dxTCPSocket::onAccept(const InetHostAddress &ia, tpport_t port){
		return true;
};

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
	memcpy((*msg)->SMSBody, testMsg+1, testMsg->nLenMsg);
	return 0;
}

int OnAccept(myTcpStream *pStream,CSMSStorage* pSMSStorage){
	char* msg=new char[10000];
	unsigned int len=0,i,l;
	i=pStream->read(msg,sizeof(OAKSREQTRANSFERMOINFO));
	if (i<0) {
			syslog(LOG_ERR, "read error");
			exit(-1);
	}
	if (i!=sizeof(OAKSREQTRANSFERMOINFO)) {
			syslog(LOG_ERR, "read head error");
	}
	len+=i;
	l=((POAKSREQTRANSFERMOINFO)(msg))->nLenMsg;
	while (i=pStream->read(msg+len,l)) {
		if (i<0) {
			syslog(LOG_ERR, "read body error");
			exit(-1);
		}
		len+=i;
		l-=i;
		if (l<0){
			syslog(LOG_ERR, "read body error 1");
			exit(-1);
		}
		if (l==0){
			break;
		}
	};
	
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
		InetAddress addr;
		myTcpStream tcp;
		C18dxTCPSocket socket(addr,atoi(testport));
        try   {
			while(socket.isPendingConnection()){
				tcp.open(socket);
				switch(fork()){
					case 0:
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
