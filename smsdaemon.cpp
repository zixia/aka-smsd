#include "sms.h"
#include "smsdaemon.h"
#include <sys/types.h>
#include <sys/wait.h>

using namespace SMS;

int CSMSDaemon::Run(){

		//防止子类未正确初始化
		assert(m_pSMSProtocol);

		m_pSMSProtocol->Run(m_pSMSStorage);

		return 1;
}


int CSMSDaemon::OnSignalChild(){
	int statloc;
	return wait(&statloc);
}

int CSMSDaemon::OnSignalTerm(){
	syslog(LOG_ERR,"Terminated by SIGTERM(kill)");
	exit(0);
	return 0;
}

int CSMSDaemon::OnSignalAlarm(){
	siglongjmp(m_jmpBuf,1);
}


int CSMSDaemon::setAlarm(int seconds){
      int retCode=sigsetjmp(m_jmpBuf,1);
      if (retCode==0) {
	      alarm(seconds);
      } else {
	      alarm(0);
      }
      return retCode;
}

CSMSDaemon::~CSMSDaemon(){
		delete m_pSMSProtocol;
		delete m_pSMSStorage;
}
