#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>     
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <asm/errno.h>
#include "smsdiskstorage.h"

using namespace std;

namespace SMS {

CSMSDiskStorage* __smsDiskStorage;

void __smsDiskStorage_notify_handler(int sig, siginfo_t *si, void *data)
{
	 __smsDiskStorage->OnNotify();
}

int CSMSDiskStorage::set_notifier() {
	if (m_IncomingDirectory.length()<1) {
		return SUCCESS;
	}
	struct sigaction act;
	act.sa_sigaction = __smsDiskStorage_notify_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &act, NULL);
	if (m_fp==0) {
retry:
		m_fp=open(m_IncomingDirectory.c_str(), O_RDONLY);
		if (m_fp<0) {
			if (errno==ENOENT) {
				if (!mkdir(m_IncomingDirectory.c_str(),0755))
					goto retry;
			}
				
			syslog(LOG_ERR," open dir  %s error: %d!", m_IncomingDirectory.c_str(), errno);
			exit(-1);
		}
	}
	syslog(LOG_ERR, "start monitor: %s %d", m_IncomingDirectory.c_str(),m_fp);
	if (fcntl(m_fp, F_SETSIG, SIGUSR1)<0) {
		syslog(LOG_ERR," fcntl F_SETSIG failed : %d ",errno);
		exit(-1);
	}
	if (fcntl(m_fp, F_NOTIFY, DN_CREATE|DN_MULTISHOT )<0) {
		syslog(LOG_ERR," fcntl F_NOTIFY failed : %d ", errno);
		exit(-1);
	}

	return 0;
}


CSMSDiskStorage::CSMSDiskStorage(CSMSProtocol *pSMSPProtocol, const string & OutgoingDirectory, const string & IncomingDirectory ):m_OutgoingDirectory(OutgoingDirectory),
	m_IncomingDirectory(IncomingDirectory),m_pDIR(NULL),m_dataSize(0),m_pDataBuf(NULL),m_bufSize(0),CSMSStorage(pSMSPProtocol){
	__smsDiskStorage=this;
	m_fp=0;

}

CSMSDiskStorage::~CSMSDiskStorage(){
	delete[] m_pDataBuf;
	__smsDiskStorage=NULL;
}



}


