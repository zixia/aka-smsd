#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>     
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "smsdiskstorage.h"

using namespace std;

namespace SMS {

CSMSDiskStorage* __smsDiskStorage;

void __smsDiskStorage_notify_handler(int sig, siginfo_t *si, void *data)
{
	 __smsDiskStorage->OnNotify();
}

int CSMSDiskStorage::set_notifier() {
	struct sigaction act;
	act.sa_sigaction = __smsDiskStorage_notify_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGDSNOTIFY, &act, NULL);
	int fd=open(m_IncomingDirectory.c_str(), O_RDONLY);

	syslog(LOG_ERR, "start monitor: %s", m_IncomingDirectory.c_str());
	fcntl(fd, F_SETSIG, SIGDSNOTIFY );
	fcntl(fd, F_NOTIFY, DN_CREATE|DN_MULTISHOT);

	return 0;
}


CSMSDiskStorage::CSMSDiskStorage(CSMSProtocol *pSMSPProtocol, const string & OutgoingDirectory, const string & IncomingDirectory ):m_OutgoingDirectory(OutgoingDirectory),
	m_IncomingDirectory(IncomingDirectory),m_pDIR(NULL),m_dataSize(0),m_pDataBuf(NULL),m_bufSize(0),CSMSStorage(pSMSPProtocol){
	__smsDiskStorage=this;

}

CSMSDiskStorage::~CSMSDiskStorage(){
	delete[] m_pDataBuf;
	__smsDiskStorage=NULL;
}



}


