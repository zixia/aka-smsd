#include "sms.h"
#include <sys/types.h>
#include <sys/wait.h>

using namespace SMS;

int CSMSDaemon::Run(){

		//防止子类未正确初始化
		assert(m_pSMSProtocol);

		m_pSMSProtocol->Run(m_pSMSStorage);

/*
		pid_t child=NULL;

		m_pSMSPProtocol->Listen();

		for (; ; )
		{
			try{
				m_pSMSPProtocol->Accept();
				if (child)
				{
					kill(child);
				}
				child=fork();
				switch (child){
				case 0 : // child 
					ProcessChild();
					return 0;
					break;
				case -1: // error 
					throw runtime_error("can't fork child process");
					break;
				default:
					m_pSMSPProtocol->Close();
				}
			} catch (exception e) {
				syslog(LOG_ERR, "Accept connection error: %s", e.what());
			}
		}
*/
	
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

CSMSDaemon::~CSMSDaemon(){
		delete m_pSMSProtocol;
		delete m_pSMSStorage;
}
