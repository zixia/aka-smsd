#ifndef SMS_09AE022D_EC13_4b0f_9A93_06A65268A4DB
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include <errno.h>

namespace SMS {
class CSMSTcpStream:public tcpstream{
public:
	ssize_t write(void* buf, ssize_t bufLen, timeout_t timeout=0){
		sigset_t sigmask,oldmask;
		int i=0;
		int sended=0;
		sigemptyset(&sigmask);
		sigaddset(&sigmask,SIGUSR1);
		sigaddset(&sigmask,SIGUSR2);
		sigprocmask(SIG_BLOCK,&sigmask,&oldmask);

		while (i=tcpstream::writeData(((char*)buf)+sended,bufLen-sended,timeout)) {
			if ( i<0) {
				if (getErrorNumber()==errTimeout)
					return TIMEOUT;
				if  (errno==EINTR) 
					continue;
				else {
					sigprocmask(SIG_SETMASK,&oldmask,NULL);
					return ERROR;
				}
			}
			sended+=i;
			if (sended>=bufLen)
				break;
		}
		sigprocmask(SIG_SETMASK,&oldmask,NULL);
		return sended;
	}

	ssize_t read(void* buf, ssize_t bufLen, timeout_t timeout=0){
		if (bufLen==0)
			return 0;
		int readed;
		int i;
		readed=0;
		while (i=tcpstream::readData(((char*)buf)+readed,bufLen-readed,timeout)) {
			if (i<0){
				if (getErrorNumber()==errTimeout)
					return TIMEOUT;
				if (errno==EINTR) { 
					continue;
				} else  {
					return ERROR;
				}
			} else 
				readed+=i;
			if (readed>=bufLen)
				break;
		}
		return readed;
	}

};

}

#endif
